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
#include "GSComboComponent.h"
#include "GSComboSettings.h"
#include "GSRoundClearSettings.h"
#include "GSRoundSummaryWidget.h"
#include "GSTargetDummy.h"
#include "AbilitySystemGlobals.h"
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
#include "GSProgressionComponent.h"
#include "GSXPCurve.h"
#include "GSLevelRewards.h"
#include "GSProfileSaveGame.h"
#include "GSProfileSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GSGameplayEffects.h"
#include "GSGameplayAbility_Reload.h"
#include "GSMovementAbilitySettings.h"
#include "GSMovementSettings.h"
#include "Editor/EditorPerformanceSettings.h"
#include "GSUpgradeComponent.h"
#include "GSUpgradeData.h"
#include "GSUpgradePool.h"
#include "GSCollisionChannels.h"
#include "GSMapBoundary.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GSUpgradeCardWidget.h"
#include "GSUpgradeChoiceWidget.h"
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

	/**
	 * Level-ups open upgrade offers, and an open offer pauses the game. Tests that need the world to keep running after
	 * a level-up take the first choice of every offer (which unpauses). Returns how many were picked.
	 */
	int32 PickAllUpgradeOffers()
	{
		AGSGameMode* GameMode = GetPIEWorld() ? GetPIEWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
		AGSPlayerState* PS = GetPC() ? GetPC()->GetPlayerState<AGSPlayerState>() : nullptr;
		int32 Picked = 0;
		while (GameMode && PS && PS->GetUpgradeComponent()->HasOpenOffer() && GameMode->ChooseUpgrade(PS, 0))
		{
			++Picked;
		}
		return Picked;
	}

	/** Spawns one enemy near the player (server) for target-side tests. */
	AGSEnemyCharacter* SpawnTestEnemy()
	{
		UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		TArray<AGSEnemyCharacter*> Spawned = UGSDebugLibrary::SpawnEnemiesAround(GetPC(), Data, 1);
		return Spawned.Num() > 0 ? Spawned[0] : nullptr;
	}

	/** Spawns a MeleeBandit near the player and kills it as the player (it drops cash and an XP orb). */
	AGSEnemyCharacter* KillTestEnemyAsPlayer(FAutomationTestBase* Test)
	{
		AGSEnemyCharacter* Enemy = SpawnTestEnemy();
		UAbilitySystemComponent* EnemyASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		if (Test->TestNotNull(TEXT("Spawned enemy"), EnemyASC))
		{
			ApplyDamage(EnemyASC, GetMaxHealth(EnemyASC) + 100.f, GetPlayerASC());
		}
		return Enemy;
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

	// The old in-run market phases are gone from the loop.
	Step([this]()
	{
		const UEnum* PhaseEnum = StaticEnum<EGSRoundPhase>();
		TestEqual(TEXT("No MarketDecision phase"), PhaseEnum->GetIndexByNameString(TEXT("MarketDecision")), static_cast<int32>(INDEX_NONE));
		TestEqual(TEXT("No MarketVisit phase"), PhaseEnum->GetIndexByNameString(TEXT("MarketVisit")), static_cast<int32>(INDEX_NONE));

		// Skip Combat: RoundClear.
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		TestTrue(TEXT("Combat uses the fallback timer"),
			FMath::IsNearlyEqual(GameState->GetPhaseTimeRemaining(), GameMode->GetPhaseDuration(EGSRoundPhase::Combat), 1.f));

		GameMode->AdvancePhase();
		TestTrue(TEXT("Combat -> RoundClear"), IsPhase(EGSRoundPhase::RoundClear));
		TestEqual(TEXT("Still round 1 in RoundClear"), GameState->GetRoundNumber(), 1);
		TestTrue(TEXT("RoundClear has an end time"), GameState->GetPhaseEndTime() > 0.0);
	});

	// The RoundClear timer goes straight to the next round's PreRound.
	WaitUntil(this, TEXT("RoundClear timer to start round 2's PreRound"), []()
	{
		const AGSGameState* GameState = GetGameState();
		return GameState && GameState->GetRoundNumber() == 2;
	}, GetDefault<UGSRoundSettings>()->RoundClearDuration + 10.0);

	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		TestTrue(TEXT("RoundClear timer -> PreRound"), IsPhase(EGSRoundPhase::PreRound));

		// Skips walk the same loop: PreRound -> Combat -> RoundClear -> round 3 PreRound.
		GameMode->AdvancePhase();
		TestTrue(TEXT("PreRound -> Combat (round 2)"), IsPhase(EGSRoundPhase::Combat));
		GameMode->AdvancePhase();
		TestTrue(TEXT("Combat -> RoundClear (round 2)"), IsPhase(EGSRoundPhase::RoundClear));
		GameMode->AdvancePhase();
		TestTrue(TEXT("Skipping RoundClear -> PreRound"), IsPhase(EGSRoundPhase::PreRound));
		TestEqual(TEXT("Next round started"), GameState->GetRoundNumber(), 3);

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
		// No XP from the kills: a level-up would pause the game for an upgrade choice (and a pick could change Damage).
		UGSDebugLibrary::SetAttributeOn(GetPC(), TEXT("XPGain"), 0.f);
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

	/** The permanent cash balance (UGSProfileSubsystem of the PIE GameInstance). */
	int32 GetPlayerCash()
	{
		const UGSProfileSubsystem* Profile = UGSProfileSubsystem::Get(GetPIEWorld());
		return Profile ? Profile->GetCash() : -1;
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
			Sum += (Pickup && Pickup->IsCash()) ? Pickup->GetValue() : 0;
		}
		return Sum;
	}

	UGSComboComponent* GetPlayerCombo()
	{
		const AGSPlayerState* PS = GetPC() ? GetPC()->GetPlayerState<AGSPlayerState>() : nullptr;
		return PS ? PS->GetComboComponent() : nullptr;
	}

	int32 CountMagnetizedCash()
	{
		int32 Count = 0;
		for (const AGSCashPickup* Pickup : GetCashSubsystem()->GetActivePickups())
		{
			Count += (Pickup && Pickup->IsCash() && Pickup->IsMagnetized()) ? 1 : 0;
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
		// The drop is worth exactly CashValue: end any kill combo an earlier kill of the test started.
		if (UGSComboComponent* Combo = GetPlayerCombo())
		{
			Combo->EndCombo(EGSComboEndReason::Reset);
		}
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
		TestEqual(TEXT("Drop spawned the split's piece count"), Cash->GetNumActiveOfKind(EGSPickupKind::Cash), State->Pieces);
		TestEqual(TEXT("Drop pieces add up to the enemy's CashValue"), SumActiveCash(), State->CashValue);
		TestTrue(TEXT("Pool prewarmed at least PoolPrewarm pickups"), Cash->GetNumCreated() >= Settings->PoolPrewarm);
	});
	WaitWorldSeconds(this, 1.0);
	Step([this, State]()
	{
		UGSCashPickupSubsystem* Cash = GetCashSubsystem();
		const UGSCashSettings* Settings = GetGameMode()->GetCashSettings();
		TestEqual(TEXT("Out of reach: pieces stay on the ground"), Cash->GetNumActiveOfKind(EGSPickupKind::Cash), State->Pieces);
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
	WaitUntil(this, TEXT("pieces in range to be collected"), []() { return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) == 0; }, 10.0);
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
		TestEqual(TEXT("RoundClear: every piece flies to the player"), CountMagnetizedCash(), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash));
	});
	WaitUntil(this, TEXT("vacuumed pieces to arrive"), []() { return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) == 0; }, 10.0);
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
		return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) == 0;
	}, 10.0);
	Step([this, State]()
	{
		UGSCashPickupSubsystem* Cash = GetCashSubsystem();
		TestEqual(TEXT("Expired: every piece counted"), Cash->GetNumExpired(), State->ExpiredBefore + State->Pieces);
		TestEqual(TEXT("Expired: no cash paid"), GetPlayerCash(), State->CashBefore);
		TestTrue(TEXT("Pieces blink before they vanish"), State->bSawBlinking);
		// The kill's XP orb never expires, so it is still out; everything else is back in the pool.
		TestEqual(TEXT("Expired pickups are back in the pool (XP orbs stay)"), Cash->GetNumPooled() + Cash->GetNumActiveOfKind(EGSPickupKind::XP), Cash->GetNumCreated());

		IConsoleManager::Get().FindConsoleVariable(TEXT("gs.Cash.Lifetime"))->Set(0.f, ECVF_SetByCode);
		GetGameMode()->SetRoundTimerPaused(false);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// XP orbs
// ---------------------------------------------------------------------------

namespace GSTests
{
	AGSCashPickup* FindActiveXPOrb()
	{
		for (AGSCashPickup* Pickup : GetCashSubsystem()->GetActivePickups())
		{
			if (Pickup && Pickup->IsXPOrb())
			{
				return Pickup;
			}
		}
		return nullptr;
	}

	int32 GetPlayerTotalXP()
	{
		const AGSPlayerState* PS = GetPC() ? GetPC()->GetPlayerState<AGSPlayerState>() : nullptr;
		return PS ? PS->GetProgressionComponent()->GetTotalXP() : -1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSXPOrbsTest, "BanditBaller.Economy.XPOrbs", GSTests::TestFlags)
bool FGSXPOrbsTest::RunTest(const FString& Parameters)
{
	struct FOrbState
	{
		FVector PlayerHome = FVector(0.f, 0.f, 100.f);
		FVector EnemyAt = FVector(1000.f, 0.f, 0.f);
		FVector DropOrigin = FVector::ZeroVector;
		int32 CashValue = 0;
		int32 XPValue = 0;
		int32 ExpectedXP = 0;
		int32 TotalBefore = 0;
		int32 EarnedBefore = 0;
		int32 CollectedBefore = 0;
	};
	TSharedRef<FOrbState> S = MakeShared<FOrbState>();

	BeginPIE(this);

	// A kill drops one orb out of reach; no XP yet.
	Step([this, S]()
	{
		AGSGameMode* GameMode = GetGameMode();
		const UGSCashSettings* Settings = GameMode->GetCashSettings();
		if (!TestNotNull(TEXT("Pickup settings (DA_CashSettings)"), Settings))
		{
			return;
		}
		GameMode->SetRoundTimerPaused(true);
		GameMode->RestartRunWithSeed(41);
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		GetPlayer()->TeleportTo(S->PlayerHome, FRotator::ZeroRotator);
		GetPlayerASC()->SetNumericAttributeBase(UGSAttributeSet::GetXPGainAttribute(), 1.5f);
		IConsoleManager::Get().FindConsoleVariable(TEXT("gs.Cash.Lifetime"))->Set(1.5f, ECVF_SetByCode);
		TestEqual(TEXT("XP orbs never expire by default"), Settings->XPOrbLifetime, 0.f);

		const UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		S->XPValue = Data ? Data->XPValue : 0;
		S->ExpectedXP = FMath::RoundToInt(S->XPValue * 1.5f);
		S->TotalBefore = GetPlayerTotalXP();
		S->EarnedBefore = GetGameState()->GetRunStats()->GetXPEarned();
		S->CollectedBefore = GetCashSubsystem()->GetNumXPOrbsCollected();
		if (!KillEnemyForCash(this, S->EnemyAt, S->DropOrigin, S->CashValue))
		{
			return;
		}
		TestEqual(TEXT("Kill grants no XP"), GetPlayerTotalXP(), S->TotalBefore);
		TestEqual(TEXT("One XP orb"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP), 1);
		const AGSCashPickup* Orb = FindActiveXPOrb();
		if (TestNotNull(TEXT("XP orb"), Orb))
		{
			TestEqual(TEXT("Orb worth the enemy's XPValue"), Orb->GetValue(), S->XPValue);
			TestEqual(TEXT("Orb has no expiry"), Orb->GetExpireAtServerTime(), 0.0);
			TestTrue(TEXT("Orb uses the XP orb mesh"), Settings->XPOrbLook.Mesh && Orb->GetMesh()->GetStaticMesh() == Settings->XPOrbLook.Mesh);
			TestTrue(TEXT("Orb uses the XP orb material"), Settings->XPOrbLook.Material && Orb->GetMesh()->GetMaterial(0) == Settings->XPOrbLook.Material);
			TestFalse(TEXT("Orb is visible"), Orb->IsHidden());
		}
	});

	// Cash expires (1.5 s); the orb stays and doesn't blink.
	WaitWorldSeconds(this, 2.5);
	Step([this, S]()
	{
		TestEqual(TEXT("Cash expired"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash), 0);
		const AGSCashPickup* Orb = FindActiveXPOrb();
		TestTrue(TEXT("Orb still on the ground"), Orb && !Orb->IsMagnetized());
		TestFalse(TEXT("Orb doesn't blink"), Orb && Orb->IsBlinking());
		TestEqual(TEXT("Still no XP"), GetPlayerTotalXP(), S->TotalBefore);
		if (Orb)
		{
			GetPlayer()->TeleportTo(Orb->GetActorLocation() + FVector(-60.f, 0.f, 60.f), FRotator::ZeroRotator);
		}
	});
	WaitUntil(this, TEXT("orb in range to be collected"), []() { return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP) == 0; }, 10.0);

	// XP at pickup with XPGain; popup where it was collected; then the RoundClear vacuum.
	Step([this, S]()
	{
		TestEqual(TEXT("Collected: XPValue x XPGain"), GetPlayerTotalXP() - S->TotalBefore, S->ExpectedXP);
		TestEqual(TEXT("Run stats XP earned at pickup"), GetGameState()->GetRunStats()->GetXPEarned() - S->EarnedBefore, S->ExpectedXP);
		TestEqual(TEXT("Orb counted as collected"), GetCashSubsystem()->GetNumXPOrbsCollected(), S->CollectedBefore + 1);
		const UGSHUDWidget* HUD = GetPC()->GetGSHUD();
		TestTrue(TEXT("+N XP popup shown"), HUD && HUD->GetActiveXPPopupTotal() >= S->ExpectedXP);
		TestTrue(TEXT("Popup where the orb was collected"), HUD && HUD->GetLastXPPopupAnchor().Equals(GetCashSubsystem()->GetLastXPCollectLocation(), 1.f));

		GetPlayer()->TeleportTo(S->PlayerHome, FRotator::ZeroRotator);
		S->TotalBefore = GetPlayerTotalXP();
		KillEnemyForCash(this, S->EnemyAt, S->DropOrigin, S->CashValue);
	});
	WaitWorldSeconds(this, 1.0);
	Step([this]()
	{
		TestTrue(TEXT("Out of reach: orb waits"), FindActiveXPOrb() && !FindActiveXPOrb()->IsMagnetized());
		GetGameMode()->SetPhase(EGSRoundPhase::RoundClear);
		TestTrue(TEXT("RoundClear: orb flies to the player"), FindActiveXPOrb() && FindActiveXPOrb()->IsMagnetized());
	});
	WaitUntil(this, TEXT("vacuumed orb to arrive"), []() { return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP) == 0; }, 10.0);

	// gs.XP.Add stays instant; a new run clears orbs on the ground.
	Step([this, S]()
	{
		TestEqual(TEXT("RoundClear vacuum paid the orb"), GetPlayerTotalXP() - S->TotalBefore, S->ExpectedXP);

		const int32 Before = GetPlayerTotalXP();
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("XPAdd"), { TEXT("5") });
		TestEqual(TEXT("gs.XP.Add is instant"), GetPlayerTotalXP() - Before, 5);

		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRound(1);
		GameMode->SetRoundTimerPaused(true);
		KillEnemyForCash(this, S->EnemyAt, S->DropOrigin, S->CashValue);
		TestEqual(TEXT("Orb on the ground before the new run"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP), 1);
		GameMode->RestartRunWithSeed(42);
		TestEqual(TEXT("New run clears XP orbs"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP), 0);
		TestEqual(TEXT("New run: XP 0"), GetPlayerTotalXP(), 0);

		// A dead player collects nothing.
		KillEnemyForCash(this, S->EnemyAt, S->DropOrigin, S->CashValue);
		const AGSCashPickup* Orb = FindActiveXPOrb();
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		ApplyDamage(GetPlayerASC(), 100000.f);
		TestTrue(TEXT("Player is dead"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		if (Orb)
		{
			GetPlayer()->SetActorLocation(Orb->GetActorLocation() + FVector(-60.f, 0.f, 60.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
	});
	WaitWorldSeconds(this, 1.5);
	Step([this]()
	{
		TestEqual(TEXT("Dead player: orb stays"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP), 1);
		TestEqual(TEXT("Dead player: no XP"), GetPlayerTotalXP(), 0);
		IConsoleManager::Get().FindConsoleVariable(TEXT("gs.Cash.Lifetime"))->Set(0.f, ECVF_SetByCode);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// XP and player levels
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSXPAndLevelsTest, "BanditBaller.Progression.XPAndLevels", GSTests::TestFlags)
bool FGSXPAndLevelsTest::RunTest(const FString& Parameters)
{
	struct FXPState
	{
		int32 TotalBefore = 0;
		int32 OrbsBefore = 0;
		int32 MeleeXP = 0;
	};
	TSharedRef<FXPState> S = MakeShared<FXPState>();

	BeginPIE(this);
	Step([this, S]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		UGSRunStats* Stats = GameState ? GameState->GetRunStats() : nullptr;
		AGSPlayerController* PC = GetPC();
		AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
		UGSProgressionComponent* Progression = PS ? PS->GetProgressionComponent() : nullptr;
		const UGSXPCurve* Curve = GameMode ? GameMode->GetXPCurve() : nullptr;
		UAbilitySystemComponent* ASC = GetPlayerASC();
		if (!TestNotNull(TEXT("Run stats"), Stats) || !TestNotNull(TEXT("Progression component"), Progression)
			|| !TestNotNull(TEXT("XPCurve on the GameMode (DA_XPCurve)"), Curve) || !TestNotNull(TEXT("Player ASC"), ASC))
		{
			return;
		}

		// Curve: level 2 = 100 XP, then ~+25% per level.
		TestEqual(TEXT("XP to reach level 2"), Curve->GetXPToReachLevel(2), 100);
		TestEqual(TEXT("XP to reach level 3 (+25%)"), Curve->GetXPToReachLevel(3), 125);
		TestEqual(TEXT("Max level"), Curve->GetMaxLevel(), 50);
		FString Table;
		for (int32 Level = 2; Level <= 10; ++Level)
		{
			Table += FString::Printf(TEXT(" L%d=%d"), Level, Curve->GetXPToReachLevel(Level));
		}
		AddInfo(FString::Printf(TEXT("[xp] XP needed:%s (total to L10 %lld)"), *Table, Curve->GetTotalXPToReachLevel(10)));

		// Fresh run: level 1, nothing earned; XPGain starts at 1 (GE_InitPlayerStats).
		GameMode->SetRoundTimerPaused(true);
		GameMode->RestartRunWithSeed(777);
		TestEqual(TEXT("Start level"), Progression->GetLevel(), 1);
		TestEqual(TEXT("Start XP"), Progression->GetCurrentXP(), 0);
		TestEqual(TEXT("XP for level 2"), Progression->GetXPForNextLevel(), 100);
		TestEqual(TEXT("XPGain default"), ASC->GetNumericAttribute(UGSAttributeSet::GetXPGainAttribute()), 1.f, 0.001f);

		// Partial XP, then one big gain: two level-ups in a row with the remainder carried over.
		GameMode->AwardXP(PS, 60.f, false);
		TestEqual(TEXT("60 XP stays in level 1"), Progression->GetLevel(), 1);
		TestEqual(TEXT("60 XP into level 1"), Progression->GetCurrentXP(), 60);
		GameMode->AwardXP(PS, 200.f, false);
		TestEqual(TEXT("260 XP -> level 3"), Progression->GetLevel(), 3);
		TestEqual(TEXT("Leftover carries over (260 - 100 - 125)"), Progression->GetCurrentXP(), 35);
		TestEqual(TEXT("XP for level 4"), Progression->GetXPForNextLevel(), Curve->GetXPToReachLevel(4));
		if (const UGSHUDWidget* HUD = PC->GetGSHUD())
		{
			TestEqual(TEXT("HUD level follows OnLevelUp"), HUD->GetDisplayedLevel(), 3);
			TestEqual(TEXT("HUD XP bar fill"), HUD->GetDisplayedXPPercent(), 35.f / Curve->GetXPToReachLevel(4), 0.001f);
		}
		else
		{
			AddError(TEXT("No HUD widget on the player controller"));
		}

		// The two level-ups opened upgrade offers (game paused); take them so orbs can fly.
		TestEqual(TEXT("Two upgrade offers"), PickAllUpgradeOffers(), 2);

		// A kill grants no XP itself; it drops one orb worth XPValue.
		S->TotalBefore = Progression->GetTotalXP();
		S->OrbsBefore = GetCashSubsystem()->GetNumXPOrbsDropped();
		KillTestEnemyAsPlayer(this);
		TestEqual(TEXT("Kill grants no XP by itself"), Progression->GetTotalXP(), S->TotalBefore);
		TestEqual(TEXT("Kill drops one XP orb"), GetCashSubsystem()->GetNumXPOrbsDropped(), S->OrbsBefore + 1);
		GetCashSubsystem()->VacuumAll();
	});
	WaitUntil(this, TEXT("XP orb 1 collected"), []() { return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP) == 0; }, 10.0);
	Step([this, S]()
	{
		UGSProgressionComponent* Progression = GetPC()->GetPlayerState<AGSPlayerState>()->GetProgressionComponent();
		const UGSEnemyData* MeleeData = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		S->MeleeXP = MeleeData ? MeleeData->XPValue : 0;
		TestTrue(TEXT("Melee bandit has an XPValue"), S->MeleeXP > 0);
		TestEqual(TEXT("Collected orb grants XPValue"), Progression->GetTotalXP() - S->TotalBefore, S->MeleeXP);

		// XPGain applies at pickup.
		GetPlayerASC()->SetNumericAttributeBase(UGSAttributeSet::GetXPGainAttribute(), 2.f);
		S->TotalBefore = Progression->GetTotalXP();
		KillTestEnemyAsPlayer(this);
		GetCashSubsystem()->VacuumAll();
	});
	WaitUntil(this, TEXT("XP orb 2 collected"), []() { return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP) == 0; }, 10.0);
	Step([this, S]()
	{
		AGSGameMode* GameMode = GetGameMode();
		UGSRunStats* Stats = GetGameState()->GetRunStats();
		AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		UGSProgressionComponent* Progression = PS->GetProgressionComponent();
		const UGSXPCurve* Curve = GameMode->GetXPCurve();
		TestEqual(TEXT("Orb grants XPValue x XPGain (2) at pickup"), Progression->GetTotalXP() - S->TotalBefore, S->MeleeXP * 2);
		GetPlayerASC()->SetNumericAttributeBase(UGSAttributeSet::GetXPGainAttribute(), 1.f);

		// Run stats.
		const int32 MeleeXP = S->MeleeXP;
		TestEqual(TEXT("Run stats XP earned"), Stats->GetXPEarned(), 260 + MeleeXP * 3);
		TestEqual(TEXT("Run stats level reached"), Stats->GetLevelReached(), Progression->GetLevel());

		// Debug set level: exactly level N with 0 XP into it.
		GameMode->SetPlayerLevelForDebug(PS, 10);
		TestEqual(TEXT("SetLevel 10"), Progression->GetLevel(), 10);
		TestEqual(TEXT("SetLevel leaves 0 XP"), Progression->GetCurrentXP(), 0);
		TestEqual(TEXT("XP for level 11"), Progression->GetXPForNextLevel(), Curve->GetXPToReachLevel(11));
		TestEqual(TEXT("Level reached follows SetLevel"), Stats->GetLevelReached(), 10);

		// Max level caps: no XP bar past it.
		GameMode->AwardXP(PS, 100000000.f, false);
		TestEqual(TEXT("Capped at max level"), Progression->GetLevel(), Curve->GetMaxLevel());
		TestTrue(TEXT("IsMaxLevel"), Progression->IsMaxLevel());
		TestEqual(TEXT("No XP into max level"), Progression->GetCurrentXP(), 0);
		TestEqual(TEXT("No next level at max"), Progression->GetXPForNextLevel(), 0);

		// A new run resets levels and XP stats.
		GameMode->RestartRunWithSeed(778);
		TestEqual(TEXT("New run: level 1"), Progression->GetLevel(), 1);
		TestEqual(TEXT("New run: total XP 0"), Progression->GetTotalXP(), 0);
		TestEqual(TEXT("New run: XP earned 0"), Stats->GetXPEarned(), 0);
		TestEqual(TEXT("New run: level reached 1"), Stats->GetLevelReached(), 1);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Level-up rewards
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSLevelUpRewardsTest, "BanditBaller.Progression.LevelUpRewards", GSTests::TestFlags)
bool FGSLevelUpRewardsTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		float BaseMaxHealth = 0.f;
		float PerStack = 0.f;
		TSubclassOf<UGameplayEffect> StackEffect;
	};
	TSharedRef<FState> State = MakeShared<FState>();

	struct FContext
	{
		AGSGameMode* GameMode = nullptr;
		AGSPlayerState* PS = nullptr;
		UGSProgressionComponent* Progression = nullptr;
		UGSAbilitySystemComponent* ASC = nullptr;
		const UGSHUDWidget* HUD = nullptr;
		bool IsValid() const { return GameMode && PS && Progression && ASC; }
	};
	auto GetContext = []()
	{
		FContext C;
		C.GameMode = GetGameMode();
		AGSPlayerController* PC = GetPC();
		C.PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
		C.Progression = C.PS ? C.PS->GetProgressionComponent() : nullptr;
		C.ASC = GetPlayerASC();
		C.HUD = PC ? PC->GetGSHUD() : nullptr;
		return C;
	};

	// Damage, level up once (XP for exactly the next level), check +1 stack and full health (and the HUD).
	auto LevelUpOnceDamaged = [this, State, GetContext](const TCHAR* Where, float Damage, int32 ExpectedStacks)
	{
		const FContext C = GetContext();
		if (!C.IsValid())
		{
			AddError(FString::Printf(TEXT("%s: missing GameMode / PlayerState / progression / ASC"), Where));
			return;
		}
		ApplyDamage(C.ASC, Damage);
		TestTrue(FString::Printf(TEXT("%s: damaged before the level-up"), Where), GetHealth(C.ASC) < GetMaxHealth(C.ASC));
		const int32 LevelBefore = C.Progression->GetLevel();
		C.GameMode->AwardXP(C.PS, static_cast<float>(C.Progression->GetXPForNextLevel()), false);
		TestEqual(FString::Printf(TEXT("%s: one level gained"), Where), C.Progression->GetLevel(), LevelBefore + 1);
		TestEqual(FString::Printf(TEXT("%s: MaxHealth +%d stacks"), Where, ExpectedStacks), GetMaxHealth(C.ASC), State->BaseMaxHealth + ExpectedStacks * State->PerStack, 0.01f);
		TestEqual(FString::Printf(TEXT("%s: fully healed"), Where), GetHealth(C.ASC), GetMaxHealth(C.ASC), 0.01f);
		if (C.HUD)
		{
			TestEqual(FString::Printf(TEXT("%s: HUD MaxHealth"), Where), C.HUD->GetDisplayedMaxHealth(), GetMaxHealth(C.ASC), 0.01f);
			TestEqual(FString::Printf(TEXT("%s: HUD Health"), Where), C.HUD->GetDisplayedHealth(), GetHealth(C.ASC), 0.01f);
			TestEqual(FString::Printf(TEXT("%s: HUD level"), Where), C.HUD->GetDisplayedLevel(), C.Progression->GetLevel());
		}
		TestEqual(FString::Printf(TEXT("%s: one upgrade offer"), Where), PickAllUpgradeOffers(), 1);
	};

	BeginPIE(this);
	Step([this, State, GetContext]()
	{
		const FContext C = GetContext();
		const UGSLevelRewards* Rewards = C.GameMode ? C.GameMode->GetLevelRewards() : nullptr;
		const UGSXPCurve* Curve = C.GameMode ? C.GameMode->GetXPCurve() : nullptr;
		if (!TestTrue(TEXT("Context"), C.IsValid()) || !TestNotNull(TEXT("LevelRewards on the GameMode (DA_LevelRewards)"), Rewards)
			|| !TestNotNull(TEXT("XP curve"), Curve) || !TestTrue(TEXT("At least one every-level reward"), Rewards->EveryLevelEffects.Num() > 0)
			|| !TestNotNull(TEXT("Full heal effect"), Rewards->FullHealEffect.Get()))
		{
			return;
		}
		TestEqual(TEXT("No level-specific rewards yet"), Rewards->LevelSpecificRewards.Num(), 0);

		C.GameMode->SetRoundTimerPaused(true);
		C.GameMode->RestartRunWithSeed(901);

		State->StackEffect = Rewards->EveryLevelEffects[0];
		const UGameplayEffect* StackCDO = State->StackEffect->GetDefaultObject<UGameplayEffect>();
		TestTrue(TEXT("Every-level reward is GE_LevelUp_MaxHealth (UGSLevelUpMaxHealthEffect)"), StackCDO->IsA<UGSLevelUpMaxHealthEffect>());
		TestTrue(TEXT("MaxHealth reward is infinite"), StackCDO->DurationPolicy == EGameplayEffectDurationType::Infinite);
		if (StackCDO->Modifiers.Num() > 0)
		{
			StackCDO->Modifiers[0].ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, State->PerStack);
		}
		TestEqual(TEXT("+10 MaxHealth per stack"), State->PerStack, 10.f, 0.001f);
		State->BaseMaxHealth = GetMaxHealth(C.ASC);

		// Damaged, then two level-ups from one XP gain: +20 MaxHealth, full health.
		ApplyDamage(C.ASC, 30.f);
		TestEqual(TEXT("Took 30 damage"), GetHealth(C.ASC), State->BaseMaxHealth - 30.f, 0.01f);
		C.GameMode->AwardXP(C.PS, static_cast<float>(Curve->GetTotalXPToReachLevel(3)), false);
		TestEqual(TEXT("Two level-ups -> level 3"), C.Progression->GetLevel(), 3);
		TestEqual(TEXT("MaxHealth +20 after two level-ups"), GetMaxHealth(C.ASC), State->BaseMaxHealth + 20.f, 0.01f);
		TestEqual(TEXT("Health full after level-ups"), GetHealth(C.ASC), GetMaxHealth(C.ASC), 0.01f);
		TestEqual(TEXT("Two MaxHealth stacks"), C.ASC->GetGameplayEffectCount(State->StackEffect, nullptr), 2);
		if (TestNotNull(TEXT("HUD"), C.HUD))
		{
			TestEqual(TEXT("HUD MaxHealth follows"), C.HUD->GetDisplayedMaxHealth(), State->BaseMaxHealth + 20.f, 0.01f);
			TestEqual(TEXT("HUD Health follows"), C.HUD->GetDisplayedHealth(), State->BaseMaxHealth + 20.f, 0.01f);
			TestEqual(TEXT("HUD level still follows"), C.HUD->GetDisplayedLevel(), 3);
			TestEqual(TEXT("HUD XP bar still follows (0 XP into level 3)"), C.HUD->GetDisplayedXPPercent(), 0.f, 0.001f);
		}
		TestEqual(TEXT("Two level-ups -> two upgrade offers"), PickAllUpgradeOffers(), 2);

		// RoundClear works the same.
		C.GameMode->AdvancePhase(); // PreRound -> Combat
		C.GameMode->AdvancePhase(); // Combat -> RoundClear
		TestTrue(TEXT("In RoundClear"), GetGameState()->GetRoundPhase() == EGSRoundPhase::RoundClear);
	});
	Step([LevelUpOnceDamaged]() { LevelUpOnceDamaged(TEXT("RoundClear"), 25.f, 3); });

	// Market District visit works the same.
	Step([this]() { TestTrue(TEXT("Travel to the Market District starts"), GetGameMode()->TravelToMarketDistrict()); });
	WaitUntil(this, TEXT("party in the Market District"), []()
	{
		return GetGameState() && GetGameState()->GetCurrentArea() == EGSArea::MarketDistrict && !GetGameMode()->IsAreaTravelInProgress();
	});
	Step([LevelUpOnceDamaged]() { LevelUpOnceDamaged(TEXT("Market District"), 40.f, 4); });
	Step([]() { GetGameMode()->ReturnToTown(); });
	WaitUntil(this, TEXT("party back in town"), []()
	{
		return GetGameState() && GetGameState()->GetCurrentArea() == EGSArea::Town && !GetGameMode()->IsAreaTravelInProgress();
	});

	Step([this, State, GetContext]()
	{
		const FContext C = GetContext();
		if (!C.IsValid())
		{
			return;
		}
		// A new run removes the stacks; Health is clamped to the lower MaxHealth.
		C.GameMode->RestartRunWithSeed(902);
		TestEqual(TEXT("New run: level 1"), C.Progression->GetLevel(), 1);
		TestEqual(TEXT("New run: no MaxHealth stacks"), C.ASC->GetGameplayEffectCount(State->StackEffect, nullptr), 0);
		TestEqual(TEXT("New run: base MaxHealth"), GetMaxHealth(C.ASC), State->BaseMaxHealth, 0.01f);
		TestEqual(TEXT("New run: Health clamped to MaxHealth"), GetHealth(C.ASC), State->BaseMaxHealth, 0.01f);

		// A dead player still gets the stat reward but is not healed back up.
		ApplyDamage(C.ASC, 10000.f);
		TestTrue(TEXT("Player is dead"), C.ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		C.GameMode->AwardXP(C.PS, static_cast<float>(C.Progression->GetXPForNextLevel()), false);
		TestEqual(TEXT("Dead: MaxHealth reward applied"), GetMaxHealth(C.ASC), State->BaseMaxHealth + State->PerStack, 0.01f);
		TestEqual(TEXT("Dead: no heal"), GetHealth(C.ASC), 0.f, 0.01f);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Level-up feedback (HUD banner, XP popups, pulses, glow, sound)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSLevelUpFeedbackTest, "BanditBaller.Progression.LevelUpFeedback", GSTests::TestFlags)
bool FGSLevelUpFeedbackTest::RunTest(const FString& Parameters)
{
	TSharedRef<int32> NonKillXP = MakeShared<int32>(0);
	TSharedRef<int32> PopupsBefore = MakeShared<int32>(0);
	TSharedRef<int32> KillXP = MakeShared<int32>(0);

	BeginPIE(this);
	Step([this, PopupsBefore, KillXP]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSPlayerController* PC = GetPC();
		AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
		UGSProgressionComponent* Progression = PS ? PS->GetProgressionComponent() : nullptr;
		const UGSHUDWidget* HUD = PC ? PC->GetGSHUD() : nullptr;
		const AGSCharacter* Player = GetPlayer();
		const UGSLevelRewards* Rewards = GetGameState() ? GetGameState()->GetLevelRewards() : nullptr;
		if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("Progression"), Progression) || !TestNotNull(TEXT("HUD"), HUD)
			|| !TestNotNull(TEXT("Player"), Player) || !TestNotNull(TEXT("Level rewards replicated on GameState"), Rewards))
		{
			return;
		}
		GameMode->SetRoundTimerPaused(true);
		GameMode->RestartRunWithSeed(903);

		// Reward text comes from the reward effects (no second copy of the +10).
		TestEqual(TEXT("Describe one level"), Rewards->DescribeRewards(2, 2).ToString(), FString(TEXT("+10 Max Health")));
		TestEqual(TEXT("Describe two levels"), Rewards->DescribeRewards(2, 3).ToString(), FString(TEXT("+20 Max Health")));
		TestFalse(TEXT("Banner hidden before any level-up"), HUD->IsLevelUpBannerShowing());

		// Kill: no "+N XP" yet (no XP on the kill); the orb's XP pops where the orb is collected.
		*PopupsBefore = HUD->GetActiveXPPopupTotal();
		const AGSEnemyCharacter* Enemy = KillTestEnemyAsPlayer(this);
		*KillXP = Enemy && Enemy->GetEnemyData() ? Enemy->GetEnemyData()->XPValue : 0;
		TestEqual(TEXT("Kill alone shows no XP popup"), HUD->GetActiveXPPopupTotal(), *PopupsBefore);
		GetCashSubsystem()->VacuumAll();
	});
	WaitUntil(this, TEXT("the kill's XP orb to be collected"), []() { return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP) == 0; }, 10.0);
	Step([this, NonKillXP, PopupsBefore, KillXP]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		UGSProgressionComponent* Progression = PS->GetProgressionComponent();
		const UGSHUDWidget* HUD = GetPC()->GetGSHUD();
		const AGSCharacter* Player = GetPlayer();
		TestEqual(TEXT("Collected orb shows +XPValue XP popup"), HUD->GetActiveXPPopupTotal() - *PopupsBefore, *KillXP);
		TestTrue(TEXT("Popup anchored where the orb was collected"),
			HUD->GetLastXPPopupAnchor().Equals(GetCashSubsystem()->GetLastXPCollectLocation(), 1.f));

		// Two level-ups from one gain: banner "Level 3" + "+20 Max Health", pulse, one sound, glow.
		const int32 SoundsBefore = HUD->GetLevelUpSoundsPlayed();
		*NonKillXP = static_cast<int32>(GameMode->GetXPCurve()->GetTotalXPToReachLevel(3)) - Progression->GetTotalXP();
		GameMode->AwardXP(PS, static_cast<float>(*NonKillXP), false);
		TestEqual(TEXT("Level 3"), Progression->GetLevel(), 3);
		TestTrue(TEXT("Banner showing"), HUD->IsLevelUpBannerShowing());
		TestEqual(TEXT("Banner shows the new level"), HUD->GetLevelUpBannerLevel(), 3);
		TestEqual(TEXT("Banner rewards for both levels"), HUD->GetLevelUpRewardText().ToString(), FString(TEXT("+20 Max Health")));
		TestEqual(TEXT("Level label follows"), HUD->GetDisplayedLevel(), 3);
		TestTrue(TEXT("XP bar pulses"), HUD->IsXPBarPulsing());
		TestEqual(TEXT("One level-up sound for both levels"), HUD->GetLevelUpSoundsPlayed() - SoundsBefore, 1);
		TestTrue(TEXT("Player glows"), Player->IsLevelUpGlowActive());
	});

	// XP with no source (AwardXP directly, gs.XP.Add) pops above the player after the pairing window.
	WaitUntil(this, TEXT("+N XP popup for the non-kill XP"), [NonKillXP]()
	{
		const UGSHUDWidget* HUD = GetPC() ? GetPC()->GetGSHUD() : nullptr;
		return HUD && HUD->GetActiveXPPopupTotal() >= *NonKillXP;
	}, 3.0);

	Step([this]()
	{
		// Another level-up while the banner is up merges into it.
		AGSPlayerController* PC = GetPC();
		AGSPlayerState* PS = PC->GetPlayerState<AGSPlayerState>();
		UGSProgressionComponent* Progression = PS->GetProgressionComponent();
		const UGSHUDWidget* HUD = PC->GetGSHUD();
		GetGameMode()->AwardXP(PS, static_cast<float>(Progression->GetXPForNextLevel()), false);
		TestEqual(TEXT("Merged banner: level 4"), HUD->GetLevelUpBannerLevel(), 4);
		TestEqual(TEXT("Merged banner: rewards for levels 2-4"), HUD->GetLevelUpRewardText().ToString(), FString(TEXT("+30 Max Health")));

		// The three level-ups opened upgrade offers (game paused); take them so the glow can play out.
		TestEqual(TEXT("Three upgrade offers"), PickAllUpgradeOffers(), 3);
	});

	// Everything fades out again.
	WaitUntil(this, TEXT("banner, pulse and glow to finish"), []()
	{
		const UGSHUDWidget* HUD = GetPC() ? GetPC()->GetGSHUD() : nullptr;
		const AGSCharacter* Player = GetPlayer();
		return HUD && Player && !HUD->IsLevelUpBannerShowing() && !HUD->IsXPBarPulsing() && !Player->IsLevelUpGlowActive();
	}, 8.0);
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Profile: permanent cash
// ---------------------------------------------------------------------------

namespace GSTests
{
	UGSProfileSubsystem* GetProfile()
	{
		return UGSProfileSubsystem::Get(GetPIEWorld());
	}

	/** Cash in the profile's save file on disk (-1 if there is no file). */
	int32 GetSavedCash(const UGSProfileSubsystem& Profile)
	{
		const UGSProfileSaveGame* Saved = Cast<UGSProfileSaveGame>(UGameplayStatics::LoadGameFromSlot(Profile.GetSlotName(), 0));
		return Saved ? Saved->Cash : -1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSProfileCashTest, "BanditBaller.Profile.PermanentCash", GSTests::TestFlags)
bool FGSProfileCashTest::RunTest(const FString& Parameters)
{
	TSharedRef<TWeakObjectPtr<UWorld>> OldWorld = MakeShared<TWeakObjectPtr<UWorld>>();
	TSharedRef<int32> Bonus = MakeShared<int32>(0); // the round-clear bonus paid on the way

	BeginPIE(this);
	Step([this, OldWorld, Bonus]()
	{
		UGSProfileSubsystem* Profile = GetProfile();
		AGSGameMode* GameMode = GetGameMode();
		AGSPlayerController* PC = GetPC();
		UGSRunStats* Stats = GetGameState() ? GetGameState()->GetRunStats() : nullptr;
		const UGSHUDWidget* HUD = PC ? PC->GetGSHUD() : nullptr;
		if (!TestNotNull(TEXT("Profile subsystem"), Profile) || !TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("Run stats"), Stats)
			|| !TestNotNull(TEXT("HUD"), HUD))
		{
			return;
		}
		GameMode->SetRoundTimerPaused(true);

		// Tests never touch the real profile.
		TestEqual(TEXT("Automation tests use the test slot"), Profile->GetSlotName(), FString(UGSProfileSubsystem::AutomationSlotName));
		Profile->ResetProfile();
		TestEqual(TEXT("Reset: cash 0"), Profile->GetCash(), 0);
		TestEqual(TEXT("Reset: save file deleted"), GetSavedCash(*Profile), -1);
		TestEqual(TEXT("Reset: HUD shows 0"), HUD->GetDisplayedCash(), 0);

		// A pickup goes into the profile; the HUD follows OnCashChanged; CashEarned counts it for this run.
		const int32 EarnedBefore = Stats->GetCashEarned();
		GameMode->AwardCash(PC, 25, PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector);
		TestEqual(TEXT("Pickup -> profile"), Profile->GetCash(), 25);
		TestEqual(TEXT("Pickup -> HUD"), HUD->GetDisplayedCash(), 25);
		TestEqual(TEXT("Pickup -> run stats CashEarned"), Stats->GetCashEarned() - EarnedBefore, 25);

		// Spending.
		TestTrue(TEXT("Spend 10"), Profile->TrySpendCash(10, TEXT("Test")));
		TestEqual(TEXT("After spending"), Profile->GetCash(), 15);
		TestEqual(TEXT("HUD after spending"), HUD->GetDisplayedCash(), 15);
		TestFalse(TEXT("Can't overspend"), Profile->TrySpendCash(100, TEXT("Test")));
		TestEqual(TEXT("Overspend changes nothing"), Profile->GetCash(), 15);
		TestEqual(TEXT("Not saved on change"), GetSavedCash(*Profile), -1);

		// RoundClear pays its bonus into the profile and saves (when it ends, after its vacuum paid out).
		GameMode->AdvancePhase(); // PreRound -> Combat
		GameMode->AdvancePhase(); // Combat -> RoundClear
		*Bonus = Stats->GetLastRoundClearPayout().GetTotal();
		TestTrue(TEXT("RoundClear paid a bonus"), *Bonus > 0);
		TestEqual(TEXT("Round-clear bonus -> profile"), Profile->GetCash(), 15 + *Bonus);
		TestEqual(TEXT("Not saved yet during RoundClear"), GetSavedCash(*Profile), -1);
		GameMode->AdvancePhase(); // RoundClear -> next round
		TestEqual(TEXT("Saved at the end of RoundClear (bonus included)"), GetSavedCash(*Profile), 15 + *Bonus);
		if (const UGSProfileSaveGame* Saved = Cast<UGSProfileSaveGame>(UGameplayStatics::LoadGameFromSlot(Profile->GetSlotName(), 0)))
		{
			TestEqual(TEXT("Save has the current version"), Saved->SaveVersion, UGSProfileSaveGame::CurrentVersion);
			TestEqual(TEXT("No owned items yet"), Saved->OwnedItems.Num(), 0);
		}

		// A new run resets the run's CashEarned but keeps the permanent balance.
		GameMode->RestartRunWithSeed(906);
		TestEqual(TEXT("New run: CashEarned back to 0"), Stats->GetCashEarned(), 0);
		TestEqual(TEXT("New run: balance kept"), Profile->GetCash(), 15 + *Bonus);

		// Run end (death) saves.
		Profile->AddCash(5, TEXT("Test"));
		TestEqual(TEXT("Unsaved change"), GetSavedCash(*Profile), 15 + *Bonus);
		*OldWorld = GetPIEWorld();
		ApplyDamage(GetPlayerASC(), 100000.f);
		TestTrue(TEXT("Player died"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		TestEqual(TEXT("Saved at run end (death)"), GetSavedCash(*Profile), 20 + *Bonus);
	});

	// Restarting the map after death keeps the balance (the profile lives on the GameInstance).
	Step([]() { GetPC()->ServerRequestRestart(); });
	WaitUntil(this, TEXT("restarted map with a new, ready player"), [OldWorld]()
	{
		return GetPIEWorld() && GetPIEWorld() != OldWorld->Get() && IsPlayerReady();
	}, 30.0);
	Step([this, Bonus]()
	{
		UGSProfileSubsystem* Profile = GetProfile();
		if (!TestNotNull(TEXT("Profile after restart"), Profile))
		{
			return;
		}
		TestEqual(TEXT("Balance survives death + restart"), Profile->GetCash(), 20 + *Bonus);
		if (const UGSHUDWidget* HUD = GetPC() ? GetPC()->GetGSHUD() : nullptr)
		{
			TestEqual(TEXT("New HUD shows the balance"), HUD->GetDisplayedCash(), 20 + *Bonus);
		}
		Profile->ResetProfile(); // leave the test slot empty
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Upgrades: data, stacking stat effects, cap, reset
// ---------------------------------------------------------------------------

namespace GSTests
{
	UGSUpgradeComponent* GetPlayerUpgrades()
	{
		const AGSPlayerController* PC = GetPC();
		const AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
		return PS ? PS->GetUpgradeComponent() : nullptr;
	}

	float GetAttr(const FGameplayAttribute& Attribute)
	{
		const UGSAbilitySystemComponent* ASC = GetPlayerASC();
		return ASC ? ASC->GetNumericAttribute(Attribute) : -1.f;
	}

	/** Stack count of Upgrade's active effect on the player (0 if not active). */
	int32 GetActiveUpgradeEffectStacks(const UGSUpgradeData* Upgrade)
	{
		const UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		const UGSAbilitySystemComponent* ASC = GetPlayerASC();
		if (!Upgrades || !ASC)
		{
			return 0;
		}
		for (const FGSUpgradeStack& Entry : Upgrades->GetUpgradeStacks())
		{
			if (Entry.Upgrade == Upgrade)
			{
				return ASC->GetActiveGameplayEffect(Entry.EffectHandle) ? ASC->GetCurrentStackCount(Entry.EffectHandle) : 0;
			}
		}
		return 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSStatUpgradesTest, "BanditBaller.Upgrades.StatUpgrades", GSTests::TestFlags)
bool FGSStatUpgradesTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		float BaseMoveSpeed = 0.f;
		float ExpectedReload = 0.f;
		float BaseReload = 0.f;
		int32 AmmoBefore = 0;
		double ReloadStart = 0.0;
	};
	TSharedRef<FState> S = MakeShared<FState>();

	// Data assets.
	Step([this, S]()
	{
		struct FExpect { const TCHAR* Name; const TCHAR* Text; };
		for (const FExpect& E : { FExpect{ TEXT("ReloadSpeed"), TEXT("+10%") }, FExpect{ TEXT("Damage"), TEXT("+10%") }, FExpect{ TEXT("MoveSpeed"), TEXT("+8%") } })
		{
			const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(E.Name);
			if (!TestNotNull(FString::Printf(TEXT("DA_Upgrade_%s exists"), E.Name), Upgrade))
			{
				continue;
			}
			TestTrue(FString::Printf(TEXT("%s is Common"), E.Name), Upgrade->Rarity == EGSUpgradeRarity::Common);
			TestEqual(FString::Printf(TEXT("%s has no cap"), E.Name), Upgrade->MaxStacks, 0);
			TestFalse(FString::Printf(TEXT("%s has a display name"), E.Name), Upgrade->DisplayName.IsEmpty());
			TestTrue(FString::Printf(TEXT("%s effect is an upgrade effect"), E.Name), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect::StaticClass()));
			const FString Description = Upgrade->GetFormattedDescription().ToString();
			TestTrue(FString::Printf(TEXT("%s description '%s' shows %s"), E.Name, *Description, E.Text), Description.Contains(E.Text));
		}

		TestEqual(TEXT("ReloadSpeed starts at 1"), GetAttr(UGSAttributeSet::GetReloadSpeedAttribute()), 1.f, 0.001f);
		TestEqual(TEXT("Damage starts at 1"), GetAttr(UGSAttributeSet::GetDamageAttribute()), 1.f, 0.001f);
		S->BaseMoveSpeed = GetAttr(UGSAttributeSet::GetMoveSpeedAttribute());
		TestTrue(TEXT("MoveSpeed initialized"), S->BaseMoveSpeed > 0.f);
		TestEqual(TEXT("No picks at start"), GetPlayerUpgrades() ? GetPlayerUpgrades()->GetTotalPicks() : -1, 0);
	});

	// Reload Speed x5 (through gs.Upgrade.Grant) -> 1.5, and the real reload gets shorter.
	Step([this, S]()
	{
		for (int32 i = 0; i < 5; ++i)
		{
			UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("ReloadSpeed") });
		}
		const UGSUpgradeData* Reload = UGSDebugLibrary::FindUpgradeData(TEXT("ReloadSpeed"));
		TestEqual(TEXT("ReloadSpeed stacks tracked"), GetPlayerUpgrades()->GetStacks(Reload), 5);
		TestEqual(TEXT("ReloadSpeed GE has 5 stacks"), GetActiveUpgradeEffectStacks(Reload), 5);
		TestEqual(TEXT("ReloadSpeed = 1 + 5 x 10%"), GetAttr(UGSAttributeSet::GetReloadSpeedAttribute()), 1.5f, 0.001f);

		UGSWeaponComponent* Weapons = GetPlayer()->GetWeaponComponent();
		S->BaseReload = Weapons->GetCurrentWeaponData()->ReloadTime;
		S->ExpectedReload = UGSGameplayAbility_Reload::GetReloadDuration(S->BaseReload, 1.5f);
		TestEqual(TEXT("Reload duration = ReloadTime / ReloadSpeed"), S->ExpectedReload, S->BaseReload / 1.5f, 0.001f);

		S->AmmoBefore = Weapons->GetCurrentAmmo();
		GetPlayerASC()->AbilityInputTagPressed(GSGameplayTags::Ability_Fire);
		GetPlayerASC()->AbilityInputTagReleased(GSGameplayTags::Ability_Fire);
	});
	WaitUntil(this, TEXT("a shot to spend ammo"), [S]() { return GetPlayer()->GetWeaponComponent()->GetCurrentAmmo() < S->AmmoBefore; });
	WaitUntil(this, TEXT("State.Firing to clear"), []() { return !GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Firing); }, 5.0);
	Step([this, S]()
	{
		S->ReloadStart = GetPIEWorld()->GetTimeSeconds();
		GetPlayerASC()->AbilityInputTagPressed(GSGameplayTags::Ability_Reload);
		GetPlayerASC()->AbilityInputTagReleased(GSGameplayTags::Ability_Reload);
		TestTrue(TEXT("Reload started"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Reloading));
	});
	WaitUntil(this, TEXT("faster reload to refill"), []()
	{
		const UGSWeaponComponent* Weapons = GetPlayer()->GetWeaponComponent();
		return Weapons->GetCurrentAmmo() == Weapons->GetCurrentWeaponData()->MagazineSize;
	});
	Step([this, S]()
	{
		const double Elapsed = GetPIEWorld()->GetTimeSeconds() - S->ReloadStart;
		AddInfo(FString::Printf(TEXT("Reload with ReloadSpeed 1.5 took %.2f s (expected %.2f, base %.2f)"), Elapsed, S->ExpectedReload, S->BaseReload));
		TestTrue(TEXT("Reload took at least the scaled time"), Elapsed >= S->ExpectedReload - 0.05);
		TestTrue(TEXT("Reload was shorter than the base time"), Elapsed < S->BaseReload);

		// Damage x3 -> 1.3 (GA_Fire multiplies weapon damage by it).
		const UGSUpgradeData* Damage = UGSDebugLibrary::FindUpgradeData(TEXT("Damage"));
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		for (int32 i = 0; i < 3; ++i)
		{
			TestTrue(TEXT("Damage granted"), Upgrades->GrantUpgrade(Damage, TEXT("Test")));
		}
		TestEqual(TEXT("Damage = 1 + 3 x 10%"), GetAttr(UGSAttributeSet::GetDamageAttribute()), 1.3f, 0.001f);
		TestEqual(TEXT("Damage GE has 3 stacks"), GetActiveUpgradeEffectStacks(Damage), 3);

		// Move Speed x2 -> base x 1.16, and the movement component follows.
		const UGSUpgradeData* Move = UGSDebugLibrary::FindUpgradeData(TEXT("MoveSpeed"));
		Upgrades->GrantUpgrade(Move, TEXT("Test"));
		Upgrades->GrantUpgrade(Move, TEXT("Test"));
		const float ExpectedSpeed = S->BaseMoveSpeed * 1.16f;
		TestEqual(TEXT("MoveSpeed = base x (1 + 2 x 8%)"), GetAttr(UGSAttributeSet::GetMoveSpeedAttribute()), ExpectedSpeed, 0.5f);
		TestEqual(TEXT("MaxWalkSpeed follows MoveSpeed"), GetPlayer()->GetCharacterMovement()->MaxWalkSpeed, ExpectedSpeed, 0.5f);

		TestEqual(TEXT("Total picks"), Upgrades->GetTotalPicks(), 10);
		TestEqual(TEXT("Three upgrades tracked"), Upgrades->GetUpgradeStacks().Num(), 3);

		// Cap: MaxStacks 1 allows one pick only.
		UGSUpgradeData* Capped = NewObject<UGSUpgradeData>(GetTransientPackage(),
			MakeUniqueObjectName(GetTransientPackage(), UGSUpgradeData::StaticClass(), TEXT("DA_Upgrade_TestCapped")), RF_Transient);
		Capped->Effect = UGSUpgradeEffect_Damage::StaticClass();
		Capped->MaxStacks = 1;
		TestTrue(TEXT("Capped upgrade: first pick"), Upgrades->GrantUpgrade(Capped, TEXT("Test")));
		TestFalse(TEXT("Capped upgrade: second pick refused"), Upgrades->GrantUpgrade(Capped, TEXT("Test")));
		TestEqual(TEXT("Capped upgrade stays at 1"), Upgrades->GetStacks(Capped), 1);
		TestEqual(TEXT("Damage +10% from the capped upgrade"), GetAttr(UGSAttributeSet::GetDamageAttribute()), 1.4f, 0.001f);

		TestEqual(TEXT("Unknown upgrade name is refused"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("NoSuchUpgrade")), 0);

		// A new run removes every upgrade.
		GetGameMode()->RestartRunWithSeed(4242);
		TestEqual(TEXT("New run: no picks"), Upgrades->GetTotalPicks(), 0);
		TestEqual(TEXT("New run: no stacks"), Upgrades->GetStacks(Damage), 0);
		TestEqual(TEXT("New run: ReloadSpeed back to 1"), GetAttr(UGSAttributeSet::GetReloadSpeedAttribute()), 1.f, 0.001f);
		TestEqual(TEXT("New run: Damage back to 1"), GetAttr(UGSAttributeSet::GetDamageAttribute()), 1.f, 0.001f);
		TestEqual(TEXT("New run: MoveSpeed back to base"), GetAttr(UGSAttributeSet::GetMoveSpeedAttribute()), S->BaseMoveSpeed, 0.5f);
		TestEqual(TEXT("New run: MaxWalkSpeed back to base"), GetPlayer()->GetCharacterMovement()->MaxWalkSpeed, S->BaseMoveSpeed, 0.5f);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Extra Jump: JumpMaxCount sync, consistent air jumps, coyote / slide-hop / dash, landing reset
// ---------------------------------------------------------------------------

namespace GSTests
{
	/** Enqueue: let Frames frames pass. */
	void WaitFrames(FAutomationTestBase* Test, int32 Frames)
	{
		TSharedRef<int32> Left = MakeShared<int32>(Frames);
		WaitUntil(Test, FString::Printf(TEXT("%d frames"), Frames), [Left]() { return --(*Left) < 0; });
	}

	/** Enqueue: press jump, release it on the next frame (like a key tap). */
	void TapJump(FAutomationTestBase* Test)
	{
		Step([]() { GetPlayer()->Jump(); });
		WaitFrames(Test, 1);
		Step([]() { GetPlayer()->StopJumping(); });
	}

	bool IsFalling()
	{
		const AGSCharacter* Player = GetPlayer();
		return Player && Player->GetCharacterMovement()->IsFalling();
	}

	/** Enqueue: wait until the player stands on the ground again with its jumps reset. */
	void WaitLanded(FAutomationTestBase* Test)
	{
		WaitUntil(Test, TEXT("player to land"), []()
		{
			const AGSCharacter* Player = GetPlayer();
			return Player && Player->GetCharacterMovement()->IsMovingOnGround() && Player->JumpCurrentCount == 0;
		});
	}

	/** Enqueue: teleport the player Height cm up (like walking off a ledge) and wait until it is falling. */
	void LiftPlayer(FAutomationTestBase* Test, float Height, TSharedRef<double> FallStart)
	{
		Step([Height]() { GetPlayer()->SetActorLocation(GetPlayer()->GetActorLocation() + FVector(0.f, 0.f, Height), false, nullptr, ETeleportType::TeleportPhysics); });
		WaitUntil(Test, TEXT("player to start falling"), [FallStart]()
		{
			if (IsFalling())
			{
				*FallStart = GetPIEWorld()->GetTimeSeconds();
				return true;
			}
			return false;
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSExtraJumpTest, "BanditBaller.Upgrades.ExtraJump", GSTests::TestFlags)
bool FGSExtraJumpTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		bool bOldThrottle = true;
		float AirJumpZ = 0.f;
		float JumpZ = 0.f;
		float Coyote = 0.f;
		int32 FeedbackBefore = 0;
	};
	TSharedRef<FState> S = MakeShared<FState>();
	TSharedRef<double> FallStart = MakeShared<double>(0.0);

	// Data, defaults, and timing that doesn't depend on the editor being in front.
	Step([this, S]()
	{
		UEditorPerformanceSettings* Perf = GetMutableDefault<UEditorPerformanceSettings>();
		S->bOldThrottle = Perf->bThrottleCPUWhenNotForeground;
		Perf->bThrottleCPUWhenNotForeground = false;
		GetGameMode()->SetRoundTimerPaused(true); // stay in PreRound: no enemies

		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("ExtraJump"));
		if (TestNotNull(TEXT("DA_Upgrade_ExtraJump exists"), Upgrade))
		{
			TestTrue(TEXT("Extra Jump is Rare"), Upgrade->Rarity == EGSUpgradeRarity::Rare);
			TestEqual(TEXT("Extra Jump has no cap"), Upgrade->MaxStacks, 0);
			TestTrue(TEXT("Extra Jump effect is an upgrade effect"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect::StaticClass()));
			const FString Description = Upgrade->GetFormattedDescription().ToString();
			TestTrue(FString::Printf(TEXT("Description '%s' shows +1"), *Description), Description.Contains(TEXT("+1")));
		}

		const AGSCharacter* Player = GetPlayer();
		const UGSMovementSettings* Settings = Player->GetMovementSettings();
		if (TestNotNull(TEXT("Movement settings"), Settings))
		{
			S->AirJumpZ = Settings->AirJumpZVelocity;
			S->JumpZ = Settings->JumpZVelocity;
			S->Coyote = Settings->CoyoteTime;
		}
		TestEqual(TEXT("ExtraJumps starts at 0"), GetAttr(UGSAttributeSet::GetExtraJumpsAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("JumpMaxCount starts at 1"), Player->JumpMaxCount, 1);
		S->FeedbackBefore = Player->GetAirJumpFeedbackCount();
	});

	// Without the upgrade there is no air jump.
	TapJump(this);
	WaitUntil(this, TEXT("ground jump"), []() { return IsFalling() && GetPlayer()->JumpCurrentCount == 1; });
	TapJump(this);
	WaitFrames(this, 5);
	Step([this]() { TestEqual(TEXT("No air jump without the upgrade"), GetPlayer()->GetAirJumpsSinceLanded(), 0); });
	WaitLanded(this);

	// Three stacks: JumpMaxCount 4.
	Step([this]()
	{
		for (int32 i = 0; i < 3; ++i)
		{
			UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("ExtraJump") });
		}
		TestEqual(TEXT("ExtraJumps = 3"), GetAttr(UGSAttributeSet::GetExtraJumpsAttribute()), 3.f, 0.001f);
		TestEqual(TEXT("JumpMaxCount = 1 + 3"), GetPlayer()->JumpMaxCount, 4);
		TestEqual(TEXT("Stacks tracked"), GetPlayerUpgrades()->GetStacks(UGSDebugLibrary::FindUpgradeData(TEXT("ExtraJump"))), 3);
	});

	// Ground jump, then three air jumps: one while still rising, two while falling ever faster. Each sets the same Z.
	TapJump(this);
	WaitUntil(this, TEXT("rising after the ground jump"), [S]() { return IsFalling() && GetPlayer()->JumpCurrentCount == 1 && GetPlayer()->GetVelocity().Z < S->JumpZ * 0.6f; });
	TapJump(this);
	WaitUntil(this, TEXT("air jump 1 (rising)"), []() { return GetPlayer()->GetAirJumpsSinceLanded() == 1; });
	Step([this, S]() { TestEqual(TEXT("Air jump 1 sets AirJumpZVelocity"), GetPlayer()->GetLastAirJumpZVelocity(), S->AirJumpZ, 0.5f); });
	WaitUntil(this, TEXT("falling after air jump 1"), []() { return GetPlayer()->GetVelocity().Z < -300.f; });
	TapJump(this);
	WaitUntil(this, TEXT("air jump 2 (falling)"), []() { return GetPlayer()->GetAirJumpsSinceLanded() == 2; });
	Step([this, S]() { TestEqual(TEXT("Air jump 2 sets the same Z"), GetPlayer()->GetLastAirJumpZVelocity(), S->AirJumpZ, 0.5f); });
	WaitUntil(this, TEXT("falling fast after air jump 2"), []() { return GetPlayer()->GetVelocity().Z < -700.f; });
	TapJump(this);
	WaitUntil(this, TEXT("air jump 3 (falling fast)"), []() { return GetPlayer()->GetAirJumpsSinceLanded() == 3; });
	Step([this, S]() { TestEqual(TEXT("Air jump 3 sets the same Z"), GetPlayer()->GetLastAirJumpZVelocity(), S->AirJumpZ, 0.5f); });
	TapJump(this);
	WaitFrames(this, 5);
	Step([this, S]()
	{
		TestEqual(TEXT("A 4th air jump is refused"), GetPlayer()->GetAirJumpsSinceLanded(), 3);
		TestEqual(TEXT("JumpCurrentCount at the max"), GetPlayer()->JumpCurrentCount, 4);
		TestEqual(TEXT("One puff/sound per air jump"), GetPlayer()->GetAirJumpFeedbackCount() - S->FeedbackBefore, 3);
	});
	WaitLanded(this);
	Step([this]() { TestEqual(TEXT("Landing resets air jumps"), GetPlayer()->GetAirJumpsSinceLanded(), 0); });

	// Coyote time: walking off a ledge and jumping right away is still the ground jump; air jumps remain.
	LiftPlayer(this, 300.f, FallStart);
	TapJump(this);
	WaitUntil(this, TEXT("coyote jump"), []() { return GetPlayer()->JumpCurrentCount >= 1 && GetPlayer()->GetVelocity().Z > 0.f; });
	Step([this, S, FallStart]()
	{
		AddInfo(FString::Printf(TEXT("Coyote jump %.3f s after leaving the ground (window %.2f s)"), GetPIEWorld()->GetTimeSeconds() - *FallStart, S->Coyote));
		TestEqual(TEXT("Coyote jump counts as the ground jump"), GetPlayer()->JumpCurrentCount, 1);
		TestEqual(TEXT("Coyote jump is not an air jump"), GetPlayer()->GetAirJumpsSinceLanded(), 0);
	});
	TapJump(this);
	WaitUntil(this, TEXT("air jump after the coyote jump"), []() { return GetPlayer()->GetAirJumpsSinceLanded() == 1; });
	WaitLanded(this);

	// Past the coyote window the first press is an air jump.
	LiftPlayer(this, 600.f, FallStart);
	WaitUntil(this, TEXT("coyote window to pass"), [S, FallStart]() { return GetPIEWorld()->GetTimeSeconds() - *FallStart > S->Coyote + 0.05f; });
	TapJump(this);
	WaitUntil(this, TEXT("air jump after walking off"), []() { return GetPlayer()->GetAirJumpsSinceLanded() == 1; });
	Step([this, S]()
	{
		TestEqual(TEXT("Late jump after walking off uses an air jump"), GetPlayer()->JumpCurrentCount, 2);
		TestEqual(TEXT("Late jump sets AirJumpZVelocity"), GetPlayer()->GetLastAirJumpZVelocity(), S->AirJumpZ, 0.5f);
	});
	WaitLanded(this);

	// Slide-hop keeps the slide speed, then air jumps work as usual.
	Step([this]()
	{
		const UGSSlideSettings* Slide = LoadObject<UGSSlideSettings>(nullptr, TEXT("/Game/BanditBaller/Data/DA_SlideSettings.DA_SlideSettings"));
		if (TestNotNull(TEXT("DA_SlideSettings"), Slide))
		{
			GetPlayer()->StartSlide(*Slide);
		}
		TestTrue(TEXT("Sliding"), GetPlayer()->IsSliding());
	});
	WaitFrames(this, 3);
	TapJump(this);
	WaitUntil(this, TEXT("slide-hop"), []() { return IsFalling() && GetPlayer()->JumpCurrentCount == 1; });
	Step([this]()
	{
		TestFalse(TEXT("Slide-hop ends the slide"), GetPlayer()->IsSliding());
		AddInfo(FString::Printf(TEXT("Slide-hop horizontal speed %.0f"), GetPlayer()->GetVelocity().Size2D()));
		TestTrue(TEXT("Slide-hop keeps the slide speed"), GetPlayer()->GetVelocity().Size2D() > 300.f);
	});
	WaitFrames(this, 5);
	TapJump(this);
	WaitUntil(this, TEXT("air jump after the slide-hop"), []() { return GetPlayer()->GetAirJumpsSinceLanded() == 1; });
	Step([this, S]() { TestEqual(TEXT("Air jump after slide-hop sets AirJumpZVelocity"), GetPlayer()->GetLastAirJumpZVelocity(), S->AirJumpZ, 0.5f); });
	WaitLanded(this);

	// Jump during an air dash: queued, then the air jump happens when the dash ends.
	TSharedRef<const UGSDashSettings*> Dash = MakeShared<const UGSDashSettings*>(nullptr);
	LiftPlayer(this, 400.f, FallStart);
	WaitUntil(this, TEXT("coyote window to pass"), [S, FallStart]() { return GetPIEWorld()->GetTimeSeconds() - *FallStart > S->Coyote + 0.05f; });
	Step([this, Dash]()
	{
		*Dash = LoadObject<UGSDashSettings>(nullptr, TEXT("/Game/BanditBaller/Data/DA_DashSettings.DA_DashSettings"));
		if (!TestNotNull(TEXT("DA_DashSettings"), *Dash))
		{
			return;
		}
		AGSCharacter* Player = GetPlayer();
		Player->StartDash(Player->GetActorForwardVector(), **Dash);
		TestTrue(TEXT("Dashing"), Player->IsDashing());
		Player->Jump();
		TestTrue(TEXT("Jump during the dash is queued"), Player->IsJumpQueuedAfterDash());
		TestEqual(TEXT("No jump spent during the dash"), Player->GetAirJumpsSinceLanded(), 0);
	});
	WaitUntil(this, TEXT("dash to end and the queued air jump to happen"), []() { return !GetPlayer()->IsDashing() && GetPlayer()->GetAirJumpsSinceLanded() == 1; });
	Step([this, S]()
	{
		GetPlayer()->StopJumping();
		TestFalse(TEXT("Queue cleared"), GetPlayer()->IsJumpQueuedAfterDash());
		TestEqual(TEXT("Air jump after the dash sets AirJumpZVelocity"), GetPlayer()->GetLastAirJumpZVelocity(), S->AirJumpZ, 0.5f);
	});
	WaitLanded(this);

	// Jump during a ground dash: the ground jump happens when the dash ends.
	Step([this, Dash]()
	{
		AGSCharacter* Player = GetPlayer();
		if (*Dash)
		{
			Player->StartDash(Player->GetActorForwardVector(), **Dash);
		}
		Player->Jump();
		TestTrue(TEXT("Ground dash: jump queued"), Player->IsJumpQueuedAfterDash());
	});
	WaitUntil(this, TEXT("ground jump after the dash"), []() { return !GetPlayer()->IsDashing() && IsFalling() && GetPlayer()->JumpCurrentCount == 1; });
	Step([this]()
	{
		GetPlayer()->StopJumping();
		TestEqual(TEXT("Ground dash jump is the ground jump"), GetPlayer()->GetAirJumpsSinceLanded(), 0);
	});
	WaitLanded(this);

	// A new run removes the extra jumps.
	Step([this, S]()
	{
		GetGameMode()->RestartRunWithSeed(5150);
		TestEqual(TEXT("New run: ExtraJumps 0"), GetAttr(UGSAttributeSet::GetExtraJumpsAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("New run: JumpMaxCount 1"), GetPlayer()->JumpMaxCount, 1);
		GetMutableDefault<UEditorPerformanceSettings>()->bThrottleCPUWhenNotForeground = S->bOldThrottle;
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Upgrade offers: pool, roll rules, queue, choose, reroll, reset, simulator
// ---------------------------------------------------------------------------

namespace GSTests
{
	UGSUpgradePool* MakeTestPool(const TArray<UGSUpgradeData*>& Upgrades)
	{
		UGSUpgradePool* Pool = NewObject<UGSUpgradePool>(GetTransientPackage(), NAME_None, RF_Transient);
		for (UGSUpgradeData* Upgrade : Upgrades)
		{
			Pool->Upgrades.Add(Upgrade);
		}
		return Pool;
	}

	UGSUpgradeData* MakeTestUpgrade(const TCHAR* Name, EGSUpgradeRarity Rarity, int32 MaxStacks = 0)
	{
		// Unique name: a fixed one would collide when the tests run again in the same editor session.
		UGSUpgradeData* Upgrade = NewObject<UGSUpgradeData>(GetTransientPackage(),
			MakeUniqueObjectName(GetTransientPackage(), UGSUpgradeData::StaticClass(), FName(Name)), RF_Transient);
		Upgrade->Effect = UGSUpgradeEffect_Damage::StaticClass();
		Upgrade->Rarity = Rarity;
		Upgrade->MaxStacks = MaxStacks;
		return Upgrade;
	}

	bool AllDifferent(const TArray<UGSUpgradeData*>& Offer)
	{
		return TSet<UGSUpgradeData*>(Offer).Num() == Offer.Num() && !Offer.Contains(nullptr);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSUpgradeOffersTest, "BanditBaller.Upgrades.Offers", GSTests::TestFlags)
bool FGSUpgradeOffersTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		TArray<EGSUpgradeOfferChange> Changes;
		TArray<int32> Rerolls;
		float MaxHealthAtOpen = -1.f;
		float HealthAtOpen = -1.f;
		float BaseMaxHealth = 0.f;
		TArray<UGSUpgradeData*> FirstOffer;
	};
	TSharedRef<FState> S = MakeShared<FState>();

	// Pool data and the pure roll rules.
	Step([this, S]()
	{
		GetGameMode()->SetRoundTimerPaused(true); // stay in PreRound: no kills, nothing else uses the run stream

		const UGSUpgradePool* Pool = GetGameMode()->GetUpgradePool();
		if (!TestNotNull(TEXT("UpgradePool set on the GameMode"), Pool))
		{
			return;
		}
		TestEqual(TEXT("Pool asset"), Pool->GetName(), FString(TEXT("DA_UpgradePool")));
		TestEqual(TEXT("Pool has the 4 upgrades"), Pool->Upgrades.Num(), 4);
		TestEqual(TEXT("Common weight"), Pool->GetRarityWeight(EGSUpgradeRarity::Common), 100.f);
		TestEqual(TEXT("Rare weight"), Pool->GetRarityWeight(EGSUpgradeRarity::Rare), 20.f);
		TestEqual(TEXT("Epic weight"), Pool->GetRarityWeight(EGSUpgradeRarity::Epic), 6.f);
		TestEqual(TEXT("Legendary weight"), Pool->GetRarityWeight(EGSUpgradeRarity::Legendary), 1.f);
		TestEqual(TEXT("3 choices per offer"), Pool->ChoicesPerOffer, 3);
		TestEqual(TEXT("3 rerolls per run"), Pool->RerollsPerRun, 3);
		TestEqual(TEXT("Four rarities"), UGSUpgradePool::GetAllRarities().Num(), 4);
		TestEqual(TEXT("Epic after Rare"), static_cast<int32>(EGSUpgradeRarity::Epic), 2);
		TestEqual(TEXT("Legendary last"), static_cast<int32>(EGSUpgradeRarity::Legendary), 3);

		const UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		TestEqual(TEXT("Run starts with 3 rerolls"), Upgrades->GetRerollsLeft(), 3);
		TestFalse(TEXT("No offer at start"), Upgrades->HasOpenOffer());
		TestEqual(TEXT("Nothing queued at start"), Upgrades->GetQueuedOffers(), 0);

		auto NoStacks = [](const UGSUpgradeData*) { return 0; };

		// Real pool: always 3 different upgrades (empty Epic / Legendary tiers never waste a pick), and the lone Rare
		// shows up in about 1 - (100/120)^3 = 42% of offers.
		FRandomStream Stream(7);
		int32 WithRare = 0;
		bool bAllFull = true, bAllDifferent = true;
		for (int32 i = 0; i < 3000; ++i)
		{
			const TArray<UGSUpgradeData*> Offer = Pool->RollOffer(Stream, NoStacks);
			bAllFull &= Offer.Num() == 3;
			bAllDifferent &= AllDifferent(Offer);
			WithRare += Offer.ContainsByPredicate([](const UGSUpgradeData* U) { return U->Rarity == EGSUpgradeRarity::Rare; }) ? 1 : 0;
		}
		TestTrue(TEXT("Every offer has 3 choices"), bAllFull);
		TestTrue(TEXT("Choices in an offer are different"), bAllDifferent);
		AddInfo(FString::Printf(TEXT("Rare in %.1f%% of 3000 offers (expected 42.1%%)"), 100.0 * WithRare / 3000.0));
		TestTrue(TEXT("Rare frequency matches the tier weights"), FMath::Abs(WithRare / 3000.0 - 0.4213) < 0.03);

		// Same seed -> same offers.
		FRandomStream A(99), B(99);
		bool bSame = true;
		for (int32 i = 0; i < 20; ++i)
		{
			bSame &= Pool->RollOffer(A, NoStacks) == Pool->RollOffer(B, NoStacks);
		}
		TestTrue(TEXT("Same seed rolls the same offers"), bSame);

		// Fewer upgrades than choices: offer what's left.
		UGSUpgradePool* Small = MakeTestPool({ MakeTestUpgrade(TEXT("TestUp_A"), EGSUpgradeRarity::Common), MakeTestUpgrade(TEXT("TestUp_B"), EGSUpgradeRarity::Legendary) });
		const TArray<UGSUpgradeData*> SmallOffer = Small->RollOffer(Stream, NoStacks);
		TestEqual(TEXT("Two upgrades -> offer of 2"), SmallOffer.Num(), 2);
		TestTrue(TEXT("Small offer different"), AllDifferent(SmallOffer));

		// An upgrade at MaxStacks is never offered; an empty pool offers nothing.
		UGSUpgradeData* Capped = MakeTestUpgrade(TEXT("TestUp_Capped"), EGSUpgradeRarity::Common, 1);
		UGSUpgradePool* WithCapped = MakeTestPool({ Capped, MakeTestUpgrade(TEXT("TestUp_C"), EGSUpgradeRarity::Common),
			MakeTestUpgrade(TEXT("TestUp_D"), EGSUpgradeRarity::Common), MakeTestUpgrade(TEXT("TestUp_E"), EGSUpgradeRarity::Rare) });
		bool bCappedSeen = false;
		for (int32 i = 0; i < 200; ++i)
		{
			const TArray<UGSUpgradeData*> Offer = WithCapped->RollOffer(Stream, [Capped](const UGSUpgradeData* U) { return U == Capped ? 1 : 0; });
			bCappedSeen |= Offer.Contains(Capped);
			bAllFull &= Offer.Num() == 3;
		}
		TestFalse(TEXT("Upgrade at MaxStacks is never offered"), bCappedSeen);
		TestTrue(TEXT("The other 3 still fill the offer"), bAllFull);
		TestEqual(TEXT("Empty pool -> empty offer"), MakeTestPool({})->RollOffer(Stream, NoStacks).Num(), 0);

		// Listen to the events like the UI will.
		UGSUpgradeComponent* Mutable = GetPlayerUpgrades();
		Mutable->OnUpgradeOfferChangedNative.AddLambda([S](EGSUpgradeOfferChange Change)
		{
			S->Changes.Add(Change);
			if (Change == EGSUpgradeOfferChange::Opened && S->MaxHealthAtOpen < 0.f)
			{
				S->MaxHealthAtOpen = GetMaxHealth(GetPlayerASC());
				S->HealthAtOpen = GetHealth(GetPlayerASC());
			}
		});
		Mutable->OnRerollsLeftChangedNative.AddLambda([S](int32 Left) { S->Rerolls.Add(Left); });

		// Damage first, so the level-up heal has something to restore.
		S->BaseMaxHealth = GetMaxHealth(GetPlayerASC());
		ApplyDamage(GetPlayerASC(), 30.f);
	});

	// One level-up opens one offer, after the MaxHealth reward and heal.
	Step([this, S]()
	{
		GetGameMode()->AwardXP(GetPC()->GetPlayerState<AGSPlayerState>(), 100.f, /*bApplyXPGain*/ false);
		const UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		TestTrue(TEXT("Level-up opened an offer"), Upgrades->HasOpenOffer());
		TestEqual(TEXT("3 choices"), Upgrades->GetCurrentChoices().Num(), 3);
		TestTrue(TEXT("Choices different"), AllDifferent(Upgrades->GetCurrentChoices()));
		TestTrue(TEXT("Opened event"), S->Changes.Num() == 1 && S->Changes[0] == EGSUpgradeOfferChange::Opened);
		TestEqual(TEXT("Offer opened after the MaxHealth reward"), S->MaxHealthAtOpen, S->BaseMaxHealth + 10.f, 0.01f);
		TestEqual(TEXT("Offer opened after the heal"), S->HealthAtOpen, S->MaxHealthAtOpen, 0.01f);

		// Two more level-ups at once queue two offers behind the open one.
		GetGameMode()->AwardXP(GetPC()->GetPlayerState<AGSPlayerState>(), 125.f + 156.f, false);
		TestEqual(TEXT("Two offers queued"), Upgrades->GetQueuedOffers(), 2);
		TestEqual(TEXT("The open offer didn't change"), S->Changes.Num(), 1);

		// Rerolls: 3, then refused.
		const TArray<UGSUpgradeData*> Before = Upgrades->GetCurrentChoices();
		for (int32 i = 0; i < 3; ++i)
		{
			TestTrue(FString::Printf(TEXT("Reroll %d"), i + 1), GetGameMode()->RerollUpgradeOffer(GetPC()->GetPlayerState<AGSPlayerState>()));
			TestEqual(TEXT("Rerolled offer has 3 choices"), Upgrades->GetCurrentChoices().Num(), 3);
		}
		TestEqual(TEXT("No rerolls left"), Upgrades->GetRerollsLeft(), 0);
		TestTrue(TEXT("Rerolls-left events 2, 1, 0"), S->Rerolls == TArray<int32>({ 2, 1, 0 }));
		TestEqual(TEXT("Three Rerolled events"), S->Changes.FilterByPredicate([](EGSUpgradeOfferChange C) { return C == EGSUpgradeOfferChange::Rerolled; }).Num(), 3);
		const TArray<UGSUpgradeData*> AfterRerolls = Upgrades->GetCurrentChoices();
		TestFalse(TEXT("4th reroll refused"), GetGameMode()->RerollUpgradeOffer(GetPC()->GetPlayerState<AGSPlayerState>()));
		TestTrue(TEXT("Refused reroll keeps the offer"), Upgrades->GetCurrentChoices() == AfterRerolls);
		AddInfo(FString::Printf(TEXT("Offer before rerolls %s; after: %s"), *FString::JoinBy(Before, TEXT(", "), [](const UGSUpgradeData* U) { return U->GetName(); }),
			*FString::JoinBy(AfterRerolls, TEXT(", "), [](const UGSUpgradeData* U) { return U->GetName(); })));

		// Choosing.
		AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		TestFalse(TEXT("Index out of range refused"), GetGameMode()->ChooseUpgrade(PS, 5));
		TestTrue(TEXT("Bad index keeps the offer"), Upgrades->GetCurrentChoices() == AfterRerolls);
		const UGSUpgradeData* Picked = AfterRerolls[1];
		S->Changes.Reset();
		TestTrue(TEXT("Choose 1"), GetGameMode()->ChooseUpgrade(PS, 1));
		TestEqual(TEXT("Chosen upgrade got a stack"), Upgrades->GetStacks(Picked), 1);
		TestTrue(TEXT("The next queued offer replaced it (one Opened, no Closed)"), S->Changes == TArray<EGSUpgradeOfferChange>({ EGSUpgradeOfferChange::Opened }));
		TestEqual(TEXT("One left in the queue"), Upgrades->GetQueuedOffers(), 1);
		TestTrue(TEXT("Choose 0"), GetGameMode()->ChooseUpgrade(PS, 0));
		TestEqual(TEXT("Queue empty"), Upgrades->GetQueuedOffers(), 0);
		TestTrue(TEXT("Last offer open"), Upgrades->HasOpenOffer());
		S->Changes.Reset();
		TestTrue(TEXT("Choose 2"), GetGameMode()->ChooseUpgrade(PS, 2));
		TestFalse(TEXT("No offer after the last pick"), Upgrades->HasOpenOffer());
		TestTrue(TEXT("Only Closed after the last pick"), S->Changes == TArray<EGSUpgradeOfferChange>({ EGSUpgradeOfferChange::Closed }));
		TestEqual(TEXT("Three picks"), Upgrades->GetTotalPicks(), 3);
		TestFalse(TEXT("Choose with no offer refused"), GetGameMode()->ChooseUpgrade(PS, 0));

		// New run: rerolls refilled, offers dropped, upgrades gone; reroll without an offer refused.
		GetGameMode()->QueueUpgradeOfferForDebug(PS);
		TestTrue(TEXT("Debug offer opened"), Upgrades->HasOpenOffer());
		S->Changes.Reset();
		GetGameMode()->RestartRunWithSeed(31);
		TestTrue(TEXT("New run closes the open offer"), S->Changes == TArray<EGSUpgradeOfferChange>({ EGSUpgradeOfferChange::Closed }));
		TestFalse(TEXT("New run: no offer"), Upgrades->HasOpenOffer());
		TestEqual(TEXT("New run: rerolls refilled"), Upgrades->GetRerollsLeft(), 3);
		TestEqual(TEXT("New run: no picks"), Upgrades->GetTotalPicks(), 0);
		TestFalse(TEXT("Reroll with no offer refused"), GetGameMode()->RerollUpgradeOffer(PS));
		TestEqual(TEXT("Refused reroll costs nothing"), Upgrades->GetRerollsLeft(), 3);

		// Same run seed -> same first offer (the roll uses the run stream).
		GetGameMode()->AwardXP(PS, 100.f, false);
		S->FirstOffer = Upgrades->GetCurrentChoices();
		GetGameMode()->RestartRunWithSeed(31);
		GetGameMode()->AwardXP(PS, 100.f, false);
		TestTrue(TEXT("Same seed -> same first offer"), Upgrades->GetCurrentChoices() == S->FirstOffer);

		// The simulator uses its own stream: same result for the same seed, run stream untouched.
		const int32 RunSeedState = GetGameMode()->GetRunStream().GetCurrentSeed();
		const FGSUpgradeOfferSimulation Sim1 = UGSDebugLibrary::SimulateUpgradeOffers(2000, 5);
		const FGSUpgradeOfferSimulation Sim2 = UGSDebugLibrary::SimulateUpgradeOffers(2000, 5);
		TestEqual(TEXT("Run stream untouched by the simulator"), GetGameMode()->GetRunStream().GetCurrentSeed(), RunSeedState);
		TestEqual(TEXT("Simulated offers"), Sim1.Offers, 2000);
		TestEqual(TEXT("Simulated choices"), Sim1.TotalChoices, 6000);
		TestEqual(TEXT("No short offers"), Sim1.ShortOffers, 0);
		TestTrue(TEXT("Simulator is deterministic"), Sim1.UpgradeOffers.OrderIndependentCompareEqual(Sim2.UpgradeOffers));
		TestEqual(TEXT("No Epic choices (empty tier)"), Sim1.RarityChoices.FindRef(TEXT("Epic")), 0);
		TestEqual(TEXT("No Legendary choices (empty tier)"), Sim1.RarityChoices.FindRef(TEXT("Legendary")), 0);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Level-up choice screen: pause, banner first, cards, input lock, keys / gamepad / mouse, rerolls, queued offers
// ---------------------------------------------------------------------------

namespace GSTests
{
	UGSUpgradeChoiceWidget* GetChoiceWidget()
	{
		const AGSPlayerController* PC = GetPC();
		return PC ? PC->GetUpgradeChoiceWidget() : nullptr;
	}

	bool IsWorldPaused()
	{
		const UWorld* World = GetPIEWorld();
		return World && World->IsPaused();
	}

	/** Enqueue: wait until the choice screen shows cards and accepts input. */
	void WaitChoiceReady(FAutomationTestBase* Test, const FString& What)
	{
		WaitUntil(Test, What, []()
		{
			const UGSUpgradeChoiceWidget* Widget = GetChoiceWidget();
			return Widget && Widget->AreCardsShowing() && !Widget->IsInputLocked();
		}, 10.0);
	}

	/** The cards show the open offer, in order, with rarity colors and owned stacks. */
	bool CardsMatchOffer(FAutomationTestBase* Test, const TCHAR* What)
	{
		const UGSUpgradeChoiceWidget* Widget = GetChoiceWidget();
		const UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		const TArray<UGSUpgradeData*> Choices = Upgrades->GetCurrentChoices();
		bool bOk = Test->TestEqual(FString::Printf(TEXT("%s: one card per choice"), What), Widget->GetCardCount(), Choices.Num());
		for (int32 i = 0; bOk && i < Choices.Num(); ++i)
		{
			const UGSUpgradeCardWidget* Card = Widget->GetCard(i);
			bOk &= Test->TestTrue(FString::Printf(TEXT("%s: card %d shows choice %d"), What, i, i), Card && Card->GetUpgrade() == Choices[i]);
			bOk &= Test->TestTrue(FString::Printf(TEXT("%s: card %d border is the rarity color"), What, i),
				Card && Card->GetBorderColor().Equals(Card->GetRarityColor(Choices[i]->Rarity)));
			bOk &= Test->TestEqual(FString::Printf(TEXT("%s: card %d owned text"), What, i), Card ? Card->GetOwnedText().ToString() : FString(),
				FString::Printf(TEXT("Owned: %d"), Upgrades->GetStacks(Choices[i])));
		}
		return bOk;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSUpgradeChoiceUITest, "BanditBaller.UI.UpgradeChoice", GSTests::TestFlags)
bool FGSUpgradeChoiceUITest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		TArray<UGSUpgradeData*> Offer;
		UGSUpgradeData* Picked = nullptr;
		int32 PicksBefore = 0;
	};
	TSharedRef<FState> S = MakeShared<FState>();

	// A level-up pauses the game at once; the banner plays while the cards wait.
	Step([this]()
	{
		GetGameMode()->SetRoundTimerPaused(true);
		const UGSUpgradeChoiceWidget* Widget = GetChoiceWidget();
		if (!TestNotNull(TEXT("Choice widget created by the PlayerController"), Widget))
		{
			return;
		}
		TestFalse(TEXT("No cards at start"), Widget->AreCardsShowing());
		TestFalse(TEXT("Not paused at start"), IsWorldPaused());

		GetGameMode()->AwardXP(GetPC()->GetPlayerState<AGSPlayerState>(), 100.f, false);
		TestTrue(TEXT("Offer pauses the game"), IsWorldPaused());
		TestTrue(TEXT("GameMode owns the pause"), GetGameMode()->IsPausedForUpgradeChoice());
		TestTrue(TEXT("Cards wait for the banner"), Widget->IsRevealPending() && !Widget->AreCardsShowing());
		TestTrue(TEXT("LEVEL UP banner is playing"), GetPC()->GetGSHUD() && GetPC()->GetGSHUD()->IsLevelUpBannerShowing());
	});
	WaitUntil(this, TEXT("cards to appear after the banner"), []() { return GetChoiceWidget()->AreCardsShowing(); }, 10.0);

	// Cards, labels, cursor, and the input lock.
	Step([this, S]()
	{
		UGSUpgradeChoiceWidget* Widget = GetChoiceWidget();
		CardsMatchOffer(this, TEXT("First offer"));
		TestEqual(TEXT("Reroll label"), Widget->GetRerollLabel().ToString(), FString(TEXT("Reroll (3 left)")));
		TestTrue(TEXT("Reroll enabled"), Widget->IsRerollEnabled());
		TestTrue(TEXT("Mouse cursor shown"), GetPC()->ShouldShowMouseCursor());
		TestTrue(TEXT("Input locked right after the cards appear"), Widget->IsInputLocked());

		S->Offer = GetPlayerUpgrades()->GetCurrentChoices();
		TestTrue(TEXT("Key 2 is consumed while locked"), Widget->HandleChoiceKey(EKeys::Two));
		TestFalse(TEXT("Click while locked is ignored"), Widget->TryChoose(0));
		TestTrue(TEXT("Nothing picked while locked"), GetPlayerUpgrades()->GetTotalPicks() == 0 && GetPlayerUpgrades()->GetCurrentChoices() == S->Offer);
	});
	WaitChoiceReady(this, TEXT("input lock to end"));

	// R rerolls: new cards, one reroll used, locked again.
	Step([this, S]()
	{
		UGSUpgradeChoiceWidget* Widget = GetChoiceWidget();
		TestTrue(TEXT("R rerolls"), Widget->HandleChoiceKey(EKeys::R));
		TestEqual(TEXT("One reroll used"), GetPlayerUpgrades()->GetRerollsLeft(), 2);
		TestEqual(TEXT("Reroll label follows"), Widget->GetRerollLabel().ToString(), FString(TEXT("Reroll (2 left)")));
		CardsMatchOffer(this, TEXT("After reroll"));
		TestTrue(TEXT("Reroll locks input again"), Widget->IsInputLocked());
		TestFalse(TEXT("Pick right after a reroll is ignored"), Widget->TryChoose(0));
	});
	WaitChoiceReady(this, TEXT("lock after reroll 1"));
	Step([this]() { TestTrue(TEXT("Gamepad Y rerolls"), GetChoiceWidget()->HandleChoiceKey(EKeys::Gamepad_FaceButton_Top) && GetPlayerUpgrades()->GetRerollsLeft() == 1); });
	WaitChoiceReady(this, TEXT("lock after reroll 2"));
	Step([this]() { TestTrue(TEXT("Reroll button path"), GetChoiceWidget()->TryReroll() && GetPlayerUpgrades()->GetRerollsLeft() == 0); });
	WaitChoiceReady(this, TEXT("lock after reroll 3"));
	Step([this, S]()
	{
		UGSUpgradeChoiceWidget* Widget = GetChoiceWidget();
		TestFalse(TEXT("Reroll button greyed out at 0"), Widget->IsRerollEnabled());
		TestEqual(TEXT("Reroll label at 0"), Widget->GetRerollLabel().ToString(), FString(TEXT("Reroll (0 left)")));
		const TArray<UGSUpgradeData*> Before = GetPlayerUpgrades()->GetCurrentChoices();
		Widget->HandleChoiceKey(EKeys::R);
		TestTrue(TEXT("R at 0 rerolls changes nothing"), GetPlayerUpgrades()->GetCurrentChoices() == Before);

		// Key 2 picks the second card; the last offer closes: hidden, unpaused, game input back.
		S->Picked = Before[1];
		TestTrue(TEXT("Key 2 consumed"), Widget->HandleChoiceKey(EKeys::Two));
		TestEqual(TEXT("Second card picked"), GetPlayerUpgrades()->GetStacks(S->Picked), 1);
		TestFalse(TEXT("Cards hidden after the last pick"), Widget->AreCardsShowing());
		TestFalse(TEXT("Game unpaused"), IsWorldPaused());
		TestFalse(TEXT("Mouse cursor hidden"), GetPC()->ShouldShowMouseCursor());

		// Several level-ups at once (gs.XP.Add 1000 from level 2: levels 3-6) queue several offers.
		S->PicksBefore = GetPlayerUpgrades()->GetTotalPicks();
		GetGameMode()->AwardXP(GetPC()->GetPlayerState<AGSPlayerState>(), 1000.f, false);
		TestEqual(TEXT("1000 XP: one open + 3 queued"), GetPlayerUpgrades()->GetQueuedOffers(), 3);
		TestTrue(TEXT("Paused again"), IsWorldPaused());
	});
	WaitChoiceReady(this, TEXT("first queued offer"));

	// Gamepad: select with the D-pad, confirm with A. The next offer swaps in at once (no banner wait), locked.
	Step([this]()
	{
		UGSUpgradeChoiceWidget* Widget = GetChoiceWidget();
		Widget->HandleChoiceKey(EKeys::Gamepad_DPad_Right);
		TestEqual(TEXT("D-pad selects the first card"), Widget->GetSelectedIndex(), 0);
		Widget->HandleChoiceKey(EKeys::Gamepad_DPad_Right);
		TestEqual(TEXT("D-pad moves the selection"), Widget->GetSelectedIndex(), 1);
		TestTrue(TEXT("A picks the selected card"), Widget->HandleChoiceKey(EKeys::Gamepad_FaceButton_Bottom));
		TestEqual(TEXT("Two queued left"), GetPlayerUpgrades()->GetQueuedOffers(), 2);
		TestTrue(TEXT("Next offer shows at once"), Widget->AreCardsShowing() && !Widget->IsRevealPending());
		TestTrue(TEXT("Next offer is locked"), Widget->IsInputLocked());
		CardsMatchOffer(this, TEXT("Queued offer 2"));
		TestTrue(TEXT("Still paused between queued offers"), IsWorldPaused());
	});
	WaitChoiceReady(this, TEXT("queued offer 2"));
	Step([this]() { TestTrue(TEXT("Mouse click picks"), GetChoiceWidget()->TryChoose(2)); });
	WaitChoiceReady(this, TEXT("queued offer 3"));
	Step([this]() { TestTrue(TEXT("Numpad 1 picks"), GetChoiceWidget()->HandleChoiceKey(EKeys::NumPadOne) && GetPlayerUpgrades()->GetQueuedOffers() == 0); });
	WaitChoiceReady(this, TEXT("queued offer 4"));
	Step([this, S]()
	{
		TestTrue(TEXT("Key 3 picks the last"), GetChoiceWidget()->HandleChoiceKey(EKeys::Three));
		TestEqual(TEXT("Four more picks"), GetPlayerUpgrades()->GetTotalPicks(), S->PicksBefore + 4);
		TestFalse(TEXT("Hidden after the queue"), GetChoiceWidget()->AreCardsShowing());
		TestFalse(TEXT("Unpaused after the queue"), IsWorldPaused());
		TestFalse(TEXT("GameMode pause released"), GetGameMode()->IsPausedForUpgradeChoice());
	});
	EndPIE();
	return true;
}


// ---------------------------------------------------------------------------
// Map boundary: blocks pawns only (walls + ceiling), spawns stay inside
// ---------------------------------------------------------------------------

namespace GSTests
{
	AGSMapBoundary* GetBoundary()
	{
		return AGSMapBoundary::Find(GetPIEWorld());
	}

	/** True if the trace's first blocking hit is the boundary. */
	bool TraceHitsBoundary(const FHitResult& Hit, bool bHit)
	{
		return bHit && Hit.GetActor() && Hit.GetActor()->IsA<AGSMapBoundary>();
	}

	/** A frozen MeleeBandit at Location (no AI), for movement tests. */
	AGSEnemyCharacter* SpawnFrozenBandit(const FVector& Location, const FVector& FaceTowards)
	{
		UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		AGSEnemyCharacter* Enemy = Data ? AGSEnemyCharacter::SpawnEnemy(GetPIEWorld(), Data, Location, FaceTowards) : nullptr;
		if (AAIController* AIC = Enemy ? Cast<AAIController>(Enemy->GetController()) : nullptr)
		{
			if (UBrainComponent* Brain = AIC->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("Boundary test"));
			}
			AIC->StopMovement();
		}
		return Enemy;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSMapBoundaryTest, "BanditBaller.Map.Boundary", GSTests::TestFlags)
bool FGSMapBoundaryTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FBoundaryState
	{
		bool bOldThrottle = true;
		FVector Center = FVector::ZeroVector;
		float MaxX = 0.f;
		float GroundZ = 0.f;
		float CeilingZ = 0.f;
		float MaxPlayerTop = -1e9f;
		float HealthBefore = 0.f;
		FVector2D LaunchXY = FVector2D::ZeroVector;
		TWeakObjectPtr<AGSEnemyCharacter> Bandit;
	};
	TSharedRef<FBoundaryState> S = MakeShared<FBoundaryState>();

	// Placement, collision setup, and what passes through.
	Step([this, S]()
	{
		UEditorPerformanceSettings* Perf = GetMutableDefault<UEditorPerformanceSettings>();
		S->bOldThrottle = Perf->bThrottleCPUWhenNotForeground;
		Perf->bThrottleCPUWhenNotForeground = false;
		GetGameMode()->SetRoundTimerPaused(true);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const AGSMapBoundary* Boundary = GetBoundary();
		if (!TestNotNull(TEXT("L_Town_Graybox has an AGSMapBoundary"), Boundary))
		{
			return;
		}
		TestEqual(TEXT("Ceiling 75 m above the ground"), Boundary->CeilingHeight, 7500.f);
		TestTrue(TEXT("Player starts inside"), Boundary->IsInside(GetPlayer()->GetActorLocation()));
		TestNotNull(TEXT("gs.Debug.Boundary cvar"), IConsoleManager::Get().FindConsoleVariable(TEXT("gs.Debug.Boundary")));
		const FBox Bounds = Boundary->GetPlayBounds();
		S->Center = Bounds.GetCenter();
		S->MaxX = Bounds.Max.X;
		S->GroundZ = Boundary->GetGroundZ();
		S->CeilingZ = Boundary->GetCeilingZ();

		for (const UBoxComponent* Box : Boundary->GetBoxes())
		{
			const FString Name = Box->GetName();
			TestTrue(Name + TEXT(": object type MapBoundary"), Box->GetCollisionObjectType() == GSCollision::MapBoundary);
			TestTrue(Name + TEXT(": blocks pawns"), Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
			for (const ECollisionChannel Channel : { ECC_Visibility, ECC_Camera, ECC_WorldStatic, ECC_WorldDynamic, ECC_PhysicsBody })
			{
				TestTrue(FString::Printf(TEXT("%s: ignores channel %d"), *Name, static_cast<int32>(Channel)), Box->GetCollisionResponseToChannel(Channel) == ECR_Ignore);
			}
			TestFalse(Name + TEXT(": doesn't affect the NavMesh"), Box->CanEverAffectNavigation());
		}
		TestTrue(TEXT("Player capsule blocks MapBoundary"), GetPlayer()->GetCapsuleComponent()->GetCollisionResponseToChannel(GSCollision::MapBoundary) == ECR_Block);
		if (AGSEnemyCharacter* Enemy = SpawnTestEnemy())
		{
			TestTrue(TEXT("Enemy capsule blocks MapBoundary"), Enemy->GetCapsuleComponent()->GetCollisionResponseToChannel(GSCollision::MapBoundary) == ECR_Block);
			RemoveAllEnemies(GetPIEWorld());
		}

		// Traces through the east wall and through the ceiling.
		UWorld* World = GetPIEWorld();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBoundaryTest), false, GetPlayer());
		const FVector WallStart(Bounds.Max.X - 200.f, S->Center.Y + 1234.f, S->GroundZ + 150.f);
		const FVector WallEnd = WallStart + FVector(3000.f, 0.f, 0.f);
		const FVector UpStart(S->Center.X + 777.f, S->Center.Y, S->CeilingZ - 300.f);
		const FVector UpEnd = UpStart + FVector(0.f, 0.f, 2000.f);
		for (const TPair<FVector, FVector>& Line : { TPair<FVector, FVector>(WallStart, WallEnd), TPair<FVector, FVector>(UpStart, UpEnd) })
		{
			FHitResult Hit;
			TestFalse(TEXT("Player shots (Visibility) pass through"), TraceHitsBoundary(Hit, World->LineTraceSingleByChannel(Hit, Line.Key, Line.Value, ECC_Visibility, Params)));
			TestFalse(TEXT("Camera (Camera channel) passes through"), TraceHitsBoundary(Hit, World->LineTraceSingleByChannel(Hit, Line.Key, Line.Value, ECC_Camera, Params)));
			TestFalse(TEXT("Enemy shots (object query) pass through"), TraceHitsBoundary(Hit, World->LineTraceSingleByObjectType(Hit, Line.Key, Line.Value, AGSEnemyCharacter::GetShotObjectQueryParams(), Params)));
			TestFalse(TEXT("Pickup traces (WorldStatic) pass through"), TraceHitsBoundary(Hit, World->LineTraceSingleByObjectType(Hit, Line.Key, Line.Value, FCollisionObjectQueryParams(ECC_WorldStatic), Params)));
			TestTrue(TEXT("A pawn-channel sweep is blocked"), TraceHitsBoundary(Hit, World->SweepSingleByChannel(Hit, Line.Key, Line.Value, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(34.f, 88.f), Params)));
		}

		// Spawn points near a corner stay inside.
		GetPlayer()->TeleportTo(FVector(Bounds.Max.X - 600.f, Bounds.Max.Y - 600.f, S->GroundZ + 120.f), FRotator::ZeroRotator);
	});
	WaitFrames(this, 3);
	Step([this, S]()
	{
		FGSSpawnSearchStats Stats;
		const TArray<FGSSpawnPoint> Points = UGSSpawnLocationFinder::FindSpawnLocations(GetPC(), 12, &Stats);
		AddInfo(FString::Printf(TEXT("Corner spawn search: %s"), *Stats.ToString()));
		bool bAllInside = Points.Num() > 0;
		for (const FGSSpawnPoint& Point : Points)
		{
			bAllInside &= GetBoundary()->IsInside(Point.Location);
		}
		TestTrue(TEXT("Spawn points near a corner are inside the boundary"), bAllInside);

		// Player launched at the east wall; an enemy launched at it a few meters along.
		GetPlayer()->TeleportTo(FVector(S->MaxX - 300.f, S->Center.Y, S->GroundZ + 120.f), FRotator::ZeroRotator);
		GetPlayer()->LaunchCharacter(FVector(3000.f, 0.f, 300.f), true, true);
		S->Bandit = SpawnFrozenBandit(FVector(S->MaxX - 400.f, S->Center.Y + 600.f, S->GroundZ), FVector(S->MaxX + 1000.f, S->Center.Y + 600.f, S->GroundZ));
		if (TestTrue(TEXT("Bandit spawned"), S->Bandit.IsValid()))
		{
			S->Bandit->LaunchCharacter(FVector(3000.f, 0.f, 300.f), true, true);
		}
	});
	WaitWorldSeconds(this, 1.0);
	Step([this, S]()
	{
		TestTrue(TEXT("Player stopped by the wall"), GetBoundary()->IsInside(GetPlayer()->GetActorLocation()));
		TestTrue(TEXT("Enemy stopped by the wall"), S->Bandit.IsValid() && GetBoundary()->IsInside(S->Bandit->GetActorLocation()));

		// Dash into the wall from 3 m away (the dash covers 6 m).
		GetPlayer()->TeleportTo(FVector(S->MaxX - 300.f, S->Center.Y - 600.f, S->GroundZ + 120.f), FRotator::ZeroRotator);
	});
	WaitFrames(this, 3);
	Step([]()
	{
		if (const UGSDashSettings* Dash = LoadObject<UGSDashSettings>(nullptr, TEXT("/Game/BanditBaller/Data/DA_DashSettings.DA_DashSettings")))
		{
			GetPlayer()->StartDash(FVector(1.f, 0.f, 0.f), *Dash);
		}
	});
	WaitUntil(this, TEXT("dash into the wall to end"), []() { return !GetPlayer()->IsDashing(); }, 5.0);
	Step([this, S]()
	{
		TestTrue(TEXT("Dash stopped by the wall"), GetBoundary()->IsInside(GetPlayer()->GetActorLocation()));

		// Slide into the wall, facing it.
		GetPlayer()->TeleportTo(FVector(S->MaxX - 400.f, S->Center.Y - 1200.f, S->GroundZ + 120.f), FRotator::ZeroRotator);
		GetPlayer()->SetActorRotation(FRotator::ZeroRotator);
	});
	WaitFrames(this, 3);
	Step([this]()
	{
		if (const UGSSlideSettings* Slide = LoadObject<UGSSlideSettings>(nullptr, TEXT("/Game/BanditBaller/Data/DA_SlideSettings.DA_SlideSettings")))
		{
			GetPlayer()->StartSlide(*Slide);
		}
		TestTrue(TEXT("Sliding toward the wall"), GetPlayer()->IsSliding());
	});
	WaitWorldSeconds(this, 1.2);
	Step([this, S]()
	{
		TestTrue(TEXT("Slide stopped by the wall"), GetBoundary()->IsInside(GetPlayer()->GetActorLocation()));

		// Launched straight up under the ceiling: it stops there, no damage, no teleport.
		const FVector Start(S->Center.X + 500.f, S->Center.Y + 500.f, S->CeilingZ - 400.f);
		GetPlayer()->TeleportTo(Start, FRotator::ZeroRotator);
		S->LaunchXY = FVector2D(Start);
		S->HealthBefore = GetHealth(GetPlayerASC());
		S->MaxPlayerTop = -1e9f;
		GetPlayer()->LaunchCharacter(FVector(0.f, 0.f, 4000.f), true, true);
	});
	WaitUntil(this, TEXT("launch to peak at the ceiling"), [S]()
	{
		const AGSCharacter* Player = GetPlayer();
		S->MaxPlayerTop = FMath::Max(S->MaxPlayerTop, static_cast<float>(Player->GetActorLocation().Z + Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		return Player->GetVelocity().Z < -200.f;
	}, 5.0);
	Step([this, S]()
	{
		AddInfo(FString::Printf(TEXT("Highest capsule top %.0f, ceiling at %.0f"), S->MaxPlayerTop, S->CeilingZ));
		TestTrue(TEXT("Ceiling stops the launch"), S->MaxPlayerTop <= S->CeilingZ + 2.f);
		TestTrue(TEXT("Reached the ceiling"), S->MaxPlayerTop >= S->CeilingZ - 50.f);
		TestEqual(TEXT("No damage from the ceiling"), GetHealth(GetPlayerASC()), S->HealthBefore, 0.01f);
		TestTrue(TEXT("No teleport (same XY)"), FVector2D::Distance(FVector2D(GetPlayer()->GetActorLocation()), S->LaunchXY) < 50.f);

		// Air jumps still work under the ceiling (gs.Upgrade.Grant with a count).
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("ExtraJump"), TEXT("3") });
		TestEqual(TEXT("gs.Upgrade.Grant ExtraJump 3"), GetPlayerUpgrades()->GetStacks(UGSDebugLibrary::FindUpgradeData(TEXT("ExtraJump"))), 3);
		S->MaxPlayerTop = -1e9f;
		GetPlayer()->Jump();
	});
	WaitUntil(this, TEXT("air jump under the ceiling"), []() { return GetPlayer()->GetAirJumpsSinceLanded() >= 1; }, 3.0);
	Step([]() { GetPlayer()->StopJumping(); });
	WaitUntil(this, TEXT("air jump to hit the ceiling and fall"), [S]()
	{
		const AGSCharacter* Player = GetPlayer();
		S->MaxPlayerTop = FMath::Max(S->MaxPlayerTop, static_cast<float>(Player->GetActorLocation().Z + Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		return Player->GetVelocity().Z < -200.f;
	}, 5.0);
	Step([this, S]()
	{
		TestEqual(TEXT("The air jump happened"), GetPlayer()->GetAirJumpsSinceLanded(), 1);
		TestTrue(TEXT("Air jump stopped by the ceiling"), S->MaxPlayerTop <= S->CeilingZ + 2.f);
		GetPlayer()->TeleportTo(S->Center + FVector(0.f, 0.f, 150.f), FRotator::ZeroRotator);
		GetMutableDefault<UEditorPerformanceSettings>()->bThrottleCPUWhenNotForeground = S->bOldThrottle;
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Kill combo
// ---------------------------------------------------------------------------

namespace GSTests
{
	int32 SumActiveXPOrbs()
	{
		int32 Sum = 0;
		for (const AGSCashPickup* Pickup : GetCashSubsystem()->GetActivePickups())
		{
			Sum += (Pickup && Pickup->IsXPOrb()) ? Pickup->GetValue() : 0;
		}
		return Sum;
	}

	/** What one kill dropped: cash pieces and XP orb value. */
	struct FComboKillDrop
	{
		int32 Cash = 0;
		int32 XP = 0;
	};

	/** Kill a MeleeBandit (spawn ring, out of pickup reach) as the player; returns what its death dropped. */
	FComboKillDrop KillForCombo(FAutomationTestBase* Test)
	{
		const int32 CashBefore = SumActiveCash();
		const int32 XPBefore = SumActiveXPOrbs();
		KillTestEnemyAsPlayer(Test);
		return { SumActiveCash() - CashBefore, SumActiveXPOrbs() - XPBefore };
	}

	/** Enqueue: wait Seconds of real time (the PIE world may be paused). */
	void WaitRealSeconds(FAutomationTestBase* Test, double Seconds)
	{
		TSharedRef<double> End = MakeShared<double>(-1.0);
		WaitUntil(Test, FString::Printf(TEXT("%.1f s of real time"), Seconds), [End, Seconds]()
		{
			if (*End < 0.0)
			{
				*End = FPlatformTime::Seconds() + Seconds;
			}
			return FPlatformTime::Seconds() >= *End;
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSKillComboTest, "BanditBaller.Economy.KillCombo", GSTests::TestFlags)
bool FGSKillComboTest::RunTest(const FString& Parameters)
{
	struct FComboState
	{
		FVector PlayerHome = FVector(0.f, 0.f, 100.f);
		int32 CashValue = 0;
		int32 XPValue = 0;
		int32 MaxStepCombo = 0;
		int32 ExpectedGroundCash = 0;
		int32 CashBefore = 0;
		double LastKillTime = 0.0;
		float PausedRemaining = 0.f;
		int32 EndedCount = -1;
		EGSComboEndReason EndedReason = EGSComboEndReason::Reset;
		double EndedTime = 0.0;
		FDelegateHandle EndedHandle;
	};
	TSharedRef<FComboState> S = MakeShared<FComboState>();

	BeginPIE(this);

	// Data, then one combo through every multiplier step.
	Step([this, S]()
	{
		AGSGameMode* GameMode = GetGameMode();
		const UGSComboSettings* Settings = GameMode ? GameMode->GetComboSettings() : nullptr;
		UGSComboComponent* Combo = GetPlayerCombo();
		const UGSHUDWidget* HUD = GetPC() ? GetPC()->GetGSHUD() : nullptr;
		if (!TestNotNull(TEXT("BP_GSGameMode has ComboSettings (DA_ComboSettings)"), Settings) || !TestNotNull(TEXT("Combo component on the PlayerState"), Combo)
			|| !TestNotNull(TEXT("HUD"), HUD))
		{
			return;
		}
		GameMode->SetRoundTimerPaused(true);
		GameMode->RestartRunWithSeed(2201);
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		GetPlayer()->TeleportTo(S->PlayerHome, FRotator::ZeroRotator);
		// Collected orbs give no XP, so the vacuum below can't level up (and pause) mid-test. gs.XP.Add still levels.
		GetPlayerASC()->SetNumericAttributeBase(UGSAttributeSet::GetXPGainAttribute(), 0.f);
		S->EndedHandle = Combo->OnComboEndedNative.AddLambda([S](int32 FinalCount, EGSComboEndReason Reason)
		{
			S->EndedCount = FinalCount;
			S->EndedReason = Reason;
			S->EndedTime = GetPIEWorld() ? GetPIEWorld()->GetTimeSeconds() : 0.0;
		});

		TestEqual(TEXT("Combo window 2.5 s"), Settings->ComboWindow, 2.5f);
		TestTrue(TEXT("Damage ends the combo by default"), Settings->bEndComboOnDamage);
		TestFalse(TEXT("The combo doesn't multiply XP by default"), Settings->bApplyComboToXP);
		const TArray<FGSComboStep>& Steps = Settings->CashMultiplierSteps;
		if (TestEqual(TEXT("4 multiplier steps"), Steps.Num(), 4))
		{
			const float Expected[] = { 1.f, 1.25f, 1.5f, 2.f };
			TestEqual(TEXT("First step at 1 kill"), Steps[0].MinCombo, 1);
			for (int32 Index = 0; Index < 4; ++Index)
			{
				TestEqual(FString::Printf(TEXT("Step %d multiplier"), Index), Steps[Index].Multiplier, Expected[Index]);
				if (Index > 0)
				{
					TestTrue(FString::Printf(TEXT("Step %d starts after step %d"), Index, Index - 1), Steps[Index].MinCombo > Steps[Index - 1].MinCombo);
					TestEqual(FString::Printf(TEXT("One kill short of step %d: previous multiplier"), Index),
						Settings->GetMultiplierForCombo(Steps[Index].MinCombo - 1), Steps[Index - 1].Multiplier);
					TestEqual(FString::Printf(TEXT("Step %d reached at %d kills"), Index, Steps[Index].MinCombo),
						Settings->GetMultiplierForCombo(Steps[Index].MinCombo), Steps[Index].Multiplier);
				}
			}
			S->MaxStepCombo = Steps.Last().MinCombo;
		}
		TestEqual(TEXT("No combo: x1"), Settings->GetMultiplierForCombo(0), 1.f);
		TestEqual(TEXT("Huge combo: the last step"), Settings->GetMultiplierForCombo(1000), 2.f);
		TestEqual(TEXT("New run: no combo"), Combo->GetComboCount(), 0);
		TestFalse(TEXT("New run: no combo on the HUD"), HUD->IsComboShowing());

		const UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		S->CashValue = Data ? Data->CashValue : 0;
		S->XPValue = Data ? Data->XPValue : 0;
		if (!TestTrue(TEXT("MeleeBandit drops cash and XP"), S->CashValue > 0 && S->XPValue > 0) || S->MaxStepCombo <= 0)
		{
			return;
		}

		// Every kill adds one, restarts the window and pays at the step it reaches; XP stays flat.
		for (int32 Kill = 1; Kill <= S->MaxStepCombo; ++Kill)
		{
			const FComboKillDrop Drop = KillForCombo(this);
			const float Multiplier = Settings->GetMultiplierForCombo(Kill);
			TestEqual(FString::Printf(TEXT("Kill %d: combo count"), Kill), Combo->GetComboCount(), Kill);
			TestEqual(FString::Printf(TEXT("Kill %d: multiplier"), Kill), Combo->GetCashMultiplier(), Multiplier);
			TestEqual(FString::Printf(TEXT("Kill %d: cash drop = round(CashValue x %.2f)"), Kill, Multiplier), Drop.Cash, FMath::RoundToInt(S->CashValue * Multiplier));
			TestEqual(FString::Printf(TEXT("Kill %d: XP orb = XPValue"), Kill), Drop.XP, S->XPValue);
			TestEqual(FString::Printf(TEXT("Kill %d: full window"), Kill), Combo->GetTimeRemaining(), Settings->ComboWindow, 0.01f);
			TestEqual(FString::Printf(TEXT("Kill %d: HUD count"), Kill), HUD->GetDisplayedComboCount(), Kill);
			TestEqual(FString::Printf(TEXT("Kill %d: HUD multiplier"), Kill), HUD->GetDisplayedComboMultiplier(), Multiplier);
			TestTrue(FString::Printf(TEXT("Kill %d: HUD shows the combo from %d kills"), Kill, HUD->GetComboMinCountToShow()),
				HUD->IsComboShowing() == (Kill >= HUD->GetComboMinCountToShow()));
			S->ExpectedGroundCash += FMath::RoundToInt(S->CashValue * Multiplier);
		}
		TestEqual(TEXT("HUD count text"), HUD->GetComboCountText().ToString(), FString::Printf(TEXT("%d COMBO"), S->MaxStepCombo));
		TestEqual(TEXT("HUD multiplier text"), HUD->GetComboMultiplierText().ToString(), FString(TEXT("x2 CASH")));
		TestEqual(TEXT("Run stats: best combo"), GetGameState()->GetRunStats()->GetBestCombo(), S->MaxStepCombo);

		// Target dummies aren't enemies: killing one doesn't count.
		AGSTargetDummy* Dummy = nullptr;
		for (TActorIterator<AGSTargetDummy> It(GetPIEWorld()); It; ++It)
		{
			Dummy = *It;
			break;
		}
		if (TestNotNull(TEXT("A target dummy in the town"), Dummy))
		{
			ApplyDamage(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Dummy), 100000.f, GetPlayerASC());
			TestEqual(TEXT("A target dummy kill doesn't count"), Combo->GetComboCount(), S->MaxStepCombo);
		}
		S->LastKillTime = GetPIEWorld()->GetTimeSeconds();
	});

	// The window drains with game time; the next kill restarts it.
	WaitWorldSeconds(this, 1.0);
	Step([this, S]()
	{
		UGSComboComponent* Combo = GetPlayerCombo();
		const UGSComboSettings* Settings = GetGameMode()->GetComboSettings();
		const UGSHUDWidget* HUD = GetPC()->GetGSHUD();
		const float Elapsed = static_cast<float>(GetPIEWorld()->GetTimeSeconds() - S->LastKillTime);
		TestTrue(TEXT("Combo still running after 1 s"), Combo->IsComboActive());
		TestEqual(TEXT("The window drains with game time"), Combo->GetTimeRemaining(), Settings->ComboWindow - Elapsed, 0.05f);
		TestTrue(FString::Printf(TEXT("HUD timer bar drained (%.2f)"), HUD->GetDisplayedComboTimerPercent()),
			HUD->GetDisplayedComboTimerPercent() > 0.f && HUD->GetDisplayedComboTimerPercent() < 0.9f);
		TestEqual(TEXT("HUD timer bar follows the window"), HUD->GetDisplayedComboTimerPercent(), Combo->GetTimeRemainingFraction(), 0.15f);

		const FComboKillDrop Drop = KillForCombo(this);
		TestEqual(TEXT("A kill inside the window continues the combo"), Combo->GetComboCount(), S->MaxStepCombo + 1);
		TestEqual(TEXT("... and restarts the window"), Combo->GetTimeRemaining(), Settings->ComboWindow, 0.01f);
		TestEqual(TEXT("Past the last step: still the top multiplier"), Drop.Cash,
			FMath::RoundToInt(S->CashValue * Settings->GetMultiplierForCombo(S->MaxStepCombo + 1)));
		S->ExpectedGroundCash += Drop.Cash;
		S->LastKillTime = GetPIEWorld()->GetTimeSeconds();
		S->EndedCount = -1;
	});
	WaitUntil(this, TEXT("combo window to run out"), []() { return GetPlayerCombo() && !GetPlayerCombo()->IsComboActive(); }, 10.0);

	// Pickups keep the multiplier of the kill that dropped them, though the combo is over.
	Step([this, S]()
	{
		const UGSComboSettings* Settings = GetGameMode()->GetComboSettings();
		TestEqual(TEXT("Ended: final count"), S->EndedCount, S->MaxStepCombo + 1);
		TestTrue(TEXT("Ended: timed out"), S->EndedReason == EGSComboEndReason::TimedOut);
		const double Lasted = S->EndedTime - S->LastKillTime;
		TestTrue(FString::Printf(TEXT("Ended one window after the last kill (%.2f s)"), Lasted),
			Lasted >= Settings->ComboWindow - 0.01 && Lasted <= Settings->ComboWindow + 0.5);
		TestEqual(TEXT("Ended: back to x1"), GetPlayerCombo()->GetCashMultiplier(), 1.f);
		TestFalse(TEXT("Ended: no combo on the HUD"), GetPC()->GetGSHUD()->IsComboShowing());
		TestEqual(TEXT("Run stats keep the best combo"), GetGameState()->GetRunStats()->GetBestCombo(), S->MaxStepCombo + 1);

		TestEqual(TEXT("Cash on the ground = the combo's multiplied drops"), SumActiveCash(), S->ExpectedGroundCash);
		S->CashBefore = GetPlayerCash();
		GetGameMode()->SetPhase(EGSRoundPhase::RoundClear);
	});
	WaitUntil(this, TEXT("vacuumed pickups to arrive"), []() { return GetCashSubsystem()->GetNumActive() == 0; }, 15.0);

	// The window stops while the game is paused (level-up -> upgrade choice).
	Step([this, S]()
	{
		TestEqual(TEXT("Collected after the combo ended: paid at the multipliers of the kills"), GetPlayerCash() - S->CashBefore, S->ExpectedGroundCash);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRound(1);

		KillForCombo(this);
		const UGSComboComponent* Combo = GetPlayerCombo();
		TestEqual(TEXT("New combo"), Combo->GetComboCount(), 1);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("XPAdd"), { TEXT("100") });
		TestTrue(TEXT("Level-up opened an upgrade offer and paused the game"), GameMode->IsPausedForUpgradeChoice() && GetPIEWorld()->IsPaused());
		S->PausedRemaining = Combo->GetTimeRemaining();
		TestTrue(TEXT("Window running when the pause starts"), S->PausedRemaining > 0.f);
	});
	WaitRealSeconds(this, 1.0);
	Step([this, S]()
	{
		const UGSComboComponent* Combo = GetPlayerCombo();
		TestTrue(TEXT("Still paused"), GetPIEWorld()->IsPaused());
		TestEqual(TEXT("Paused: the window doesn't drain"), Combo->GetTimeRemaining(), S->PausedRemaining, 0.001f);
		TestEqual(TEXT("Paused: combo kept"), Combo->GetComboCount(), 1);
		TestTrue(TEXT("Upgrade picked"), PickAllUpgradeOffers() > 0);
		TestFalse(TEXT("Picking unpaused the game"), GetPIEWorld()->IsPaused());
	});
	WaitWorldSeconds(this, 0.5);

	// Damage, the two options, and the run reset.
	Step([this, S]()
	{
		AGSGameMode* GameMode = GetGameMode();
		UGSComboComponent* Combo = GetPlayerCombo();
		const UGSHUDWidget* HUD = GetPC()->GetGSHUD();
		TestTrue(TEXT("Unpaused: the window drains again"), Combo->IsComboActive() && Combo->GetTimeRemaining() < S->PausedRemaining - 0.45f);

		KillForCombo(this);
		TestEqual(TEXT("Second kill"), Combo->GetComboCount(), 2);
		TestTrue(TEXT("A combo of 2 shows on the HUD"), HUD->IsComboShowing());
		ApplyDamage(GetPlayerASC(), 5.f);
		TestEqual(TEXT("God mode blocks the damage: combo kept"), Combo->GetComboCount(), 2);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		const float HealthBefore = GetHealth(GetPlayerASC());
		ApplyDamage(GetPlayerASC(), 5.f);
		TestTrue(TEXT("Player lost health"), GetHealth(GetPlayerASC()) < HealthBefore);
		TestEqual(TEXT("Damage ends the combo"), Combo->GetComboCount(), 0);
		TestTrue(TEXT("Ended: damage taken"), S->EndedReason == EGSComboEndReason::DamageTaken);
		TestFalse(TEXT("Damage: no combo on the HUD"), HUD->IsComboShowing());

		// bEndComboOnDamage off, bApplyComboToXP on.
		UGSComboSettings* Override = DuplicateObject<UGSComboSettings>(GameMode->GetComboSettings(), GetTransientPackage());
		Override->bEndComboOnDamage = false;
		Override->bApplyComboToXP = true;
		GameMode->SetComboSettingsOverride(Override);
		const int32 Kills = Override->CashMultiplierSteps.Num() > 1 ? Override->CashMultiplierSteps[1].MinCombo : 1;
		for (int32 Kill = 1; Kill <= Kills; ++Kill)
		{
			const FComboKillDrop Drop = KillForCombo(this);
			const float Multiplier = Override->GetMultiplierForCombo(Kill);
			TestEqual(FString::Printf(TEXT("bApplyComboToXP, kill %d: XP orb = round(XPValue x %.2f)"), Kill, Multiplier), Drop.XP, FMath::RoundToInt(S->XPValue * Multiplier));
			TestEqual(FString::Printf(TEXT("bApplyComboToXP, kill %d: cash still multiplied"), Kill), Drop.Cash, FMath::RoundToInt(S->CashValue * Multiplier));
		}
		const float HealthBeforeOption = GetHealth(GetPlayerASC());
		ApplyDamage(GetPlayerASC(), 5.f);
		TestTrue(TEXT("Player lost health again"), GetHealth(GetPlayerASC()) < HealthBeforeOption);
		TestEqual(TEXT("bEndComboOnDamage off: damage keeps the combo"), Combo->GetComboCount(), Kills);
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		GameMode->SetComboSettingsOverride(nullptr);

		// A new run ends the combo and clears the best combo.
		GameMode->RestartRunWithSeed(2202);
		TestEqual(TEXT("New run: combo ended"), Combo->GetComboCount(), 0);
		TestTrue(TEXT("New run: ended with Reset"), S->EndedReason == EGSComboEndReason::Reset);
		TestEqual(TEXT("New run: best combo 0"), GetGameState()->GetRunStats()->GetBestCombo(), 0);
		TestTrue(TEXT("gs.Run.Stats shows the best combo"), GetGameState()->GetRunStats()->ToDebugString().Contains(TEXT("Best combo")));

		Combo->OnComboEndedNative.Remove(S->EndedHandle);
		GameMode->SetRoundTimerPaused(false);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Round-clear bonus + RoundClear summary
// ---------------------------------------------------------------------------

namespace GSTests
{
	UGSRoundSummaryWidget* GetRoundSummary()
	{
		return GetPC() ? GetPC()->GetRoundSummaryWidget() : nullptr;
	}

	const FGSRoundStats& GetRoundStats()
	{
		return GetGameState()->GetRunStats()->GetCurrentRound();
	}

	const FGSRoundClearPayout& GetLastPayout()
	{
		return GetGameState()->GetRunStats()->GetLastRoundClearPayout();
	}

	/** Enqueue: kill the wave as it spawns until Combat ends in RoundClear. */
	void ClearWaveAsPlayer(FAutomationTestBase* Test)
	{
		WaitUntil(Test, TEXT("wave killed -> RoundClear"), []()
		{
			KillAllEnemiesAsPlayer();
			return IsPhase(EGSRoundPhase::RoundClear);
		}, 30.0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSRoundClearBonusTest, "BanditBaller.Round.ClearBonus", GSTests::TestFlags)
bool FGSRoundClearBonusTest::RunTest(const FString& Parameters)
{
	struct FBonusState
	{
		int32 CashBefore = 0;
		int32 EarnedBefore = 0;
		int32 FirstTotal = 0;
		int32 CashValue = 0;
		int32 XPValue = 0;
	};
	TSharedRef<FBonusState> S = MakeShared<FBonusState>();

	BeginPIE(this);

	// Settings, then round 1: no damage, cleared fast.
	Step([this, S]()
	{
		AGSGameMode* GameMode = GetGameMode();
		const UGSRoundClearSettings* Settings = GameMode ? GameMode->GetRoundClearSettings() : nullptr;
		const UGSRoundSummaryWidget* Summary = GetRoundSummary();
		if (!TestNotNull(TEXT("BP_GSGameMode has RoundClearSettings (DA_RoundClearSettings)"), Settings)
			|| !TestNotNull(TEXT("Round summary widget (RoundSummaryWidgetClass on BP_GSPlayerController)"), Summary))
		{
			return;
		}
		TestEqual(TEXT("Base bonus $20"), Settings->BaseBonus, 20);
		TestEqual(TEXT("No-damage bonus $15"), Settings->NoDamageBonus, 15);
		TestEqual(TEXT("Fast-clear bonus $15"), Settings->FastClearBonus, 15);
		TestEqual(TEXT("Fast-clear time: 8 s + 1.5 s per enemy (4 enemies -> 14 s)"), Settings->GetFastClearTime(4), 14.f);

		GameMode->SetRoundTimerPaused(true);
		GameMode->RestartRunWithSeed(2301);
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		GetPlayer()->TeleportTo(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);
		const UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		S->CashValue = Data ? Data->CashValue : 0;
		S->XPValue = Data ? Data->XPValue : 0;

		TestEqual(TEXT("PreRound starts round 1's stats"), GetRoundStats().Round, 1);
		TestTrue(TEXT("Combat hasn't started"), GetRoundStats().CombatStartTime < 0.0);
		TestEqual(TEXT("New run: no payout yet"), GetLastPayout().Round, 0);
		TestFalse(TEXT("No summary in PreRound"), Summary->IsSummaryShowing());

		S->CashBefore = GetPlayerCash();
		S->EarnedBefore = GetCashEarnedStat();
		GameMode->AdvancePhase(); // -> Combat
		TestTrue(TEXT("Combat start recorded"), GetRoundStats().CombatStartTime >= 0.0);
		TestEqual(TEXT("Wave size recorded"), GetRoundStats().WaveEnemies, GameMode->GetWaveDirector()->GetTotal());
	});
	ClearWaveAsPlayer(this);
	Step([this, S]()
	{
		const UGSRoundClearSettings* Settings = GetGameMode()->GetRoundClearSettings();
		const FGSRoundClearPayout& Payout = GetLastPayout();
		const FGSRoundStats& Round = GetRoundStats();
		TestEqual(TEXT("Payout for round 1"), Payout.Round, 1);
		TestEqual(TEXT("Base bonus"), Payout.BaseBonus, Settings->BaseBonus);
		TestTrue(TEXT("No damage taken"), Payout.bNoDamage && Payout.NoDamageBonus == Settings->NoDamageBonus);
		TestEqual(TEXT("Fast-clear limit from the wave size"), Payout.FastClearTime, Settings->GetFastClearTime(Round.WaveEnemies));
		TestTrue(FString::Printf(TEXT("Cleared fast (%.1f s of %.1f s)"), Payout.ClearTime, Payout.FastClearTime),
			Payout.ClearTime > 0.f && Payout.bFastClear && Payout.FastClearBonus == Settings->FastClearBonus);
		TestEqual(TEXT("Total = base + no damage + fast clear"), Payout.GetTotal(), Settings->BaseBonus + Settings->NoDamageBonus + Settings->FastClearBonus);
		TestEqual(TEXT("Every wave enemy counted as a kill this round"), Round.Kills, Round.WaveEnemies);
		// The bonus is in the profile at once; pickups keep arriving from the vacuum.
		TestEqual(TEXT("Profile += bonus + cash collected so far"), GetPlayerCash() - S->CashBefore, Payout.GetTotal() + Round.CashCollected);
		S->FirstTotal = Payout.GetTotal();

		const UGSRoundSummaryWidget* Summary = GetRoundSummary();
		TestTrue(TEXT("Summary shows in RoundClear"), Summary->IsSummaryShowing());
		TestEqual(TEXT("Summary round"), Summary->GetShownRound(), 1);
		TestEqual(TEXT("Summary title"), Summary->GetTitleText().ToString(), FString(TEXT("ROUND 1 CLEAR")));
		TestEqual(TEXT("Summary base line"), Summary->GetBaseBonusText().ToString(), FString::Printf(TEXT("+$%d"), Settings->BaseBonus));
		TestEqual(TEXT("Summary no-damage line"), Summary->GetNoDamageBonusText().ToString(), FString::Printf(TEXT("+$%d"), Settings->NoDamageBonus));
		TestEqual(TEXT("Summary fast-clear line"), Summary->GetFastClearBonusText().ToString(), FString::Printf(TEXT("+$%d"), Settings->FastClearBonus));
		TestEqual(TEXT("Summary total"), Summary->GetTotalText().ToString(), FString::Printf(TEXT("+$%d"), Payout.GetTotal()));
	});
	WaitUntil(this, TEXT("RoundClear vacuum to finish"), []() { return GetCashSubsystem()->GetNumActive() == 0; }, 15.0);
	Step([this, S]()
	{
		const FGSRoundClearPayout& Payout = GetLastPayout();
		const FGSRoundStats& Round = GetRoundStats();
		TestTrue(TEXT("Cash collected this round: every drop, at least CashValue each"), Round.CashCollected >= Round.Kills * S->CashValue && Round.CashCollected > 0);
		TestEqual(TEXT("XP earned this round: one orb per kill"), Round.XPEarned, Round.Kills * S->XPValue);
		TestEqual(TEXT("Profile += cash collected + bonus"), GetPlayerCash() - S->CashBefore, Round.CashCollected + Payout.GetTotal());
		TestEqual(TEXT("Run stats CashEarned += cash collected + bonus"), GetCashEarnedStat() - S->EarnedBefore, Round.CashCollected + Payout.GetTotal());

		const UGSRoundSummaryWidget* Summary = GetRoundSummary();
		TestTrue(TEXT("Summary still up"), Summary->IsSummaryShowing());
		TestEqual(TEXT("Summary kills"), Summary->GetKillsText().ToString(), FString::FromInt(Round.Kills));
		TestEqual(TEXT("Summary cash follows the vacuum"), Summary->GetCashCollectedText().ToString(), FString::Printf(TEXT("$%d"), Round.CashCollected));
		TestEqual(TEXT("Summary XP"), Summary->GetXPEarnedText().ToString(), FString::Printf(TEXT("%d XP"), Round.XPEarned));

		// Round 2: damage taken and an impossible time limit -> base bonus only.
		AGSGameMode* GameMode = GetGameMode();
		UGSRoundClearSettings* Slow = DuplicateObject<UGSRoundClearSettings>(GameMode->GetRoundClearSettings(), GetTransientPackage());
		Slow->FastClearBaseTime = 0.f;
		Slow->FastClearTimePerEnemy = 0.f;
		GameMode->SetRoundClearSettingsOverride(Slow);
		GameMode->AdvancePhase(); // RoundClear -> round 2 PreRound
		TestFalse(TEXT("Summary hidden in the next PreRound"), Summary->IsSummaryShowing());
		TestTrue(TEXT("Round 2 stats start over"), GetRoundStats().Round == 2 && GetRoundStats().Kills == 0 && GetRoundStats().CashCollected == 0
			&& GetRoundStats().XPEarned == 0 && GetRoundStats().DamageTaken == 0.f);

		UGSDebugLibrary::SetGodMode(GetPC(), false);
		ApplyDamage(GetPlayerASC(), 5.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		TestEqual(TEXT("Damage taken this round"), GetRoundStats().DamageTaken, 5.f, 0.01f);
		S->CashBefore = GetPlayerCash();
		GameMode->AdvancePhase(); // -> Combat
	});
	ClearWaveAsPlayer(this);
	Step([this, S]()
	{
		AGSGameMode* GameMode = GetGameMode();
		const UGSRoundClearSettings* Settings = GameMode->GetRoundClearSettings();
		const FGSRoundClearPayout& Payout = GetLastPayout();
		TestEqual(TEXT("Payout for round 2"), Payout.Round, 2);
		TestEqual(TEXT("Same base bonus in round 2"), Payout.BaseBonus, Settings->BaseBonus);
		TestTrue(TEXT("Damage taken: no no-damage bonus"), !Payout.bNoDamage && Payout.NoDamageBonus == 0);
		TestTrue(TEXT("Over the time limit: no fast-clear bonus"), !Payout.bFastClear && Payout.FastClearBonus == 0);
		TestEqual(TEXT("Total = base only"), Payout.GetTotal(), Settings->BaseBonus);
		TestEqual(TEXT("Profile += base bonus + cash collected so far"), GetPlayerCash() - S->CashBefore, Payout.GetTotal() + GetRoundStats().CashCollected);

		const UGSRoundSummaryWidget* Summary = GetRoundSummary();
		TestEqual(TEXT("Summary title"), Summary->GetTitleText().ToString(), FString(TEXT("ROUND 2 CLEAR")));
		TestEqual(TEXT("Missed no-damage line"), Summary->GetNoDamageBonusText().ToString(), FString(TEXT("missed")));
		TestEqual(TEXT("Missed fast-clear line"), Summary->GetFastClearBonusText().ToString(), FString(TEXT("missed")));
		TestEqual(TEXT("Summary total"), Summary->GetTotalText().ToString(), FString::Printf(TEXT("+$%d"), Settings->BaseBonus));
		GameMode->SetRoundClearSettingsOverride(nullptr);

		// RoundClear without Combat (debug jumps) pays nothing and shows nothing.
		GameMode->SetRound(10);
		const int32 CashAtRound10 = GetPlayerCash();
		GameMode->SetPhase(EGSRoundPhase::RoundClear);
		TestEqual(TEXT("RoundClear from PreRound: no payout"), GetLastPayout().Round, 2);
		TestEqual(TEXT("RoundClear from PreRound: no cash"), GetPlayerCash(), CashAtRound10);
		TestFalse(TEXT("RoundClear from PreRound: no summary"), Summary->IsSummaryShowing());

		// No growth by round: round 10 pays what round 1 paid for the same result (no damage, fast).
		GameMode->SetRound(10);
		RemoveAllEnemies(GetPIEWorld());
		GameMode->AdvancePhase(); // -> Combat
		GameMode->AdvancePhase(); // skip -> RoundClear
		TestEqual(TEXT("Payout for round 10"), GetLastPayout().Round, 10);
		TestEqual(TEXT("Round 10 pays the same as round 1"), GetLastPayout().GetTotal(), S->FirstTotal);
		TestEqual(TEXT("Summary shows round 10"), Summary->GetShownRound(), 10);

		// A new run starts with no payout and no round stats.
		TestTrue(TEXT("gs.Run.Stats shows the last clear bonus"), GetGameState()->GetRunStats()->ToDebugString().Contains(TEXT("Last clear bonus: round 10")));
		GameMode->RestartRunWithSeed(2302);
		TestEqual(TEXT("New run: payout cleared"), GetLastPayout().Round, 0);
		TestTrue(TEXT("New run: round 1 stats from zero"), GetRoundStats().Round == 1 && GetRoundStats().Kills == 0 && GetRoundStats().CashCollected == 0);
		TestFalse(TEXT("New run: no summary"), Summary->IsSummaryShowing());

		GameMode->SetRoundTimerPaused(false);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
	});
	EndPIE();
	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
