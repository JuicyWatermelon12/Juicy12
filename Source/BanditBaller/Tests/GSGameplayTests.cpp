// BanditBaller
//
// Gameplay automation tests. Each test starts PIE in the editor's current map (L_Town_Graybox),
// waits for the local player, removes placed enemies so nothing interferes, runs its checks against
// the real GAS / weapon setup, and ends PIE.
//
// Run: Session Frontend > Automation > "BanditBaller", or `Automation RunTests BanditBaller`.
// Stop PIE before running.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "GSAbilitySystemComponent.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSDebugLibrary.h"
#include "GSEnemyCharacter.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LevelStreaming.h"
#include "GSEnemyData.h"
#include "GSGameMode.h"
#include "GSGameplayTags.h"
#include "GSGameState.h"
#include "GSRoundSettings.h"
#include "GSRunRandom.h"
#include "GSRunStats.h"
#include "GSSpawnLocationFinder.h"
#include "GSSpawnSettings.h"
#include "GSWaveDirector.h"
#include "GSWaveSettings.h"
#include "GSCashPickup.h"
#include "GSCashPickupSubsystem.h"
#include "GSCashSettings.h"
#include "GSHUDWidget.h"
#include "GSSpawnTelegraph.h"
#include "HAL/IConsoleManager.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "GSHealthComponent.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSWeaponComponent.h"
#include "GSWeaponData.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"

namespace GSTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
	constexpr double DefaultTimeout = 15.0;

	UWorld* GetPIEWorld()
	{
		return GEditor ? GEditor->PlayWorld.Get() : nullptr;
	}

	AGSPlayerController* GetPC()
	{
		UWorld* World = GetPIEWorld();
		return World ? Cast<AGSPlayerController>(World->GetFirstPlayerController()) : nullptr;
	}

	AGSCharacter* GetPlayer()
	{
		const AGSPlayerController* PC = GetPC();
		return PC ? Cast<AGSCharacter>(PC->GetPawn()) : nullptr;
	}

	UGSAbilitySystemComponent* GetPlayerASC()
	{
		const AGSPlayerController* PC = GetPC();
		const AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
		return PS ? PS->GetGSAbilitySystemComponent() : nullptr;
	}

	float GetHealth(const UAbilitySystemComponent* ASC)
	{
		return ASC ? ASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute()) : -1.f;
	}

	float GetMaxHealth(const UAbilitySystemComponent* ASC)
	{
		return ASC ? ASC->GetNumericAttribute(UGSAttributeSet::GetMaxHealthAttribute()) : -1.f;
	}

	/** Player spawned, stats initialized, weapon equipped and standing on the ground. */
	bool IsPlayerReady()
	{
		const AGSCharacter* Player = GetPlayer();
		const UGSAbilitySystemComponent* ASC = GetPlayerASC();
		const UGSWeaponComponent* Weapons = Player ? Player->GetWeaponComponent() : nullptr;
		return Player && ASC && GetMaxHealth(ASC) > 0.f && GetHealth(ASC) > 0.f
			&& Weapons && Weapons->GetCurrentWeaponData()
			&& !Player->GetCharacterMovement()->IsFalling();
	}

	void RemoveAllEnemies(UWorld* World)
	{
		TArray<AActor*> ToDestroy;
		for (TActorIterator<AGSEnemyCharacter> It(World); It; ++It)
		{
			ToDestroy.Add(*It);
		}
		for (AActor* Actor : ToDestroy)
		{
			if (AController* Controller = Cast<APawn>(Actor)->GetController())
			{
				Controller->Destroy();
			}
			Actor->Destroy();
		}
	}

	TSubclassOf<UGameplayEffect> GetDamageEffect()
	{
		return LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/BanditBaller/Abilities/GE_Damage.GE_Damage_C"));
	}

	/** GE_Damage with SetByCaller Data.Damage = Amount, from Source (defaults to the target itself). */
	void ApplyDamage(UAbilitySystemComponent* Target, float Amount, UAbilitySystemComponent* Source = nullptr)
	{
		Source = Source ? Source : Target;
		const TSubclassOf<UGameplayEffect> DamageEffect = GetDamageEffect();
		if (!Target || !Source || !DamageEffect)
		{
			return;
		}
		FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(DamageEffect, 1.f, Source->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, Amount);
		Source->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target);
	}

	/** Instant +Amount to Health (bypasses IncomingDamage) to exercise the MaxHealth clamp. */
	void ApplyHeal(UAbilitySystemComponent* Target, float Amount)
	{
		UGameplayEffect* Heal = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
		Heal->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Mod = Heal->Modifiers.AddDefaulted_GetRef();
		Mod.Attribute = UGSAttributeSet::GetHealthAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));
		Target->ApplyGameplayEffectToSelf(Heal, 1.f, Target->MakeEffectContext());
	}

	/** Spawns one enemy near the player (server) for target-side tests. */
	AGSEnemyCharacter* SpawnTestEnemy()
	{
		UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		TArray<AGSEnemyCharacter*> Spawned = UGSDebugLibrary::SpawnEnemiesAround(GetPC(), Data, 1);
		return Spawned.Num() > 0 ? Spawned[0] : nullptr;
	}

	/** Enqueue: poll Predicate every frame until true; fail the test after Timeout seconds. */
	void WaitUntil(FAutomationTestBase* Test, const FString& What, TFunction<bool()> Predicate, double Timeout = DefaultTimeout)
	{
		TSharedRef<double> Start = MakeShared<double>(-1.0);
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Test, What, Predicate, Timeout, Start]() -> bool
		{
			if (*Start < 0.0)
			{
				*Start = FPlatformTime::Seconds();
			}
			if (Predicate())
			{
				return true;
			}
			if (FPlatformTime::Seconds() - *Start > Timeout)
			{
				Test->AddError(FString::Printf(TEXT("Timed out after %.0fs waiting for: %s"), Timeout, *What));
				return true;
			}
			return false;
		}));
	}

	/** Enqueue a one-shot step. */
	void Step(TFunction<void()> Body)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Body]() -> bool { Body(); return true; }));
	}

	void BeginPIE(FAutomationTestBase* Test)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
		WaitUntil(Test, TEXT("PIE player ready (pawn, ASC, weapon, grounded)"), &IsPlayerReady, 30.0);
		Step([]() { if (UWorld* World = GetPIEWorld()) { RemoveAllEnemies(World); } });
	}

	void EndPIE()
	{
		ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	}
}

using namespace GSTests;

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSDamageReducesHealthTest, "BanditBaller.GAS.DamageReducesHealth", GSTests::TestFlags)
bool FGSDamageReducesHealthTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		UGSAbilitySystemComponent* ASC = GetPlayerASC();
		if (!TestNotNull(TEXT("Player ASC"), ASC))
		{
			return;
		}
		const float Before = GetHealth(ASC);
		ApplyDamage(ASC, 25.f);
		TestEqual(TEXT("Health after 25 damage"), GetHealth(ASC), Before - 25.f, 0.01f);
	});
	EndPIE();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSHealthClampsTest, "BanditBaller.GAS.HealthClampsToZeroAndMax", GSTests::TestFlags)
bool FGSHealthClampsTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		AGSEnemyCharacter* Enemy = SpawnTestEnemy();
		UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		if (!TestNotNull(TEXT("Spawned enemy ASC"), ASC))
		{
			return;
		}
		const float Max = GetMaxHealth(ASC);
		TestTrue(TEXT("Enemy MaxHealth initialized"), Max > 0.f);

		// Heal well past MaxHealth from below it.
		ApplyDamage(ASC, Max * 0.5f);
		ApplyHeal(ASC, Max * 10.f);
		TestEqual(TEXT("Health clamps at MaxHealth after overheal"), GetHealth(ASC), Max, 0.01f);
		TestEqual(TEXT("Health base value clamps at MaxHealth"), ASC->GetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute()), Max, 0.01f);

		// Overkill damage stops at zero.
		ApplyDamage(ASC, Max * 10.f);
		TestEqual(TEXT("Health clamps at 0 after overkill"), GetHealth(ASC), 0.f, 0.01f);
	});
	EndPIE();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSDeathAddsDeadTagTest, "BanditBaller.GAS.DeathAddsStateDead", GSTests::TestFlags)
bool FGSDeathAddsDeadTagTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		AGSEnemyCharacter* Enemy = SpawnTestEnemy();
		UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		if (!TestNotNull(TEXT("Spawned enemy ASC"), ASC))
		{
			return;
		}
		TestFalse(TEXT("Alive enemy has no State.Dead"), ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead));

		ApplyDamage(ASC, GetMaxHealth(ASC) + 100.f, GetPlayerASC());
		TestTrue(TEXT("State.Dead added on death"), ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		TestTrue(TEXT("Health component reports dead"), Enemy->GetHealthComponent() && Enemy->GetHealthComponent()->IsDead());

		// Damage effects are blocked once dead (GE requirement ignores State.Dead).
		ApplyDamage(ASC, 10.f);
		TestEqual(TEXT("Dead enemy stays at 0 health"), GetHealth(ASC), 0.f, 0.01f);
	});
	EndPIE();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSFireAndReloadTest, "BanditBaller.Weapons.FireConsumesAmmoReloadRefills", GSTests::TestFlags)
bool FGSFireAndReloadTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	TSharedRef<int32> AmmoBefore = MakeShared<int32>(0);
	Step([this, AmmoBefore]()
	{
		const AGSCharacter* Player = GetPlayer();
		UGSWeaponComponent* Weapons = Player ? Player->GetWeaponComponent() : nullptr;
		UGSAbilitySystemComponent* ASC = GetPlayerASC();
		if (!TestNotNull(TEXT("Weapon component"), Weapons) || !TestNotNull(TEXT("Player ASC"), ASC))
		{
			return;
		}
		*AmmoBefore = Weapons->GetCurrentAmmo();
		TestEqual(TEXT("Magazine starts full"), *AmmoBefore, Weapons->GetCurrentWeaponData()->MagazineSize);

		// One tap of the fire input.
		ASC->AbilityInputTagPressed(GSGameplayTags::Ability_Fire);
		ASC->AbilityInputTagReleased(GSGameplayTags::Ability_Fire);
	});
	WaitUntil(this, TEXT("ammo to drop after firing"), [AmmoBefore]()
	{
		const AGSCharacter* Player = GetPlayer();
		return Player && Player->GetWeaponComponent()->GetCurrentAmmo() < *AmmoBefore;
	});
	// Reload is blocked by State.Firing (the refire window), so wait for it to clear first.
	WaitUntil(this, TEXT("State.Firing to clear after the shot"), []()
	{
		return !GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Firing);
	}, 5.0);
	Step([this, AmmoBefore]()
	{
		UGSWeaponComponent* Weapons = GetPlayer()->GetWeaponComponent();
		TestEqual(TEXT("One tap consumes one round"), Weapons->GetCurrentAmmo(), *AmmoBefore - 1);

		UGSAbilitySystemComponent* ASC = GetPlayerASC();
		ASC->AbilityInputTagPressed(GSGameplayTags::Ability_Reload);
		ASC->AbilityInputTagReleased(GSGameplayTags::Ability_Reload);
		TestTrue(TEXT("Reload started (State.Reloading)"), ASC->HasMatchingGameplayTag(GSGameplayTags::State_Reloading));
	});
	WaitUntil(this, TEXT("reload to refill the magazine"), []()
	{
		const AGSCharacter* Player = GetPlayer();
		const UGSWeaponComponent* Weapons = Player ? Player->GetWeaponComponent() : nullptr;
		return Weapons && Weapons->GetCurrentAmmo() == Weapons->GetCurrentWeaponData()->MagazineSize;
	});
	Step([this]()
	{
		TestFalse(TEXT("State.Reloading cleared after reload"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Reloading));
	});
	EndPIE();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSDashInvulnerabilityTest, "BanditBaller.Abilities.DashInvulnerabilityBlocksDamage", GSTests::TestFlags)
bool FGSDashInvulnerabilityTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([]()
	{
		UGSAbilitySystemComponent* ASC = GetPlayerASC();
		ASC->AbilityInputTagPressed(GSGameplayTags::Ability_Dash);
		ASC->AbilityInputTagReleased(GSGameplayTags::Ability_Dash);
	});
	WaitUntil(this, TEXT("dash to grant State.Invulnerable"), []()
	{
		return GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Invulnerable);
	}, 3.0);
	Step([this]()
	{
		UGSAbilitySystemComponent* ASC = GetPlayerASC();
		if (!ASC->HasMatchingGameplayTag(GSGameplayTags::State_Invulnerable))
		{
			return;
		}
		const float Before = GetHealth(ASC);
		ApplyDamage(ASC, 25.f);
		TestEqual(TEXT("Damage during dash i-frames is ignored"), GetHealth(ASC), Before, 0.01f);
	});
	WaitUntil(this, TEXT("i-frames to expire"), []()
	{
		return !GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Invulnerable);
	}, 5.0);
	Step([this]()
	{
		// Control: the same damage lands once the window is over.
		UGSAbilitySystemComponent* ASC = GetPlayerASC();
		const float Before = GetHealth(ASC);
		ApplyDamage(ASC, 25.f);
		TestEqual(TEXT("Damage after i-frames applies"), GetHealth(ASC), Before - 25.f, 0.01f);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Map: town tags, Market District sublevel and area travel
// ---------------------------------------------------------------------------

namespace GSTests
{
	int32 CountTagged(const UWorld* World, FName Tag)
	{
		int32 Count = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			Count += It->ActorHasTag(Tag) ? 1 : 0;
		}
		return Count;
	}

	bool IsOnNavMesh(const UWorld* World, const FVector& Location)
	{
		const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		FNavLocation Out;
		return NavSys && NavSys->ProjectPointToNavigation(Location, Out, FVector(500.f, 500.f, 1000.f));
	}

	bool IsInMarketDistrictLevel(const AActor* Actor)
	{
		return Actor && Actor->GetLevel() && Actor->GetLevel()->GetOutermost()->GetName().Contains(TEXT("L_MarketDistrict_Graybox"));
	}

	AGSGameMode* GetGameMode() { return GetPIEWorld() ? GetPIEWorld()->GetAuthGameMode<AGSGameMode>() : nullptr; }
	AGSGameState* GetGameState() { return GetPIEWorld() ? GetPIEWorld()->GetGameState<AGSGameState>() : nullptr; }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSMapTownAndMarketTest, "BanditBaller.Map.TownAndMarketDistrict", GSTests::TestFlags)
bool FGSMapTownAndMarketTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	// Town: travel anchors + spawns present, no shop/casino left in the plaza, district not loaded.
	Step([this]()
	{
		UWorld* World = GetPIEWorld();
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		if (!TestNotNull(TEXT("AGSGameMode"), GameMode) || !TestNotNull(TEXT("AGSGameState"), GameState))
		{
			return;
		}
		TestTrue(TEXT("Starts in the town"), GameState->GetCurrentArea() == EGSArea::Town);
		TestFalse(TEXT("Market closed by default"), GameState->IsMarketOpenThisRound());

		const AActor* TownReturn = AGSGameMode::FindTaggedActor(World, TEXT("TownReturn"));
		TestNotNull(TEXT("Town has a TownReturn point"), TownReturn);
		TestNotNull(TEXT("Town has a MarketPortalSpot point"), AGSGameMode::FindTaggedActor(World, TEXT("MarketPortalSpot")));
		TestTrue(TEXT("TownReturn is on the town navmesh"), TownReturn && IsOnNavMesh(World, TownReturn->GetActorLocation()));
		TestTrue(TEXT("Town has EnemySpawn points"), CountTagged(World, TEXT("EnemySpawn")) > 0);
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(TEXT("EnemySpawn")))
			{
				TestTrue(FString::Printf(TEXT("%s is on the town navmesh"), *It->GetActorNameOrLabel()), IsOnNavMesh(World, It->GetActorLocation()));
			}
		}
		TestEqual(TEXT("No Casino in the town"), CountTagged(World, TEXT("Casino")), 0);
		TestEqual(TEXT("No MarketStall in the town"), CountTagged(World, TEXT("MarketStall")), 0);
		TestEqual(TEXT("No legacy Market tags in the town"), CountTagged(World, TEXT("Market")), 0);

		ULevelStreaming* District = GameMode->GetMarketDistrictStreamingLevel();
		if (TestNotNull(TEXT("Town map has the L_MarketDistrict_Graybox sublevel"), District))
		{
			TestFalse(TEXT("Market District is unloaded at start"), District->IsLevelLoaded());
		}

		TestTrue(TEXT("Travel to the Market District starts"), GameMode->TravelToMarketDistrict());
	});
	WaitUntil(this, TEXT("party to arrive in the Market District"), []()
	{
		const AGSGameState* GameState = GetGameState();
		return GameState && GameState->GetCurrentArea() == EGSArea::MarketDistrict;
	});

	// Market District contents, safety, and arrival.
	Step([this]()
	{
		UWorld* World = GetPIEWorld();
		const AActor* Arrival = AGSGameMode::FindTaggedActor(World, TEXT("MarketArrival"));
		const AActor* Exit = AGSGameMode::FindTaggedActor(World, TEXT("MarketExitPortalSpot"));
		const AActor* Casino = AGSGameMode::FindTaggedActor(World, TEXT("Casino"));
		TestTrue(TEXT("MarketArrival is in the district sublevel"), IsInMarketDistrictLevel(Arrival));
		TestTrue(TEXT("MarketExitPortalSpot is in the district sublevel"), IsInMarketDistrictLevel(Exit));
		TestTrue(TEXT("Casino is in the district sublevel"), IsInMarketDistrictLevel(Casino));
		TestTrue(TEXT("District has at least 3 MarketStalls"), CountTagged(World, TEXT("MarketStall")) >= 3);

		TestFalse(TEXT("No navmesh at MarketArrival (enemies can't path in)"), Arrival && IsOnNavMesh(World, Arrival->GetActorLocation()));
		TestFalse(TEXT("No navmesh at the Casino"), Casino && IsOnNavMesh(World, Casino->GetActorLocation()));

		const APawn* Player = GetPlayer();
		TestTrue(TEXT("Player teleported to MarketArrival"), Player && Arrival && FVector::Dist2D(Player->GetActorLocation(), Arrival->GetActorLocation()) < 300.f);

		const TArray<AGSEnemyCharacter*> Spawned = UGSDebugLibrary::SpawnEnemiesAround(GetPC(), UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit")), 3);
		TestEqual(TEXT("Enemy spawning is refused in the Market District"), Spawned.Num(), 0);

		TestTrue(TEXT("Return to town starts"), GetGameMode()->ReturnToTown());
	});
	WaitUntil(this, TEXT("party back in town and district unloaded"), []()
	{
		const AGSGameState* GameState = GetGameState();
		const ULevelStreaming* District = GetGameMode() ? GetGameMode()->GetMarketDistrictStreamingLevel() : nullptr;
		return GameState && GameState->GetCurrentArea() == EGSArea::Town && District && !District->IsLevelLoaded();
	});
	Step([this]()
	{
		UWorld* World = GetPIEWorld();
		const AActor* TownReturn = AGSGameMode::FindTaggedActor(World, TEXT("TownReturn"));
		const APawn* Player = GetPlayer();
		TestTrue(TEXT("Player returned to TownReturn"), Player && TownReturn && FVector::Dist2D(Player->GetActorLocation(), TownReturn->GetActorLocation()) < 300.f);
		TestEqual(TEXT("District actors gone after unload"), CountTagged(World, TEXT("Casino")), 0);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Round loop state machine
// ---------------------------------------------------------------------------

namespace GSTests
{
	bool IsPhase(EGSRoundPhase Phase)
	{
		const AGSGameState* GameState = GetGameState();
		return GameState && GameState->GetRoundPhase() == Phase;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSRoundPhaseLoopTest, "BanditBaller.Round.PhaseLoop", GSTests::TestFlags)
bool FGSRoundPhaseLoopTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	// Play starts round 1 in PreRound with a running timer.
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		if (!TestNotNull(TEXT("AGSGameMode"), GameMode) || !TestNotNull(TEXT("AGSGameState"), GameState))
		{
			return;
		}
		TestEqual(TEXT("Round 1 at start"), GameState->GetRoundNumber(), 1);
		TestTrue(TEXT("Starts in PreRound"), IsPhase(EGSRoundPhase::PreRound));
		TestTrue(TEXT("PreRound has an end time"), GameState->GetPhaseEndTime() > 0.0);
		TestTrue(TEXT("PreRound time left within its duration"),
			GameState->GetPhaseTimeRemaining() > 0.f && GameState->GetPhaseTimeRemaining() <= GameMode->GetPhaseDuration(EGSRoundPhase::PreRound) + 0.01f);
	});

	// The PreRound timer advances to Combat on its own.
	WaitUntil(this, TEXT("PreRound timer to start Combat"), []() { return IsPhase(EGSRoundPhase::Combat); },
		GetDefault<UGSRoundSettings>()->PreRoundDuration + 10.0);

	// Skips walk the loop; MarketDecision resolves straight to the next round's PreRound (market closed).
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		TestTrue(TEXT("Combat uses the fallback timer"),
			FMath::IsNearlyEqual(GameState->GetPhaseTimeRemaining(), GameMode->GetPhaseDuration(EGSRoundPhase::Combat), 1.f));

		GameMode->AdvancePhase();
		TestTrue(TEXT("Combat -> RoundClear"), IsPhase(EGSRoundPhase::RoundClear));
		TestEqual(TEXT("Still round 1 in RoundClear"), GameState->GetRoundNumber(), 1);

		GameMode->AdvancePhase();
		TestTrue(TEXT("RoundClear -> MarketDecision -> PreRound"), IsPhase(EGSRoundPhase::PreRound));
		TestEqual(TEXT("Next round started"), GameState->GetRoundNumber(), 2);
		TestFalse(TEXT("Market stays closed for now"), GameState->IsMarketOpenThisRound());

		GameMode->SetRound(5);
		TestEqual(TEXT("gs.Round.Set jumps to round 5"), GameState->GetRoundNumber(), 5);
		TestTrue(TEXT("gs.Round.Set restarts at PreRound"), IsPhase(EGSRoundPhase::PreRound));
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Death -> restart gives back control
// ---------------------------------------------------------------------------

namespace GSTests
{
	bool IsViewportIgnoringInput()
	{
		const UWorld* World = GetPIEWorld();
		UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
		return Viewport && Viewport->IgnoreInput();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSRestartRestoresInputTest, "BanditBaller.Player.RestartRestoresInput", GSTests::TestFlags)
bool FGSRestartRestoresInputTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	TSharedRef<TWeakObjectPtr<UWorld>> OldWorld = MakeShared<TWeakObjectPtr<UWorld>>();
	Step([this, OldWorld]()
	{
		TestFalse(TEXT("Game input active at start"), IsViewportIgnoringInput());

		// Die for real: the server shows the death screen (UI-only input).
		*OldWorld = GetPIEWorld();
		ApplyDamage(GetPlayerASC(), 100000.f);
	});
	WaitUntil(this, TEXT("death screen to take input"), []() { return IsViewportIgnoringInput(); }, 5.0);
	Step([]()
	{
		// Same request the death screen sends on R.
		GetPC()->ServerRequestRestart();
	});
	WaitUntil(this, TEXT("restarted level with a new, ready player"), [OldWorld]()
	{
		return GetPIEWorld() && GetPIEWorld() != OldWorld->Get() && IsPlayerReady();
	}, 30.0);
	Step([this]()
	{
		TestFalse(TEXT("Viewport accepts game input after restart"), IsViewportIgnoringInput());
		const APawn* Player = GetPlayer();
		TestTrue(TEXT("Respawned pawn takes input"), Player && Player->InputEnabled());
		TestFalse(TEXT("Respawned player is not dead"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Run seed + run stats
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSRunSeedAndStatsTest, "BanditBaller.Run.SeedAndStats", GSTests::TestFlags)
bool FGSRunSeedAndStatsTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		UGSRunStats* Stats = GameState ? GameState->GetRunStats() : nullptr;
		AGSPlayerController* PC = GetPC();
		if (!TestNotNull(TEXT("AGSGameMode"), GameMode) || !TestNotNull(TEXT("Run stats"), Stats) || !TestNotNull(TEXT("Player"), PC))
		{
			return;
		}
		TestTrue(TEXT("A seed was chosen at run start"), GameMode->GetRunSeed() != 0);

		// Same seed -> same rolls through the helper.
		auto Roll = [PC]()
		{
			TArray<int32> Values;
			for (int32 Index = 0; Index < 8; ++Index)
			{
				Values.Add(UGSRunRandom::RandRange(PC, 0, 1000000));
			}
			return Values;
		};
		GameMode->RestartRunWithSeed(4242);
		const TArray<int32> First = Roll();
		GameMode->RestartRunWithSeed(4242);
		const TArray<int32> Second = Roll();
		TestTrue(TEXT("Same seed replays the same rolls"), First == Second);
		TestEqual(TEXT("Seed replicated on GameState"), GameState->GetRunSeed(), 4242);
		GameMode->RestartRunWithSeed(4243);
		TestFalse(TEXT("A different seed gives different rolls"), Roll() == First);

		// Restart resets stats.
		GameMode->RestartRunWithSeed(4242);
		TestEqual(TEXT("Kills reset"), Stats->GetTotalKills(), 0);
		TestEqual(TEXT("Rounds cleared reset"), Stats->GetRoundsCleared(), 0);
		TestEqual(TEXT("Round restarts at 1"), GameState->GetRoundNumber(), 1);

		// Kill + damage dealt (player is the source).
		AGSEnemyCharacter* Enemy = SpawnTestEnemy();
		UAbilitySystemComponent* EnemyASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		if (TestNotNull(TEXT("Spawned enemy"), EnemyASC))
		{
			const float EnemyMax = GetMaxHealth(EnemyASC);
			ApplyDamage(EnemyASC, EnemyMax + 100.f, GetPlayerASC());
			TestEqual(TEXT("Kill counted"), Stats->GetTotalKills(), 1);
			TestEqual(TEXT("Kill counted per type"), Stats->GetKillsOf(TEXT("MeleeBandit")), 1);
			TestEqual(TEXT("Damage dealt = health removed (no overkill)"), Stats->GetDamageDealt(), EnemyMax, 0.01f);
		}

		// Damage taken.
		ApplyDamage(GetPlayerASC(), 10.f);
		TestEqual(TEXT("Damage taken"), Stats->GetDamageTaken(), 10.f, 0.01f);

		// Ending combat clears the round.
		GameMode->AdvancePhase(); // PreRound -> Combat
		GameMode->AdvancePhase(); // Combat -> RoundClear
		TestEqual(TEXT("Round cleared counted"), Stats->GetRoundsCleared(), 1);
		TestTrue(TEXT("Run time is running"), Stats->GetRunTime() >= 0.f);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Spawn location finder
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSSpawnLocationFinderTest, "BanditBaller.Spawn.LocationFinder", GSTests::TestFlags)
bool FGSSpawnLocationFinderTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		UWorld* World = GetPIEWorld();
		AGSPlayerController* PC = GetPC();
		const APawn* Player = GetPlayer();
		const UGSSpawnSettings& Settings = UGSSpawnLocationFinder::GetSettings(World);
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!TestNotNull(TEXT("Player"), Player) || !TestNotNull(TEXT("NavSys"), NavSys))
		{
			return;
		}

		FGSSpawnSearchStats Stats;
		const TArray<FGSSpawnPoint> Points = UGSSpawnLocationFinder::FindSpawnLocations(PC, 12, &Stats);
		TestEqual(TEXT("All 12 points found at the town center"), Points.Num(), 12);
		TestEqual(TEXT("Stats add up"), Stats.Ring + Stats.Fallback + Stats.Failed, 12);
		TestTrue(TEXT("Open town center mostly uses the ring"), Stats.Ring >= 10);

		FNavLocation PlayerNav;
		NavSys->ProjectPointToNavigation(Player->GetActorLocation(), PlayerNav, FVector(300.f, 300.f, 1000.f));
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FGSSpawnPoint& Point = Points[Index];
			const FString Name = FString::Printf(TEXT("Point %d"), Index);
			FNavLocation OnNav;
			TestTrue(Name + TEXT(" is on the NavMesh"), NavSys->ProjectPointToNavigation(Point.Location, OnNav, FVector(10.f, 10.f, 100.f)));
			const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Point.Location, PlayerNav.Location);
			TestTrue(Name + TEXT(" is reachable"), Path && Path->IsValid() && !Path->IsPartial());
			if (!Point.bFallback)
			{
				const float Distance = FVector::Dist2D(Point.Location, Player->GetActorLocation());
				TestTrue(Name + TEXT(" is inside the ring"), Distance >= Settings.MinRadius - 1.f && Distance <= Settings.MaxRadius + 1.f);
			}
			for (int32 Other = 0; Other < Index; ++Other)
			{
				TestTrue(Name + TEXT(" keeps MinSpacing"), FVector::Dist2D(Point.Location, Points[Other].Location) >= Settings.MinSpacing - 1.f);
			}
		}

		// Same seed -> same points.
		GetGameMode()->RestartRunWithSeed(99);
		const TArray<FGSSpawnPoint> A = UGSSpawnLocationFinder::FindSpawnLocations(PC, 5);
		GetGameMode()->RestartRunWithSeed(99);
		const TArray<FGSSpawnPoint> B = UGSSpawnLocationFinder::FindSpawnLocations(PC, 5);
		bool bSame = A.Num() == B.Num();
		for (int32 Index = 0; bSame && Index < A.Num(); ++Index)
		{
			bSame = A[Index].Location.Equals(B[Index].Location, 0.1);
		}
		TestTrue(TEXT("Same run seed picks the same points"), bSame);

		TestTrue(TEXT("Travel to the Market District starts"), GetGameMode()->TravelToMarketDistrict());
	});
	WaitUntil(this, TEXT("party in the Market District"), []()
	{
		return GetGameState() && GetGameState()->GetCurrentArea() == EGSArea::MarketDistrict;
	});
	Step([this]()
	{
		FGSSpawnSearchStats Stats;
		TestEqual(TEXT("No spawn points while in the Market District"), UGSSpawnLocationFinder::FindSpawnLocations(GetPC(), 5, &Stats).Num(), 0);
		GetGameMode()->ReturnToTown();
	});
	WaitUntil(this, TEXT("party back in town"), []()
	{
		return GetGameState() && GetGameState()->GetCurrentArea() == EGSArea::Town;
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Wave director
// ---------------------------------------------------------------------------

namespace GSTests
{
	/** Kill every living enemy with the player as the source (counts as player kills). */
	int32 KillAllEnemiesAsPlayer()
	{
		int32 Count = 0;
		TArray<AGSEnemyCharacter*> Targets;
		for (TActorIterator<AGSEnemyCharacter> It(GetPIEWorld()); It; ++It)
		{
			const UGSHealthComponent* Health = It->GetHealthComponent();
			if (!Health || !Health->IsDead())
			{
				Targets.Add(*It);
			}
		}
		for (AGSEnemyCharacter* Enemy : Targets)
		{
			ApplyDamage(Enemy->GetAbilitySystemComponent(), 100000.f, GetPlayerASC());
			++Count;
		}
		return Count;
	}

	int32 CountLivingEnemies()
	{
		int32 Count = 0;
		for (TActorIterator<AGSEnemyCharacter> It(GetPIEWorld()); It; ++It)
		{
			const UGSHealthComponent* Health = It->GetHealthComponent();
			Count += (!Health || !Health->IsDead()) ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSWaveDirectorTest, "BanditBaller.Round.WaveDirector", GSTests::TestFlags)
bool FGSWaveDirectorTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	TSharedRef<int32> Expected = MakeShared<int32>(0);
	Step([this, Expected]()
	{
		AGSGameMode* GameMode = GetGameMode();
		UGSWaveDirector* Director = GameMode ? GameMode->GetWaveDirector() : nullptr;
		if (!TestNotNull(TEXT("Wave director"), Director))
		{
			return;
		}
		const FGSWaveRow* Row = Director->FindWaveRow(1);
		if (!TestNotNull(TEXT("DT_Waves has a Round1 row"), Row))
		{
			return;
		}
		for (const FGSWaveEntry& Entry : Row->Enemies)
		{
			*Expected += Entry.Count;
		}

		GameMode->RestartRunWithSeed(5);   // round 1, PreRound
		GameMode->AdvancePhase();          // -> Combat: wave starts
		TestTrue(TEXT("Combat started a wave"), Director->IsWaveActive());
		TestEqual(TEXT("Wave size matches DT_Waves Round1"), Director->GetTotal(), *Expected);
		TestTrue(TEXT("First batch chosen immediately (telegraphing)"), Director->GetPending() > 0 || Director->GetSpawned() > 0);
		TestEqual(TEXT("GameState mirrors the wave total"), GetGameState()->GetWaveEnemiesTotal(), *Expected);
	});

	// Kill enemies as they arrive; Combat must end on its own once the whole wave is dead.
	WaitUntil(this, TEXT("wave cleared -> RoundClear"), []()
	{
		KillAllEnemiesAsPlayer();
		return IsPhase(EGSRoundPhase::RoundClear);
	}, 60.0);
	Step([this, Expected]()
	{
		UGSWaveDirector* Director = GetGameMode()->GetWaveDirector();
		TestFalse(TEXT("Wave finished"), Director->IsWaveActive());
		TestEqual(TEXT("Every wave enemy was killed"), Director->GetKilled(), *Expected);
		TestEqual(TEXT("Nothing remaining"), GetGameState()->GetWaveEnemiesRemaining(), 0);
		TestEqual(TEXT("Kills recorded in run stats"), GetGameState()->GetRunStats()->GetTotalKills(), *Expected);

		// Skipping Combat mid-wave removes the leftovers.
		GetGameMode()->SetRound(2);
		GetGameMode()->AdvancePhase(); // Combat
		TestTrue(TEXT("Round 2 wave started its first batch"), GetGameMode()->GetWaveDirector()->GetPending() > 0 || CountLivingEnemies() > 0);
		GetGameMode()->AdvancePhase(); // skip -> RoundClear
	});
	WaitUntil(this, TEXT("leftovers removed after skipping combat"), []() { return CountLivingEnemies() == 0; }, 5.0);
	Step([this]()
	{
		TestFalse(TEXT("Wave stopped by the skip"), GetGameMode()->GetWaveDirector()->IsWaveActive());
		TestEqual(TEXT("GameState wave counts reset"), GetGameState()->GetWaveEnemiesTotal(), 0);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Wave budget, alive cap, spawn telegraph
// ---------------------------------------------------------------------------

namespace GSTests
{
	int32 CountTelegraphs()
	{
		int32 Count = 0;
		for (TActorIterator<AGSSpawnTelegraph> It(GetPIEWorld()); It; ++It)
		{
			++Count;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSWaveBudgetTest, "BanditBaller.Round.WaveBudget", GSTests::TestFlags)
bool FGSWaveBudgetTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		UGSWaveDirector* Director = GameMode->GetWaveDirector();
		const UGSWaveSettings& Settings = Director->GetSettings();
		if (!TestTrue(TEXT("DA_WaveSettings has an enemy pool"), Settings.EnemyPool.Num() > 0))
		{
			return;
		}
		TestTrue(TEXT("DT_Waves defines some rounds"), Director->GetLastTableRound() > 0);

		// Purchase respects the budget, unlocks and the seed.
		const int32 Round = Director->GetLastTableRound() + 10;
		const float Budget = Settings.GetBudget(Round);
		float CheapestUnlocked = TNumericLimits<float>::Max();
		for (const UGSEnemyData* Data : Settings.EnemyPool)
		{
			if (Data && Data->MinRound <= Round)
			{
				CheapestUnlocked = FMath::Min(CheapestUnlocked, Data->SpawnCost);
			}
		}

		TArray<UGSEnemyData*> First, Second;
		float Left = 0.f;
		GameMode->RestartRunWithSeed(31);
		UGSWaveDirector::PurchaseWave(GetPC(), Settings.EnemyPool, Budget, Round, Settings.MaxGeneratedEnemies, First, &Left);
		GameMode->RestartRunWithSeed(31);
		UGSWaveDirector::PurchaseWave(GetPC(), Settings.EnemyPool, Budget, Round, Settings.MaxGeneratedEnemies, Second);
		TestTrue(TEXT("Same seed buys the same wave"), First == Second);
		TestTrue(TEXT("Generated wave is not empty"), First.Num() > 0);

		float Spent = 0.f;
		for (const UGSEnemyData* Data : First)
		{
			Spent += Data->SpawnCost;
			TestTrue(TEXT("Only unlocked enemies are bought"), Data->MinRound <= Round);
		}
		TestTrue(TEXT("Spending stays within the budget"), Spent <= Budget + 0.01f);
		TestTrue(TEXT("Leftover budget can't buy anything else"), Left < CheapestUnlocked);

		// MinRound: round 1 can't buy anything locked behind later rounds.
		TArray<UGSEnemyData*> RoundOne;
		UGSWaveDirector::PurchaseWave(GetPC(), Settings.EnemyPool, 100.f, 1, 200, RoundOne);
		for (const UGSEnemyData* Data : RoundOne)
		{
			TestTrue(TEXT("Round 1 purchase only uses MinRound 1 enemies"), Data->MinRound <= 1);
		}

		// Alive cap + telegraph on a generated round, with a small cap so it is hit quickly. God mode keeps the
		// player alive (spawning needs a living player).
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		IConsoleManager::Get().FindConsoleVariable(TEXT("gs.Wave.MaxAlive"))->Set(4);
		GameMode->SetRound(Round);
		GameMode->AdvancePhase(); // Combat
		TestTrue(TEXT("Generated wave started"), Director->IsWaveActive() && Director->GetTotal() > 4);
		TestTrue(TEXT("First batch shows telegraphs before spawning"), Director->GetPending() > 0 && CountTelegraphs() > 0);
		TestEqual(TEXT("Nothing spawned during the telegraph"), Director->GetSpawned(), 0);
	});

	// Run for a while: alive + pending never exceeds the cap; the cap is reached and holds.
	TSharedRef<bool> bCapRespected = MakeShared<bool>(true);
	WaitUntil(this, TEXT("alive cap reached"), [bCapRespected]()
	{
		UGSWaveDirector* Director = GetGameMode()->GetWaveDirector();
		if (Director->GetAlive() + Director->GetPending() > Director->GetMaxAlive())
		{
			*bCapRespected = false;
		}
		return Director->GetAlive() == Director->GetMaxAlive();
	}, 30.0);
	Step([this, bCapRespected]()
	{
		UGSWaveDirector* Director = GetGameMode()->GetWaveDirector();
		TestTrue(TEXT("Alive + telegraphing never exceeded the cap"), *bCapRespected);
		TestEqual(TEXT("Alive enemies at the cap"), Director->GetAlive(), 4);
		TestTrue(TEXT("Remaining spawns are waiting"), Director->GetSpawned() < Director->GetTotal());
		TestEqual(TEXT("Telegraphs are cleaned up after spawning"), CountTelegraphs(), Director->GetPending());

		// Killing frees room for the waiting spawns.
		KillAllEnemiesAsPlayer();
	});
	WaitUntil(this, TEXT("waiting spawns fill the freed room"), []()
	{
		return GetGameMode()->GetWaveDirector()->GetAlive() > 0;
	}, 30.0);
	Step([]()
	{
		IConsoleManager::Get().FindConsoleVariable(TEXT("gs.Wave.MaxAlive"))->Set(0);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Enemy difficulty scaling
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSDifficultyScalingTest, "BanditBaller.Round.DifficultyScaling", GSTests::TestFlags)
bool FGSDifficultyScalingTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		UGSEnemyData* Melee = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		UGSEnemyData* Ranged = UGSDebugLibrary::FindEnemyData(TEXT("RangedBandit"));
		if (!TestNotNull(TEXT("MeleeBandit data"), Melee) || !TestNotNull(TEXT("RangedBandit data"), Ranged))
		{
			return;
		}

		auto CheckEnemy = [this](const FString& What, UGSEnemyData* Data, float HealthMult, float DamageMult)
		{
			TArray<AGSEnemyCharacter*> Spawned = UGSDebugLibrary::SpawnEnemiesAround(GetPC(), Data, 1);
			UAbilitySystemComponent* ASC = Spawned.Num() > 0 ? Spawned[0]->GetAbilitySystemComponent() : nullptr;
			if (!TestNotNull(What + TEXT(" spawned"), ASC))
			{
				return;
			}
			const float ExpectedMax = Data->MaxHealth * HealthMult;
			TestEqual(What + TEXT(" MaxHealth = base x curve"), GetMaxHealth(ASC), ExpectedMax, 0.01f);
			TestEqual(What + TEXT(" spawns at full health"), GetHealth(ASC), ExpectedMax, 0.01f);
			TestEqual(What + TEXT(" Damage = base x curve"), ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute()), Data->DamageMultiplier * DamageMult, 0.001f);
			// Base values untouched: the scaling lives in the GE modifier, not in overwritten stats.
			TestEqual(What + TEXT(" MaxHealth base value unchanged"), ASC->GetNumericAttributeBase(UGSAttributeSet::GetMaxHealthAttribute()), Data->MaxHealth, 0.01f);
		};

		// Round 1.
		GameMode->SetRound(1);
		const float Health1 = GameMode->GetEnemyHealthMultiplier(1);
		const float Damage1 = GameMode->GetEnemyDamageMultiplier(1);
		TestEqual(TEXT("GameState shows round 1 HP multiplier"), GameState->GetEnemyHealthMultiplier(), Health1, 0.001f);
		CheckEnemy(TEXT("Round 1 melee"), Melee, Health1, Damage1);

		// Round 10: tougher.
		GameMode->SetRound(10);
		const float Health10 = GameMode->GetEnemyHealthMultiplier(10);
		const float Damage10 = GameMode->GetEnemyDamageMultiplier(10);
		TestTrue(TEXT("DA_DifficultyCurve ramps health by round 10"), Health10 > Health1);
		TestTrue(TEXT("DA_DifficultyCurve ramps damage by round 10"), Damage10 > Damage1);
		TestEqual(TEXT("GameState shows round 10 HP multiplier"), GameState->GetEnemyHealthMultiplier(), Health10, 0.001f);
		TestEqual(TEXT("GameState shows round 10 DMG multiplier"), GameState->GetEnemyDamageMultiplier(), Damage10, 0.001f);
		CheckEnemy(TEXT("Round 10 melee"), Melee, Health10, Damage10);
		CheckEnemy(TEXT("Round 10 ranged"), Ranged, Health10, Damage10);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Balance report: time-to-kill through the real weapon fire path
// ---------------------------------------------------------------------------

namespace GSTests
{
	struct FTTKCase
	{
		int32 Round = 1;
		FString Enemy;
		FString Weapon;
	};

	struct FTTKState
	{
		TArray<FTTKCase> Cases;
		int32 CaseIndex = 0;
		enum class EStep : uint8 { Setup, Aim, Fire } Step = EStep::Setup;
		TWeakObjectPtr<AGSEnemyCharacter> Target;
		int32 AimFrames = 0;
		int32 AmmoBefore = 0;
		float TargetMaxHealth = 0.f;
		double CaseStartTime = 0.0;
		double FirstHitTime = -1.0;
		TArray<FString> Report;
	};

	/** Point the local player's view at Target's chest. */
	void AimAt(const AActor& Target)
	{
		AGSPlayerController* PC = GetPC();
		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
		PC->SetControlRotation((Target.GetActorLocation() + FVector(0.f, 0.f, 30.f) - ViewLocation).Rotation());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSTimeToKillTest, "BanditBaller.Balance.TimeToKill", GSTests::TestFlags)
bool FGSTimeToKillTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	TSharedRef<FTTKState> State = MakeShared<FTTKState>();
	for (const int32 Round : { 1, 5, 10 })
	{
		for (const TCHAR* Enemy : { TEXT("MeleeBandit"), TEXT("RangedBandit") })
		{
			for (const TCHAR* Weapon : { TEXT("Rifle"), TEXT("Pistol") })
			{
				State->Cases.Add({ Round, Enemy, Weapon });
			}
		}
	}

	Step([]()
	{
		UGSDebugLibrary::SetGodMode(GetPC(), true);
	});

	// One case at a time: freeze a bandit 4 m in front of the player, aim at it every frame and press fire every
	// frame (the weapon's own refire limit sets the pace). Measures shots fired and first-hit-to-death time.
	WaitUntil(this, TEXT("all time-to-kill cases"), [this, State]()
	{
		UWorld* World = GetPIEWorld();
		AGSGameMode* GameMode = GetGameMode();
		AGSCharacter* Player = GetPlayer();
		if (!World || !GameMode || !Player)
		{
			return false;
		}
		const double Now = World->GetTimeSeconds();
		if (State->CaseIndex >= State->Cases.Num())
		{
			return true;
		}
		const FTTKCase& Case = State->Cases[State->CaseIndex];
		UGSWeaponComponent* Weapons = Player->GetWeaponComponent();

		switch (State->Step)
		{
		case FTTKState::EStep::Setup:
		{
			GameMode->SetRound(Case.Round);
			GameMode->SetRoundTimerPaused(true);
			RemoveAllEnemies(World);

			UGSWeaponData* WeaponData = UGSDebugLibrary::FindWeaponData(Case.Weapon);
			UGSDebugLibrary::GiveWeaponTo(GetPC(), WeaponData);

			Player->TeleportTo(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);
			const FVector EnemyNav(400.f, 0.f, 0.f);
			AGSEnemyCharacter* Enemy = AGSEnemyCharacter::SpawnEnemy(World, UGSDebugLibrary::FindEnemyData(Case.Enemy), EnemyNav, Player->GetActorLocation());
			if (!TestNotNull(FString::Printf(TEXT("Spawned %s"), *Case.Enemy), Enemy))
			{
				++State->CaseIndex;
				return false;
			}
			// Freeze it: no AI, no movement.
			if (AAIController* AIC = Cast<AAIController>(Enemy->GetController()))
			{
				if (UBrainComponent* Brain = AIC->GetBrainComponent())
				{
					Brain->StopLogic(TEXT("TTK test"));
				}
				AIC->StopMovement();
			}
			Enemy->GetCharacterMovement()->DisableMovement();

			State->Target = Enemy;
			State->TargetMaxHealth = GetMaxHealth(Enemy->GetAbilitySystemComponent());
			State->AimFrames = 0;
			State->FirstHitTime = -1.0;
			State->CaseStartTime = Now;
			State->Step = FTTKState::EStep::Aim;
			return false;
		}

		case FTTKState::EStep::Aim:
		{
			AGSEnemyCharacter* Enemy = State->Target.Get();
			if (!Enemy)
			{
				State->Step = FTTKState::EStep::Setup;
				++State->CaseIndex;
				return false;
			}
			AimAt(*Enemy);
			if (++State->AimFrames >= 5) // let the camera settle on the new view
			{
				State->AmmoBefore = Weapons->GetCurrentAmmo();
				State->Step = FTTKState::EStep::Fire;
			}
			return false;
		}

		case FTTKState::EStep::Fire:
		{
			AGSEnemyCharacter* Enemy = State->Target.Get();
			UAbilitySystemComponent* EnemyASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
			const bool bDead = !Enemy || !EnemyASC || Enemy->GetHealthComponent()->IsDead();
			if (!bDead)
			{
				AimAt(*Enemy);
				UGSAbilitySystemComponent* ASC = GetPlayerASC();
				ASC->AbilityInputTagPressed(GSGameplayTags::Ability_Fire);
				ASC->AbilityInputTagReleased(GSGameplayTags::Ability_Fire);
				if (State->FirstHitTime < 0.0 && GetHealth(EnemyASC) < State->TargetMaxHealth)
				{
					State->FirstHitTime = Now;
				}
			}

			const bool bNowDead = !Enemy || Enemy->GetHealthComponent()->IsDead();
			const bool bTimedOut = Now - State->CaseStartTime > 10.0;
			if (bNowDead || bTimedOut)
			{
				const UGSWeaponData* WeaponData = Weapons->GetCurrentWeaponData();
				const int32 Shots = State->AmmoBefore - Weapons->GetCurrentAmmo();
				const int32 Expected = FMath::CeilToInt(State->TargetMaxHealth / WeaponData->Damage);
				const float IdealTime = (Expected - 1) * WeaponData->GetRefireInterval(1.f);
				const float Measured = State->FirstHitTime >= 0.0 ? static_cast<float>(Now - State->FirstHitTime) : -1.f;

				const FString Line = FString::Printf(TEXT("[ttk] Round %2d  %-12s %5.0f HP  %-6s (%2.0f dmg): %d shots (expected %d), %.2f s measured, %.2f s ideal%s"),
					Case.Round, *Case.Enemy, State->TargetMaxHealth, *Case.Weapon, WeaponData->Damage, Shots, Expected, Measured, IdealTime,
					bTimedOut ? TEXT("  TIMED OUT") : TEXT(""));
				UE_LOG(LogBanditBaller, Log, TEXT("%s"), *Line);
				AddInfo(Line);
				TestFalse(FString::Printf(TEXT("Round %d %s vs %s did not time out"), Case.Round, *Case.Enemy, *Case.Weapon), bTimedOut);
				TestEqual(FString::Printf(TEXT("Round %d %s vs %s: shots to kill"), Case.Round, *Case.Enemy, *Case.Weapon), Shots, Expected);

				++State->CaseIndex;
				State->Step = FTTKState::EStep::Setup;
			}
			return false;
		}
		}
		return false;
	}, 180.0);

	Step([]()
	{
		GetGameMode()->SetRoundTimerPaused(false);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
	});
	EndPIE();
	return true;
}


// ---------------------------------------------------------------------------
// Economy: cash pickups
// ---------------------------------------------------------------------------

namespace GSTests
{
	UGSCashPickupSubsystem* GetCashSubsystem()
	{
		UWorld* World = GetPIEWorld();
		return World ? World->GetSubsystem<UGSCashPickupSubsystem>() : nullptr;
	}

	int32 GetPlayerCash()
	{
		const AGSPlayerController* PC = GetPC();
		const AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
		return PS ? PS->GetCash() : -1;
	}

	int32 GetCashEarnedStat()
	{
		const AGSGameState* GS = GetGameState();
		return GS && GS->GetRunStats() ? GS->GetRunStats()->GetCashEarned() : -1;
	}

	int32 SumActiveCash()
	{
		int32 Sum = 0;
		for (const AGSCashPickup* Pickup : GetCashSubsystem()->GetActivePickups())
		{
			Sum += Pickup ? Pickup->GetValue() : 0;
		}
		return Sum;
	}

	int32 CountMagnetizedCash()
	{
		int32 Count = 0;
		for (const AGSCashPickup* Pickup : GetCashSubsystem()->GetActivePickups())
		{
			Count += (Pickup && Pickup->IsMagnetized()) ? 1 : 0;
		}
		return Count;
	}

	/** Spawn a frozen MeleeBandit at Location and kill it as the player, so it drops its CashValue. */
	bool KillEnemyForCash(FAutomationTestBase* Test, const FVector& Location, FVector& OutDropOrigin, int32& OutCashValue)
	{
		UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		AGSEnemyCharacter* Enemy = Data ? AGSEnemyCharacter::SpawnEnemy(GetPIEWorld(), Data, Location, GetPlayer()->GetActorLocation()) : nullptr;
		if (!Test->TestNotNull(TEXT("Spawned a MeleeBandit to drop cash"), Enemy))
		{
			return false;
		}
		if (AAIController* AIC = Cast<AAIController>(Enemy->GetController()))
		{
			if (UBrainComponent* Brain = AIC->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("Cash test"));
			}
			AIC->StopMovement();
		}
		Enemy->GetCharacterMovement()->DisableMovement();

		OutDropOrigin = Enemy->GetActorLocation();
		OutCashValue = Data->CashValue;
		ApplyDamage(Enemy->GetAbilitySystemComponent(), 100000.f, GetPlayerASC());
		return Test->TestTrue(TEXT("Cash test enemy died"), Enemy->GetHealthComponent()->IsDead());
	}

	/** Enqueue: wait Seconds of PIE world time. */
	void WaitWorldSeconds(FAutomationTestBase* Test, double Seconds)
	{
		TSharedRef<double> End = MakeShared<double>(-1.0);
		WaitUntil(Test, FString::Printf(TEXT("%.1f s of game time"), Seconds), [End, Seconds]()
		{
			const UWorld* World = GetPIEWorld();
			if (!World)
			{
				return true;
			}
			if (*End < 0.0)
			{
				*End = World->GetTimeSeconds() + Seconds;
			}
			return World->GetTimeSeconds() >= *End;
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSCashPickupsTest, "BanditBaller.Economy.CashPickups", GSTests::TestFlags)
bool FGSCashPickupsTest::RunTest(const FString& Parameters)
{
	struct FCashState
	{
		FVector PlayerHome = FVector(0.f, 0.f, 100.f);
		FVector EnemyAt = FVector(1000.f, 0.f, 0.f);
		FVector DropOrigin = FVector::ZeroVector;
		int32 CashValue = 0;
		int32 Pieces = 0;
		int32 CashBefore = 0;
		int32 EarnedBefore = 0;
		int32 CollectedBefore = 0;
		int32 CreatedBefore = 0;
		int32 ExpiredBefore = 0;
		bool bSawBlinking = false;
	};
	TSharedRef<FCashState> State = MakeShared<FCashState>();

	BeginPIE(this);

	// 1) Split rules + a drop that lands out of reach.
	Step([this, State]()
	{
		AGSGameMode* GameMode = GetGameMode();
		UGSCashPickupSubsystem* Cash = GetCashSubsystem();
		const UGSCashSettings* Settings = GameMode ? GameMode->GetCashSettings() : nullptr;
		if (!TestNotNull(TEXT("Cash subsystem"), Cash) || !TestNotNull(TEXT("BP_GSGameMode has CashSettings (DA_CashSettings)"), Settings))
		{
			return;
		}
		GameMode->SetRound(1);
		GameMode->SetRoundTimerPaused(true);
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		GetPlayer()->TeleportTo(State->PlayerHome, FRotator::ZeroRotator);

		for (const int32 Total : { 1, 3, 10, 15, 37, 500 })
		{
			TArray<int32> Pieces;
			UGSCashPickupSubsystem::SplitValue(*Settings, Total, Pieces);
			int32 Sum = 0;
			bool bAllPositive = true;
			for (const int32 Piece : Pieces)
			{
				Sum += Piece;
				bAllPositive &= Piece >= 1;
			}
			TestEqual(FString::Printf(TEXT("Split $%d: pieces sum to the value"), Total), Sum, Total);
			TestTrue(FString::Printf(TEXT("Split $%d: every piece worth >= 1"), Total), bAllPositive);
			TestTrue(FString::Printf(TEXT("Split $%d: %d pieces within [min(%d, value), %d]"), Total, Pieces.Num(), Settings->MinPieces, Settings->MaxPieces),
				Pieces.Num() >= FMath::Min(Settings->MinPieces, Total) && Pieces.Num() <= Settings->MaxPieces);
		}

		const float Radius = GetPlayerASC()->GetNumericAttribute(UGSAttributeSet::GetPickupRadiusAttribute());
		TestTrue(FString::Printf(TEXT("Player starts with a PickupRadius (%.0f cm)"), Radius), Radius > 0.f);
		TestTrue(TEXT("Test drop lands outside the pickup radius"), Radius < 1000.f - Settings->ScatterRadiusMax - 100.f);

		State->CashBefore = GetPlayerCash();
		State->EarnedBefore = GetCashEarnedStat();
		State->CollectedBefore = Cash->GetNumCollected();

		if (!KillEnemyForCash(this, State->EnemyAt, State->DropOrigin, State->CashValue))
		{
			return;
		}
		TArray<int32> Expected;
		UGSCashPickupSubsystem::SplitValue(*Settings, State->CashValue, Expected);
		State->Pieces = Expected.Num();
		TestTrue(TEXT("Enemy has a CashValue"), State->CashValue > 0);
		TestEqual(TEXT("Drop spawned the split's piece count"), Cash->GetNumActive(), State->Pieces);
		TestEqual(TEXT("Drop pieces add up to the enemy's CashValue"), SumActiveCash(), State->CashValue);
		TestTrue(TEXT("Pool prewarmed at least PoolPrewarm pickups"), Cash->GetNumCreated() >= Settings->PoolPrewarm);
	});
	WaitWorldSeconds(this, 1.0);
	Step([this, State]()
	{
		UGSCashPickupSubsystem* Cash = GetCashSubsystem();
		const UGSCashSettings* Settings = GetGameMode()->GetCashSettings();
		TestEqual(TEXT("Out of reach: pieces stay on the ground"), Cash->GetNumActive(), State->Pieces);
		TestEqual(TEXT("Out of reach: nothing flies to the player"), CountMagnetizedCash(), 0);
		TestEqual(TEXT("Out of reach: no cash yet"), GetPlayerCash(), State->CashBefore);
		for (const AGSCashPickup* Pickup : Cash->GetActivePickups())
		{
			const float Flat = FVector::Dist2D(Pickup->GetActorLocation(), State->DropOrigin);
			TestTrue(FString::Printf(TEXT("Piece scattered within %.0f cm of the body (%.0f)"), Settings->ScatterRadiusMax, Flat), Flat <= Settings->ScatterRadiusMax + 1.f);
			TestTrue(TEXT("Piece is visible"), !Pickup->IsHidden());
		}

		// 2) Walk into range (beside the corpse): everything flies in and pays out.
		GetPlayer()->TeleportTo(State->DropOrigin - FVector(120.f, 0.f, 0.f), FRotator::ZeroRotator);
	});
	WaitUntil(this, TEXT("pieces in range to be collected"), []() { return GetCashSubsystem()->GetNumActive() == 0; }, 10.0);
	Step([this, State]()
	{
		UGSCashPickupSubsystem* Cash = GetCashSubsystem();
		TestEqual(TEXT("Collected: player cash += CashValue"), GetPlayerCash(), State->CashBefore + State->CashValue);
		TestEqual(TEXT("Collected: run stats CashEarned += CashValue"), GetCashEarnedStat(), State->EarnedBefore + State->CashValue);
		TestEqual(TEXT("Collected: every piece counted"), Cash->GetNumCollected(), State->CollectedBefore + State->Pieces);
		if (const UGSHUDWidget* HUD = GetPC()->GetGSHUD())
		{
			TestEqual(TEXT("HUD cash counter follows OnCashChanged"), HUD->GetDisplayedCash(), GetPlayerCash());
			TestTrue(TEXT("HUD shows a +$N popup"), HUD->GetActiveCashPopupTotal() > 0);
		}
		else
		{
			AddError(TEXT("No UGSHUDWidget on the player controller"));
		}

		// 3) Pool reuse + RoundClear vacuum.
		GetPlayer()->TeleportTo(State->PlayerHome, FRotator::ZeroRotator);
		State->CreatedBefore = Cash->GetNumCreated();
		State->CashBefore = GetPlayerCash();
		KillEnemyForCash(this, State->EnemyAt, State->DropOrigin, State->CashValue);
		TestEqual(TEXT("Second drop reuses pooled pickups (none created)"), Cash->GetNumCreated(), State->CreatedBefore);
	});
	WaitWorldSeconds(this, 1.0);
	Step([this, State]()
	{
		TestEqual(TEXT("Before RoundClear: pieces wait on the ground"), CountMagnetizedCash(), 0);
		GetGameMode()->SetPhase(EGSRoundPhase::RoundClear);
		TestEqual(TEXT("RoundClear: every piece flies to the player"), CountMagnetizedCash(), GetCashSubsystem()->GetNumActive());
	});
	WaitUntil(this, TEXT("vacuumed pieces to arrive"), []() { return GetCashSubsystem()->GetNumActive() == 0; }, 10.0);
	Step([this, State]()
	{
		TestEqual(TEXT("RoundClear vacuum paid the whole drop"), GetPlayerCash(), State->CashBefore + State->CashValue);

		// 4) Expiry: short lifetime, out of reach.
		IConsoleManager::Get().FindConsoleVariable(TEXT("gs.Cash.Lifetime"))->Set(2.f, ECVF_SetByCode);
		UGSCashPickupSubsystem* Cash = GetCashSubsystem();
		State->ExpiredBefore = Cash->GetNumExpired();
		State->CashBefore = GetPlayerCash();
		KillEnemyForCash(this, State->EnemyAt, State->DropOrigin, State->CashValue);
	});
	WaitUntil(this, TEXT("uncollected pieces to expire"), [State]()
	{
		for (const AGSCashPickup* Pickup : GetCashSubsystem()->GetActivePickups())
		{
			State->bSawBlinking |= Pickup && Pickup->IsBlinking();
		}
		return GetCashSubsystem()->GetNumActive() == 0;
	}, 10.0);
	Step([this, State]()
	{
		UGSCashPickupSubsystem* Cash = GetCashSubsystem();
		TestEqual(TEXT("Expired: every piece counted"), Cash->GetNumExpired(), State->ExpiredBefore + State->Pieces);
		TestEqual(TEXT("Expired: no cash paid"), GetPlayerCash(), State->CashBefore);
		TestTrue(TEXT("Pieces blink before they vanish"), State->bSawBlinking);
		TestEqual(TEXT("Expired pickups are back in the pool"), Cash->GetNumPooled(), Cash->GetNumCreated());

		IConsoleManager::Get().FindConsoleVariable(TEXT("gs.Cash.Lifetime"))->Set(0.f, ECVF_SetByCode);
		GetGameMode()->SetRoundTimerPaused(false);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
	});
	EndPIE();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
