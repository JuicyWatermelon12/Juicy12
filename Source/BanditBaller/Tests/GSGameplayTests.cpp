// BanditBaller
//
// Gameplay automation tests. Each test opens L_Town_Graybox in the editor if needed (the startup map is the hub,
// L_Hub_Graybox; Map.Hub opens that instead), starts PIE, waits for the local player, removes placed enemies so
// nothing interferes, runs its checks against the real GAS / weapon setup, and ends PIE.
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
#include "Misc/PackageName.h"
#include "Camera/PlayerCameraManager.h"
#include "GSStartRunPortal.h"
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
#include "GSRunSummaryWidget.h"
#include "Components/Button.h"
#include "GSHubGameMode.h"
#include "GSTargetDummy.h"
#include "AbilitySystemGlobals.h"
#include "GSHUDWidget.h"
#include "GSSpawnTelegraph.h"
#include "HAL/IConsoleManager.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
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
#include "GSGameplayAbility_Dash.h"
#include "GSGameplayAbility_Reload.h"
#include "GSGameplayAbility_Slide.h"
#include "GSAbilityIconWidget.h"
#include "GSCombatSettings.h"
#include "GSCriticalHit.h"
#include "GSGameplayAbility_Fire.h"
#include "GSWeapon.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/WidgetTree.h"
#include "GSMovementAbilitySettings.h"
#include "GSMovementSettings.h"
#include "Editor/EditorPerformanceSettings.h"
#include "GSFloatingGun.h"
#include "GSFloatingGunComponent.h"
#include "GSFloatingGunSettings.h"
#include "GSRazorCardsComponent.h"
#include "GSRazorCardsSettings.h"
#include "GSBarbedWireComponent.h"
#include "GSBarbedWireSettings.h"
#include "GSDynamiteComponent.h"
#include "GSDynamiteSettings.h"
#include "GSLobbedThrow.h"
#include "GSLuckyCoinComponent.h"
#include "GSLuckyCoinSettings.h"
#include "GSFirePatch.h"
#include "GSMolotovComponent.h"
#include "GSMolotovSettings.h"
#include "GSTomahawk.h"
#include "GSTomahawkComponent.h"
#include "GSTomahawkSettings.h"
#include "GSThrownSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GSFloatingShieldsComponent.h"
#include "GSProjectileSubsystem.h"
#include "GSSecondWindComponent.h"
#include "GSShieldComponent.h"
#include "GSUpgradeComponent.h"
#include "GSUpgradeData.h"
#include "GSUpgradePool.h"
#include "GSCollisionChannels.h"
#include "GSMapBoundary.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GSUpgradeCardWidget.h"
#include "GSUpgradeChoiceWidget.h"
#include "GSUpgradeIconWidget.h"
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

	/** The run map most tests play in, and the hub (the editor's startup map). */
	const FString TownMap = TEXT("/Game/BanditBaller/Maps/L_Town_Graybox");
	const FString HubMap = TEXT("/Game/BanditBaller/Maps/L_Hub_Graybox");

	/** Enqueue: open MapPath in the editor unless it already is the editor's map. */
	void OpenEditorMap(const FString& MapPath)
	{
		Step([MapPath]()
		{
			const UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			if (!EditorWorld || EditorWorld->GetOutermost()->GetName() != MapPath)
			{
				FAutomationEditorCommonUtils::LoadMap(MapPath);
			}
		});
	}

	/** Open MapPath (the town by default; the startup map is the hub), start PIE and wait for the player. */
	void BeginPIE(FAutomationTestBase* Test, const FString& MapPath = TownMap)
	{
		OpenEditorMap(MapPath);
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
		WaitUntil(Test, TEXT("PIE player ready (pawn, ASC, weapon, grounded)"), &IsPlayerReady, 30.0);
		Step([]() { if (UWorld* World = GetPIEWorld()) { RemoveAllEnemies(World); } });
	}

	void EndPIE()
	{
		ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	}

	/** Enqueue: wait Seconds of PIE world time (defined with the economy helpers below). */
	void WaitWorldSeconds(FAutomationTestBase* Test, double Seconds);

	/** Enqueue: wait Seconds of real time (defined with the HUD helpers below). */
	void WaitRealSeconds(FAutomationTestBase* Test, double Seconds);

	/** The PIE world's round GameMode (defined with the map helpers below). */
	AGSGameMode* GetGameMode();

	// --- Test speed (test-only; nothing here changes game rules or tuning assets) ---

	/** Editor throttling saved by SpeedUpWorld, put back by RestoreWorldSpeed. */
	bool GSavedThrottle = true;
	bool GThrottleSaved = false;

	/**
	 * Run the PIE world's game time Dilation x faster (world timers, phases, spawns, pickups, movement) and keep the
	 * frame rate up with the editor in the background (no CPU throttling), so each game-time step stays short. Rules
	 * are checked in game time, so they don't change. Real-time parts (widget animations, input locks) aren't sped up.
	 * Call RestoreWorldSpeed at the end of the test.
	 */
	void SpeedUpWorld(float Dilation)
	{
		UEditorPerformanceSettings* Perf = GetMutableDefault<UEditorPerformanceSettings>();
		if (!GThrottleSaved)
		{
			GSavedThrottle = Perf->bThrottleCPUWhenNotForeground;
			GThrottleSaved = true;
		}
		Perf->bThrottleCPUWhenNotForeground = false;
		if (UWorld* World = GetPIEWorld())
		{
			World->GetWorldSettings()->SetTimeDilation(Dilation);
		}
	}

	void RestoreWorldSpeed()
	{
		if (GThrottleSaved)
		{
			GetMutableDefault<UEditorPerformanceSettings>()->bThrottleCPUWhenNotForeground = GSavedThrottle;
			GThrottleSaved = false;
		}
		if (UWorld* World = GetPIEWorld())
		{
			World->GetWorldSettings()->SetTimeDilation(1.f);
		}
	}

	/** Set a float tuning property (EditDefaultsOnly, often protected) on one object, e.g. a widget's durations. */
	bool SetFloatProperty(UObject* Object, FName Name, float Value)
	{
		FFloatProperty* Property = Object ? FindFProperty<FFloatProperty>(Object->GetClass(), Name) : nullptr;
		if (Property)
		{
			Property->SetPropertyValue_InContainer(Object, Value);
		}
		return Property != nullptr;
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

// ---------------------------------------------------------------------------
// Held fire: a full-auto burst keeps an exact shot schedule inside GA_Fire
// ---------------------------------------------------------------------------

namespace GSTests
{
	int32 GetPlayerAmmo()
	{
		const AGSCharacter* Player = GetPlayer();
		const UGSWeaponComponent* Weapons = Player ? Player->GetWeaponComponent() : nullptr;
		return Weapons ? Weapons->GetCurrentAmmo() : -1;
	}

	/** The equipped weapon's magazine for the player (MagazineSize x MagazineSizeMultiplier, rounded up). */
	int32 GetPlayerMagazineSize()
	{
		const AGSCharacter* Player = GetPlayer();
		return Player ? Player->GetWeaponComponent()->GetCurrentMagazineSize() : -1;
	}

	bool IsPlayerFiring()
	{
		const UGSAbilitySystemComponent* ASC = GetPlayerASC();
		return ASC && ASC->HasMatchingGameplayTag(GSGameplayTags::State_Firing);
	}

	bool IsPlayerReloading()
	{
		const UGSAbilitySystemComponent* ASC = GetPlayerASC();
		return ASC && ASC->HasMatchingGameplayTag(GSGameplayTags::State_Reloading);
	}

	/** The equipped weapon's refire interval at the player's FireRate now (game time). */
	double GetPlayerRefireInterval()
	{
		const AGSCharacter* Player = GetPlayer();
		const UGSWeaponData* Data = Player ? Player->GetWeaponComponent()->GetCurrentWeaponData() : nullptr;
		const UGSAbilitySystemComponent* ASC = GetPlayerASC();
		return Data && ASC ? Data->GetRefireInterval(ASC->GetNumericAttribute(UGSAttributeSet::GetFireRateAttribute())) : 0.0;
	}

	void TapInput(const FGameplayTag& InputTag)
	{
		GetPlayerASC()->AbilityInputTagPressed(InputTag);
		GetPlayerASC()->AbilityInputTagReleased(InputTag);
	}

	void ReleaseFire()
	{
		GetPlayerASC()->AbilityInputTagReleased(GSGameplayTags::Ability_Fire);
	}

	/**
	 * A held burst, watched once a frame. The first shot fires on the press; GA_Fire then schedules each shot one refire
	 * interval after the previous shot's scheduled time. So by world time T the burst has fired exactly
	 * ScheduleShots + floor((T - ScheduleStart) / Interval) shots, whatever the frame rate.
	 */
	struct FHeldBurst
	{
		int32 StartAmmo = 0;
		int32 Shots = 0;
		double PressTime = 0.0;
		/** Scheduled time of shot number ScheduleShots (the press, or the last shot before an interval change). */
		double ScheduleStart = 0.0;
		int32 ScheduleShots = 1;
		double Interval = 0.0;
		/** World time of the frame the latest shot was seen in. */
		double LastSeen = 0.0;
		double LastPoll = 0.0;
		/** Longest frame (game time) seen while watching. */
		double MaxFrame = 0.0;
		int32 MaxShotsInOneFrame = 0;
		int32 OffSchedule = 0;
		FString FirstOffSchedule;

		/** Shots the schedule allows by Now (a shot due within 1 ms of Now may or may not have fired yet). */
		void GetExpectedShots(double Now, int32& OutLow, int32& OutHigh) const
		{
			OutLow = ScheduleShots + FMath::FloorToInt((Now - ScheduleStart - 1e-3) / Interval);
			OutHigh = ScheduleShots + FMath::FloorToInt((Now - ScheduleStart + 1e-3) / Interval);
		}

		/** When the next shot is due. */
		double GetNextDue() const { return ScheduleStart + (Shots - ScheduleShots + 1) * Interval; }

		/** Average interval from ScheduleStart's shot to the frame the latest shot was seen in. */
		double GetAverageInterval() const
		{
			const int32 Count = Shots - ScheduleShots;
			return Count > 0 ? (LastSeen - ScheduleStart) / Count : 0.0;
		}

		/** The latest shot is seen at most one frame late, spread over the measured intervals. */
		double GetAverageTolerance() const { return MaxFrame / FMath::Max(Shots - ScheduleShots, 1) + 1e-4; }

		/** FireRate changed after the latest shot: later shots follow NewInterval from that shot's scheduled time. */
		void ChangeInterval(double NewInterval)
		{
			ScheduleStart += (Shots - ScheduleShots) * Interval;
			ScheduleShots = Shots;
			Interval = NewInterval;
		}
	};

	/** Once a frame while the burst runs: count new shots and check the count against the schedule. */
	void PollBurst(FHeldBurst& Burst)
	{
		const double Now = GetPIEWorld()->GetTimeSeconds();
		Burst.MaxFrame = FMath::Max(Burst.MaxFrame, Now - Burst.LastPoll);
		Burst.LastPoll = Now;
		const int32 Shots = Burst.StartAmmo - GetPlayerAmmo();
		if (Shots > Burst.Shots)
		{
			Burst.MaxShotsInOneFrame = FMath::Max(Burst.MaxShotsInOneFrame, Shots - Burst.Shots);
			Burst.LastSeen = Now;
		}
		Burst.Shots = Shots;

		int32 Low = 0, High = 0;
		Burst.GetExpectedShots(Now, Low, High);
		if ((Shots < Low || Shots > High) && Burst.OffSchedule++ == 0)
		{
			Burst.FirstOffSchedule = FString::Printf(TEXT("%d shots %.4f s after the press, expected %d-%d"), Shots, Now - Burst.PressTime, Low, High);
		}
	}

	/** Press and hold fire (the first shot fires on the press) and start watching the burst. */
	void PressAndHoldFire(FHeldBurst& Burst)
	{
		const double Now = GetPIEWorld()->GetTimeSeconds();
		Burst = FHeldBurst();
		Burst.StartAmmo = GetPlayerAmmo();
		Burst.Interval = GetPlayerRefireInterval();
		Burst.PressTime = Burst.ScheduleStart = Burst.LastPoll = Now;
		GetPlayerASC()->AbilityInputTagPressed(GSGameplayTags::Ability_Fire);
		PollBurst(Burst);
	}

	void EnqueueHoldFire(TSharedRef<FHeldBurst> Burst)
	{
		Step([Burst]() { PressAndHoldFire(*Burst); });
	}

	/** Enqueue: keep fire held until the burst has fired Shots shots, checking the schedule every frame. */
	void EnqueueWaitShots(FAutomationTestBase* Test, TSharedRef<FHeldBurst> Burst, int32 Shots)
	{
		WaitUntil(Test, FString::Printf(TEXT("%d shots with fire held"), Shots), [Burst, Shots]()
		{
			PollBurst(*Burst);
			return Burst->Shots >= Shots;
		});
	}

	/** The burst kept to its schedule every frame, and its average interval is Expected (within a frame over the burst). */
	void CheckBurst(FAutomationTestBase* Test, const FHeldBurst& Burst, const FString& What, double Expected)
	{
		Test->TestEqual(*FString::Printf(TEXT("%s: the shot count matched the schedule in every frame%s%s"), *What,
			Burst.OffSchedule > 0 ? TEXT("; first miss: ") : TEXT(""), *Burst.FirstOffSchedule), Burst.OffSchedule, 0);
		const double Average = Burst.GetAverageInterval();
		Test->AddInfo(FString::Printf(TEXT("%s: %d shots, %.4f s apart on average (refire %.4f s; longest frame %.4f s, up to %d shots in one frame)"),
			*What, Burst.Shots, Average, Expected, Burst.MaxFrame, Burst.MaxShotsInOneFrame));
		Test->TestEqual(*FString::Printf(TEXT("%s: average interval over the burst"), *What), Average, Expected, Burst.GetAverageTolerance());
	}

	/** State a held-fire step waits on: when firing ended, and the last frame it was still on. */
	struct FFireEnd
	{
		double Due = 0.0;
		double LastFiring = 0.0;
		double Ended = -1.0;
		int32 Ammo = 0;
	};

	/** Enqueue: wait until State.Firing ends, remembering the frame it ended in and the last frame before it. */
	void EnqueueWaitFireEnd(FAutomationTestBase* Test, const FString& What, TSharedRef<FFireEnd> End, TFunction<void()> OnEnded = nullptr)
	{
		WaitUntil(Test, What, [End, OnEnded]()
		{
			const double Now = GetPIEWorld()->GetTimeSeconds();
			if (IsPlayerFiring())
			{
				End->LastFiring = Now;
				return false;
			}
			End->Ended = Now;
			if (OnEnded)
			{
				OnEnded();
			}
			return true;
		});
	}

	/** Firing ended in the first frame at or after End.Due (it was still on in the frame before). */
	void CheckEndedAtDue(FAutomationTestBase* Test, const FFireEnd& End, const FString& What)
	{
		Test->AddInfo(FString::Printf(TEXT("%s: due %.4f, still firing at %.4f, ended at %.4f"), *What, End.Due, End.LastFiring, End.Ended));
		Test->TestTrue(*FString::Printf(TEXT("%s: State.Firing lasted until the next shot's due time, not longer"), *What),
			End.LastFiring < End.Due + 1e-3 && End.Ended >= End.Due - 1e-3);
	}

	/** Enqueue: once firing has ended, reload the magazine to full. */
	void EnqueueReloadToFull(FAutomationTestBase* Test)
	{
		WaitUntil(Test, TEXT("firing to end before reloading"), []() { return !IsPlayerFiring(); });
		Step([]()
		{
			if (GetPlayerAmmo() < GetPlayerMagazineSize())
			{
				TapInput(GSGameplayTags::Ability_Reload);
			}
		});
		WaitUntil(Test, TEXT("a full magazine"), []() { return !IsPlayerReloading() && GetPlayerAmmo() == GetPlayerMagazineSize(); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSHeldFireTest, "BanditBaller.Weapons.HeldFire", GSTests::TestFlags)
bool FGSHeldFireTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		double Interval = 0.0;
		int32 AmmoBefore = 0;
		bool bReloadRefusedWhileFiring = false;
		bool bReloadRightAfter = false;
		double PausedTime = 0.0;
		int32 PausedAmmo = 0;
	};
	TSharedRef<FState> S = MakeShared<FState>();
	TSharedRef<FHeldBurst> Burst = MakeShared<FHeldBurst>();
	TSharedRef<FFireEnd> End = MakeShared<FFireEnd>();

	// The rifle at FireRate 1. Game time runs x4: the schedule is checked in game time, so that changes nothing.
	Step([this, S]()
	{
		SpeedUpWorld(4.f);
		GetGameMode()->SetRoundTimerPaused(true); // stay in PreRound: no enemies
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const UGSWeaponData* Rifle = GetPlayer()->GetWeaponComponent()->GetCurrentWeaponData();
		if (!TestNotNull(TEXT("A weapon is equipped"), Rifle))
		{
			return;
		}
		TestTrue(TEXT("The rifle is full auto"), Rifle->FireMode == EGSFireMode::FullAuto);
		TestEqual(TEXT("The rifle fires 600 rounds a minute"), Rifle->FireRateRPM, 600.f);
		S->Interval = GetPlayerRefireInterval();
		TestEqual(TEXT("Rifle refire interval at FireRate 1: 0.100 s"), S->Interval, 0.1, 1e-6);
		TestEqual(TEXT("Magazine full"), GetPlayerAmmo(), Rifle->MagazineSize);
	});

	// 1) Hold for 6 shots: exact schedule, reload refused while firing. Release: no further shot, State.Firing lasts
	//    until the 7th shot would have been due, and a reload works in that very frame.
	EnqueueHoldFire(Burst);
	Step([S]()
	{
		TapInput(GSGameplayTags::Ability_Reload);
		S->bReloadRefusedWhileFiring = IsPlayerFiring() && !IsPlayerReloading();
	});
	EnqueueWaitShots(this, Burst, 6);
	Step([Burst, End]()
	{
		PollBurst(*Burst);
		ReleaseFire();
		End->Due = Burst->GetNextDue();
		End->Ammo = GetPlayerAmmo();
	});
	EnqueueWaitFireEnd(this, TEXT("State.Firing to end after the release"), End, [S]()
	{
		TapInput(GSGameplayTags::Ability_Reload);
		S->bReloadRightAfter = IsPlayerReloading();
	});
	Step([this, S, Burst, End]()
	{
		TestTrue(TEXT("Reload is refused during a burst (State.Firing)"), S->bReloadRefusedWhileFiring);
		CheckBurst(this, *Burst, TEXT("Held rifle burst"), S->Interval);
		TestEqual(TEXT("Released after 6 shots"), Burst->Shots, 6);
		TestEqual(TEXT("Release: no shot after the release"), GetPlayerAmmo(), End->Ammo);
		CheckEndedAtDue(this, *End, TEXT("Release"));
		TestTrue(TEXT("Reload starts in the frame firing ends"), S->bReloadRightAfter);
	});
	WaitUntil(this, TEXT("the reload to refill the magazine"), []() { return !IsPlayerReloading() && GetPlayerAmmo() == GetPlayerMagazineSize(); });

	// 2) Hold through the whole magazine: exact schedule to the last round; the auto-reload starts when the next shot
	//    would have been due; with fire still held, firing resumes after the reload.
	EnqueueHoldFire(Burst);
	WaitUntil(this, TEXT("the magazine to run empty with fire held"), [Burst]()
	{
		PollBurst(*Burst);
		return GetPlayerAmmo() == 0;
	});
	Step([Burst, End]() { End->Due = Burst->GetNextDue(); End->Ammo = 0; });
	EnqueueWaitFireEnd(this, TEXT("the empty burst to end"), End);
	Step([this, Burst, End, S]()
	{
		CheckBurst(this, *Burst, TEXT("A whole magazine held"), S->Interval);
		TestEqual(TEXT("Empty: every round fired"), Burst->Shots, GetPlayerMagazineSize());
		CheckEndedAtDue(this, *End, TEXT("Empty magazine"));
		TestTrue(TEXT("Empty magazine: the auto-reload started"), IsPlayerReloading());
	});
	WaitUntil(this, TEXT("held fire to resume after the auto-reload"), []()
	{
		const int32 Ammo = GetPlayerAmmo();
		return !IsPlayerReloading() && Ammo > 0 && Ammo < GetPlayerMagazineSize();
	});
	Step([]() { ReleaseFire(); });
	EnqueueReloadToFull(this);

	// 3) A FireRate change mid-burst applies from the next shot: 4 shots at FireRate 1, Fire Rate x10 (1.8), 12 more.
	EnqueueHoldFire(Burst);
	EnqueueWaitShots(this, Burst, 4);
	Step([this, S, Burst]()
	{
		PollBurst(*Burst);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FireRate"), TEXT("10") });
		TestEqual(TEXT("FireRate 1.8 mid-burst"), GetPlayerASC()->GetNumericAttribute(UGSAttributeSet::GetFireRateAttribute()), 1.8f, 0.001f);
		Burst->ChangeInterval(GetPlayerRefireInterval());
	});
	EnqueueWaitShots(this, Burst, 16);
	Step([this, S, Burst]()
	{
		ReleaseFire();
		CheckBurst(this, *Burst, TEXT("After a mid-burst FireRate change to 1.8"), S->Interval / 1.8);
	});
	EnqueueReloadToFull(this);

	// 4) Slow frames: game time x16 makes every frame longer than the 0.0556 s interval. Each frame fires the shots it
	//    owes, so the count still matches the schedule and the average stays exact.
	Step([]() { SpeedUpWorld(16.f); });
	EnqueueHoldFire(Burst);
	EnqueueWaitShots(this, Burst, 20);
	Step([this, S, Burst]()
	{
		ReleaseFire();
		SpeedUpWorld(4.f);
		CheckBurst(this, *Burst, TEXT("Frames longer than the refire interval"), S->Interval / 1.8);
		TestTrue(FString::Printf(TEXT("Setup: frames (up to %.3f s) longer than the interval"), Burst->MaxFrame), Burst->MaxFrame > Burst->Interval);
		TestTrue(TEXT("A slow frame fired more than one shot (catch-up)"), Burst->MaxShotsInOneFrame >= 2);
	});

	// Back to FireRate 1 for the rest (a new run removes the upgrade).
	WaitUntil(this, TEXT("firing to end"), []() { return !IsPlayerFiring(); });
	Step([this]()
	{
		GetGameMode()->RestartRunWithSeed(77);
		GetGameMode()->SetRoundTimerPaused(true);
		TestEqual(TEXT("New run: FireRate 1"), GetPlayerASC()->GetNumericAttribute(UGSAttributeSet::GetFireRateAttribute()), 1.f, 0.001f);
	});
	EnqueueReloadToFull(this);

	// 5) The game pause pauses a burst: no shot and no game time while paused; the schedule carries on after.
	EnqueueHoldFire(Burst);
	EnqueueWaitShots(this, Burst, 3);
	Step([S, Burst]()
	{
		PollBurst(*Burst);
		UGameplayStatics::SetGamePaused(GetPIEWorld(), true);
		S->PausedTime = GetPIEWorld()->GetTimeSeconds();
		S->PausedAmmo = GetPlayerAmmo();
	});
	WaitRealSeconds(this, 0.3);
	Step([this, S, Burst]()
	{
		TestTrue(TEXT("Pause: the game is paused"), GetPIEWorld()->IsPaused());
		TestEqual(TEXT("Pause: no shot while paused"), GetPlayerAmmo(), S->PausedAmmo);
		TestEqual(TEXT("Pause: game time stood still"), GetPIEWorld()->GetTimeSeconds(), S->PausedTime, 1e-6);
		TestTrue(TEXT("Pause: the burst is still on"), IsPlayerFiring());
		UGameplayStatics::SetGamePaused(GetPIEWorld(), false);
		Burst->LastPoll = GetPIEWorld()->GetTimeSeconds();
	});
	EnqueueWaitShots(this, Burst, 8);
	Step([this, S, Burst]()
	{
		ReleaseFire();
		CheckBurst(this, *Burst, TEXT("A burst across a pause"), S->Interval);
	});
	EnqueueReloadToFull(this);

	// 6) A safe zone starting mid-burst stops it when the next shot is due; leaving it with fire still held resumes.
	EnqueueHoldFire(Burst);
	EnqueueWaitShots(this, Burst, 3);
	Step([S, Burst, End]()
	{
		PollBurst(*Burst);
		GetPlayerASC()->AddLooseGameplayTag(GSGameplayTags::State_SafeZone);
		End->Due = Burst->GetNextDue();
		End->Ammo = GetPlayerAmmo();
	});
	EnqueueWaitFireEnd(this, TEXT("the burst to stop in the safe zone"), End);
	WaitWorldSeconds(this, 0.5);
	Step([this, End]()
	{
		CheckEndedAtDue(this, *End, TEXT("Safe zone"));
		TestEqual(TEXT("Safe zone: no shot after it started"), GetPlayerAmmo(), End->Ammo);
		TestFalse(TEXT("Safe zone: held fire doesn't start a new burst"), IsPlayerFiring());
		TestFalse(TEXT("Safe zone: no reload"), IsPlayerReloading());
		GetPlayerASC()->RemoveLooseGameplayTag(GSGameplayTags::State_SafeZone);
	});
	WaitUntil(this, TEXT("held fire to resume after the safe zone"), [End]() { return GetPlayerAmmo() < End->Ammo; });
	Step([]() { ReleaseFire(); });

	// 7) Semi-auto (pistol): one shot per press however long it is held; a second press inside the refire interval
	//    does nothing; the next press after it fires.
	WaitUntil(this, TEXT("firing to end"), []() { return !IsPlayerFiring(); });
	Step([this, S]()
	{
		UGSDebugLibrary::GiveWeaponTo(GetPC(), UGSDebugLibrary::FindWeaponData(TEXT("Pistol")));
		const UGSWeaponData* Pistol = GetPlayer()->GetWeaponComponent()->GetCurrentWeaponData();
		TestTrue(TEXT("The pistol is semi-auto"), Pistol && Pistol->FireMode == EGSFireMode::SemiAuto);
		S->AmmoBefore = GetPlayerAmmo();
		GetPlayerASC()->AbilityInputTagPressed(GSGameplayTags::Ability_Fire);
	});
	WaitWorldSeconds(this, 0.7); // more than three pistol refire intervals, fire held
	Step([this, S]()
	{
		TestEqual(TEXT("Semi-auto held: one shot"), GetPlayerAmmo(), S->AmmoBefore - 1);
		TestFalse(TEXT("Semi-auto held: firing ended after the refire interval"), IsPlayerFiring());
		ReleaseFire();
		TapInput(GSGameplayTags::Ability_Fire);
		TapInput(GSGameplayTags::Ability_Fire);
		TestEqual(TEXT("Semi-auto double tap: one shot"), GetPlayerAmmo(), S->AmmoBefore - 2);
	});
	WaitUntil(this, TEXT("firing to end"), []() { return !IsPlayerFiring(); });
	Step([this, S]()
	{
		TapInput(GSGameplayTags::Ability_Fire);
		TestEqual(TEXT("Semi-auto: the next press after the interval fires"), GetPlayerAmmo(), S->AmmoBefore - 3);
	});
	WaitUntil(this, TEXT("firing to end"), []() { return !IsPlayerFiring(); });

	// 8) Death during a burst ends it at once: no shot, no reload.
	Step([this]()
	{
		UGSDebugLibrary::GiveWeaponTo(GetPC(), UGSDebugLibrary::FindWeaponData(TEXT("Rifle")));
		TestTrue(TEXT("The rifle again"), GetPlayer()->GetWeaponComponent()->GetCurrentWeaponData()->FireMode == EGSFireMode::FullAuto);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
	});
	EnqueueHoldFire(Burst);
	EnqueueWaitShots(this, Burst, 3);
	Step([this, S, Burst]()
	{
		PollBurst(*Burst);
		S->AmmoBefore = GetPlayerAmmo();
		ApplyDamage(GetPlayerASC(), 100000.f);
		TestTrue(TEXT("Death: the player is dead"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		TestFalse(TEXT("Death: the burst ended at once"), IsPlayerFiring());
	});
	WaitWorldSeconds(this, 0.5);
	Step([this, S]()
	{
		TestEqual(TEXT("Death: no shot after death"), GetPlayerAmmo(), S->AmmoBefore);
		TestFalse(TEXT("Death: no reload"), IsPlayerReloading());
		ReleaseFire();
		RestoreWorldSpeed();
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

	/** First actor with Tag in World (nullptr if none). */
	const AActor* FindTagged(const UWorld* World, FName Tag)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(Tag))
			{
				return *It;
			}
		}
		return nullptr;
	}

	AGSGameMode* GetGameMode() { return GetPIEWorld() ? GetPIEWorld()->GetAuthGameMode<AGSGameMode>() : nullptr; }
	AGSGameState* GetGameState() { return GetPIEWorld() ? GetPIEWorld()->GetGameState<AGSGameState>() : nullptr; }
	AGSHubGameMode* GetHubGameMode() { return GetPIEWorld() ? GetPIEWorld()->GetAuthGameMode<AGSHubGameMode>() : nullptr; }

	UGSHUDWidget* GetHUD()
	{
		return GetPC() ? GetPC()->GetGSHUD() : nullptr;
	}

	/** Enqueue: wait until PIE has moved to a new world of MapPath (map travel) with a ready player. */
	void WaitForMapTravel(FAutomationTestBase* Test, TSharedRef<TWeakObjectPtr<UWorld>> FromWorld, const FString& MapPath)
	{
		WaitUntil(Test, FString::Printf(TEXT("PIE to open %s with a ready player"), *MapPath), [FromWorld, MapPath]()
		{
			const UWorld* World = GetPIEWorld();
			return World && World != FromWorld->Get() && World->GetOutermost()->GetName().EndsWith(FPackageName::GetShortName(MapPath)) && IsPlayerReady();
		}, 30.0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSMapTownTest, "BanditBaller.Map.Town", GSTests::TestFlags)
bool FGSMapTownTest::RunTest(const FString& Parameters)
{
	TSharedRef<TWeakObjectPtr<UWorld>> PreviousWorld = MakeShared<TWeakObjectPtr<UWorld>>();

	BeginPIE(this);

	// The town: the run's map. Spawns and NavMesh, no hub content, no sublevels, no market travel points.
	Step([this, PreviousWorld]()
	{
		UWorld* World = GetPIEWorld();
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		if (!TestNotNull(TEXT("AGSGameMode (BP_GSGameMode, the global default)"), GameMode) || !TestNotNull(TEXT("AGSGameState"), GameState))
		{
			return;
		}
		TestTrue(TEXT("PIE runs the town map"), World->GetOutermost()->GetName().EndsWith(TEXT("L_Town_Graybox")));
		TestNull(TEXT("No hub GameMode in the town"), GetHubGameMode());
		TestTrue(TEXT("Area is the town"), GameState->GetCurrentArea() == EGSArea::Town);
		TestTrue(TEXT("The run started (round 1 or later)"), GameState->GetRoundNumber() >= 1);
		TestEqual(TEXT("No sublevels (the Market District is the hub map now)"), World->GetStreamingLevels().Num(), 0);
		for (const TCHAR* Tag : { TEXT("TownReturn"), TEXT("MarketPortalSpot"), TEXT("MarketArrival"), TEXT("MarketExitPortalSpot"),
			TEXT("Casino"), TEXT("MarketStall"), TEXT("Market") })
		{
			TestEqual(FString::Printf(TEXT("No '%s' actor in the town"), Tag), CountTagged(World, Tag), 0);
		}
		TestTrue(TEXT("Town has EnemySpawn points"), CountTagged(World, TEXT("EnemySpawn")) > 0);
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(TEXT("EnemySpawn")))
			{
				TestTrue(FString::Printf(TEXT("%s is on the town navmesh"), *It->GetActorNameOrLabel()), IsOnNavMesh(World, It->GetActorLocation()));
			}
		}
		TestTrue(TEXT("NavMesh under the player"), GetPlayer() && IsOnNavMesh(World, GetPlayer()->GetActorLocation()));
		TestNotNull(TEXT("Town has its AGSMapBoundary"), AGSMapBoundary::Find(World));
		TestFalse(TEXT("No safe zone in the town"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_SafeZone));
		TestFalse(TEXT("HUD: not a safe area"), GetHUD() && GetHUD()->IsInSafeArea());

		// gs.Market.Teleport (repointed): leave the run for the hub map.
		*PreviousWorld = World;
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("MarketTeleport"), {});
	});
	WaitForMapTravel(this, PreviousWorld, HubMap);
	Step([this, PreviousWorld]()
	{
		TestNotNull(TEXT("gs.Market.Teleport opened the hub (AGSHubGameMode)"), GetHubGameMode());
		TestTrue(TEXT("Hub area"), GetGameState() && GetGameState()->GetCurrentArea() == EGSArea::MarketDistrict);

		// gs.Market.Return (repointed): start a run from the hub, as the portal does.
		*PreviousWorld = GetPIEWorld();
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("MarketReturn"), {});
		TestTrue(TEXT("gs.Market.Return starts a run"), GetHubGameMode() && GetHubGameMode()->IsStartingRun());
	});
	WaitForMapTravel(this, PreviousWorld, TownMap);
	Step([this]()
	{
		TestNotNull(TEXT("gs.Market.Return opened the town (AGSGameMode)"), GetGameMode());
		TestTrue(TEXT("A fresh run: round 1"), GetGameState() && GetGameState()->GetRoundNumber() == 1);
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
		// The phase timers run in game time: let it run 4x faster.
		SpeedUpWorld(4.f);
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

		// The loop stops on death: GameOver, with no timer to leave it.
		ApplyDamage(GetPlayerASC(), 100000.f);
		TestTrue(TEXT("Death -> GameOver"), IsPhase(EGSRoundPhase::GameOver));
		TestTrue(TEXT("GameOver has no end time"), GameState->GetPhaseEndTime() <= 0.0);
		GameMode->AdvancePhase();
		TestTrue(TEXT("A skip doesn't leave GameOver"), IsPhase(EGSRoundPhase::GameOver));
	});
	// Longer than the PreRound it interrupted: still GameOver in round 5.
	WaitWorldSeconds(this, GetDefault<UGSRoundSettings>()->PreRoundDuration + 1.0);
	Step([this]()
	{
		TestTrue(TEXT("Still GameOver after the PreRound time"), IsPhase(EGSRoundPhase::GameOver));
		TestEqual(TEXT("No new round after death"), GetGameState()->GetRoundNumber(), 5);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Death -> run summary -> R -> the hub gives back control
// ---------------------------------------------------------------------------

namespace GSTests
{
	bool IsViewportIgnoringInput()
	{
		const UWorld* World = GetPIEWorld();
		UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
		return Viewport && Viewport->IgnoreInput();
	}

	UGSRunSummaryWidget* GetRunSummary()
	{
		return GetPC() ? GetPC()->GetRunSummaryWidget() : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSReturnToHubRestoresInputTest, "BanditBaller.Player.ReturnToHubRestoresInput", GSTests::TestFlags)
bool FGSReturnToHubRestoresInputTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	TSharedRef<TWeakObjectPtr<UWorld>> OldWorld = MakeShared<TWeakObjectPtr<UWorld>>();
	Step([this, OldWorld]()
	{
		TestFalse(TEXT("Game input active at start"), IsViewportIgnoringInput());
		TestFalse(TEXT("No cursor at start"), GetPC()->ShouldShowMouseCursor());

		// Die for real: the server shows the run summary (UI-only input with the cursor).
		*OldWorld = GetPIEWorld();
		ApplyDamage(GetPlayerASC(), 100000.f);
	});
	WaitUntil(this, TEXT("run summary to take input"), []() { return IsViewportIgnoringInput(); }, 5.0);
	Step([this]()
	{
		TestNotNull(TEXT("Run summary shown"), GetRunSummary());
		TestTrue(TEXT("Cursor shown for the button"), GetPC()->ShouldShowMouseCursor());
	});
	// R on the summary once its input lock is over (the key path the widget's NativeOnKeyDown takes).
	WaitUntil(this, TEXT("run summary input lock to end"), []() { return GetRunSummary() && !GetRunSummary()->IsInputLocked(); }, 5.0);
	Step([this]()
	{
		UGSRunSummaryWidget* Summary = GetRunSummary();
		TestTrue(TEXT("R is a return key"), Summary && Summary->HandleReturnKey(EKeys::R));
		TestTrue(TEXT("R asks to return to the hub"), Summary && Summary->IsReturnRequested());
		TestTrue(TEXT("The GameMode is returning to the hub"), GetGameMode() && GetGameMode()->IsReturningToHub());
	});
	WaitForMapTravel(this, OldWorld, HubMap);
	Step([this]()
	{
		TestNotNull(TEXT("Return to Market goes back to the hub (the market)"), GetHubGameMode());
		TestNull(TEXT("No run running in the hub"), GetGameMode());
		TestFalse(TEXT("Viewport accepts game input in the hub"), IsViewportIgnoringInput());
		TestFalse(TEXT("No cursor in the hub"), GetPC()->ShouldShowMouseCursor());
		TestNull(TEXT("No run summary in the hub"), GetRunSummary());
		const APawn* Player = GetPlayer();
		TestTrue(TEXT("The hub's pawn takes input"), Player && Player->InputEnabled());
		TestFalse(TEXT("The hub's player is not dead"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
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
	// The forced fallback below must be logged (at least once; the open-center search may log its own).
	AddExpectedMessage(TEXT("using EnemySpawn fallback"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0, /*IsRegex*/ false);
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

		const AGSMapBoundary* Boundary = AGSMapBoundary::Find(World);
		if (!TestNotNull(TEXT("Map boundary"), Boundary))
		{
			return;
		}
		FNavLocation PlayerNav;
		NavSys->ProjectPointToNavigation(Player->GetActorLocation(), PlayerNav, FVector(300.f, 300.f, 1000.f));
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FGSSpawnPoint& Point = Points[Index];
			const FString Name = FString::Printf(TEXT("Point %d"), Index);
			FNavLocation OnNav;
			TestTrue(Name + TEXT(" is on the NavMesh"), NavSys->ProjectPointToNavigation(Point.Location, OnNav, FVector(10.f, 10.f, 100.f)));
			TestTrue(Name + TEXT(" is inside the map boundary"), Boundary->IsInside(Point.Location, 60.f));
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
		// (No spawn points in the Market District: covered by Map.Hub, the hub map.)

		// No valid ring point (the player 50 m up: the ring has no NavMesh there) -> logged EnemySpawn fallbacks,
		// still on the NavMesh, spaced and inside the boundary.
		APawn* MutablePlayer = GetPlayer();
		const FVector Home = MutablePlayer->GetActorLocation();
		MutablePlayer->SetActorLocation(Home + FVector(0.f, 0.f, 5000.f), false, nullptr, ETeleportType::TeleportPhysics);
		FGSSpawnSearchStats FallbackStats;
		const TArray<FGSSpawnPoint> Fallbacks = UGSSpawnLocationFinder::FindSpawnLocations(PC, 2, &FallbackStats);
		MutablePlayer->SetActorLocation(Home, false, nullptr, ETeleportType::TeleportPhysics);
		TestEqual(TEXT("Fallback: 2 points"), Fallbacks.Num(), 2);
		TestEqual(TEXT("Fallback: counted as fallbacks, not ring points"), FallbackStats.Fallback, 2);
		TestEqual(TEXT("Fallback: no ring point"), FallbackStats.Ring, 0);
		for (int32 Index = 0; Index < Fallbacks.Num(); ++Index)
		{
			const FGSSpawnPoint& Point = Fallbacks[Index];
			const FString Name = FString::Printf(TEXT("Fallback %d"), Index);
			FNavLocation OnNav;
			TestTrue(Name + TEXT(" is flagged"), Point.bFallback);
			TestTrue(Name + TEXT(" is on the NavMesh"), NavSys->ProjectPointToNavigation(Point.Location, OnNav, FVector(10.f, 10.f, 100.f)));
			TestTrue(Name + TEXT(" is inside the map boundary"), Boundary->IsInside(Point.Location, 60.f));
		}
		TestTrue(TEXT("Fallbacks keep MinSpacing"), Fallbacks.Num() < 2 || FVector::Dist2D(Fallbacks[0].Location, Fallbacks[1].Location) >= Settings.MinSpacing - 1.f);
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

	// Killing the first arrivals while the rest of the wave is still to come doesn't end Combat.
	WaitUntil(this, TEXT("first wave enemies alive"), []() { return GetGameMode()->GetWaveDirector()->GetAlive() > 0; }, 10.0);
	Step([this, Expected]()
	{
		const int32 Killed = KillAllEnemiesAsPlayer();
		const UGSWaveDirector* Director = GetGameMode()->GetWaveDirector();
		TestTrue(TEXT("Some of the wave killed"), Killed > 0 && Killed < *Expected);
		TestTrue(TEXT("Enemies still to come"), Director->GetRemaining() > 0);
		TestTrue(TEXT("Combat goes on while wave enemies remain"), IsPhase(EGSRoundPhase::Combat));
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
		// The kills' XP orbs (and any health orbs) never expire, so they are still out; everything else is back in the pool.
		TestEqual(TEXT("Expired pickups are back in the pool (orbs stay)"),
			Cash->GetNumPooled() + Cash->GetNumActiveOfKind(EGSPickupKind::XP) + Cash->GetNumActiveOfKind(EGSPickupKind::Health), Cash->GetNumCreated());

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

		// Curve (DA_XPCurve): level 2 = 100 XP, then +30 more for each level after that. No cap.
		TestEqual(TEXT("DA_XPCurve BaseXP"), Curve->BaseXP, 100);
		TestEqual(TEXT("DA_XPCurve XPIncreasePerLevel"), Curve->XPIncreasePerLevel, 30);
		TestEqual(TEXT("XP to reach level 1"), Curve->GetXPToReachLevel(1), 0);
		TestEqual(TEXT("XP to reach level 2"), Curve->GetXPToReachLevel(2), 100);
		TestEqual(TEXT("XP to reach level 3 (+30)"), Curve->GetXPToReachLevel(3), 130);
		TestEqual(TEXT("XP to reach level 10"), Curve->GetXPToReachLevel(10), 340);
		TestEqual(TEXT("XP to reach level 51 (past the old cap)"), Curve->GetXPToReachLevel(51), 1570);
		TestEqual(TEXT("XP to reach level 1000"), Curve->GetXPToReachLevel(1000), 30040);
		bool bStepsEqual = true;
		int64 Sum = 0;
		for (int32 Level = 2; Level <= 300; ++Level)
		{
			bStepsEqual &= Level == 2 || Curve->GetXPToReachLevel(Level) - Curve->GetXPToReachLevel(Level - 1) == 30;
			Sum += Curve->GetXPToReachLevel(Level);
			bStepsEqual &= Curve->GetTotalXPToReachLevel(Level) == Sum;
		}
		TestTrue(TEXT("Each level needs exactly 30 more, and the totals match the per-level sum (to level 300)"), bStepsEqual);
		FString Table;
		for (const int32 Level : { 2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 50, 100 })
		{
			Table += FString::Printf(TEXT(" L%d=%d (total %lld)"), Level, Curve->GetXPToReachLevel(Level), Curve->GetTotalXPToReachLevel(Level));
		}
		AddInfo(FString::Printf(TEXT("[xp] XP needed:%s"), *Table));

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
		TestEqual(TEXT("Leftover carries over (260 - 100 - 130)"), Progression->GetCurrentXP(), 30);
		TestEqual(TEXT("XP for level 4"), Progression->GetXPForNextLevel(), Curve->GetXPToReachLevel(4));
		if (const UGSHUDWidget* HUD = PC->GetGSHUD())
		{
			TestEqual(TEXT("HUD level follows OnLevelUp"), HUD->GetDisplayedLevel(), 3);
			TestEqual(TEXT("HUD XP bar fill"), HUD->GetDisplayedXPPercent(), 30.f / Curve->GetXPToReachLevel(4), 0.001f);
			TestEqual(TEXT("HUD level label"), HUD->GetLevelLabelText(), FString(TEXT("LV 3")));
			TestEqual(TEXT("HUD XP text"), HUD->GetXPLabelText(), FString(TEXT("30 / 160 XP")));
		}
		else
		{
			AddError(TEXT("No HUD widget on the player controller"));
		}

		// The two level-ups queued one offer each (one open, one waiting; game paused); take them so orbs can fly.
		const UGSUpgradeComponent* Upgrades = PS->GetUpgradeComponent();
		TestTrue(TEXT("Two level-ups: an offer open"), Upgrades->HasOpenOffer());
		TestEqual(TEXT("Two level-ups: one more offer queued"), Upgrades->GetQueuedOffers(), 1);
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

		// A big gain past the old cap of 50: from level 10 to level 60 and 7 XP into it, one offer per level gained.
		PickAllUpgradeOffers(); // SetLevel 10 queued 9 offers
		const UGSUpgradeComponent* Upgrades = PS->GetUpgradeComponent();
		TestFalse(TEXT("No offer before the big gain"), Upgrades->HasOpenOffer());
		const int64 Gain = Curve->GetTotalXPToReachLevel(60) - Curve->GetTotalXPToReachLevel(10) + 7;
		GameMode->AwardXP(PS, static_cast<float>(Gain), false);
		TestTrue(TEXT("Big gain: an offer open"), Upgrades->HasOpenOffer());
		TestEqual(TEXT("Big gain: one offer per level gained (50)"), Upgrades->GetQueuedOffers() + 1, 50);
		TestEqual(TEXT("No cap: level 60"), Progression->GetLevel(), 60);
		TestEqual(TEXT("Leftover carries over past 50"), Progression->GetCurrentXP(), 7);
		TestEqual(TEXT("A next level is always there"), Progression->GetXPForNextLevel(), Curve->GetXPToReachLevel(61));
		PickAllUpgradeOffers();

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
	// (The in-town Market District visit is gone: the hub is its own map with no XP.)

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

	// Going back to the hub after death keeps the balance (the profile lives on the GameInstance).
	Step([]() { GetPC()->ServerRequestReturnToHub(); });
	WaitForMapTravel(this, OldWorld, HubMap);
	Step([this, Bonus]()
	{
		UGSProfileSubsystem* Profile = GetProfile();
		if (!TestNotNull(TEXT("Profile in the hub"), Profile))
		{
			return;
		}
		TestNotNull(TEXT("Death + R leads to the hub"), GetHubGameMode());
		TestEqual(TEXT("Balance survives death + return to the hub"), Profile->GetCash(), 20 + *Bonus);
		if (const UGSHUDWidget* HUD = GetPC() ? GetPC()->GetGSHUD() : nullptr)
		{
			TestEqual(TEXT("New HUD shows the balance"), HUD->GetDisplayedCash(), 20 + *Bonus);
		}

		// A known balance for the next session: quitting (end of PIE) saves it.
		Profile->ResetProfile();
		Profile->AddCash(321, TEXT("Test"));
	});
	EndPIE();
	// Ending PIE is only requested above; wait until it is over before the next session.
	WaitUntil(this, TEXT("PIE to end"), []() { return GetPIEWorld() == nullptr; }, 15.0);

	// A new session (new GameInstance) loads the balance from the save.
	BeginPIE(this);
	Step([this]()
	{
		UGSProfileSubsystem* Profile = GetProfile();
		if (!TestNotNull(TEXT("Profile in the next session"), Profile))
		{
			return;
		}
		TestEqual(TEXT("Quit saved the balance"), GetSavedCash(*Profile), 321);
		TestEqual(TEXT("Next session loads the balance"), Profile->GetCash(), 321);
		if (const UGSHUDWidget* HUD = GetPC() ? GetPC()->GetGSHUD() : nullptr)
		{
			TestEqual(TEXT("Loaded balance on the HUD"), HUD->GetDisplayedCash(), 321);
		}

		// gs.Profile.Reset (the console command) clears it and deletes the save.
		GEngine->Exec(GetPIEWorld(), TEXT("gs.Profile.Reset"));
		TestEqual(TEXT("gs.Profile.Reset: cash 0"), Profile->GetCash(), 0);
		TestEqual(TEXT("gs.Profile.Reset: save deleted"), GetSavedCash(*Profile), -1);
		if (const UGSHUDWidget* HUD = GetPC() ? GetPC()->GetGSHUD() : nullptr)
		{
			TestEqual(TEXT("gs.Profile.Reset: HUD shows 0"), HUD->GetDisplayedCash(), 0);
		}
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

	/** Active effects of Upgrade's GE on the player (one per picked upgrade, whatever the number of picks). */
	int32 GetActiveUpgradeEffectCount(const UGSUpgradeData* Upgrade)
	{
		const UGSAbilitySystemComponent* ASC = GetPlayerASC();
		return ASC && Upgrade && Upgrade->Effect ? ASC->GetGameplayEffectCount(Upgrade->Effect, nullptr) : 0;
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
			TestTrue(FString::Printf(TEXT("%s is a Stat Boost"), E.Name), Upgrade->Category == EGSUpgradeCategory::StatBoost);
			TestFalse(FString::Printf(TEXT("%s has a display name"), E.Name), Upgrade->DisplayName.IsEmpty());
			TestTrue(FString::Printf(TEXT("%s effect is an upgrade effect"), E.Name), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect::StaticClass()));
			const FString Description = Upgrade->FormatValueText(Upgrade->BaseValue).ToString(); // a Common pick's card line
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
		TestEqual(TEXT("ReloadSpeed: one active GE for all 5 picks"), GetActiveUpgradeEffectCount(Reload), 1);
		TestEqual(TEXT("ReloadSpeed total value 5 x 0.1"), GetPlayerUpgrades()->GetTotalValue(Reload), 0.5f, 0.0001f);
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
		TestEqual(TEXT("Damage: one active GE for all 3 picks"), GetActiveUpgradeEffectCount(Damage), 1);

		// Move Speed x2 -> base x 1.16, and the movement component follows.
		const UGSUpgradeData* Move = UGSDebugLibrary::FindUpgradeData(TEXT("MoveSpeed"));
		Upgrades->GrantUpgrade(Move, TEXT("Test"));
		Upgrades->GrantUpgrade(Move, TEXT("Test"));
		const float ExpectedSpeed = S->BaseMoveSpeed * 1.16f;
		TestEqual(TEXT("MoveSpeed = base x (1 + 2 x 8%)"), GetAttr(UGSAttributeSet::GetMoveSpeedAttribute()), ExpectedSpeed, 0.5f);
		TestEqual(TEXT("MaxWalkSpeed follows MoveSpeed"), GetPlayer()->GetCharacterMovement()->MaxWalkSpeed, ExpectedSpeed, 0.5f);

		TestEqual(TEXT("Total picks"), Upgrades->GetTotalPicks(), 10);
		TestEqual(TEXT("Three upgrades tracked"), Upgrades->GetUpgradeStacks().Num(), 3);

		// No pick cap: 50 more Damage picks are all granted and keep adding.
		for (int32 i = 0; i < 50; ++i)
		{
			Upgrades->GrantUpgrade(Damage, TEXT("Test"));
		}
		TestEqual(TEXT("No cap: 53 Damage picks"), Upgrades->GetStacks(Damage), 53);
		TestEqual(TEXT("Damage = 1 + 53 x 10%"), GetAttr(UGSAttributeSet::GetDamageAttribute()), 6.3f, 0.001f);

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
// Fire Rate and Magnet: Common upgrades on existing attributes (exact held-fire interval, real pickup reach)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSFireRateAndMagnetTest, "BanditBaller.Upgrades.FireRateAndMagnet", GSTests::TestFlags)
bool FGSFireRateAndMagnetTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		double BaseInterval = 0.0;
		float BaseRadius = 0.f;
		int32 CashBefore = 0;
		int32 XPBefore = 0;
		int32 CashValue = 0;
		int32 XPValue = 0;
		FVector DropOrigin = FVector::ZeroVector;
	};
	TSharedRef<FState> S = MakeShared<FState>();
	TSharedRef<FHeldBurst> BaseBurst = MakeShared<FHeldBurst>();
	TSharedRef<FHeldBurst> FastBurst = MakeShared<FHeldBurst>();

	// Data, and the base values. Shots run at normal game speed without editor throttling, so frames stay short and the
	// measured average is within a millisecond (the shot count is checked against the exact schedule every frame).
	Step([this, S]()
	{
		SpeedUpWorld(1.f);
		GetGameMode()->SetRoundTimerPaused(true); // stay in PreRound: no enemies
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const UGSUpgradePool* Pool = GetGameMode()->GetUpgradePool();
		struct FExpect { const TCHAR* Name; const TCHAR* Text; UClass* EffectClass; };
		for (const FExpect& E : { FExpect{ TEXT("FireRate"), TEXT("+8%"), UGSUpgradeEffect_FireRate::StaticClass() },
			FExpect{ TEXT("Magnet"), TEXT("+15%"), UGSUpgradeEffect_Magnet::StaticClass() } })
		{
			const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(E.Name);
			if (!TestNotNull(FString::Printf(TEXT("DA_Upgrade_%s exists"), E.Name), Upgrade))
			{
				continue;
			}
			TestTrue(FString::Printf(TEXT("%s is a Stat Boost"), E.Name), Upgrade->Category == EGSUpgradeCategory::StatBoost);
			TestFalse(FString::Printf(TEXT("%s has a display name"), E.Name), Upgrade->DisplayName.IsEmpty());
			TestTrue(FString::Printf(TEXT("%s effect is GE_Upgrade_%s"), E.Name, E.Name), Upgrade->Effect && Upgrade->Effect->IsChildOf(E.EffectClass));
			const FString Description = Upgrade->FormatValueText(Upgrade->BaseValue).ToString(); // a Common pick's card line
			TestTrue(FString::Printf(TEXT("%s description '%s' shows %s"), E.Name, *Description, E.Text), Description.Contains(E.Text));
			TestTrue(FString::Printf(TEXT("%s is in DA_UpgradePool"), E.Name), Pool && Pool->Upgrades.Contains(Upgrade));
		}

		TestEqual(TEXT("FireRate starts at 1"), GetAttr(UGSAttributeSet::GetFireRateAttribute()), 1.f, 0.001f);
		S->BaseRadius = GetAttr(UGSAttributeSet::GetPickupRadiusAttribute());
		TestEqual(TEXT("PickupRadius starts at 300 (GE_InitPlayerStats)"), S->BaseRadius, 300.f, 0.5f);

		const UGSWeaponData* Rifle = GetPlayer()->GetWeaponComponent()->GetCurrentWeaponData();
		if (TestNotNull(TEXT("A weapon is equipped"), Rifle))
		{
			TestTrue(TEXT("The equipped weapon is full auto (rifle)"), Rifle->FireMode == EGSFireMode::FullAuto);
			TestTrue(TEXT("Magazine holds both bursts (12 + 16 shots)"), GetPlayer()->GetWeaponComponent()->GetCurrentAmmo() >= 28);
			S->BaseInterval = Rifle->GetRefireInterval(1.f);
			TestEqual(TEXT("Rifle refire interval at FireRate 1: 0.100 s (600 rounds a minute)"), S->BaseInterval, 0.1, 1e-6);
		}
	});

	// Fire Rate: hold fire for 12 shots, then again with 10 stacks (x1.8) for 16 shots.
	EnqueueHoldFire(BaseBurst);
	EnqueueWaitShots(this, BaseBurst, 12);
	Step([]() { ReleaseFire(); });
	WaitUntil(this, TEXT("State.Firing to clear"), []() { return !IsPlayerFiring(); });
	Step([this]()
	{
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FireRate"), TEXT("10") });
		TestEqual(TEXT("Fire Rate: 10 stacks tracked"), GetPlayerUpgrades()->GetStacks(UGSDebugLibrary::FindUpgradeData(TEXT("FireRate"))), 10);
		TestEqual(TEXT("FireRate = 1 + 10 x 8%"), GetAttr(UGSAttributeSet::GetFireRateAttribute()), 1.8f, 0.001f);
	});
	EnqueueHoldFire(FastBurst);
	EnqueueWaitShots(this, FastBurst, 16);
	Step([]() { ReleaseFire(); });
	WaitUntil(this, TEXT("State.Firing to clear"), []() { return !IsPlayerFiring(); });
	Step([this, S, BaseBurst, FastBurst]()
	{
		const double Expected = GetPlayerRefireInterval();
		TestEqual(TEXT("Refire interval = base / 1.8 (0.0556 s)"), Expected, S->BaseInterval / 1.8, 1e-6);

		// Exact: one shot per refire interval, no frame lost per shot.
		CheckBurst(this, *BaseBurst, TEXT("FireRate 1 (rifle)"), 0.1);
		CheckBurst(this, *FastBurst, TEXT("FireRate 1.8"), 0.1 / 1.8);
		const double Base = BaseBurst->GetAverageInterval();
		const double Fast = FMath::Max(FastBurst->GetAverageInterval(), 1e-6);
		const double RatioTolerance = 1.8 * (BaseBurst->GetAverageTolerance() / Base + FastBurst->GetAverageTolerance() / Fast);
		TestEqual(TEXT("Fire Rate x10 fires 1.8 times as fast"), Base / Fast, 1.8, RatioTolerance);
	});

	// Magnet: a drop beyond the base radius stays put; with 10 stacks (x2.5) its cash and its XP orb both fly in.
	Step([this, S]()
	{
		SpeedUpWorld(4.f); // the toss, the waits and the magnet flight run in game time
		GetPlayer()->TeleportTo(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);
		S->CashBefore = GetPlayerCash();
		S->XPBefore = GetPlayerTotalXP();
		const UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		S->XPValue = Data ? Data->XPValue : 0;
		KillEnemyForCash(this, FVector(1000.f, 0.f, 0.f), S->DropOrigin, S->CashValue);
	});
	WaitWorldSeconds(this, 1.0);
	Step([S]() { GetPlayer()->TeleportTo(S->DropOrigin - FVector(520.f, 0.f, 0.f), FRotator::ZeroRotator); });
	WaitWorldSeconds(this, 1.0);
	Step([this, S]()
	{
		UGSCashPickupSubsystem* Cash = GetCashSubsystem();
		TestTrue(TEXT("The drop has cash pieces"), Cash->GetNumActiveOfKind(EGSPickupKind::Cash) > 0);
		TestEqual(TEXT("The drop has one XP orb"), Cash->GetNumActiveOfKind(EGSPickupKind::XP), 1);
		float Nearest = TNumericLimits<float>::Max(), Farthest = 0.f;
		for (const AGSCashPickup* Pickup : Cash->GetActivePickups())
		{
			const float Distance = FVector::Dist(Pickup->GetActorLocation(), GetPlayer()->GetActorLocation());
			Nearest = FMath::Min(Nearest, Distance);
			Farthest = FMath::Max(Farthest, Distance);
			TestFalse(TEXT("Base radius: nothing flies to the player"), Pickup->IsMagnetized());
		}
		AddInfo(FString::Printf(TEXT("Pickups %.0f-%.0f cm from the player (base radius %.0f, with Magnet x10 %.0f)"), Nearest, Farthest, S->BaseRadius, S->BaseRadius * 2.5f));
		TestTrue(TEXT("Setup: every pickup lies beyond the base radius"), Nearest > S->BaseRadius + 20.f);
		TestTrue(TEXT("Setup: every pickup lies inside the Magnet x10 radius"), Farthest < S->BaseRadius * 2.5f - 20.f);
		TestEqual(TEXT("Base radius: no cash yet"), GetPlayerCash(), S->CashBefore);
		TestEqual(TEXT("Base radius: no XP yet"), GetPlayerTotalXP(), S->XPBefore);

		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Magnet"), TEXT("10") });
		TestEqual(TEXT("Magnet: 10 stacks tracked"), GetPlayerUpgrades()->GetStacks(UGSDebugLibrary::FindUpgradeData(TEXT("Magnet"))), 10);
		TestEqual(TEXT("PickupRadius = base x (1 + 10 x 15%)"), GetAttr(UGSAttributeSet::GetPickupRadiusAttribute()), S->BaseRadius * 2.5f, 0.5f);
	});
	WaitUntil(this, TEXT("the cash and the XP orb to fly in"), []()
	{
		const UGSCashPickupSubsystem* Cash = GetCashSubsystem();
		return Cash->GetNumActiveOfKind(EGSPickupKind::Cash) == 0 && Cash->GetNumActiveOfKind(EGSPickupKind::XP) == 0;
	}, 10.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Magnet: the drop's cash was collected"), GetPlayerCash(), S->CashBefore + S->CashValue);
		TestEqual(TEXT("Magnet: the XP orb was collected"), GetPlayerTotalXP(), S->XPBefore + S->XPValue);

		// A new run removes both upgrades.
		GetGameMode()->RestartRunWithSeed(31);
		TestEqual(TEXT("New run: FireRate back to 1"), GetAttr(UGSAttributeSet::GetFireRateAttribute()), 1.f, 0.001f);
		TestEqual(TEXT("New run: PickupRadius back to base"), GetAttr(UGSAttributeSet::GetPickupRadiusAttribute()), S->BaseRadius, 0.5f);
		TestEqual(TEXT("New run: no picks"), GetPlayerUpgrades()->GetTotalPicks(), 0);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Bigger Mags and Leech: magazine size x multiplier (rounded up, no refill, HUD), heal per credited kill
// ---------------------------------------------------------------------------

namespace GSTests
{
	FString GetHUDAmmoText()
	{
		const UGSHUDWidget* HUD = GetHUD();
		return HUD ? HUD->GetAmmoText() : FString(TEXT("<no HUD>"));
	}

	/** Enqueue: wait until the HUD's ammo line reads Expected (it's read every frame). */
	void WaitHUDAmmo(FAutomationTestBase* Test, const FString& Expected)
	{
		WaitUntil(Test, FString::Printf(TEXT("the HUD ammo to read \"%s\""), *Expected), [Expected]() { return GetHUDAmmoText() == Expected; }, 5.0);
	}

	/** Kill a fresh MeleeBandit with damage from Source (the player's ASC credits the kill to the player). */
	void KillEnemyWithSource(FAutomationTestBase* Test, UAbilitySystemComponent* Source)
	{
		AGSEnemyCharacter* Enemy = SpawnTestEnemy();
		UAbilitySystemComponent* EnemyASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
		if (Test->TestNotNull(TEXT("Spawned an enemy to kill"), EnemyASC))
		{
			ApplyDamage(EnemyASC, GetMaxHealth(EnemyASC) + 100.f, Source ? Source : EnemyASC);
			Test->TestTrue(TEXT("The enemy died"), Enemy->GetHealthComponent()->IsDead());
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSBiggerMagsAndLeechTest, "BanditBaller.Upgrades.BiggerMagsAndLeech", GSTests::TestFlags)
bool FGSBiggerMagsAndLeechTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	// Data and base values.
	Step([this]()
	{
		SpeedUpWorld(4.f);
		GetGameMode()->SetRoundTimerPaused(true); // stay in PreRound: no wave
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const UGSUpgradePool* Pool = GetGameMode()->GetUpgradePool();
		struct FExpect { const TCHAR* Name; EGSUpgradeCategory Category; const TCHAR* Text; UClass* EffectClass; };
		for (const FExpect& E : { FExpect{ TEXT("BiggerMags"), EGSUpgradeCategory::StatBoost, TEXT("+15%"), UGSUpgradeEffect_BiggerMags::StaticClass() },
			FExpect{ TEXT("Leech"), EGSUpgradeCategory::Ability, TEXT("+1"), UGSUpgradeEffect_Leech::StaticClass() } })
		{
			const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(E.Name);
			if (!TestNotNull(FString::Printf(TEXT("DA_Upgrade_%s exists"), E.Name), Upgrade))
			{
				continue;
			}
			TestTrue(FString::Printf(TEXT("%s category"), E.Name), Upgrade->Category == E.Category);
			TestFalse(FString::Printf(TEXT("%s has a display name"), E.Name), Upgrade->DisplayName.IsEmpty());
			TestTrue(FString::Printf(TEXT("%s effect is GE_Upgrade_%s"), E.Name, E.Name), Upgrade->Effect && Upgrade->Effect->IsChildOf(E.EffectClass));
			const FString Description = Upgrade->FormatValueText(Upgrade->BaseValue).ToString(); // a Common pick's card line
			TestTrue(FString::Printf(TEXT("%s description '%s' shows %s"), E.Name, *Description, E.Text), Description.Contains(E.Text));
			TestTrue(FString::Printf(TEXT("%s is in DA_UpgradePool"), E.Name), Pool && Pool->Upgrades.Contains(Upgrade));
		}

		TestEqual(TEXT("MagazineSizeMultiplier starts at 1"), GetAttr(UGSAttributeSet::GetMagazineSizeMultiplierAttribute()), 1.f, 0.001f);
		TestEqual(TEXT("HealthOnKill starts at 0"), GetAttr(UGSAttributeSet::GetHealthOnKillAttribute()), 0.f, 0.001f);
		const UGSWeaponData* Rifle = GetPlayer()->GetWeaponComponent()->GetCurrentWeaponData();
		TestTrue(TEXT("The rifle is equipped"), Rifle && Rifle->MagazineSize == 30);
		TestEqual(TEXT("Rifle magazine 30"), GetPlayerMagazineSize(), 30);
		TestEqual(TEXT("Pistol data magazine 12"), UGSDebugLibrary::FindWeaponData(TEXT("Pistol"))->MagazineSize, 12);
	});
	WaitHUDAmmo(this, TEXT("30 / 30"));

	// Bigger Mags: spend 2 rounds, then one stack: magazine ceil(30 x 1.15) = 35, the 28 rounds stay (no refill), the HUD
	// follows at once. A reload fills to 35.
	// Two taps, one round each (a held burst could catch up several shots in one long frame).
	for (int32 Tap = 0; Tap < 2; ++Tap)
	{
		Step([]() { TapInput(GSGameplayTags::Ability_Fire); });
		WaitUntil(this, TEXT("firing to end"), []() { return !IsPlayerFiring(); });
	}
	Step([this]()
	{
		TestEqual(TEXT("2 rounds spent"), GetPlayerAmmo(), 28);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("BiggerMags") });
		TestEqual(TEXT("MagazineSizeMultiplier = 1.15"), GetAttr(UGSAttributeSet::GetMagazineSizeMultiplierAttribute()), 1.15f, 0.001f);
		TestEqual(TEXT("Rifle magazine ceil(30 x 1.15) = 35"), GetPlayerMagazineSize(), 35);
		TestEqual(TEXT("The pick doesn't refill"), GetPlayerAmmo(), 28);
		TestFalse(TEXT("The magazine isn't full any more"), GetPlayer()->GetWeaponComponent()->IsCurrentMagazineFull());
	});
	WaitHUDAmmo(this, TEXT("28 / 35"));
	Step([]() { TapInput(GSGameplayTags::Ability_Reload); });
	WaitUntil(this, TEXT("the reload to fill 35 rounds"), []() { return !IsPlayerReloading() && GetPlayerAmmo() == 35; });
	WaitHUDAmmo(this, TEXT("35 / 35"));

	// Four stacks: x1.6. Rifle 30 x 1.6 = 48 exactly (no extra round from float noise), pistol ceil(12 x 1.6) = 20.
	Step([this]()
	{
		TestTrue(TEXT("Full at 35: a reload is refused"), GetPlayer()->GetWeaponComponent()->IsCurrentMagazineFull());
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("BiggerMags"), TEXT("3") });
		TestEqual(TEXT("4 stacks"), GetPlayerUpgrades()->GetStacks(UGSDebugLibrary::FindUpgradeData(TEXT("BiggerMags"))), 4);
		TestEqual(TEXT("MagazineSizeMultiplier = 1 + 4 x 15%"), GetAttr(UGSAttributeSet::GetMagazineSizeMultiplierAttribute()), 1.6f, 0.001f);
		TestEqual(TEXT("Rifle magazine 30 x 1.6 = 48"), GetPlayerMagazineSize(), 48);
		TestEqual(TEXT("Still no refill"), GetPlayerAmmo(), 35);
		TestEqual(TEXT("Pistol magazine ceil(12 x 1.6) = 20"),
			GetPlayer()->GetWeaponComponent()->GetMagazineSize(UGSDebugLibrary::FindWeaponData(TEXT("Pistol"))), 20);
	});
	WaitHUDAmmo(this, TEXT("35 / 48"));
	Step([this]()
	{
		UGSDebugLibrary::GiveWeaponTo(GetPC(), UGSDebugLibrary::FindWeaponData(TEXT("Pistol")));
		TestEqual(TEXT("Pistol equipped with its bigger magazine"), GetPlayerAmmo(), 20);
		TestEqual(TEXT("Pistol magazine 20"), GetPlayerMagazineSize(), 20);
	});
	WaitHUDAmmo(this, TEXT("20 / 20"));
	Step([this]()
	{
		UGSDebugLibrary::GiveWeaponTo(GetPC(), UGSDebugLibrary::FindWeaponData(TEXT("Rifle")));
		TestEqual(TEXT("Rifle again: magazine 48"), GetPlayerMagazineSize(), 48);
	});

	// Leech: no heal without it; +1 per stack per kill credited to the player; never above MaxHealth; kills that aren't
	// the player's don't heal.
	Step([this]()
	{
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		UGSAbilitySystemComponent* ASC = GetPlayerASC();
		ApplyDamage(ASC, 30.f);
		TestEqual(TEXT("Damaged to 70"), GetHealth(ASC), 70.f, 0.01f);

		KillEnemyWithSource(this, ASC);
		TestEqual(TEXT("No Leech: a kill heals nothing"), GetHealth(ASC), 70.f, 0.01f);

		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Leech") });
		TestEqual(TEXT("HealthOnKill = 1"), GetAttr(UGSAttributeSet::GetHealthOnKillAttribute()), 1.f, 0.001f);
		KillEnemyWithSource(this, ASC);
		TestEqual(TEXT("Leech x1: a kill heals 1"), GetHealth(ASC), 71.f, 0.01f);

		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Leech"), TEXT("4") });
		TestEqual(TEXT("HealthOnKill = 5"), GetAttr(UGSAttributeSet::GetHealthOnKillAttribute()), 5.f, 0.001f);
		KillEnemyWithSource(this, ASC);
		TestEqual(TEXT("Leech x5: a kill heals 5"), GetHealth(ASC), 76.f, 0.01f);

		KillEnemyWithSource(this, nullptr); // the enemy's own damage: not the player's kill
		TestEqual(TEXT("A kill that isn't the player's heals nothing"), GetHealth(ASC), 76.f, 0.01f);

		ApplyHeal(ASC, 100.f);
		ApplyDamage(ASC, 2.f);
		TestEqual(TEXT("Damaged to 98"), GetHealth(ASC), 98.f, 0.01f);
		KillEnemyWithSource(this, ASC);
		TestEqual(TEXT("Leech stops at MaxHealth"), GetHealth(ASC), GetMaxHealth(ASC), 0.01f);
		TestEqual(TEXT("MaxHealth unchanged"), GetMaxHealth(ASC), 100.f, 0.01f);

		// A new run removes both upgrades.
		GetGameMode()->RestartRunWithSeed(91);
		TestEqual(TEXT("New run: MagazineSizeMultiplier 1"), GetAttr(UGSAttributeSet::GetMagazineSizeMultiplierAttribute()), 1.f, 0.001f);
		TestEqual(TEXT("New run: rifle magazine 30"), GetPlayerMagazineSize(), 30);
		TestEqual(TEXT("New run: HealthOnKill 0"), GetAttr(UGSAttributeSet::GetHealthOnKillAttribute()), 0.f, 0.001f);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Loaded Dice: CritChance, the crit roll on the run stream, double damage, critical damage numbers
// ---------------------------------------------------------------------------

namespace GSTests
{
	/** Spawn a MeleeBandit at Location facing the player, with its AI and movement stopped. */
	AGSEnemyCharacter* SpawnFrozenBandit(const FVector& Location)
	{
		AGSEnemyCharacter* Enemy = AGSEnemyCharacter::SpawnEnemy(GetPIEWorld(), UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit")), Location,
			GetPlayer()->GetActorLocation());
		if (!Enemy)
		{
			return nullptr;
		}
		if (AAIController* AIC = Cast<AAIController>(Enemy->GetController()))
		{
			if (UBrainComponent* Brain = AIC->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("Crit test"));
			}
			AIC->StopMovement();
		}
		Enemy->GetCharacterMovement()->DisableMovement();
		return Enemy;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSLoadedDiceTest, "BanditBaller.Upgrades.LoadedDice", GSTests::TestFlags)
bool FGSLoadedDiceTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		TWeakObjectPtr<AGSEnemyCharacter> Target;
		float HealthBefore = 0.f;
		int32 CritNumbersBefore = 0;
	};
	TSharedRef<FState> S = MakeShared<FState>();

	// Data, the clamp, and the roll itself (GSCombat::RollCriticalHit) on the run stream.
	Step([this]()
	{
		GetGameMode()->SetRoundTimerPaused(true); // stay in PreRound: no wave
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("LoadedDice"));
		if (TestNotNull(TEXT("DA_Upgrade_LoadedDice exists"), Upgrade))
		{
			TestTrue(TEXT("Loaded Dice is a Stat Boost"), Upgrade->Category == EGSUpgradeCategory::StatBoost);
			TestEqual(TEXT("Overflow: 30% of crits deal triple damage at 130%"), Upgrade->FormatNowText(1.3f).ToString(),
				FString(TEXT("+130% critical hit chance (30% of crits deal triple damage)")));
			TestEqual(TEXT("No overflow at 100%"), Upgrade->FormatNowText(1.f).ToString(), FString(TEXT("+100% critical hit chance")));
			TestTrue(TEXT("Loaded Dice effect is GE_Upgrade_LoadedDice"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_LoadedDice::StaticClass()));
			const FString Description = Upgrade->FormatValueText(Upgrade->BaseValue).ToString(); // a Common pick's card line
			TestTrue(FString::Printf(TEXT("Description '%s' shows +5%%"), *Description), Description.Contains(TEXT("+5%")));
			const UGSUpgradePool* Pool = GetGameMode()->GetUpgradePool();
			TestTrue(TEXT("Loaded Dice is in DA_UpgradePool"), Pool && Pool->Upgrades.Contains(Upgrade));
		}
		const UGSCombatSettings* Combat = GetGameMode()->GetCombatSettings();
		if (TestNotNull(TEXT("CombatSettings (DA_CombatSettings) set on the GameMode"), Combat))
		{
			TestEqual(TEXT("CritMultiplier 2"), Combat->CritMultiplier, 2.f);
			TestEqual(TEXT("TripleCritMultiplier 3"), Combat->TripleCritMultiplier, 3.f);
		}
		TestEqual(TEXT("Triple chance at 100%"), GSCombat::GetTripleCritChance(1.f), 0.f);
		TestEqual(TEXT("Triple chance at 130%"), GSCombat::GetTripleCritChance(1.3f), 0.3f, 0.0001f);
		TestEqual(TEXT("Triple chance at 250%: all"), GSCombat::GetTripleCritChance(2.5f), 1.f);

		UGSAbilitySystemComponent* ASC = GetPlayerASC();
		FRandomStream& Stream = GetGameMode()->GetRunStream();
		TestEqual(TEXT("CritChance starts at 0"), GetAttr(UGSAttributeSet::GetCritChanceAttribute()), 0.f, 0.0001f);
		const int32 SeedBefore = Stream.GetCurrentSeed();
		bool bAnyCrit = false;
		for (int32 i = 0; i < 100; ++i)
		{
			const FGSCritRoll Roll = GSCombat::RollCriticalHit(ASC);
			bAnyCrit |= Roll.bCritical || Roll.DamageMultiplier != 1.f;
		}
		TestFalse(TEXT("CritChance 0: never a crit"), bAnyCrit);
		TestEqual(TEXT("CritChance 0: the run stream is untouched"), Stream.GetCurrentSeed(), SeedBefore);

		// 10 stacks: 50%. Rolls use the run stream: the same stream state gives the same crits.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("LoadedDice"), TEXT("10") });
		TestEqual(TEXT("10 stacks: CritChance 0.5"), GetAttr(UGSAttributeSet::GetCritChanceAttribute()), 0.5f, 0.0001f);
		const FRandomStream Saved = Stream;
		TArray<bool> First, Second;
		int32 Crits = 0;
		bool bMultiplierRight = true;
		for (int32 i = 0; i < 2000; ++i)
		{
			const FGSCritRoll Roll = GSCombat::RollCriticalHit(ASC);
			First.Add(Roll.bCritical);
			Crits += Roll.bCritical ? 1 : 0;
			bMultiplierRight &= Roll.DamageMultiplier == (Roll.bCritical ? 2.f : 1.f);
		}
		TestTrue(TEXT("CritChance 0.5: the rolls used the run stream"), Stream.GetCurrentSeed() != Saved.GetCurrentSeed());
		Stream = Saved;
		for (int32 i = 0; i < 2000; ++i)
		{
			Second.Add(GSCombat::RollCriticalHit(ASC).bCritical);
		}
		TestTrue(TEXT("Same run stream state -> same crits"), First == Second);
		AddInfo(FString::Printf(TEXT("CritChance 0.5: %d crits in 2000 rolls"), Crits));
		TestTrue(TEXT("CritChance 0.5: about half the rolls crit"), FMath::Abs(Crits / 2000.0 - 0.5) < 0.04);
		TestTrue(TEXT("A crit multiplies damage by 2, a normal hit by 1"), bMultiplierRight);

		// 20 stacks: 100%, always a crit without dice and never a triple.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("LoadedDice"), TEXT("10") });
		TestEqual(TEXT("20 stacks: CritChance 1"), GetAttr(UGSAttributeSet::GetCritChanceAttribute()), 1.f, 0.0001f);
		const int32 SeedAtFull = Stream.GetCurrentSeed();
		bool bAllCrit = true, bNoTriple = true;
		for (int32 i = 0; i < 100; ++i)
		{
			const FGSCritRoll Roll = GSCombat::RollCriticalHit(ASC);
			bAllCrit &= Roll.bCritical && Roll.DamageMultiplier == 2.f;
			bNoTriple &= !Roll.bTriple;
		}
		TestTrue(TEXT("CritChance 1: every hit is a double-damage crit"), bAllCrit);
		TestTrue(TEXT("CritChance 1: no triple hit"), bNoTriple);
		TestEqual(TEXT("CritChance 1: the run stream is untouched"), Stream.GetCurrentSeed(), SeedAtFull);

		// No cap: 30 stacks = 150%. Every hit crits, half of them triple (x3), on the run stream; replayable.
		TestEqual(TEXT("A 21st stack is granted (no cap)"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("LoadedDice"), 10), 10);
		TestEqual(TEXT("30 stacks: CritChance 1.5 (no clamp)"), GetAttr(UGSAttributeSet::GetCritChanceAttribute()), 1.5f, 0.0001f);
		const FRandomStream SavedTriple = Stream;
		TArray<bool> FirstTriples, SecondTriples;
		int32 Triples = 0;
		bool bCritAlways = true, bTripleMultiplier = true;
		for (int32 i = 0; i < 2000; ++i)
		{
			const FGSCritRoll Roll = GSCombat::RollCriticalHit(ASC);
			bCritAlways &= Roll.bCritical;
			Triples += Roll.bTriple ? 1 : 0;
			FirstTriples.Add(Roll.bTriple);
			bTripleMultiplier &= Roll.DamageMultiplier == (Roll.bTriple ? 3.f : 2.f) && Roll.GetLevel() == (Roll.bTriple ? 2 : 1);
		}
		TestTrue(TEXT("150%: every hit crits"), bCritAlways);
		AddInfo(FString::Printf(TEXT("CritChance 1.5: %d triple hits in 2000 rolls"), Triples));
		TestTrue(TEXT("150%: about half the crits are triple"), FMath::Abs(Triples / 2000.0 - 0.5) < 0.04);
		TestTrue(TEXT("A triple hit multiplies damage by 3 (level 2), a crit by 2 (level 1)"), bTripleMultiplier);
		TestTrue(TEXT("150%: the triple roll used the run stream"), Stream.GetCurrentSeed() != SavedTriple.GetCurrentSeed());
		Stream = SavedTriple;
		for (int32 i = 0; i < 2000; ++i)
		{
			SecondTriples.Add(GSCombat::RollCriticalHit(ASC).bTriple);
		}
		TestTrue(TEXT("Same run stream state -> same triple hits"), FirstTriples == SecondTriples);

		// 40 stacks = 200%: every hit triple, no dice at all.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("LoadedDice"), TEXT("10") });
		const int32 SeedAtDouble = Stream.GetCurrentSeed();
		bool bAllTriple = true;
		for (int32 i = 0; i < 100; ++i)
		{
			bAllTriple &= GSCombat::RollCriticalHit(ASC).bTriple;
		}
		TestTrue(TEXT("200%: every hit is a triple"), bAllTriple);
		TestEqual(TEXT("200%: the run stream is untouched"), Stream.GetCurrentSeed(), SeedAtDouble);

		// The attribute has no upper clamp, only >= 0.
		UGSDebugLibrary::SetAttributeOn(GetPC(), TEXT("CritChance"), -3.f);
		TestTrue(TEXT("CritChance clamped to at least 0"), GetAttr(UGSAttributeSet::GetCritChanceAttribute()) >= -0.0001f);

		// A new run removes the stacks (and the debug base value goes back with the restart's reset below).
		UGSDebugLibrary::SetAttributeOn(GetPC(), TEXT("CritChance"), 0.f);
		GetGameMode()->RestartRunWithSeed(23);
		GetGameMode()->SetRoundTimerPaused(true);
		TestEqual(TEXT("New run: CritChance 0"), GetAttr(UGSAttributeSet::GetCritChanceAttribute()), 0.f, 0.0001f);
	});

	// Real rifle shots at a frozen bandit 4 m ahead: a normal hit deals 20 with a plain damage number; with 20 stacks a
	// hit deals 40 and shows a critical number (bigger, another colour).
	Step([S]()
	{
		GetPlayer()->TeleportTo(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);
		S->Target = SpawnFrozenBandit(FVector(400.f, 0.f, 0.f));
	});
	TSharedRef<int32> AimFrames = MakeShared<int32>(0);
	WaitUntil(this, TEXT("aim at the bandit"), [S, AimFrames]()
	{
		if (!S->Target.IsValid())
		{
			return true;
		}
		AimAt(*S->Target);
		return ++(*AimFrames) >= 6; // let the camera settle on the new view
	});
	Step([this, S]()
	{
		if (!TestTrue(TEXT("The bandit is there"), S->Target.IsValid()))
		{
			return;
		}
		S->HealthBefore = GetHealth(S->Target->GetAbilitySystemComponent());
		TestEqual(TEXT("Round 1 bandit at 60"), S->HealthBefore, 60.f, 0.01f);
		AimAt(*S->Target);
		TapInput(GSGameplayTags::Ability_Fire);
		const float Dealt = S->HealthBefore - GetHealth(S->Target->GetAbilitySystemComponent());
		TestEqual(TEXT("No Loaded Dice: a rifle hit deals 20"), Dealt, 20.f, 0.01f);
		const UGSHUDWidget* HUD = GetHUD();
		if (TestNotNull(TEXT("HUD"), HUD))
		{
			TestEqual(TEXT("Damage number 20"), HUD->GetLastDamageNumber(), 20.f, 0.01f);
			TestFalse(TEXT("A plain damage number"), HUD->WasLastDamageNumberCritical());
			S->CritNumbersBefore = HUD->GetCriticalDamageNumbersAdded();
			TestTrue(TEXT("Critical numbers are bigger"), HUD->GetCritFontSize() > HUD->GetDamageNumberFontSize());
			TestFalse(TEXT("Critical numbers have another colour"), HUD->GetCritColor().Equals(HUD->GetDamageNumberColor()));
		}
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("LoadedDice"), TEXT("20") });
		S->HealthBefore = GetHealth(S->Target->GetAbilitySystemComponent());
	});
	WaitUntil(this, TEXT("firing to end"), []() { return !IsPlayerFiring(); });
	Step([this, S]()
	{
		if (!S->Target.IsValid())
		{
			AddError(TEXT("The bandit is gone"));
			return;
		}
		AimAt(*S->Target);
		TapInput(GSGameplayTags::Ability_Fire);
		const float Dealt = S->HealthBefore - GetHealth(S->Target->GetAbilitySystemComponent());
		TestEqual(TEXT("Loaded Dice x20: the hit is critical and deals 40"), Dealt, 40.f, 0.01f);
		TestTrue(TEXT("The crit killed the 40 HP bandit"), S->Target->GetHealthComponent()->IsDead());
		const UGSHUDWidget* HUD = GetHUD();
		if (HUD)
		{
			TestEqual(TEXT("Damage number 40"), HUD->GetLastDamageNumber(), 40.f, 0.01f);
			TestTrue(TEXT("A critical damage number"), HUD->WasLastDamageNumberCritical());
			TestEqual(TEXT("Crit level 1"), static_cast<int32>(HUD->GetLastDamageNumberCritLevel()), 1);
			TestEqual(TEXT("One more critical number"), HUD->GetCriticalDamageNumbersAdded(), S->CritNumbersBefore + 1);
			S->CritNumbersBefore = HUD->GetTripleDamageNumbersAdded();
			TestTrue(TEXT("Triple numbers are bigger than critical ones"), HUD->GetTripleCritFontSize() > HUD->GetCritFontSize());
			TestFalse(TEXT("Triple numbers have their own colour"), HUD->GetTripleCritColor().Equals(HUD->GetCritColor()));
		}
		// 40 stacks (200%): every hit is a triple hit.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("LoadedDice"), TEXT("20") });
		S->Target = SpawnFrozenBandit(FVector(400.f, 0.f, 0.f));
	});
	WaitUntil(this, TEXT("firing to end before the triple shot"), []() { return !IsPlayerFiring(); });
	TSharedRef<int32> TripleAimFrames = MakeShared<int32>(0);
	WaitUntil(this, TEXT("aim at the second bandit"), [S, TripleAimFrames]()
	{
		if (!S->Target.IsValid())
		{
			return true;
		}
		AimAt(*S->Target);
		return ++(*TripleAimFrames) >= 6;
	});
	Step([this, S]()
	{
		if (!S->Target.IsValid())
		{
			AddError(TEXT("The second bandit is gone"));
			return;
		}
		S->HealthBefore = GetHealth(S->Target->GetAbilitySystemComponent());
		AimAt(*S->Target);
		TapInput(GSGameplayTags::Ability_Fire);
		const float Dealt = S->HealthBefore - GetHealth(S->Target->GetAbilitySystemComponent());
		TestEqual(TEXT("Loaded Dice x40 (200%): the hit is a triple and deals 60"), Dealt, 60.f, 0.01f);
		const UGSHUDWidget* HUD = GetHUD();
		if (HUD)
		{
			TestEqual(TEXT("Damage number 60"), HUD->GetLastDamageNumber(), 60.f, 0.01f);
			TestEqual(TEXT("Crit level 2 (triple)"), static_cast<int32>(HUD->GetLastDamageNumberCritLevel()), 2);
			TestEqual(TEXT("One more triple number"), HUD->GetTripleDamageNumbersAdded(), S->CritNumbersBefore + 1);
		}
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Piercing Rounds: Pierce, the shot trace through targets (once each, stops at walls), full damage, a crit roll each
// ---------------------------------------------------------------------------

namespace GSTests
{
	float GetActorHealth(const AActor* Actor)
	{
		const AGSEnemyCharacter* Enemy = Cast<AGSEnemyCharacter>(Actor);
		return Enemy ? GetHealth(Enemy->GetAbilitySystemComponent()) : -1.f;
	}

	/** Names of the trace's target hits, in order (for messages). */
	FString DescribeShot(const FGSShotTrace& Shot)
	{
		FString Text = FString::JoinBy(Shot.TargetHits, TEXT(", "), [](const FHitResult& Hit) { return GetNameSafe(Hit.GetActor()); });
		if (Shot.BlockingHit.bBlockingHit)
		{
			Text += FString::Printf(TEXT(" | stopped by %s"), *GetNameSafe(Shot.BlockingHit.GetActor()));
		}
		return Text;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSPiercingRoundsTest, "BanditBaller.Upgrades.PiercingRounds", GSTests::TestFlags)
bool FGSPiercingRoundsTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		TArray<TWeakObjectPtr<AGSEnemyCharacter>> Line;
		TArray<float> HealthBefore;
		int32 NumbersBefore = 0;
		int32 CritNumbersBefore = 0;
		FRandomStream StreamBefore;
	};
	TSharedRef<FState> S = MakeShared<FState>();

	// The trace on its own, along an exact line behind the start point (-X): 4 frozen bandits 1.5 m apart.
	Step([this, S]()
	{
		GetGameMode()->SetRoundTimerPaused(true); // stay in PreRound: no wave
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		GetPlayer()->TeleportTo(FVector(0.f, 0.f, 100.f), FRotator(0.f, 180.f, 0.f));

		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("PiercingRounds"));
		if (TestNotNull(TEXT("DA_Upgrade_PiercingRounds exists"), Upgrade))
		{
			TestTrue(TEXT("Piercing Rounds is an Ability"), Upgrade->Category == EGSUpgradeCategory::Ability);
			TestTrue(TEXT("Effect is GE_Upgrade_PiercingRounds"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_PiercingRounds::StaticClass()));
			const FString Description = Upgrade->FormatValueText(Upgrade->BaseValue).ToString(); // a Common pick's card line
			TestTrue(FString::Printf(TEXT("Description '%s' shows +1"), *Description), Description.Contains(TEXT("+1")));
			const UGSUpgradePool* Pool = GetGameMode()->GetUpgradePool();
			TestTrue(TEXT("Piercing Rounds is in DA_UpgradePool"), Pool && Pool->Upgrades.Contains(Upgrade));
		}
		TestEqual(TEXT("Pierce starts at 0"), GetAttr(UGSAttributeSet::GetPierceAttribute()), 0.f, 0.001f);

		for (const float X : { -400.f, -550.f, -700.f, -850.f })
		{
			S->Line.Add(SpawnFrozenBandit(FVector(X, 0.f, 0.f)));
		}
		for (int32 i = 0; i < S->Line.Num(); ++i)
		{
			if (!TestTrue(FString::Printf(TEXT("Bandit %d spawned"), i + 1), S->Line[i].IsValid()))
			{
				return;
			}
		}
		const float Z = S->Line[0]->GetActorLocation().Z + 30.f;
		const FVector Start(-250.f, 0.f, Z), End(-1100.f, 0.f, Z);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSPierceTest), false);
		Params.AddIgnoredActor(GetPlayer());
		auto HitActor = [](const FGSShotTrace& Shot, int32 Index) { return Shot.TargetHits.IsValidIndex(Index) ? Shot.TargetHits[Index].GetActor() : nullptr; };

		const FGSShotTrace Zero = UGSGameplayAbility_Fire::TraceShot(GetPIEWorld(), Start, End, 0, Params);
		AddInfo(FString::Printf(TEXT("Pierce 0: %s"), *DescribeShot(Zero)));
		TestEqual(TEXT("Pierce 0: one target"), Zero.TargetHits.Num(), 1);
		TestTrue(TEXT("Pierce 0: the nearest bandit"), HitActor(Zero, 0) == S->Line[0].Get());
		TestTrue(TEXT("Pierce 0: the shot ends in it"), Zero.End.Equals(Zero.TargetHits.IsValidIndex(0) ? FVector(Zero.TargetHits[0].ImpactPoint) : FVector::ZeroVector, 1.f));

		const FGSShotTrace Two = UGSGameplayAbility_Fire::TraceShot(GetPIEWorld(), Start, End, 2, Params);
		AddInfo(FString::Printf(TEXT("Pierce 2: %s"), *DescribeShot(Two)));
		TestEqual(TEXT("Pierce 2: three targets"), Two.TargetHits.Num(), 3);
		TestTrue(TEXT("Pierce 2: bandits 1, 2, 3 in order"), HitActor(Two, 0) == S->Line[0].Get() && HitActor(Two, 1) == S->Line[1].Get() && HitActor(Two, 2) == S->Line[2].Get());
		TestTrue(TEXT("Pierce 2: the line ends in the third"), Two.TargetHits.Num() == 3 && Two.End.Equals(FVector(Two.TargetHits[2].ImpactPoint), 1.f));

		const FGSShotTrace Ten = UGSGameplayAbility_Fire::TraceShot(GetPIEWorld(), Start, End, 10, Params);
		AddInfo(FString::Printf(TEXT("Pierce 10: %s"), *DescribeShot(Ten)));
		TestEqual(TEXT("Pierce 10: all four bandits, each once"), Ten.TargetHits.Num(), 4);
		TSet<AActor*> Unique;
		for (const FHitResult& Hit : Ten.TargetHits)
		{
			Unique.Add(Hit.GetActor());
		}
		TestEqual(TEXT("Pierce 10: every enemy at most once"), Unique.Num(), Ten.TargetHits.Num());
		TestFalse(TEXT("Setup: nothing else on the line"), Ten.BlockingHit.bBlockingHit);
		TestTrue(TEXT("Pierce 10: the line runs to the last thing hit (bandit 4)"), Ten.TargetHits.Num() == 4 && Ten.End.Equals(FVector(Ten.TargetHits[3].ImpactPoint), 1.f));
		const FGSShotTrace Empty = UGSGameplayAbility_Fire::TraceShot(GetPIEWorld(), FVector(-250.f, 300.f, Z), FVector(-1100.f, 300.f, Z), 10, Params);
		TestTrue(TEXT("A line that hits nothing runs to its end"), Empty.TargetHits.Num() == 0 && !Empty.BlockingHit.bBlockingHit && Empty.End.Equals(FVector(-1100.f, 300.f, Z), 1.f));

		// A wall between bandits 2 and 3 stops even a big pierce.
		AStaticMeshActor* Wall = GetPIEWorld()->SpawnActor<AStaticMeshActor>(FVector(-625.f, 0.f, 150.f), FRotator::ZeroRotator);
		if (TestNotNull(TEXT("Spawned a wall"), Wall))
		{
			Wall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			Wall->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			Wall->GetStaticMeshComponent()->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Wall->SetActorScale3D(FVector(0.2f, 3.f, 3.f));
			const FGSShotTrace Walled = UGSGameplayAbility_Fire::TraceShot(GetPIEWorld(), Start, End, 10, Params);
			AddInfo(FString::Printf(TEXT("Pierce 10 with a wall: %s"), *DescribeShot(Walled)));
			TestEqual(TEXT("Wall: only the two bandits before it"), Walled.TargetHits.Num(), 2);
			TestTrue(TEXT("Wall: it stops the shot"), Walled.BlockingHit.bBlockingHit && Walled.BlockingHit.GetActor() == Wall);
			TestTrue(TEXT("Wall: the shot ends at the wall"), Walled.End.Equals(FVector(Walled.BlockingHit.ImpactPoint), 1.f));
			Wall->Destroy();
		}

		// A dead target is passed without using up the pierce.
		ApplyDamage(S->Line[0]->GetAbilitySystemComponent(), 1000.f);
		const FGSShotTrace PastDead = UGSGameplayAbility_Fire::TraceShot(GetPIEWorld(), Start, End, 0, Params);
		TestTrue(TEXT("Dead bandit 1: Pierce 0 hits bandit 2"), HitActor(PastDead, 0) == S->Line[1].Get() && PastDead.TargetHits.Num() == 1);

		RemoveAllEnemies(GetPIEWorld());
		S->Line.Reset();
	});

	// A real rifle shot (a zero-spread copy of the rifle, so the line is exact) with Piercing Rounds x2 and Loaded Dice
	// x10 (50%): three bandits on the shot's line each take one full hit, each with its own crit roll and damage number.
	Step([this, S]()
	{
		const UGSWeaponData* Rifle = UGSDebugLibrary::FindWeaponData(TEXT("Rifle"));
		UGSWeaponData* Exact = DuplicateObject<UGSWeaponData>(Rifle, GetTransientPackage());
		Exact->HipSpread = 0.f;
		Exact->ADSSpread = 0.f;
		UGSDebugLibrary::GiveWeaponTo(GetPC(), Exact);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("PiercingRounds"), TEXT("2") });
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("LoadedDice"), TEXT("10") });
		TestEqual(TEXT("Pierce 2"), GetAttr(UGSAttributeSet::GetPierceAttribute()), 2.f, 0.001f);
		GetPlayer()->TeleportTo(FVector(0.f, 0.f, 100.f), FRotator(0.f, 180.f, 0.f));
		for (int32 i = 0; i < 4; ++i)
		{
			S->Line.Add(SpawnFrozenBandit(FVector(-400.f - 150.f * i, 0.f, 0.f)));
		}
	});
	TSharedRef<int32> AimFrames = MakeShared<int32>(0);
	WaitUntil(this, TEXT("aim at the first bandit"), [S, AimFrames]()
	{
		if (S->Line.Num() == 0 || !S->Line[0].IsValid())
		{
			return true;
		}
		AimAt(*S->Line[0]);
		return ++(*AimFrames) >= 6;
	});
	Step([this, S]()
	{
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S->Line)
		{
			if (!TestTrue(TEXT("Bandits are there"), Bandit.IsValid()))
			{
				return;
			}
		}
		// Put bandits 2-4 on the shot's exact line: muzzle -> where the camera ray meets bandit 1 (what GA_Fire does).
		AimAt(*S->Line[0]);
		FVector ViewLocation;
		FRotator ViewRotation;
		GetPC()->GetPlayerViewPoint(ViewLocation, ViewRotation);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSPierceAim), false);
		Params.AddIgnoredActor(GetPlayer());
		Params.AddIgnoredActor(GetPlayer()->GetWeaponComponent()->GetCurrentWeapon());
		FHitResult CameraHit;
		GetPIEWorld()->LineTraceSingleByChannel(CameraHit, ViewLocation, ViewLocation + ViewRotation.Vector() * 5000.f, ECC_Visibility, Params);
		if (!TestTrue(TEXT("The camera ray meets bandit 1"), CameraHit.GetActor() == S->Line[0].Get()))
		{
			return;
		}
		const FVector Muzzle = GetPlayer()->GetWeaponComponent()->GetCurrentWeapon()->GetMuzzleLocation();
		const FVector AimPoint = CameraHit.ImpactPoint;
		const FVector Direction = (AimPoint - Muzzle).GetSafeNormal();
		const float HeightOnBandit = AimPoint.Z - S->Line[0]->GetActorLocation().Z;
		for (int32 i = 1; i < S->Line.Num(); ++i)
		{
			const FVector OnLine = AimPoint + Direction * (60.f + 150.f * i);
			S->Line[i]->SetActorLocation(OnLine - FVector(0.f, 0.f, HeightOnBandit), false, nullptr, ETeleportType::TeleportPhysics);
		}

		S->HealthBefore.Reset();
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S->Line)
		{
			S->HealthBefore.Add(GetActorHealth(Bandit.Get()));
		}
		const UGSHUDWidget* HUD = GetHUD();
		S->NumbersBefore = HUD ? HUD->GetDamageNumbersAdded() : 0;
		S->CritNumbersBefore = HUD ? HUD->GetCriticalDamageNumbersAdded() : 0;
		S->StreamBefore = GetGameMode()->GetRunStream();

		TapInput(GSGameplayTags::Ability_Fire);

		int32 Hits = 0, Crits = 0;
		for (int32 i = 0; i < S->Line.Num(); ++i)
		{
			const float Dealt = S->HealthBefore[i] - GetActorHealth(S->Line[i].Get());
			AddInfo(FString::Printf(TEXT("Bandit %d took %.0f"), i + 1, Dealt));
			if (i < 3)
			{
				TestTrue(FString::Printf(TEXT("Bandit %d took one full hit (20, or 40 on a crit)"), i + 1), FMath::IsNearlyEqual(Dealt, 20.f, 0.01f) || FMath::IsNearlyEqual(Dealt, 40.f, 0.01f));
				Hits += Dealt > 0.f ? 1 : 0;
				Crits += FMath::IsNearlyEqual(Dealt, 40.f, 0.01f) ? 1 : 0;
			}
			else
			{
				TestEqual(TEXT("Bandit 4 is past the pierce: untouched"), Dealt, 0.f, 0.01f);
			}
		}
		TestEqual(TEXT("Three bandits hit by one shot"), Hits, 3);
		if (HUD)
		{
			TestEqual(TEXT("One damage number per bandit hit"), HUD->GetDamageNumbersAdded() - S->NumbersBefore, 3);
			TestEqual(TEXT("Critical numbers match the crits"), HUD->GetCriticalDamageNumbersAdded() - S->CritNumbersBefore, Crits);
		}
		// Each hit rolled its own crit: exactly three draws from the run stream (no kills, so nothing else rolled).
		FRandomStream Expected = S->StreamBefore;
		Expected.FRand();
		Expected.FRand();
		Expected.FRand();
		TestEqual(TEXT("Three crit rolls on the run stream, one per hit"), GetGameMode()->GetRunStream().GetCurrentSeed(), Expected.GetCurrentSeed());

		// A new run removes the upgrade.
		GetGameMode()->RestartRunWithSeed(57);
		TestEqual(TEXT("New run: Pierce 0"), GetAttr(UGSAttributeSet::GetPierceAttribute()), 0.f, 0.001f);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Powder Keg: a chance per stack that a kill explodes (run stream), a fixed blast (x Damage), never the player, chains
// roll again after a delay, kill credit
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSPowderKegTest, "BanditBaller.Upgrades.PowderKeg", GSTests::TestFlags)
bool FGSPowderKegTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		TArray<TWeakObjectPtr<AGSEnemyCharacter>> Bandits;
		float PlayerHealth = 0.f;
		int32 ComboBefore = 0;
		int32 CashPickupsBefore = 0;
		int32 OrbsBefore = 0;
		int32 KillsBefore = 0;
		TStrongObjectPtr<UGSCombatSettings> SettingsCopy;
	};
	TSharedRef<FState> S = MakeShared<FState>();
	auto Health = [](const TWeakObjectPtr<AGSEnemyCharacter>& Bandit) { return Bandit.IsValid() ? GetHealth(Bandit->GetAbilitySystemComponent()) : -1.f; };
	auto GetPS = []() { return GetPC() ? GetPC()->GetPlayerState<AGSPlayerState>() : nullptr; };

	// Data; at 0 stacks a kill never explodes and nothing is rolled.
	Step([this, S, Health, GetPS]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true); // stay in PreRound: no wave
		GameMode->SetHealthOrbDropChanceOverride(0.f); // no health orbs: the player's health is checked exactly
		GetPlayer()->TeleportTo(FVector(-400.f, 0.f, 100.f), FRotator(0.f, 180.f, 0.f));

		const UGSCombatSettings* Combat = GameMode->GetCombatSettings();
		if (!TestNotNull(TEXT("DA_CombatSettings"), Combat))
		{
			return;
		}
		TestEqual(TEXT("3% chance per stack"), Combat->ChancePerStack, 0.03f, 0.0001f);
		TestEqual(TEXT("Blast damage 30"), Combat->BlastDamage, 30.f);
		TestEqual(TEXT("Blast radius 3 m"), Combat->BlastRadius, 300.f);
		TestEqual(TEXT("Chain delay 0.1 s"), Combat->ChainDelay, 0.1f);
		TestEqual(TEXT("Chance at 0 stacks"), Combat->GetExplodeChance(0), 0.f);
		TestEqual(TEXT("Chance at 1 stack"), Combat->GetExplodeChance(1), 0.03f, 0.0001f);
		TestEqual(TEXT("Chance at 10 stacks"), Combat->GetExplodeChance(10), 0.3f, 0.0001f);
		TestEqual(TEXT("Chance at 33 stacks"), Combat->GetExplodeChance(33), 0.99f, 0.0001f);
		TestEqual(TEXT("Chance at 34 stacks: 100%"), Combat->GetExplodeChance(34), 1.f);
		TestEqual(TEXT("Explode chance clamped to 100%"), Combat->GetExplodeChance(50), 1.f);
		TestEqual(TEXT("The raw chance isn't (50 stacks: 150%)"), Combat->GetRawExplodeChance(50), 1.5f, 0.0001f);
		TestEqual(TEXT("Double-size chance at 34 stacks (34 x 3% = 102%): 2%"), Combat->GetDoubleBlastChance(34), 0.02f, 0.0001f);
		TestEqual(TEXT("Double-size chance at 33 stacks (99%): 0"), Combat->GetDoubleBlastChance(33), 0.f);
		TestEqual(TEXT("Double-size chance at 40 stacks (120%): 20%"), Combat->GetDoubleBlastChance(40), 0.2f, 0.0001f);
		TestEqual(TEXT("Double-size chance at 67 stacks (201%): all"), Combat->GetDoubleBlastChance(67), 1.f);
		TestEqual(TEXT("A double-size blast has twice the radius"), Combat->DoubleBlastRadiusMultiplier, 2.f);
		TestNotNull(TEXT("Blast damage effect set"), Combat->BlastDamageEffect.Get());
		TestNotNull(TEXT("Placeholder blast effect set"), Combat->BlastEffect.Get());
		TestNotNull(TEXT("Placeholder blast sound set"), Combat->BlastSound.Get());

		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("PowderKeg"));
		if (TestNotNull(TEXT("DA_Upgrade_PowderKeg exists"), Upgrade))
		{
			TestTrue(TEXT("Powder Keg is an Ability"), Upgrade->Category == EGSUpgradeCategory::Ability);
			TestEqual(TEXT("Overflow shown past 100% (40 stacks)"), Upgrade->FormatNowText(40.f).ToString(),
				FString(TEXT("+120% chance that a kill explodes (20% of blasts are double size)")));
			TestEqual(TEXT("No overflow at 99%"), Upgrade->FormatNowText(33.f).ToString(), FString(TEXT("+99% chance that a kill explodes")));
			// The card shows stacks x DisplayScale as the chance, so DisplayScale must be ChancePerStack in percent.
			TestEqual(TEXT("DisplayScale = ChancePerStack in percent"), Upgrade->GetDisplayScale(), Combat->ChancePerStack * 100.f, 0.0001f);
			const FString Card = Upgrade->FormatValueText(Upgrade->BaseValue).ToString();
			const FString PerStack = FString::Printf(TEXT("+%d%% chance"), FMath::RoundToInt(Combat->ChancePerStack * 100.f));
			TestTrue(FString::Printf(TEXT("A Common pick's card line matches ChancePerStack (%s): %s"), *PerStack, *Card), Card.StartsWith(PerStack));
			TestTrue(TEXT("A Rare pick (1.5 stacks) reads +4.5%"), Upgrade->FormatValueText(1.5f).ToString().StartsWith(TEXT("+4.5% chance")));
			TestTrue(TEXT("Effect is GE_Upgrade_PowderKeg"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_PowderKeg::StaticClass()));
			const UGSUpgradePool* Pool = GameMode->GetUpgradePool();
			TestTrue(TEXT("Powder Keg is in DA_UpgradePool"), Pool && Pool->Upgrades.Contains(Upgrade));
		}
		TestEqual(TEXT("PowderKeg starts at 0"), GetAttr(UGSAttributeSet::GetPowderKegAttribute()), 0.f, 0.001f);

		// 0 stacks: no roll at all (the stream is untouched), never an explosion.
		const int32 SeedBefore = GameMode->GetRunStream().GetCurrentSeed();
		int32 Explosions = 0;
		for (int32 i = 0; i < 100; ++i)
		{
			Explosions += GameMode->RollPowderKegChance(GetPS()) ? 1 : 0;
		}
		TestEqual(TEXT("0 stacks: 100 kill rolls, no explosion"), Explosions, 0);
		TestEqual(TEXT("0 stacks: nothing rolled on the run stream"), GameMode->GetRunStream().GetCurrentSeed(), SeedBefore);

		// A (dies) at -500, B 2 m away: a real kill at 0 stacks doesn't explode.
		for (const FVector& At : { FVector(-500.f, 0.f, 0.f), FVector(-700.f, 0.f, 0.f) })
		{
			S->Bandits.Add(SpawnFrozenBandit(At));
		}
		ApplyDamage(S->Bandits[0]->GetAbilitySystemComponent(), 1000.f, GetPlayerASC());
		TestTrue(TEXT("No Powder Keg: the kill"), S->Bandits[0]->GetHealthComponent()->IsDead());
		TestEqual(TEXT("No Powder Keg: no blast"), GameMode->GetPowderKegBlasts(), 0);
		TestEqual(TEXT("No Powder Keg: B untouched"), Health(S->Bandits[1]), 60.f, 0.01f);
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
	});

	// 34 stacks (100%): every kill explodes, with no roll. 30 damage within 3 m, never the player (standing 1 m from the
	// blast, god mode off). B (20 HP) dies in it, rolls its own 100% and explodes 0.1 s later, which reaches C (out of
	// the first blast's reach). The kills are the player's.
	Step([this, S, Health, GetPS]()
	{
		AGSGameMode* GameMode = GetGameMode();
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("PowderKeg"), TEXT("34") });
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Leech") });
		TestEqual(TEXT("PowderKeg 34"), GetAttr(UGSAttributeSet::GetPowderKegAttribute()), 34.f, 0.001f);
		// 34 stacks is 102%: a 2% double-size roll per blast. This part checks the normal blast's reach, so it uses a copy of
		// the settings where a double-size blast has the same size.
		UGSCombatSettings* SameSize = DuplicateObject<UGSCombatSettings>(GameMode->GetCombatSettings(), GetTransientPackage());
		SameSize->DoubleBlastRadiusMultiplier = 1.f;
		S->SettingsCopy.Reset(SameSize);
		GameMode->SetCombatSettingsOverride(SameSize);
		TestEqual(TEXT("34 stacks: 100% chance"), GameMode->GetPowderKegChance(GetPS()), 1.f);

		const int32 SeedBefore = GameMode->GetRunStream().GetCurrentSeed();
		int32 Explosions = 0;
		for (int32 i = 0; i < 100; ++i)
		{
			Explosions += GameMode->RollPowderKegChance(GetPS()) ? 1 : 0;
		}
		TestEqual(TEXT("34 stacks: 100 kill rolls, 100 explosions"), Explosions, 100);
		TestEqual(TEXT("100%: no dice on the run stream"), GameMode->GetRunStream().GetCurrentSeed(), SeedBefore);

		for (const FVector& At : { FVector(-500.f, 0.f, 0.f), FVector(-700.f, 0.f, 0.f), FVector(-950.f, 0.f, 0.f), FVector(-500.f, -400.f, 0.f) })
		{
			S->Bandits.Add(SpawnFrozenBandit(At));
		}
		for (int32 i = 0; i < S->Bandits.Num(); ++i)
		{
			if (!TestTrue(FString::Printf(TEXT("Bandit %d spawned"), i), S->Bandits[i].IsValid()))
			{
				return;
			}
		}
		S->Bandits[1]->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 20.f);

		ApplyDamage(GetPlayerASC(), 10.f); // so Leech has room to heal
		S->PlayerHealth = GetHealth(GetPlayerASC());
		UGSComboComponent* Combo = GetPlayerCombo();
		Combo->EndCombo(EGSComboEndReason::Reset);
		S->ComboBefore = Combo->GetComboCount();
		S->CashPickupsBefore = GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash);
		S->OrbsBefore = GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP);
		S->KillsBefore = GetGameState()->GetRunStats()->GetTotalKills();

		// Kill A as the player: it explodes at once.
		ApplyDamage(S->Bandits[0]->GetAbilitySystemComponent(), 1000.f, GetPlayerASC());
		TestEqual(TEXT("One blast at once"), GameMode->GetPowderKegBlasts(), 1);
		TestTrue(TEXT("B (20 HP, 2 m) died in the blast"), S->Bandits[1]->GetHealthComponent()->IsDead());
		TestEqual(TEXT("C (4.5 m) is out of reach"), Health(S->Bandits[2]), 60.f, 0.01f);
		TestEqual(TEXT("D (4 m) is out of reach"), Health(S->Bandits[3]), 60.f, 0.01f);
		TestEqual(TEXT("B's blast waits for the chain delay"), GameMode->GetPendingPowderKegs(), 1);
		// The player is 1 m from A: never hurt by it; Leech healed the two kills (A by the gun, B by the blast).
		TestEqual(TEXT("The player isn't hurt and Leech healed both kills"), GetHealth(GetPlayerASC()), S->PlayerHealth + 2.f, 0.01f);
		TestEqual(TEXT("Blast kill counts for the combo (A + B)"), Combo->GetComboCount(), S->ComboBefore + 2);
		TestEqual(TEXT("Both kills in run stats"), GetGameState()->GetRunStats()->GetTotalKills(), S->KillsBefore + 2);
		TestTrue(TEXT("The blast kill dropped cash"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) > S->CashPickupsBefore);
		TestEqual(TEXT("Two XP orbs (A and B)"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::XP), S->OrbsBefore + 2);
	});
	// Wait on the chain itself (a fixed 0.2 s wait could end on a long first frame before the timer ticked).
	WaitUntil(this, TEXT("B's chain blast"), []() { return GetGameMode()->GetPendingPowderKegs() == 0; }, 10.0);
	Step([this, S, Health, GetPS]()
	{
		AGSGameMode* GameMode = GetGameMode();
		TestEqual(TEXT("B exploded 0.1 s later: two blasts"), GameMode->GetPowderKegBlasts(), 2);
		TestEqual(TEXT("B's blast hit C (2.5 m from B) for 30"), Health(S->Bandits[2]), 30.f, 0.01f);
		TestEqual(TEXT("D (4.7 m from B) still untouched"), Health(S->Bandits[3]), 60.f, 0.01f);
		TestEqual(TEXT("Nothing pending"), GameMode->GetPendingPowderKegs(), 0);

		// A kill that isn't the player's doesn't explode, even at 100%.
		const int32 Blasts = GameMode->GetPowderKegBlasts();
		ApplyDamage(S->Bandits[2]->GetAbilitySystemComponent(), 1000.f);
		TestTrue(TEXT("C died to its own damage"), S->Bandits[2]->GetHealthComponent()->IsDead());
		TestEqual(TEXT("A kill by someone else: no blast"), GameMode->GetPowderKegBlasts(), Blasts);
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();

		// The blast is the same size at 1 and at 20 stacks: 30 damage at 2.9 m, nothing at 3.1 m.
		GameMode->SetCombatSettingsOverride(nullptr); // the real settings from here on
		GameMode->RestartRunWithSeed(61);
		GameMode->SetRoundTimerPaused(true);
		const FVector Ground(-500.f, 0.f, 0.f);
		FVector Center = Ground;
		for (const int32 Stacks : { 1, 20 })
		{
			const int32 Have = FMath::RoundToInt(GetAttr(UGSAttributeSet::GetPowderKegAttribute()));
			UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("PowderKeg"), Stacks - Have);
			TestEqual(FString::Printf(TEXT("PowderKeg %d"), Stacks), GetAttr(UGSAttributeSet::GetPowderKegAttribute()), static_cast<float>(Stacks), 0.001f);
			const TWeakObjectPtr<AGSEnemyCharacter> Inside = SpawnFrozenBandit(Ground + FVector(0.f, -290.f, 0.f));
			const TWeakObjectPtr<AGSEnemyCharacter> Outside = SpawnFrozenBandit(Ground + FVector(0.f, 310.f, 0.f));
			if (!Inside.IsValid() || !Outside.IsValid())
			{
				AddError(TEXT("Blast-size bandits didn't spawn"));
				return;
			}
			// The blast is measured in 3D: center it at the bandits' own height (where a dead enemy's blast starts).
			Center = Inside->GetActorLocation() + FVector(0.f, 290.f, 0.f);
			AddInfo(FString::Printf(TEXT("[blast] %d stack(s): bandits %.0f and %.0f cm from the blast"), Stacks,
				FVector::Dist(Inside->GetActorLocation(), Center), FVector::Dist(Outside->GetActorLocation(), Center)));
			GameMode->ExplodePowderKeg(Center, GetPS());
			TestEqual(FString::Printf(TEXT("%d stack(s): 30 damage at 2.9 m"), Stacks), Health(Inside), 30.f, 0.01f);
			TestEqual(FString::Printf(TEXT("%d stack(s): nothing at 3.1 m (radius 3 m)"), Stacks), Health(Outside), 60.f, 0.01f);
			RemoveAllEnemies(GetPIEWorld());
		}

		// The player's Damage still scales it: x1.1 -> 33.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Damage") });
		const TWeakObjectPtr<AGSEnemyCharacter> Scaled = SpawnFrozenBandit(Ground + FVector(0.f, -290.f, 0.f));
		GameMode->ExplodePowderKeg(Center, GetPS());
		TestEqual(TEXT("Damage x1.1: 33"), Health(Scaled), 27.f, 0.01f);
		RemoveAllEnemies(GetPIEWorld());
		TestEqual(TEXT("Below 100%: no double-size blast"), GameMode->GetDoublePowderKegBlasts(), 0);

		// No cap: 67 stacks (201%). Every blast is double size without a roll: twice the radius (6 m), the same damage.
		GameMode->RestartRunWithSeed(64);
		GameMode->SetRoundTimerPaused(true);
		TestEqual(TEXT("67 picks granted (no cap)"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("PowderKeg"), 67), 67);
		{
			const TWeakObjectPtr<AGSEnemyCharacter> Inside = SpawnFrozenBandit(Ground + FVector(0.f, -590.f, 0.f));
			const TWeakObjectPtr<AGSEnemyCharacter> Outside = SpawnFrozenBandit(Ground + FVector(0.f, 610.f, 0.f));
			if (!Inside.IsValid() || !Outside.IsValid())
			{
				AddError(TEXT("Double-size bandits didn't spawn"));
				return;
			}
			const int32 SeedBefore = GameMode->GetRunStream().GetCurrentSeed(); // after spawning: spawns use the run stream
			const FVector DoubleCenter = Inside->GetActorLocation() + FVector(0.f, 590.f, 0.f);
			GameMode->ExplodePowderKeg(DoubleCenter, GetPS());
			TestEqual(TEXT("201%: a double-size blast"), GameMode->GetDoublePowderKegBlasts(), 1);
			TestEqual(TEXT("201%: radius 6 m"), GameMode->GetLastPowderKegRadius(), 600.f, 0.01f);
			TestEqual(TEXT("201%: the same damage, 30 at 5.9 m"), Health(Inside), 30.f, 0.01f);
			TestEqual(TEXT("201%: nothing at 6.1 m"), Health(Outside), 60.f, 0.01f);
			TestEqual(TEXT("201%: no dice on the run stream"), GameMode->GetRunStream().GetCurrentSeed(), SeedBefore);
			RemoveAllEnemies(GetPIEWorld());
		}

		// 40 stacks (120%): every kill explodes, and 1,000 blasts' size rolls come out close to 20% double, replayable.
		GameMode->RestartRunWithSeed(65);
		GameMode->SetRoundTimerPaused(true);
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("PowderKeg"), 40);
		TestEqual(TEXT("40 stacks: every kill explodes"), GameMode->GetPowderKegChance(GetPS()), 1.f);
		{
			const FRandomStream SavedSize = GameMode->GetRunStream();
			auto CountDoubles = [GameMode, GetPS]()
			{
				int32 Count = 0;
				for (int32 i = 0; i < 1000; ++i)
				{
					Count += GameMode->RollPowderKegDoubleSize(GetPS()) ? 1 : 0;
				}
				return Count;
			};
			const int32 Doubles = CountDoubles();
			AddInfo(FString::Printf(TEXT("[blast] 40 stacks: %d double-size blasts in 1000 rolls"), Doubles));
			TestTrue(FString::Printf(TEXT("40 stacks: ~20%% double size (%d)"), Doubles), Doubles >= 160 && Doubles <= 240);
			GameMode->GetRunStream() = SavedSize;
			TestEqual(TEXT("The same stream state replays the same sizes"), CountDoubles(), Doubles);
		}

		// 10 stacks: 1,000 kill rolls on the run stream come out close to 30%, and the same stream state replays them.
		GameMode->RestartRunWithSeed(62);
		GameMode->SetRoundTimerPaused(true);
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("PowderKeg"), 10);
		TestEqual(TEXT("10 stacks: 30% chance"), GameMode->GetPowderKegChance(GetPS()), 0.3f, 0.0001f);
		const FRandomStream Saved = GameMode->GetRunStream();
		auto CountExplosions = [GameMode, GetPS]()
		{
			int32 Count = 0;
			for (int32 i = 0; i < 1000; ++i)
			{
				Count += GameMode->RollPowderKegChance(GetPS()) ? 1 : 0;
			}
			return Count;
		};
		const int32 Explosions = CountExplosions();
		AddInfo(FString::Printf(TEXT("[blast] 10 stacks: %d explosions in 1000 kill rolls (%.1f%%)"), Explosions, Explosions / 10.f));
		TestTrue(FString::Printf(TEXT("10 stacks: ~30%% of 1000 kills explode (%d)"), Explosions), Explosions >= 260 && Explosions <= 340);
		TestTrue(TEXT("The rolls advanced the run stream"), GameMode->GetRunStream().GetCurrentSeed() != Saved.GetCurrentSeed());
		GameMode->GetRunStream() = Saved;
		TestEqual(TEXT("The same stream state replays the same explosions"), CountExplosions(), Explosions);

		GameMode->RestartRunWithSeed(63);
		TestEqual(TEXT("New run: PowderKeg 0"), GetAttr(UGSAttributeSet::GetPowderKegAttribute()), 0.f, 0.001f);
		GameMode->SetHealthOrbDropChanceOverride(-1.f);
		RestoreWorldSpeed();
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
		float AirJumpZ = 0.f;
		float JumpZ = 0.f;
		float Coyote = 0.f;
		int32 FeedbackBefore = 0;
	};
	TSharedRef<FState> S = MakeShared<FState>();
	TSharedRef<double> FallStart = MakeShared<double>(0.0);

	// Data, defaults, and timing that doesn't depend on the editor being in front. Jumps, falls and dashes run in game
	// time, 3x faster (each step stays shorter than a 60 fps frame; the 0.12 s coyote window still spans several).
	Step([this, S]()
	{
		SpeedUpWorld(3.f);
		GetGameMode()->SetRoundTimerPaused(true); // stay in PreRound: no enemies

		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("ExtraJump"));
		if (TestNotNull(TEXT("DA_Upgrade_ExtraJump exists"), Upgrade))
		{
			TestTrue(TEXT("Extra Jump is an Ability"), Upgrade->Category == EGSUpgradeCategory::Ability);
			TestTrue(TEXT("Extra Jump effect is an upgrade effect"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect::StaticClass()));
			const FString Description = Upgrade->FormatValueText(Upgrade->BaseValue).ToString(); // a Common pick's card line
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

	// No cap: many more stacks keep JumpMaxCount = 1 + stacks.
	Step([this]()
	{
		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("ExtraJump"));
		const int32 Before = GetPlayerUpgrades()->GetStacks(Upgrade);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("ExtraJump"), TEXT("25") });
		const int32 Stacks = GetPlayerUpgrades()->GetStacks(Upgrade);
		TestEqual(TEXT("No cap: 25 more stacks granted"), Stacks, Before + 25);
		TestEqual(TEXT("No cap: ExtraJumps = stacks"), GetAttr(UGSAttributeSet::GetExtraJumpsAttribute()), static_cast<float>(Stacks), 0.001f);
		TestEqual(TEXT("No cap: JumpMaxCount = 1 + stacks"), GetPlayer()->JumpMaxCount, 1 + Stacks);
	});

	// A new run removes the extra jumps.
	Step([this, S]()
	{
		GetGameMode()->RestartRunWithSeed(5150);
		TestEqual(TEXT("New run: ExtraJumps 0"), GetAttr(UGSAttributeSet::GetExtraJumpsAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("New run: JumpMaxCount 1"), GetPlayer()->JumpMaxCount, 1);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Dash Charge: ExtraDashCharges -> GA_Dash max charges, one charge to start, recharge one after another, HUD icon
// ---------------------------------------------------------------------------

namespace GSTests
{
	const UGSUpgradeData* GetDashChargeUpgrade()
	{
		return UGSDebugLibrary::FindUpgradeData(TEXT("DashCharge"));
	}

	const UGSGameplayAbility_Dash* GetPlayerDash()
	{
		return UGSGameplayAbility_Dash::FindGrantedDash(GetPlayerASC());
	}

	int32 GetDashMaxCharges()
	{
		const UGSGameplayAbility_Dash* Dash = GetPlayerDash();
		const UGSAbilitySystemComponent* ASC = GetPlayerASC();
		return Dash && ASC ? Dash->GetMaxCharges(ASC->AbilityActorInfo.Get()) : -1;
	}

	int32 GetDashChargesReady()
	{
		const UGSGameplayAbility_Dash* Dash = GetPlayerDash();
		const UGSAbilitySystemComponent* ASC = GetPlayerASC();
		return Dash && ASC ? Dash->GetAvailableCharges(ASC->AbilityActorInfo.Get()) : -1;
	}

	bool IsDashActive()
	{
		const UGSAbilitySystemComponent* ASC = GetPlayerASC();
		return ASC && ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dashing);
	}

	/** Press and release the dash input (the real GA_Dash path, with charges). Returns whether a dash started. */
	bool TapDash()
	{
		UGSAbilitySystemComponent* ASC = GetPlayerASC();
		if (!ASC || IsDashActive())
		{
			return false;
		}
		ASC->AbilityInputTagPressed(GSGameplayTags::Ability_Dash);
		ASC->AbilityInputTagReleased(GSGameplayTags::Ability_Dash);
		return IsDashActive();
	}

	/** The HUD's dash icon: the WBP_AbilityIcon whose AbilityTag is Ability.Dash. */
	const UGSAbilityIconWidget* GetDashIcon()
	{
		const UGSHUDWidget* HUD = GetHUD();
		if (!HUD || !HUD->WidgetTree)
		{
			return nullptr;
		}
		const UGSAbilityIconWidget* Found = nullptr;
		HUD->WidgetTree->ForEachWidget([&Found](UWidget* Widget)
		{
			const UGSAbilityIconWidget* Icon = Cast<UGSAbilityIconWidget>(Widget);
			if (!Found && Icon && Icon->GetAbilityTag() == GSGameplayTags::Ability_Dash)
			{
				Found = Icon;
			}
		});
		return Found;
	}

	FString GetDashIconStatus()
	{
		const UGSAbilityIconWidget* Icon = GetDashIcon();
		return Icon ? Icon->GetStatusString() : FString(TEXT("<no dash icon>"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSDashChargeTest, "BanditBaller.Upgrades.DashCharge", GSTests::TestFlags)
bool FGSDashChargeTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		double RechargeTime = 0.0;
		double FirstDashTime = 0.0;
		double FirstChargeBack = 0.0;
		double SecondChargeBack = 0.0;
	};
	TSharedRef<FState> S = MakeShared<FState>();

	// Data and the one-charge start. Dashes and recharges run in game time, 4x faster.
	Step([this, S]()
	{
		SpeedUpWorld(4.f);
		GetGameMode()->SetRoundTimerPaused(true); // stay in PreRound: no enemies

		const UGSGameplayAbility_Dash* Dash = GetPlayerDash();
		if (!TestNotNull(TEXT("GA_Dash granted"), Dash) || !TestNotNull(TEXT("GA_Dash settings"), Dash->GetSettings()))
		{
			return;
		}
		TestEqual(TEXT("DA_DashSettings: the player starts with 1 charge"), Dash->GetSettings()->MaxCharges, 1);
		S->RechargeTime = Dash->GetSettings()->RechargeTime;
		TestTrue(TEXT("Recharge time longer than a few dashes"), S->RechargeTime > 1.5);

		const UGSUpgradeData* Upgrade = GetDashChargeUpgrade();
		if (TestNotNull(TEXT("DA_Upgrade_DashCharge exists"), Upgrade))
		{
			TestTrue(TEXT("Dash Charge is an Ability"), Upgrade->Category == EGSUpgradeCategory::Ability);
			TestTrue(TEXT("Dash Charge effect is GE_Upgrade_DashCharge"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_DashCharge::StaticClass()));
			const FString Description = Upgrade->FormatValueText(Upgrade->BaseValue).ToString(); // a Common pick's card line
			TestTrue(FString::Printf(TEXT("Description '%s' shows +1"), *Description), Description.Contains(TEXT("+1")));
			const UGSUpgradePool* Pool = GetGameMode()->GetUpgradePool();
			TestTrue(TEXT("Dash Charge is in DA_UpgradePool"), Pool && Pool->Upgrades.Contains(Upgrade));
		}

		TestEqual(TEXT("ExtraDashCharges starts at 0"), GetAttr(UGSAttributeSet::GetExtraDashChargesAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("1 max charge"), GetDashMaxCharges(), 1);
		TestEqual(TEXT("1 charge ready"), GetDashChargesReady(), 1);
	});
	WaitFrames(this, 2);
	Step([this]() { TestEqual(TEXT("One charge: the icon shows READY"), GetDashIconStatus(), FString(TEXT("READY"))); });

	// One dash uses the only charge; a second press during the recharge is refused.
	Step([this, S]()
	{
		TestTrue(TEXT("Dash 1 starts"), TapDash());
		S->FirstDashTime = GetPIEWorld()->GetTimeSeconds();
		TestEqual(TEXT("No charge left after dash 1"), GetDashChargesReady(), 0);
	});
	WaitUntil(this, TEXT("dash 1 to end"), []() { return !IsDashActive(); });
	Step([this, S]()
	{
		TestTrue(TEXT("Still inside the recharge"), GetPIEWorld()->GetTimeSeconds() - S->FirstDashTime < S->RechargeTime);
		TestFalse(TEXT("A second dash within the recharge is refused"), TapDash());
		TestEqual(TEXT("The refused dash used nothing"), GetDashChargesReady(), 0);
	});
	WaitFrames(this, 2);
	Step([this]()
	{
		const FString Status = GetDashIconStatus();
		TestTrue(FString::Printf(TEXT("One charge recharging: the icon shows the countdown, no count ('%s')"), *Status),
			Status.EndsWith(TEXT("s"), ESearchCase::CaseSensitive) && !Status.Contains(TEXT("/")));
	});

	// A pick during the recharge adds a charge at once: it is ready right away, and the icon switches to the count.
	Step([this]()
	{
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("DashCharge") });
		TestEqual(TEXT("ExtraDashCharges = 1"), GetAttr(UGSAttributeSet::GetExtraDashChargesAttribute()), 1.f, 0.001f);
		TestEqual(TEXT("Max charges = 1 + 1 at once"), GetDashMaxCharges(), 2);
		TestEqual(TEXT("The new charge is ready at once"), GetDashChargesReady(), 1);
		TestEqual(TEXT("Stacks tracked"), GetPlayerUpgrades()->GetStacks(GetDashChargeUpgrade()), 1);
	});
	WaitFrames(this, 1);
	Step([this]()
	{
		const FString Status = GetDashIconStatus();
		TestTrue(FString::Printf(TEXT("A frame after the pick the icon shows 1/2 ('%s')"), *Status), Status.StartsWith(TEXT("1/2")));
		TestTrue(TEXT("Dash 2 starts with the new charge"), TapDash());
		TestEqual(TEXT("Both charges used"), GetDashChargesReady(), 0);
	});
	WaitUntil(this, TEXT("dash 2 to end"), []() { return !IsDashActive(); });

	// Recharge stays one after another: the first charge comes back one recharge after dash 1, the second one recharge
	// after that.
	WaitUntil(this, TEXT("first charge back"), [S]()
	{
		if (GetDashChargesReady() >= 1)
		{
			S->FirstChargeBack = GetPIEWorld()->GetTimeSeconds();
			return true;
		}
		return false;
	}, 10.0);
	Step([this]() { TestEqual(TEXT("Charges come back one at a time"), GetDashChargesReady(), 1); });
	WaitUntil(this, TEXT("second charge back"), [S]()
	{
		if (GetDashChargesReady() >= 2)
		{
			S->SecondChargeBack = GetPIEWorld()->GetTimeSeconds();
			return true;
		}
		return false;
	}, 10.0);
	Step([this, S]()
	{
		const double First = S->FirstChargeBack - S->FirstDashTime;
		const double Second = S->SecondChargeBack - S->FirstChargeBack;
		AddInfo(FString::Printf(TEXT("Charges back %.2f s after dash 1, then %.2f s later (recharge %.2f s, game time)"), First, Second, S->RechargeTime));
		TestEqual(TEXT("First charge back one recharge after dash 1"), First, S->RechargeTime, 0.25);
		TestEqual(TEXT("Second charge back one recharge after the first"), Second, S->RechargeTime, 0.25);
	});

	// gs.Upgrade.Grant DashCharge 2: 3 stacks, 4 charges, all ready. Four dashes in a row, the fifth refused.
	Step([this]()
	{
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("DashCharge"), TEXT("2") });
		TestEqual(TEXT("3 stacks"), GetPlayerUpgrades()->GetStacks(GetDashChargeUpgrade()), 3);
		TestEqual(TEXT("ExtraDashCharges = 3"), GetAttr(UGSAttributeSet::GetExtraDashChargesAttribute()), 3.f, 0.001f);
		TestEqual(TEXT("Max charges = 1 + 3"), GetDashMaxCharges(), 4);
		TestEqual(TEXT("All 4 ready"), GetDashChargesReady(), 4);
	});
	WaitFrames(this, 1);
	Step([this]() { TestEqual(TEXT("The icon shows 4/4"), GetDashIconStatus(), FString(TEXT("4/4"))); });
	for (int32 i = 0; i < 4; ++i)
	{
		Step([this, i]() { TestTrue(FString::Printf(TEXT("Dash %d of 4 starts"), i + 1), TapDash()); });
		WaitUntil(this, TEXT("dash to end"), []() { return !IsDashActive(); });
	}
	Step([this]()
	{
		TestEqual(TEXT("All 4 charges used"), GetDashChargesReady(), 0);
		TestFalse(TEXT("A 5th dash is refused"), TapDash());
	});
	WaitFrames(this, 1);
	Step([this]()
	{
		const FString Status = GetDashIconStatus();
		TestTrue(FString::Printf(TEXT("All used: the icon shows 0/4 and the countdown ('%s')"), *Status), Status.StartsWith(TEXT("0/4")));
	});

	// New run: the stacks are removed and the player is back to one charge.
	Step([this]()
	{
		GetGameMode()->RestartRunWithSeed(5);
		TestEqual(TEXT("New run: ExtraDashCharges 0"), GetAttr(UGSAttributeSet::GetExtraDashChargesAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("New run: 1 max charge"), GetDashMaxCharges(), 1);
		TestEqual(TEXT("New run: no Dash Charge stacks"), GetPlayerUpgrades()->GetStacks(GetDashChargeUpgrade()), 0);
		RestoreWorldSpeed();
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

	UGSUpgradeData* MakeTestUpgrade(const TCHAR* Name, EGSUpgradeCategory Category = EGSUpgradeCategory::StatBoost,
		bool bWholeUnits = false, bool bLegendaryAbility = false)
	{
		// Unique name: a fixed one would collide when the tests run again in the same editor session.
		UGSUpgradeData* Upgrade = NewObject<UGSUpgradeData>(GetTransientPackage(),
			MakeUniqueObjectName(GetTransientPackage(), UGSUpgradeData::StaticClass(), FName(Name)), RF_Transient);
		Upgrade->Effect = UGSUpgradeEffect_Damage::StaticClass();
		Upgrade->BaseValue = 0.1f;
		Upgrade->Category = Category;
		Upgrade->bWholeUnits = bWholeUnits;
		Upgrade->bLegendaryAbility = bLegendaryAbility;
		return Upgrade;
	}

	bool AllDifferent(const TArray<UGSUpgradeData*>& Offer)
	{
		return TSet<UGSUpgradeData*>(Offer).Num() == Offer.Num() && !Offer.Contains(nullptr);
	}

	bool AllDifferent(const TArray<FGSUpgradeChoice>& Offer)
	{
		TArray<UGSUpgradeData*> Upgrades;
		for (const FGSUpgradeChoice& Choice : Offer)
		{
			Upgrades.Add(Choice.Upgrade);
		}
		return AllDifferent(Upgrades);
	}

	bool OfferContains(const TArray<FGSUpgradeChoice>& Offer, const UGSUpgradeData* Upgrade)
	{
		return Offer.ContainsByPredicate([Upgrade](const FGSUpgradeChoice& C) { return C.Upgrade == Upgrade; });
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
		TArray<FGSUpgradeChoice> FirstOffer;
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
		TestEqual(TEXT("Pool has the 21 upgrades"), Pool->Upgrades.Num(), 21);
		const float StatWeights[] = { 100.f, 20.f, 6.f, 1.f };
		const float AbilityWeights[] = { 100.f, 10.f, 3.f, 0.5f };
		for (const EGSUpgradeRarity Rarity : UGSUpgradePool::GetAllRarities())
		{
			const int32 R = static_cast<int32>(Rarity);
			const FString Name = UGSUpgradeData::GetRarityName(Rarity);
			TestEqual(FString::Printf(TEXT("Stat Boost %s weight"), *Name), Pool->GetRarityWeight(EGSUpgradeCategory::StatBoost, Rarity), StatWeights[R]);
			TestEqual(FString::Printf(TEXT("Weapon %s weight"), *Name), Pool->GetRarityWeight(EGSUpgradeCategory::Weapon, Rarity), StatWeights[R]);
			TestEqual(FString::Printf(TEXT("Ability %s weight"), *Name), Pool->GetRarityWeight(EGSUpgradeCategory::Ability, Rarity), AbilityWeights[R]);
		}
		TestEqual(TEXT("Legendary ability chance 0.8%"), Pool->LegendaryAbilityChance, 0.008f, 0.00001f);
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

		// Real pool, 30,000 offers (90,000 cards): always 3 different upgrades; Legendary ability cards at the 0.8% chance
		// (always "Legendary"); the other cards spread evenly over the 14 upgrades (AppearanceWeight 1 each) with their
		// category's rarity split; a whole-unit upgrade never comes up Rare (its Rare roll counts as Common).
		FRandomStream Stream(7);
		constexpr int32 NumOffers = 30000;
		bool bAllFull = true, bAllDifferent = true, bLegendaryCardsLegendary = true, bWholeNeverRare = true;
		int32 Cards = 0, LegendaryCards = 0, NormalCards = 0;
		TMap<const UGSUpgradeData*, int32> PerUpgrade;
		// [kind][rarity]: 0 stat boosts, 1 abilities (not whole units: Powder Keg), 2 whole-unit abilities, 3 weapons
		int32 Split[4][4] = {};
		for (int32 i = 0; i < NumOffers; ++i)
		{
			const TArray<FGSUpgradeChoice> Offer = Pool->RollOffer(Stream, NoStacks);
			bAllFull &= Offer.Num() == 3;
			bAllDifferent &= AllDifferent(Offer);
			for (const FGSUpgradeChoice& Choice : Offer)
			{
				++Cards;
				if (Choice.Upgrade->bLegendaryAbility)
				{
					++LegendaryCards;
					bLegendaryCardsLegendary &= Choice.Rarity == EGSUpgradeRarity::Legendary;
					continue;
				}
				++NormalCards;
				++PerUpgrade.FindOrAdd(Choice.Upgrade);
				bWholeNeverRare &= !(Choice.Upgrade->bWholeUnits && Choice.Rarity == EGSUpgradeRarity::Rare);
				const int32 Kind = Choice.Upgrade->Category == EGSUpgradeCategory::StatBoost ? 0
					: Choice.Upgrade->Category == EGSUpgradeCategory::Weapon ? 3 : (Choice.Upgrade->bWholeUnits ? 2 : 1);
				++Split[Kind][static_cast<int32>(Choice.Rarity)];
			}
		}
		TestTrue(TEXT("Every offer has 3 choices"), bAllFull);
		TestTrue(TEXT("Choices in an offer are different"), bAllDifferent);
		TestTrue(TEXT("Legendary ability cards are Legendary"), bLegendaryCardsLegendary);
		TestTrue(TEXT("A whole-unit upgrade is never Rare"), bWholeNeverRare);
		const double LegendaryShare = static_cast<double>(LegendaryCards) / Cards;
		AddInfo(FString::Printf(TEXT("Legendary ability cards: %d of %d (%.3f%%, chance 0.8%% per card)"), LegendaryCards, Cards, 100.0 * LegendaryShare));
		TestTrue(TEXT("Legendary ability cards at about 0.8% of cards"), FMath::Abs(LegendaryShare - 0.008) < 0.0015);
		bool bEven = PerUpgrade.Num() == 20;
		for (const TPair<const UGSUpgradeData*, int32>& Entry : PerUpgrade)
		{
			bEven &= FMath::Abs(static_cast<double>(Entry.Value) / NormalCards - 1.0 / 20.0) < 0.01;
		}
		TestTrue(TEXT("The 20 other upgrades come up evenly (AppearanceWeight 1 each)"), bEven);
		auto Share = [&Split](int32 Kind, EGSUpgradeRarity Rarity)
		{
			const int32 Total = Split[Kind][0] + Split[Kind][1] + Split[Kind][2] + Split[Kind][3];
			return Total > 0 ? static_cast<double>(Split[Kind][static_cast<int32>(Rarity)]) / Total : 0.0;
		};
		const TCHAR* KindNames[] = { TEXT("Stat Boost"), TEXT("Ability (Powder Keg)"), TEXT("Whole-unit ability"), TEXT("Weapon") };
		for (int32 Kind = 0; Kind < 4; ++Kind)
		{
			AddInfo(FString::Printf(TEXT("%s cards: Common %.2f%%, Rare %.2f%%, Epic %.2f%%, Legendary %.2f%%"), KindNames[Kind],
				100.0 * Share(Kind, EGSUpgradeRarity::Common), 100.0 * Share(Kind, EGSUpgradeRarity::Rare),
				100.0 * Share(Kind, EGSUpgradeRarity::Epic), 100.0 * Share(Kind, EGSUpgradeRarity::Legendary)));
		}
		// Expected: 100 / 20 / 6 / 1 of 127; 100 / 10 / 3 / 0.5 of 113.5; whole units (100 + 10) / 0 / 3 / 0.5 of 113.5.
		TestTrue(TEXT("Stat Boost Rare 15.7%"), FMath::Abs(Share(0, EGSUpgradeRarity::Rare) - 20.0 / 127.0) < 0.01);
		TestTrue(TEXT("Stat Boost Epic 4.7%"), FMath::Abs(Share(0, EGSUpgradeRarity::Epic) - 6.0 / 127.0) < 0.006);
		TestTrue(TEXT("Stat Boost Legendary 0.8%"), FMath::Abs(Share(0, EGSUpgradeRarity::Legendary) - 1.0 / 127.0) < 0.003);
		TestTrue(TEXT("Ability Rare 8.8%"), FMath::Abs(Share(1, EGSUpgradeRarity::Rare) - 10.0 / 113.5) < 0.02);
		TestTrue(TEXT("Whole-unit ability Common 96.9% (Rare counts as Common)"), FMath::Abs(Share(2, EGSUpgradeRarity::Common) - 110.0 / 113.5) < 0.006);
		TestTrue(TEXT("Whole-unit ability Epic 2.6%"), FMath::Abs(Share(2, EGSUpgradeRarity::Epic) - 3.0 / 113.5) < 0.005);
		TestTrue(TEXT("Weapon Rare 15.7%"), FMath::Abs(Share(3, EGSUpgradeRarity::Rare) - 20.0 / 127.0) < 0.02);

		// Same seed -> same offers (upgrades and rarities).
		FRandomStream A(99), B(99);
		bool bSame = true;
		for (int32 i = 0; i < 20; ++i)
		{
			bSame &= Pool->RollOffer(A, NoStacks) == Pool->RollOffer(B, NoStacks);
		}
		TestTrue(TEXT("Same seed rolls the same offers"), bSame);

		// Fewer upgrades than choices: offer what's left.
		UGSUpgradePool* Small = MakeTestPool({ MakeTestUpgrade(TEXT("TestUp_A")), MakeTestUpgrade(TEXT("TestUp_B")) });
		const TArray<FGSUpgradeChoice> SmallOffer = Small->RollOffer(Stream, NoStacks);
		TestEqual(TEXT("Two upgrades -> offer of 2"), SmallOffer.Num(), 2);
		TestTrue(TEXT("Small offer different"), AllDifferent(SmallOffer));

		// A Legendary ability: at chance 1 the first card is one, the next cards fall back (it is already in the offer);
		// at chance 0 it only comes up when nothing else is left.
		UGSUpgradeData* TestLegendary = MakeTestUpgrade(TEXT("TestUp_Legendary"), EGSUpgradeCategory::Ability, false, true);
		UGSUpgradePool* WithLegendary = MakeTestPool({ TestLegendary, MakeTestUpgrade(TEXT("TestUp_F")), MakeTestUpgrade(TEXT("TestUp_G")), MakeTestUpgrade(TEXT("TestUp_H")) });
		WithLegendary->LegendaryAbilityChance = 1.f;
		const TArray<FGSUpgradeChoice> Forced = WithLegendary->RollOffer(Stream, NoStacks);
		TestTrue(TEXT("Chance 1: the first card is the Legendary ability, Legendary"), Forced.Num() == 3 && Forced[0].Upgrade == TestLegendary && Forced[0].Rarity == EGSUpgradeRarity::Legendary);
		TestTrue(TEXT("Chance 1: it is on one card only"), Forced.Num() == 3 && Forced[1].Upgrade != TestLegendary && Forced[2].Upgrade != TestLegendary);
		WithLegendary->LegendaryAbilityChance = 0.f;
		bool bLegendaryNever = true;
		for (int32 i = 0; i < 200; ++i)
		{
			bLegendaryNever &= !OfferContains(WithLegendary->RollOffer(Stream, NoStacks), TestLegendary);
		}
		TestTrue(TEXT("Chance 0: never while three others are left"), bLegendaryNever);
		UGSUpgradePool* OnlyLegendary = MakeTestPool({ TestLegendary, MakeTestUpgrade(TEXT("TestUp_I")) });
		OnlyLegendary->LegendaryAbilityChance = 0.f;
		TestTrue(TEXT("Chance 0 but nothing else left: it fills the offer"), OfferContains(OnlyLegendary->RollOffer(Stream, NoStacks), TestLegendary));

		// The rarity roll: a pool that only rolls Rare gives Rare to a normal upgrade and Common to a whole-unit one.
		UGSUpgradeData* Fractional = MakeTestUpgrade(TEXT("TestUp_Fractional"));
		UGSUpgradeData* Whole = MakeTestUpgrade(TEXT("TestUp_Whole"), EGSUpgradeCategory::StatBoost, true);
		UGSUpgradePool* OnlyRare = MakeTestPool({ Fractional, Whole });
		OnlyRare->StatBoostRarityWeights = { { EGSUpgradeRarity::Rare, 1.f } };
		const TArray<FGSUpgradeChoice> RareOffer = OnlyRare->RollOffer(Stream, NoStacks);
		const FGSUpgradeChoice* FractionalCard = RareOffer.FindByPredicate([Fractional](const FGSUpgradeChoice& C) { return C.Upgrade == Fractional; });
		const FGSUpgradeChoice* WholeCard = RareOffer.FindByPredicate([Whole](const FGSUpgradeChoice& C) { return C.Upgrade == Whole; });
		TestTrue(TEXT("Only Rare rolls: a normal upgrade is Rare"), FractionalCard && FractionalCard->Rarity == EGSUpgradeRarity::Rare);
		TestTrue(TEXT("Only Rare rolls: a whole-unit upgrade is Common"), WholeCard && WholeCard->Rarity == EGSUpgradeRarity::Common);
		OnlyRare->AbilityRarityWeights = { { EGSUpgradeRarity::Epic, 1.f } };
		Fractional->Category = EGSUpgradeCategory::Ability;
		const TArray<FGSUpgradeChoice> AbilityOffer = OnlyRare->RollOffer(Stream, NoStacks);
		const FGSUpgradeChoice* AbilityCard = AbilityOffer.FindByPredicate([Fractional](const FGSUpgradeChoice& C) { return C.Upgrade == Fractional; });
		TestTrue(TEXT("Each category uses its own weights (Ability: only Epic)"), AbilityCard && AbilityCard->Rarity == EGSUpgradeRarity::Epic);

		// AppearanceWeight: 0 = never offered while others are left.
		UGSUpgradeData* Hidden = MakeTestUpgrade(TEXT("TestUp_Hidden"));
		Hidden->AppearanceWeight = 0.f;
		UGSUpgradePool* WithHidden = MakeTestPool({ Hidden, MakeTestUpgrade(TEXT("TestUp_J")), MakeTestUpgrade(TEXT("TestUp_K")), MakeTestUpgrade(TEXT("TestUp_L")) });
		bool bHiddenNever = true;
		for (int32 i = 0; i < 200; ++i)
		{
			bHiddenNever &= !OfferContains(WithHidden->RollOffer(Stream, NoStacks), Hidden);
		}
		TestTrue(TEXT("AppearanceWeight 0: never offered"), bHiddenNever);

		// No pick cap: an upgrade picked a thousand times is still offered; an empty pool offers nothing.
		UGSUpgradeData* Often = MakeTestUpgrade(TEXT("TestUp_Often"));
		UGSUpgradePool* WithOften = MakeTestPool({ Often, MakeTestUpgrade(TEXT("TestUp_C")),
			MakeTestUpgrade(TEXT("TestUp_D")), MakeTestUpgrade(TEXT("TestUp_E")) });
		bool bOftenSeen = false;
		for (int32 i = 0; i < 200; ++i)
		{
			const TArray<FGSUpgradeChoice> Offer = WithOften->RollOffer(Stream, [Often](const UGSUpgradeData* U) { return U == Often ? 1000 : 0; });
			bOftenSeen |= OfferContains(Offer, Often);
			bAllFull &= Offer.Num() == 3;
		}
		TestTrue(TEXT("No cap: an upgrade with 1000 picks is still offered"), bOftenSeen);
		TestTrue(TEXT("Offers stay full"), bAllFull);
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
		const UGSXPCurve* Curve = GetGameMode()->GetXPCurve();
		GetGameMode()->AwardXP(GetPC()->GetPlayerState<AGSPlayerState>(),
			static_cast<float>(Curve->GetXPToReachLevel(3) + Curve->GetXPToReachLevel(4)), false);
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
		const FGSUpgradeChoice PickedCard = Upgrades->GetCurrentOffer()[1];
		const float PickedValue = Upgrades->GetChoiceValue(PickedCard);
		S->Changes.Reset();
		TestTrue(TEXT("Choose 1"), GetGameMode()->ChooseUpgrade(PS, 1));
		TestEqual(TEXT("Chosen upgrade got a pick"), Upgrades->GetStacks(Picked), 1);
		const FGSUpgradeStack* PickedEntry = Upgrades->FindEntry(Picked);
		TestTrue(TEXT("The pick has the card's rolled rarity"), PickedEntry && PickedEntry->Picks.Num() == 1 && PickedEntry->Picks[0].Rarity == PickedCard.Rarity);
		TestEqual(TEXT("The pick adds the card's value"), Upgrades->GetTotalValue(Picked), PickedValue, 0.0001f);
		AddInfo(FString::Printf(TEXT("Picked %s %s: %s"), *UGSUpgradeData::GetRarityName(PickedCard.Rarity), *Picked->GetName(), *Picked->FormatValue(PickedValue)));
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
		S->FirstOffer = Upgrades->GetCurrentOffer();
		GetGameMode()->RestartRunWithSeed(31);
		GetGameMode()->AwardXP(PS, 100.f, false);
		TestTrue(TEXT("Same seed -> same first offer (upgrades and rarities)"), Upgrades->GetCurrentOffer() == S->FirstOffer);

		// The simulator uses its own stream: same result for the same seed, run stream untouched.
		const int32 RunSeedState = GetGameMode()->GetRunStream().GetCurrentSeed();
		const FGSUpgradeOfferSimulation Sim1 = UGSDebugLibrary::SimulateUpgradeOffers(2000, 5);
		const FGSUpgradeOfferSimulation Sim2 = UGSDebugLibrary::SimulateUpgradeOffers(2000, 5);
		TestEqual(TEXT("Run stream untouched by the simulator"), GetGameMode()->GetRunStream().GetCurrentSeed(), RunSeedState);
		TestEqual(TEXT("Simulated offers"), Sim1.Offers, 2000);
		TestEqual(TEXT("Simulated choices"), Sim1.TotalChoices, 6000);
		TestEqual(TEXT("No short offers"), Sim1.ShortOffers, 0);
		TestTrue(TEXT("Simulator is deterministic"), Sim1.UpgradeOffers.OrderIndependentCompareEqual(Sim2.UpgradeOffers)
			&& Sim1.RarityChoices.OrderIndependentCompareEqual(Sim2.RarityChoices));
		TestTrue(TEXT("Simulator: Epic and Legendary cards"), Sim1.RarityChoices.FindRef(TEXT("Epic")) > 0 && Sim1.RarityChoices.FindRef(TEXT("Legendary")) > 0);
		int32 CategoryCards = 0;
		for (const TPair<FString, int32>& Entry : Sim1.CategoryChoices)
		{
			CategoryCards += Entry.Value;
		}
		TestEqual(TEXT("Simulator: every card has a category"), CategoryCards, Sim1.TotalChoices);
		TestTrue(TEXT("Simulator: all four categories counted"), Sim1.CategoryChoices.FindRef(TEXT("Stat Boost")) > 0 && Sim1.CategoryChoices.FindRef(TEXT("Ability")) > 0
			&& Sim1.CategoryChoices.FindRef(TEXT("Weapon")) > 0 && Sim1.CategoryChoices.Contains(TEXT("Legendary ability")));
		TestEqual(TEXT("Simulator: no Rare Legendary ability"), Sim1.CategoryRarityChoices.FindRef(TEXT("Legendary ability / Rare")), 0);
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

	/**
	 * The cards show the open offer, in order: category, the card's rolled rarity (text and frame color), what the pick
	 * gives and what the player has now.
	 */
	bool CardsMatchOffer(FAutomationTestBase* Test, const TCHAR* What)
	{
		const UGSUpgradeChoiceWidget* Widget = GetChoiceWidget();
		const UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		const TArray<FGSUpgradeChoice>& Choices = Upgrades->GetCurrentOffer();
		bool bOk = Test->TestEqual(FString::Printf(TEXT("%s: one card per choice"), What), Widget->GetCardCount(), Choices.Num());
		for (int32 i = 0; bOk && i < Choices.Num(); ++i)
		{
			const FGSUpgradeChoice& Choice = Choices[i];
			const UGSUpgradeCardWidget* Card = Widget->GetCard(i);
			if (!Test->TestTrue(FString::Printf(TEXT("%s: card %d shows choice %d"), What, i, i), Card && Card->GetUpgrade() == Choice.Upgrade))
			{
				return false;
			}
			const float Value = Upgrades->GetChoiceValue(Choice);
			const int32 Picks = Upgrades->GetStacks(Choice.Upgrade);
			const FString Expected[] = {
				UGSUpgradeCardWidget::GetCategoryLabel(Choice.Upgrade, Picks).ToString(),
				UGSUpgradeData::GetRarityName(Choice.Rarity).ToUpper(),
				Value > 0.f ? Choice.Upgrade->FormatValueText(Value).ToString() : FString(),
				Picks > 0 ? TEXT("Now: ") + Choice.Upgrade->FormatNowText(Upgrades->GetTotalValue(Choice.Upgrade)).ToString() : FString(TEXT("Now: none")) };
			const FString Shown[] = { Card->GetCategoryText().ToString(), Card->GetRarityText().ToString(), Card->GetPickText().ToString(), Card->GetNowText().ToString() };
			const TCHAR* Lines[] = { TEXT("category"), TEXT("rarity"), TEXT("pick"), TEXT("now") };
			for (int32 Line = 0; Line < 4; ++Line)
			{
				bOk &= Test->TestEqual(FString::Printf(TEXT("%s: card %d %s line"), What, i, Lines[Line]), Shown[Line], Expected[Line]);
			}
			bOk &= Test->TestTrue(FString::Printf(TEXT("%s: card %d rolled rarity"), What, i), Card->GetRarity() == Choice.Rarity);
			bOk &= Test->TestTrue(FString::Printf(TEXT("%s: card %d border is the rarity color"), What, i), Card->GetBorderColor().Equals(Card->GetRarityColor(Choice.Rarity)));
			bOk &= Test->TestFalse(FString::Printf(TEXT("%s: card %d has a description"), What, i), Card->GetDescriptionText().IsEmpty());
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
		const EGSUpgradeRarity CardRarity = Widget->GetCard(1) ? Widget->GetCard(1)->GetRarity() : EGSUpgradeRarity::Common;
		TestTrue(TEXT("Key 2 consumed"), Widget->HandleChoiceKey(EKeys::Two));
		TestEqual(TEXT("Second card picked"), GetPlayerUpgrades()->GetStacks(S->Picked), 1);
		const FGSUpgradeStack* Entry = GetPlayerUpgrades()->FindEntry(S->Picked);
		TestTrue(TEXT("The pick has the card's rarity"), Entry && Entry->Picks.Num() == 1 && Entry->Picks[0].Rarity == CardRarity);
		TestFalse(TEXT("Cards hidden after the last pick"), Widget->AreCardsShowing());
		TestFalse(TEXT("Game unpaused"), IsWorldPaused());
		TestFalse(TEXT("Mouse cursor hidden"), GetPC()->ShouldShowMouseCursor());

		// Several level-ups at once (the XP from level 2 to level 6: levels 3-6) queue several offers.
		S->PicksBefore = GetPlayerUpgrades()->GetTotalPicks();
		const UGSXPCurve* Curve = GetGameMode()->GetXPCurve();
		GetGameMode()->AwardXP(GetPC()->GetPlayerState<AGSPlayerState>(),
			static_cast<float>(Curve->GetTotalXPToReachLevel(6) - Curve->GetTotalXPToReachLevel(2)), false);
		TestEqual(TEXT("Four level-ups: one open + 3 queued"), GetPlayerUpgrades()->GetQueuedOffers(), 3);
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

		// Wave spawns and the vacuum run in game time (clear times are game time too): 4x faster.
		SpeedUpWorld(4.f);
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
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Round HUD: round info, upgrade icons, round banners
// ---------------------------------------------------------------------------

namespace GSTests
{
	/** A round banner on screen together with "LEVEL UP!" or an open upgrade offer (the choice cards). */
	bool IsRoundBannerOverlapping()
	{
		const UGSHUDWidget* HUD = GetHUD();
		const UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		return HUD && HUD->IsRoundBannerShowing() && (HUD->IsLevelUpBannerShowing() || (Upgrades && Upgrades->HasOpenOffer()));
	}

	/** Enqueue: wait until Predicate, counting every frame a round banner overlaps the level-up flow. */
	void WaitWatchingBanners(FAutomationTestBase* Test, const FString& What, TSharedRef<int32> Overlaps, TFunction<bool()> Predicate, double Timeout = DefaultTimeout)
	{
		WaitUntil(Test, What, [Overlaps, Predicate]()
		{
			*Overlaps += IsRoundBannerOverlapping() ? 1 : 0;
			return Predicate();
		}, Timeout);
	}

	FString Ellipsis(const FString& Text)
	{
		return Text + TEXT("…");
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSRoundHUDTest, "BanditBaller.UI.RoundHUD", GSTests::TestFlags)
bool FGSRoundHUDTest::RunTest(const FString& Parameters)
{
	struct FHUDState
	{
		int32 ShownBefore = 0;
		double RealEnd = 0.0;
		TObjectPtr<const UGSUpgradeData> Picked;
	};
	TSharedRef<FHUDState> S = MakeShared<FHUDState>();
	TSharedRef<int32> Overlaps = MakeShared<int32>(0);

	BeginPIE(this);

	// Round info in PreRound: round, phase, countdown.
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		const UGSHUDWidget* HUD = GetHUD();
		if (!TestNotNull(TEXT("HUD"), HUD))
		{
			return;
		}
		// Speed: game time 4x (countdown, spawns, kills); the HUD's real-time banners shortened on this HUD instance
		// (the queueing rules don't depend on their length; the real-time wait for LEVEL UP! below follows it).
		SpeedUpWorld(4.f);
		UGSHUDWidget* MutableHUD = GetHUD();
		TestTrue(TEXT("Shorter LEVEL UP! banner for the test"), SetFloatProperty(MutableHUD, TEXT("LevelUpBannerDuration"), 0.8f));
		TestTrue(TEXT("Shorter round banners for the test"), SetFloatProperty(MutableHUD, TEXT("RoundBannerDuration"), 0.7f));

		GameMode->SetRoundTimerPaused(true); // the countdown follows the phase end time and keeps counting
		GameMode->RestartRunWithSeed(2401);
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		TestEqual(TEXT("Round text"), HUD->GetRoundText().ToString(), FString(TEXT("ROUND 1")));
		TestEqual(TEXT("Phase text"), HUD->GetPhaseText().ToString(), FString(TEXT("PRE-ROUND")));
		TestTrue(TEXT("Countdown shows in PreRound"), HUD->IsCountdownShowing());
		TestEqual(TEXT("Countdown at PreRound start"), HUD->GetCountdownText().ToString(), Ellipsis(TEXT("Round 1 in 3")));
		TestFalse(TEXT("No enemy count in PreRound"), HUD->IsEnemiesShowing());
		TestTrue(TEXT("ROUND 1 banner showing"), HUD->IsRoundBannerShowing() && HUD->GetRoundBannerTitle().ToString() == TEXT("ROUND 1"));
		TestEqual(TEXT("New run: no upgrade icons"), HUD->GetUpgradeIconCount(), 0);
	});
	WaitWorldSeconds(this, 1.1);
	Step([this]()
	{
		TestEqual(TEXT("Countdown ticks down once a second"), GetHUD()->GetCountdownText().ToString(), Ellipsis(TEXT("Round 1 in 2")));

		// Round 2's banner, then a level-up while it shows.
		GetGameMode()->SetRound(2);
		TestEqual(TEXT("Round text follows"), GetHUD()->GetRoundText().ToString(), FString(TEXT("ROUND 2")));
		TestEqual(TEXT("Countdown restarts"), GetHUD()->GetCountdownText().ToString(), Ellipsis(TEXT("Round 2 in 3")));
	});
	WaitWatchingBanners(this, TEXT("ROUND 2 banner"), Overlaps, []()
	{
		return GetHUD()->IsRoundBannerShowing() && GetHUD()->GetRoundBannerTitle().ToString() == TEXT("ROUND 2");
	});
	Step([this, S]()
	{
		UGSHUDWidget* HUD = GetHUD();
		S->ShownBefore = HUD->GetRoundBannersShown();
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("XPAdd"), { TEXT("100") });
		TestTrue(TEXT("LEVEL UP! shows"), HUD->IsLevelUpBannerShowing());
		TestTrue(TEXT("Upgrade offer open (game paused)"), GetPlayerUpgrades()->HasOpenOffer() && GetPIEWorld()->IsPaused());
		TestFalse(TEXT("The level-up hides the round banner"), HUD->IsRoundBannerShowing());
		TestEqual(TEXT("... and queues it again"), HUD->GetQueuedRoundBannerCount(), 1);
		S->RealEnd = FPlatformTime::Seconds() + 0.8 + 0.7; // the test's LEVEL UP! length + margin
	});
	// LEVEL UP! ends (real time, the game is paused) while the cards are up: the round banner keeps waiting.
	WaitWatchingBanners(this, TEXT("LEVEL UP! to end with the upgrade cards up"), Overlaps, [S]() { return FPlatformTime::Seconds() >= S->RealEnd; });
	Step([this, S]()
	{
		UGSHUDWidget* HUD = GetHUD();
		TestFalse(TEXT("LEVEL UP! is over"), HUD->IsLevelUpBannerShowing());
		TestFalse(TEXT("Round banner still waits for the cards"), HUD->IsRoundBannerShowing());
		TestEqual(TEXT("Still queued"), HUD->GetQueuedRoundBannerCount(), 1);
		// The first offer has a weapon card (it goes to the weapon slots, not the icon row): pick another card.
		const TArray<UGSUpgradeData*> Choices = GetPlayerUpgrades()->GetCurrentChoices();
		const int32 NonWeapon = Choices.IndexOfByPredicate([](const UGSUpgradeData* U) { return U->Category != EGSUpgradeCategory::Weapon; });
		S->Picked = Choices[FMath::Max(NonWeapon, 0)];
		TestTrue(TEXT("Upgrade picked"), GetGameMode()->ChooseUpgrade(GetPC()->GetPlayerState<AGSPlayerState>(), FMath::Max(NonWeapon, 0)));
		TestFalse(TEXT("No offer left"), GetPlayerUpgrades()->HasOpenOffer());
		TestTrue(TEXT("Closing the offer replays ROUND 2"), HUD->IsRoundBannerShowing() && HUD->GetRoundBannerTitle().ToString() == TEXT("ROUND 2"));
		TestEqual(TEXT("One more banner shown"), HUD->GetRoundBannersShown(), S->ShownBefore + 1);

		// Owned upgrades: one icon per upgrade with its stack count.
		TestEqual(TEXT("One upgrade icon"), HUD->GetUpgradeIconCount(), 1);
		const UGSUpgradeIconWidget* Icon = HUD->FindUpgradeIcon(S->Picked);
		if (TestNotNull(TEXT("Icon for the picked upgrade"), Icon))
		{
			TestEqual(TEXT("Picked once: x1"), Icon->GetStackText().ToString(), FString(TEXT("x1")));
		}
		GetPlayerUpgrades()->GrantUpgrade(S->Picked, TEXT("Test"));
		TestEqual(TEXT("Same upgrade again: still one icon"), HUD->GetUpgradeIconCount(), 1);
		TestEqual(TEXT("Same upgrade again: x2"), Icon ? Icon->GetStackText().ToString() : FString(), FString(TEXT("x2")));
		for (const UGSUpgradeData* Upgrade : GetGameMode()->GetUpgradePool()->Upgrades)
		{
			if (Upgrade && Upgrade != S->Picked && Upgrade->Category != EGSUpgradeCategory::Weapon)
			{
				GetPlayerUpgrades()->GrantUpgrade(Upgrade, TEXT("Test"));
				TestEqual(TEXT("Another upgrade: a second icon"), HUD->GetUpgradeIconCount(), 2);
				TestTrue(TEXT("Second icon x1"), HUD->FindUpgradeIcon(Upgrade) && HUD->FindUpgradeIcon(Upgrade)->GetStackText().ToString() == TEXT("x1"));
				break;
			}
		}

		// Combat: enemies left.
		GetGameMode()->AdvancePhase();
		TestEqual(TEXT("Phase text in Combat"), HUD->GetPhaseText().ToString(), FString(TEXT("COMBAT")));
		TestFalse(TEXT("No countdown in Combat"), HUD->IsCountdownShowing());
		TestTrue(TEXT("Enemy count in Combat"), HUD->IsEnemiesShowing());
		TestEqual(TEXT("Enemies left = the wave"), HUD->GetEnemiesText().ToString(),
			FString::Printf(TEXT("%d enemies left"), GetGameMode()->GetWaveDirector()->GetTotal()));
	});
	WaitWatchingBanners(this, TEXT("a wave enemy to spawn"), Overlaps, []() { return CountLivingEnemies() > 0; });
	Step([this]()
	{
		for (TActorIterator<AGSEnemyCharacter> It(GetPIEWorld()); It; ++It)
		{
			if (!It->GetHealthComponent()->IsDead())
			{
				ApplyDamage(It->GetAbilitySystemComponent(), 100000.f, GetPlayerASC());
				break;
			}
		}
		const int32 Remaining = GetGameState()->GetWaveEnemiesRemaining();
		TestEqual(TEXT("A kill updates the count"), Remaining, GetGameMode()->GetWaveDirector()->GetTotal() - 1);
		TestEqual(TEXT("Enemies left text follows"), GetHUD()->GetEnemiesText().ToString(), FString::Printf(TEXT("%d enemies left"), Remaining));
	});
	ClearWaveAsPlayer(this);
	Step([this]()
	{
		UGSHUDWidget* HUD = GetHUD();
		TestEqual(TEXT("Phase text in RoundClear"), HUD->GetPhaseText().ToString(), FString(TEXT("ROUND CLEAR")));
		TestFalse(TEXT("No enemy count in RoundClear"), HUD->IsEnemiesShowing());
	});
	WaitWatchingBanners(this, TEXT("ROUND CLEAR banner"), Overlaps, []()
	{
		return GetHUD()->IsRoundBannerShowing() && GetHUD()->GetRoundBannerTitle().ToString() == TEXT("ROUND CLEAR");
	});

	// A level-up during ROUND CLEAR, then the next round starts while the cards are up: the stale ROUND CLEAR is
	// dropped and round 3's banner shows instead.
	Step([this]()
	{
		UGSHUDWidget* HUD = GetHUD();
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("XPAdd"), { TEXT("200") });
		TestFalse(TEXT("Level-up hides ROUND CLEAR"), HUD->IsRoundBannerShowing());
		GetGameMode()->AdvancePhase(); // RoundClear -> round 3 PreRound (a direct call works while paused)
		TestEqual(TEXT("Queued: ROUND CLEAR and ROUND 3"), HUD->GetQueuedRoundBannerCount(), 2);
		TestFalse(TEXT("Nothing shows while the offer is open"), HUD->IsRoundBannerShowing());
		TestTrue(TEXT("Picked"), PickAllUpgradeOffers() > 0);
		TestFalse(TEXT("Still waiting for LEVEL UP! to end"), HUD->IsLevelUpBannerShowing() && HUD->IsRoundBannerShowing());
	});
	WaitWatchingBanners(this, TEXT("ROUND 3 banner after LEVEL UP!"), Overlaps, []()
	{
		return GetHUD()->IsRoundBannerShowing() && GetHUD()->GetRoundBannerTitle().ToString() == TEXT("ROUND 3");
	});
	Step([this]()
	{
		TestEqual(TEXT("Stale ROUND CLEAR dropped"), GetHUD()->GetQueuedRoundBannerCount(), 0);
	});
	WaitWatchingBanners(this, TEXT("ROUND 3 banner to finish"), Overlaps, []() { return !GetHUD()->IsRoundBannerShowing(); });
	Step([this, Overlaps]()
	{
		TestEqual(TEXT("A round banner never overlapped LEVEL UP! or the upgrade cards (frames)"), *Overlaps, 0);

		// A new run clears the upgrade icons.
		GetGameMode()->RestartRunWithSeed(2402);
		TestEqual(TEXT("New run: no upgrade icons"), GetHUD()->GetUpgradeIconCount(), 0);
		GetGameMode()->SetRoundTimerPaused(false);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Hub map (L_Hub_Graybox + AGSHubGameMode)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSHubMapTest, "BanditBaller.Map.Hub", GSTests::TestFlags)
bool FGSHubMapTest::RunTest(const FString& Parameters)
{
	TSharedRef<int32> AmmoBefore = MakeShared<int32>(0);

	BeginPIE(this, HubMap);
	Step([this, AmmoBefore]()
	{
		UWorld* World = GetPIEWorld();
		TestTrue(TEXT("PIE runs the hub map"), World && World->GetOutermost()->GetName().EndsWith(TEXT("L_Hub_Graybox")));
		const AGSHubGameMode* Hub = World ? World->GetAuthGameMode<AGSHubGameMode>() : nullptr;
		const AGSGameState* GS = World ? World->GetGameState<AGSGameState>() : nullptr;
		if (!TestNotNull(TEXT("Hub GameMode (BP_GSHubGameMode from the map's World Settings)"), Hub) || !TestNotNull(TEXT("AGSGameState"), GS))
		{
			return;
		}
		TestNull(TEXT("No round GameMode in the hub"), World->GetAuthGameMode<AGSGameMode>());
		TestTrue(TEXT("Hub area is the Market District"), GS->GetCurrentArea() == EGSArea::MarketDistrict);
		TestTrue(TEXT("No round loop (round 0, WaitingToStart)"), GS->GetRoundNumber() == 0 && GS->GetRoundPhase() == EGSRoundPhase::WaitingToStart);

		// The district's content, moved into the hub.
		TestEqual(TEXT("Casino"), CountTagged(World, TEXT("Casino")), 1);
		TestEqual(TEXT("4 market stalls"), CountTagged(World, TEXT("MarketStall")), 4);
		TestEqual(TEXT("MarketArrival"), CountTagged(World, TEXT("MarketArrival")), 1);
		TestEqual(TEXT("MarketExitPortalSpot"), CountTagged(World, TEXT("MarketExitPortalSpot")), 1);
		const AActor* Arrival = FindTagged(World, TEXT("MarketArrival"));
		const AGSCharacter* Player = GetPlayer();
		if (Arrival && Player)
		{
			const float Distance = FVector::Dist2D(Player->GetActorLocation(), Arrival->GetActorLocation());
			TestTrue(FString::Printf(TEXT("Player starts at MarketArrival (%.0f cm away)"), Distance), Distance < 150.f);
			TestFalse(TEXT("No NavMesh in the hub"), IsOnNavMesh(World, Arrival->GetActorLocation()));
		}

		// Boundary around the square.
		const AGSMapBoundary* Boundary = AGSMapBoundary::Find(World);
		if (TestNotNull(TEXT("AGSMapBoundary in the hub"), Boundary) && Player)
		{
			TestTrue(TEXT("Player inside the boundary"), Boundary->IsInside(Player->GetActorLocation()));
			const AActor* Casino = nullptr;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->ActorHasTag(TEXT("Casino")))
				{
					Casino = *It;
				}
			}
			TestTrue(TEXT("Casino inside the boundary"), Casino && Boundary->IsInside(Casino->GetActorLocation()));
		}

		// No enemies here.
		TestEqual(TEXT("No enemy spawns in the hub"), UGSDebugLibrary::SpawnEnemiesAround(GetPC(), UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit")), 1).Num(), 0);
		TestEqual(TEXT("No enemies alive"), CountLivingEnemies(), 0);

		// Safe zone: no damage, no shooting.
		UGSAbilitySystemComponent* ASC = GetPlayerASC();
		TestTrue(TEXT("Hub has a SafeZoneEffect (GE_SafeZone)"), Hub->GetSafeZoneEffect() != nullptr);
		TestTrue(TEXT("Player has State.SafeZone"), ASC && ASC->HasMatchingGameplayTag(GSGameplayTags::State_SafeZone));
		const float HealthBefore = GetHealth(ASC);
		ApplyDamage(ASC, 30.f);
		TestEqual(TEXT("GE_Damage is blocked in the safe zone"), GetHealth(ASC), HealthBefore);
		UGSWeaponComponent* Weapons = Player ? Player->GetWeaponComponent() : nullptr;
		if (TestNotNull(TEXT("Weapon component"), Weapons) && ASC)
		{
			*AmmoBefore = Weapons->GetCurrentAmmo();
			ASC->AbilityInputTagPressed(GSGameplayTags::Ability_Fire);
			ASC->AbilityInputTagReleased(GSGameplayTags::Ability_Fire);
			TestFalse(TEXT("GA_Fire doesn't activate (no State.Firing)"), ASC->HasMatchingGameplayTag(GSGameplayTags::State_Firing));
		}

		// HUD: cash only, no combat elements.
		const UGSHUDWidget* HUD = GetHUD();
		if (TestNotNull(TEXT("HUD"), HUD))
		{
			TestTrue(TEXT("HUD knows it's in the safe area"), HUD->IsInSafeArea());
			TestTrue(TEXT("HUD hides the round / combat elements"), HUD->AreCombatElementsHidden());
			TestTrue(TEXT("HUD shows the cash counter"), HUD->IsCashShown());
			TestEqual(TEXT("HUD cash = profile balance"), HUD->GetDisplayedCash(), GetPlayerCash());
		}
	});
	WaitWorldSeconds(this, 0.5);
	Step([this, AmmoBefore]()
	{
		const AGSCharacter* Player = GetPlayer();
		TestEqual(TEXT("No shot fired: ammo unchanged"), Player ? Player->GetWeaponComponent()->GetCurrentAmmo() : -1, *AmmoBefore);
		TestEqual(TEXT("Still no round loop half a second later"), GetGameState()->GetRoundNumber(), 0);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Start Run portal: hub -> town, the run starts when the town loads
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSStartRunPortalTest, "BanditBaller.Map.StartRunPortal", GSTests::TestFlags)
bool FGSStartRunPortalTest::RunTest(const FString& Parameters)
{
	TSharedRef<TWeakObjectPtr<UWorld>> HubWorld = MakeShared<TWeakObjectPtr<UWorld>>();
	TSharedRef<int32> CashBefore = MakeShared<int32>(0);
	static constexpr int32 ChosenSeed = 4242;

	BeginPIE(this, HubMap);
	Step([this, HubWorld, CashBefore]()
	{
		UWorld* World = GetPIEWorld();
		AGSHubGameMode* Hub = GetHubGameMode();
		AGSStartRunPortal* Portal = nullptr;
		for (TActorIterator<AGSStartRunPortal> It(World); It; ++It)
		{
			Portal = *It;
		}
		if (!TestNotNull(TEXT("Hub GameMode"), Hub) || !TestNotNull(TEXT("A Start Run portal in the hub"), Portal))
		{
			return;
		}
		const AActor* Spot = FindTagged(World, TEXT("MarketExitPortalSpot"));
		TestTrue(TEXT("Portal stands at MarketExitPortalSpot"), Spot && FVector::Dist2D(Portal->GetActorLocation(), Spot->GetActorLocation()) < 50.f);
		TestEqual(TEXT("Portal prompt"), Portal->GetPromptText().ToString(), FString(TEXT("START RUN")));
		TestEqual(TEXT("Portal opens the town (RunMap)"), Hub->GetRunMap().ToSoftObjectPath().GetLongPackageName(), TownMap);

		// gs.Run.Seed in the hub picks the next run's seed.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("RunSeed"), { FString::FromInt(ChosenSeed) });
		TestEqual(TEXT("gs.Run.Seed in the hub sets the next run's seed"), Hub->GetNextRunSeed(), ChosenSeed);

		// Walk in.
		*HubWorld = World;
		*CashBefore = GetPlayerCash();
		TestFalse(TEXT("No run starting yet"), Hub->IsStartingRun());
		GetPlayer()->TeleportTo(Portal->GetActorLocation() + FVector(0.f, 0.f, 100.f), GetPlayer()->GetActorRotation());
	});
	WaitUntil(this, TEXT("the portal to start the run"), []() { return GetHubGameMode() && GetHubGameMode()->IsStartingRun(); }, 5.0);
	Step([this]()
	{
		const APlayerCameraManager* Camera = GetPC() ? GetPC()->PlayerCameraManager.Get() : nullptr;
		TestTrue(TEXT("Entering the portal fades the screen out"), Camera && Camera->bEnableFading && Camera->FadeColor.Equals(FLinearColor::Black));
		TestFalse(TEXT("A second start is refused while one is under way"), GetHubGameMode()->StartRun(GetPC()));
	});
	WaitForMapTravel(this, HubWorld, TownMap);

	// The run starts when the town loads: the chosen seed, fresh stats, round 1, the cash kept.
	Step([this, CashBefore]()
	{
		AGSGameMode* GameMode = GetGameMode();
		const AGSGameState* GS = GetGameState();
		if (!TestNotNull(TEXT("Town GameMode after the portal"), GameMode) || !TestNotNull(TEXT("GameState"), GS))
		{
			return;
		}
		TestEqual(TEXT("Run seed from gs.Run.Seed in the hub"), GameMode->GetRunSeed(), ChosenSeed);
		TestEqual(TEXT("GameState has the seed"), GS->GetRunSeed(), ChosenSeed);
		TestEqual(TEXT("Round 1"), GS->GetRoundNumber(), 1);
		TestTrue(TEXT("Area is the town"), GS->GetCurrentArea() == EGSArea::Town);
		const UGSRunStats* Stats = GS->GetRunStats();
		TestTrue(TEXT("Fresh run stats"), Stats && Stats->GetTotalKills() == 0 && Stats->GetCashEarned() == 0 && Stats->GetRoundsCleared() == 0);
		TestEqual(TEXT("Cash kept across the map change (profile)"), GetPlayerCash(), *CashBefore);
		TestFalse(TEXT("No safe zone in the town"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_SafeZone));
		UGSDebugLibrary::SetGodMode(GetPC(), true);
	});

	// The first wave spawns in the ring around the player.
	WaitUntil(this, TEXT("round 1's first spawns"), []() { return GetGameMode() && GetGameMode()->GetWaveDirector()->GetSpawned() >= 2; }, 20.0);
	Step([this]()
	{
		const UGSWaveDirector* Waves = GetGameMode()->GetWaveDirector();
		const UGSSpawnSettings* Spawn = GetGameMode()->GetSpawnSettings();
		if (!TestNotNull(TEXT("Spawn settings"), Spawn))
		{
			return;
		}
		TestEqual(TEXT("No fallback spawn points"), Waves->GetFallbackSpawns(), 0);
		TestTrue(FString::Printf(TEXT("Closest spawn %.0f cm >= ring min %.0f"), Waves->GetClosestSpawnDistance(), Spawn->MinRadius),
			Waves->GetClosestSpawnDistance() >= Spawn->MinRadius - 1.f);
		TestTrue(FString::Printf(TEXT("Farthest spawn %.0f cm <= ring max %.0f"), Waves->GetFarthestSpawnDistance(), Spawn->MaxRadius),
			Waves->GetFarthestSpawnDistance() <= Spawn->MaxRadius + 1.f);
		const AGSMapBoundary* Boundary = AGSMapBoundary::Find(GetPIEWorld());
		for (TActorIterator<AGSEnemyCharacter> It(GetPIEWorld()); It; ++It)
		{
			TestTrue(FString::Printf(TEXT("%s on the NavMesh"), *It->GetName()), IsOnNavMesh(GetPIEWorld(), It->GetActorLocation()));
			TestTrue(FString::Printf(TEXT("%s inside the boundary"), *It->GetName()), Boundary && Boundary->IsInside(It->GetActorLocation()));
		}
		UGSDebugLibrary::SetGodMode(GetPC(), false);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Town layout (300 x 300 m): ground, boundary, NavMesh coverage and connectivity, landmarks, spawns, edges
// ---------------------------------------------------------------------------

namespace GSTests
{
	/** Ground hit straight below (X, Y) from 5 m up (WorldStatic), or false if there is none. */
	bool TraceGround(const UWorld* World, float X, float Y, FVector& OutHit)
	{
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByObjectType(Hit, FVector(X, Y, 500.f), FVector(X, Y, -500.f), FCollisionObjectQueryParams(ECC_WorldStatic));
		OutHit = Hit.ImpactPoint;
		return bHit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSTownLayoutTest, "BanditBaller.Map.TownLayout", GSTests::TestFlags)
bool FGSTownLayoutTest::RunTest(const FString& Parameters)
{
	static constexpr float Half = 15000.f; // 300 x 300 m
	struct FEdgeTarget
	{
		FString Name;
		FVector2D Inside;
		FVector2D Out; // launch direction (towards the wall)
	};
	TSharedRef<TArray<FEdgeTarget>> Targets = MakeShared<TArray<FEdgeTarget>>();
	for (const FEdgeTarget& Target : {
		FEdgeTarget{ TEXT("north edge"), FVector2D(2500.f, 14500.f), FVector2D(0.f, 1.f) },
		FEdgeTarget{ TEXT("south edge"), FVector2D(2500.f, -14500.f), FVector2D(0.f, -1.f) },
		FEdgeTarget{ TEXT("east edge"), FVector2D(14500.f, 2500.f), FVector2D(1.f, 0.f) },
		FEdgeTarget{ TEXT("west edge"), FVector2D(-14500.f, 2500.f), FVector2D(-1.f, 0.f) },
		FEdgeTarget{ TEXT("north-east corner"), FVector2D(14400.f, 14400.f), FVector2D(1.f, 1.f) },
		FEdgeTarget{ TEXT("north-west corner"), FVector2D(-14400.f, 14400.f), FVector2D(-1.f, 1.f) },
		FEdgeTarget{ TEXT("south-east corner"), FVector2D(14400.f, -14400.f), FVector2D(1.f, -1.f) },
		FEdgeTarget{ TEXT("south-west corner"), FVector2D(-14400.f, -14400.f), FVector2D(-1.f, -1.f) } })
	{
		Targets->Add(Target);
	}

	BeginPIE(this);
	Step([this, Targets]()
	{
		UWorld* World = GetPIEWorld();
		// The wall runs below happen in game time: 4x faster.
		SpeedUpWorld(4.f);
		GetGameMode()->SetRoundTimerPaused(true);
		RemoveAllEnemies(World);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		// Boundary 300 x 300 m, ceiling 75 m, centered on the origin.
		const AGSMapBoundary* Boundary = AGSMapBoundary::Find(World);
		if (!TestNotNull(TEXT("Town boundary"), Boundary))
		{
			return;
		}
		TestEqual(TEXT("PlayAreaSize X 30000"), static_cast<float>(Boundary->PlayAreaSize.X), 2.f * Half);
		TestEqual(TEXT("PlayAreaSize Y 30000"), static_cast<float>(Boundary->PlayAreaSize.Y), 2.f * Half);
		TestEqual(TEXT("CeilingHeight 7500"), Boundary->CeilingHeight, 7500.f);
		TestTrue(TEXT("Boundary centered on the origin"), FVector2D(Boundary->GetActorLocation()).IsNearlyZero(1.f));

		// Ground under the whole area, top at Z 0; Kill Z below it.
		for (const FVector2D& P : { FVector2D(0.f, 0.f), FVector2D(14900.f, 14900.f), FVector2D(-14900.f, 14900.f), FVector2D(14900.f, -14900.f),
			FVector2D(-14900.f, -14900.f), FVector2D(14900.f, 0.f), FVector2D(-14900.f, 0.f), FVector2D(0.f, 14900.f), FVector2D(0.f, -14900.f) })
		{
			FVector Hit;
			TestTrue(FString::Printf(TEXT("Ground at (%.0f, %.0f), top at Z 0"), P.X, P.Y), TraceGround(World, P.X, P.Y, Hit) && FMath::IsNearlyEqual(Hit.Z, 0.f, 1.f));
		}
		TestTrue(TEXT("Kill Z below the ground"), World->GetWorldSettings()->KillZ < -1000.f);

		// NavMesh bounds cover the ground.
		bool bNavCovers = false;
		for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It)
		{
			const FBox Box = It->GetComponentsBoundingBox(true);
			bNavCovers |= Box.Min.X <= -Half + 1.f && Box.Min.Y <= -Half + 1.f && Box.Max.X >= Half - 1.f && Box.Max.Y >= Half - 1.f && Box.Min.Z < 0.f;
		}
		TestTrue(TEXT("NavMesh bounds cover the 300 m ground"), bNavCovers);

		// A landmark on each side, tall and far out.
		for (const TPair<const TCHAR*, FVector2D>& Side : { TPair<const TCHAR*, FVector2D>(TEXT("Landmark_North"), FVector2D(0.f, 1.f)),
			TPair<const TCHAR*, FVector2D>(TEXT("Landmark_East"), FVector2D(1.f, 0.f)), TPair<const TCHAR*, FVector2D>(TEXT("Landmark_South"), FVector2D(0.f, -1.f)),
			TPair<const TCHAR*, FVector2D>(TEXT("Landmark_West"), FVector2D(-1.f, 0.f)) })
		{
			const AActor* Landmark = FindTagged(World, Side.Key);
			if (TestNotNull(FString::Printf(TEXT("%s placed"), Side.Key), Landmark))
			{
				const FBox Box = Landmark->GetComponentsBoundingBox(true);
				TestTrue(FString::Printf(TEXT("%s is far out on its side"), Side.Key), FVector2D::DotProduct(FVector2D(Landmark->GetActorLocation()), Side.Value) > 11000.f);
				TestTrue(FString::Printf(TEXT("%s stands at least 15 m tall"), Side.Key), Box.Max.Z >= 1500.f);
			}
		}

		// PlayerStart at the center; EnemySpawn fallback points at the new edges, inside, on the NavMesh.
		const APlayerStart* Start = nullptr;
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			Start = *It;
		}
		TestTrue(TEXT("PlayerStart at the center"), Start && FVector2D(Start->GetActorLocation()).Size() < 100.f);
		TestEqual(TEXT("4 EnemySpawn points"), CountTagged(World, TEXT("EnemySpawn")), 4);
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(TEXT("EnemySpawn")))
			{
				const FVector L = It->GetActorLocation();
				TestTrue(FString::Printf(TEXT("%s near the new edge"), *It->GetActorNameOrLabel()), FMath::Max(FMath::Abs(L.X), FMath::Abs(L.Y)) >= 13000.f);
				TestTrue(FString::Printf(TEXT("%s inside the boundary"), *It->GetActorNameOrLabel()), Boundary->IsInside(L, 60.f));
				TestTrue(FString::Printf(TEXT("%s on the NavMesh"), *It->GetActorNameOrLabel()), IsOnNavMesh(World, L));
			}
		}

		// No NavMesh islands on the ground: every ground NavMesh sample on a 5 m grid has a path from the PlayerStart.
		// The grid also probes the inside of closed buildings and the silo (Recast builds a floor inside hollow meshes).
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		const ANavigationData* NavData = NavSys ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;
		FNavLocation StartNav;
		if (!TestNotNull(TEXT("NavMesh data"), NavData) || !TestTrue(TEXT("PlayerStart on the NavMesh"), Start && NavSys->ProjectPointToNavigation(Start->GetActorLocation(), StartNav, FVector(200.f, 200.f, 300.f))))
		{
			return;
		}
		int32 Samples = 0;
		int32 Elevated = 0;
		TArray<FString> Unreachable;
		for (float X = -14500.f; X <= 14500.f; X += 500.f)
		{
			for (float Y = -14500.f; Y <= 14500.f; Y += 500.f)
			{
				FNavLocation P;
				if (!NavSys->ProjectPointToNavigation(FVector(X, Y, 30.f), P, FVector(60.f, 60.f, 80.f)))
				{
					continue; // inside a building or a crate
				}
				if (P.Location.Z > 60.f)
				{
					++Elevated; // a crate top, not the ground
					continue;
				}
				++Samples;
				const FPathFindingQuery Query(nullptr, *NavData, StartNav.Location, P.Location, NavData->GetDefaultQueryFilter());
				if (!NavSys->TestPathSync(Query))
				{
					Unreachable.Add(FString::Printf(TEXT("(%.0f, %.0f)"), X, Y));
				}
			}
		}
		AddInfo(FString::Printf(TEXT("Ground NavMesh: %d samples, %d unreachable, %d elevated skipped"), Samples, Unreachable.Num(), Elevated));
		TestTrue(TEXT("NavMesh covers most of the 300 m ground"), Samples >= 2800);
		TestEqual(FString::Printf(TEXT("No ground NavMesh islands %s"), *FString::Join(Unreachable, TEXT(" "))), Unreachable.Num(), 0);

		// Every edge and corner can be walked to from the center.
		for (const FEdgeTarget& Target : *Targets)
		{
			FNavLocation P;
			const bool bOnNav = NavSys->ProjectPointToNavigation(FVector(Target.Inside, 30.f), P, FVector(300.f, 300.f, 80.f));
			TestTrue(FString::Printf(TEXT("%s: NavMesh there"), *Target.Name), bOnNav);
			TestTrue(FString::Printf(TEXT("%s: path from the center"), *Target.Name),
				bOnNav && NavSys->TestPathSync(FPathFindingQuery(nullptr, *NavData, StartNav.Location, P.Location, NavData->GetDefaultQueryFilter())));
		}
	});

	// Stand at each edge and corner and run into the wall: the player stays inside, on the ground.
	for (int32 Index = 0; Index < Targets->Num(); ++Index)
	{
		Step([Targets, Index]()
		{
			const FEdgeTarget& Target = (*Targets)[Index];
			GetPlayer()->TeleportTo(FVector(Target.Inside, 120.f), FRotator::ZeroRotator);
			GetPlayer()->LaunchCharacter(FVector(Target.Out.GetSafeNormal() * 2500.f, 200.f), true, true);
		});
		WaitWorldSeconds(this, 1.2);
		Step([this, Targets, Index]()
		{
			const FEdgeTarget& Target = (*Targets)[Index];
			const FVector L = GetPlayer()->GetActorLocation();
			const AGSMapBoundary* Boundary = AGSMapBoundary::Find(GetPIEWorld());
			TestTrue(FString::Printf(TEXT("%s: stopped by the boundary"), *Target.Name), Boundary->IsInside(L));
			TestTrue(FString::Printf(TEXT("%s: still on the ground (Z %.0f)"), *Target.Name, L.Z), L.Z > 0.f && L.Z < 300.f);

			// A spawn ring rolled in the corner stays inside: ring points in the ring, or a fallback; all on the NavMesh.
			if (!Target.Name.Contains(TEXT("corner")))
			{
				return;
			}
			const UGSSpawnSettings& Settings = UGSSpawnLocationFinder::GetSettings(GetPIEWorld());
			FGSSpawnSearchStats Stats;
			const TArray<FGSSpawnPoint> Points = UGSSpawnLocationFinder::FindSpawnLocations(GetPC(), 6, &Stats);
			AddInfo(FString::Printf(TEXT("%s spawn ring: %s"), *Target.Name, *Stats.ToString()));
			TestEqual(FString::Printf(TEXT("%s: 6 spawn points"), *Target.Name), Points.Num(), 6);
			for (int32 P = 0; P < Points.Num(); ++P)
			{
				const FGSSpawnPoint& Point = Points[P];
				const FString Name = FString::Printf(TEXT("%s spawn %d"), *Target.Name, P);
				TestTrue(Name + TEXT(" inside the boundary"), Boundary->IsInside(Point.Location, 60.f));
				TestTrue(Name + TEXT(" on the NavMesh"), IsOnNavMesh(GetPIEWorld(), Point.Location));
				const float Distance = FVector::Dist2D(Point.Location, L);
				TestTrue(Name + TEXT(" in the ring (or a fallback)"), Point.bFallback || (Distance >= Settings.MinRadius - 1.f && Distance <= Settings.MaxRadius + 1.f));
			}
		});
	}
	Step([]()
	{
		GetGameMode()->SetRoundTimerPaused(false);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Game over: death ends the run (GameOver, wave stopped), the run summary, Return to Market -> the hub
// ---------------------------------------------------------------------------

namespace GSTests
{
	/** "$1,234" as the summary formats cash. */
	FString FormatSummaryCash(int32 Amount)
	{
		return FString::Printf(TEXT("$%s"), *FText::AsNumber(Amount).ToString());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSRunGameOverTest, "BanditBaller.Run.GameOver", GSTests::TestFlags)
bool FGSRunGameOverTest::RunTest(const FString& Parameters)
{
	struct FGameOverState
	{
		int32 Cash = 0;
		float RunTime = 0.f;
		int32 LivingEnemies = 0;
		bool bOldThrottle = true;
		bool bSawFadeOut = false;
	};
	TSharedRef<FGameOverState> S = MakeShared<FGameOverState>();
	TSharedRef<TWeakObjectPtr<UWorld>> OldWorld = MakeShared<TWeakObjectPtr<UWorld>>();

	BeginPIE(this);
	Step([this, S]()
	{
		// Real frame rate in the background, so the fade-out is seen before the hub opens.
		UEditorPerformanceSettings* Perf = GetMutableDefault<UEditorPerformanceSettings>();
		S->bOldThrottle = Perf->bThrottleCPUWhenNotForeground;
		Perf->bThrottleCPUWhenNotForeground = false;

		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		UGSRunStats* Stats = GameState ? GameState->GetRunStats() : nullptr;
		UGSProfileSubsystem* Profile = GetProfile();
		AGSPlayerController* PC = GetPC();
		AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
		if (!TestNotNull(TEXT("AGSGameMode"), GameMode) || !TestNotNull(TEXT("Run stats"), Stats) || !TestNotNull(TEXT("Profile"), Profile)
			|| !TestNotNull(TEXT("PlayerState"), PS))
		{
			return;
		}
		GameMode->SetRoundTimerPaused(true);
		Profile->ResetProfile();

		// Alive: there is nothing to return from.
		TestFalse(TEXT("Return refused while alive"), GameMode->RequestReturnToHub(PC));
		TestFalse(TEXT("Not returning while alive"), GameMode->IsReturningToHub());
		TestNull(TEXT("No run summary while alive"), PC->GetRunSummaryWidget());

		// Run time formatting.
		TestEqual(TEXT("Run time 0"), UGSRunSummaryWidget::FormatRunTime(0.f).ToString(), FString(TEXT("0:00")));
		TestEqual(TEXT("Run time 65 s"), UGSRunSummaryWidget::FormatRunTime(65.4f).ToString(), FString(TEXT("1:05")));
		TestEqual(TEXT("Run time over an hour"), UGSRunSummaryWidget::FormatRunTime(3725.f).ToString(), FString(TEXT("1:02:05")));

		// Something to summarize: a level-up (its offer picked) and three upgrade stacks by hand, in PreRound.
		GameMode->SetRound(1);
		UGSProgressionComponent* Progression = PS->GetProgressionComponent();
		GameMode->AwardXP(PS, static_cast<float>(Progression->GetXPForNextLevel()), false);
		TestEqual(TEXT("Level 2"), Progression->GetLevel(), 2);
		TestEqual(TEXT("Its offer picked"), PickAllUpgradeOffers(), 1);
		UGSDebugLibrary::ExecuteDebugCommand(PC, TEXT("UpgradeGrant"), { TEXT("Damage"), TEXT("2") });
		UGSDebugLibrary::ExecuteDebugCommand(PC, TEXT("UpgradeGrant"), { TEXT("ExtraJump") });
		TestEqual(TEXT("4 upgrade picks"), PS->GetUpgradeComponent()->GetTotalPicks(), 4);

		// Round 1: two kills (combo 2) and a pickup, then cleared with its bonus.
		GameMode->AdvancePhase(); // -> Combat
		KillTestEnemyAsPlayer(this);
		KillTestEnemyAsPlayer(this);
		GameMode->AwardCash(PC, 25, PC->GetPawn()->GetActorLocation());
		GameMode->AdvancePhase(); // -> RoundClear: round 1 cleared, bonus paid
		TestEqual(TEXT("Round 1 bonus paid"), Stats->GetLastRoundClearPayout().Round, 1);
		GameMode->AdvancePhase(); // -> round 2 PreRound
		GameMode->AdvancePhase(); // -> round 2 Combat: its wave starts
		TestTrue(TEXT("Round 2 wave running"), GameMode->GetWaveDirector()->IsWaveActive());
	});
	// Let some of round 2's wave spawn (the rest is still to come).
	WaitUntil(this, TEXT("round 2's first enemies"), []() { return GetGameMode() && GetGameMode()->GetWaveDirector()->GetAlive() > 0; }, 10.0);

	// Die: the run is over.
	Step([this, S, OldWorld]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		UGSRunStats* Stats = GameState ? GameState->GetRunStats() : nullptr;
		UGSProfileSubsystem* Profile = GetProfile();
		UGSWaveDirector* Director = GameMode ? GameMode->GetWaveDirector() : nullptr;
		if (!TestNotNull(TEXT("Wave director"), Director) || !Stats || !Profile)
		{
			return;
		}
		TestTrue(TEXT("Enemies still to spawn before the death"), Director->GetRemaining() > Director->GetAlive());

		// An upgrade offer open at the moment of death (it pauses the game) is closed by the game over.
		AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		UGSUpgradeComponent* Upgrades = PS->GetUpgradeComponent();
		GameMode->QueueUpgradeOfferForDebug(PS);
		TestTrue(TEXT("Offer open before the death"), Upgrades->HasOpenOffer());
		TestTrue(TEXT("Paused for that offer"), GameMode->IsPausedForUpgradeChoice());

		S->LivingEnemies = CountLivingEnemies();
		*OldWorld = GetPIEWorld();
		ApplyDamage(GetPlayerASC(), 100000.f);
		TestTrue(TEXT("Player died"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		TestFalse(TEXT("Death closes the open offer"), Upgrades->HasOpenOffer());
		TestFalse(TEXT("Death lifts the offer's pause"), GameMode->IsPausedForUpgradeChoice() || GetPIEWorld()->IsPaused());

		// A level-up after the death (debug XP) still applies its reward but offers nothing.
		UGSProgressionComponent* Progression = PS->GetProgressionComponent();
		GameMode->AwardXP(PS, static_cast<float>(Progression->GetXPForNextLevel()), false);
		TestEqual(TEXT("Level 3 after the death"), Progression->GetLevel(), 3);
		TestFalse(TEXT("No offer after the death"), Upgrades->HasOpenOffer() || Upgrades->GetQueuedOffers() > 0);
		TestFalse(TEXT("No pause after the death"), GetPIEWorld()->IsPaused());

		// GameOver: the wave stops (nothing telegraphing or left to spawn), the living enemies stay, no phase timer.
		TestTrue(TEXT("Death -> GameOver"), GameState->GetRoundPhase() == EGSRoundPhase::GameOver);
		TestEqual(TEXT("Still round 2"), GameState->GetRoundNumber(), 2);
		TestFalse(TEXT("Wave stopped"), Director->IsWaveActive());
		TestEqual(TEXT("Telegraphs cancelled"), Director->GetPending(), 0);
		TestEqual(TEXT("Nothing left to spawn"), Director->GetRemaining(), 0);
		TestEqual(TEXT("Living enemies stay"), CountLivingEnemies(), S->LivingEnemies);
		TestTrue(TEXT("GameOver is untimed"), GameState->GetPhaseEndTime() <= 0.0);
		GameMode->AdvancePhase();
		TestTrue(TEXT("GameOver is final (a skip does nothing)"), GameState->GetRoundPhase() == EGSRoundPhase::GameOver);

		// Run stats end and the profile is saved at death.
		S->RunTime = Stats->GetRunTime();
		S->Cash = Profile->GetCash();
		TestTrue(TEXT("Run time recorded"), S->RunTime > 0.f);
		TestEqual(TEXT("Profile saved at death"), GetSavedCash(*Profile), S->Cash);
		TestEqual(TEXT("1 round cleared"), Stats->GetRoundsCleared(), 1);
		TestEqual(TEXT("Best combo 2"), Stats->GetBestCombo(), 2);
		TestEqual(TEXT("2 kills"), Stats->GetTotalKills(), 2);

		// The run summary replaces the death screen: UI input, cursor, locked at first (R is also reload).
		UGSRunSummaryWidget* Summary = GetRunSummary();
		if (!TestNotNull(TEXT("Run summary shown at death"), Summary))
		{
			return;
		}
		TestTrue(TEXT("Run summary in the viewport"), Summary->IsInViewport());
		TestTrue(TEXT("UI-only input"), IsViewportIgnoringInput());
		TestTrue(TEXT("Cursor shown"), GetPC()->ShouldShowMouseCursor());
		TestTrue(TEXT("Input locked at first"), Summary->IsInputLocked());
		TestTrue(TEXT("R consumed while locked"), Summary->HandleReturnKey(EKeys::R));
		TestFalse(TEXT("R while locked does nothing"), Summary->IsReturnRequested());
		TestFalse(TEXT("Button path refused while locked"), Summary->TryReturnToHub());
		TestFalse(TEXT("Not returning yet"), GameMode->IsReturningToHub());
	});

	// Nothing more happens after the death: no spawns, no RoundClear or bonus, frozen run time, same cash.
	WaitWorldSeconds(this, 3.0);
	Step([this, S]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSGameState* GameState = GetGameState();
		UGSRunStats* Stats = GameState ? GameState->GetRunStats() : nullptr;
		UGSProfileSubsystem* Profile = GetProfile();
		if (!GameMode || !Stats || !Profile)
		{
			AddError(TEXT("Lost the run after the death"));
			return;
		}
		UGSWaveDirector* Director = GameMode->GetWaveDirector();
		TestTrue(TEXT("Still GameOver"), GameState->GetRoundPhase() == EGSRoundPhase::GameOver);
		TestEqual(TEXT("No spawns after death"), Director->GetSpawned(), 0);
		TestEqual(TEXT("No telegraphs after death"), Director->GetPending(), 0);
		TestEqual(TEXT("No new enemies"), CountLivingEnemies(), S->LivingEnemies);
		TestEqual(TEXT("No RoundClear after death"), Stats->GetLastRoundClearPayout().Round, 1);
		TestEqual(TEXT("Rounds cleared unchanged"), Stats->GetRoundsCleared(), 1);
		TestEqual(TEXT("Run time frozen"), Stats->GetRunTime(), S->RunTime, 0.001f);
		TestEqual(TEXT("No cash after death"), Profile->GetCash(), S->Cash);

		// Killing what's left doesn't clear the round either.
		for (TActorIterator<AGSEnemyCharacter> It(GetPIEWorld()); It; ++It)
		{
			UAbilitySystemComponent* EnemyASC = It->GetAbilitySystemComponent();
			if (EnemyASC && !It->GetHealthComponent()->IsDead())
			{
				ApplyDamage(EnemyASC, GetMaxHealth(EnemyASC) + 100.f);
			}
		}
		TestEqual(TEXT("All enemies dead"), CountLivingEnemies(), 0);
		TestTrue(TEXT("Killing the leftovers: still GameOver"), GameState->GetRoundPhase() == EGSRoundPhase::GameOver);
		TestEqual(TEXT("Killing the leftovers: no bonus"), Stats->GetLastRoundClearPayout().Round, 1);
		TestEqual(TEXT("Killing the leftovers: no cash"), Profile->GetCash(), S->Cash);

		// The summary's lines.
		UGSRunSummaryWidget* Summary = GetRunSummary();
		if (!TestNotNull(TEXT("Run summary still shown"), Summary))
		{
			return;
		}
		TestEqual(TEXT("Rounds cleared line"), Summary->GetRoundsClearedText().ToString(), FString(TEXT("1")));
		TestEqual(TEXT("Level reached (3, with the level-up after the death)"), Stats->GetLevelReached(), 3);
		TestEqual(TEXT("Level reached line follows the stats"), Summary->GetLevelReachedText().ToString(), FString(TEXT("3")));
		TestEqual(TEXT("Kills line"), Summary->GetKillsText().ToString(), FString::FromInt(Stats->GetTotalKills()));
		TestEqual(TEXT("Best combo line"), Summary->GetBestComboText().ToString(), FString(TEXT("2")));
		TestTrue(TEXT("Cash earned has the pickup and the round bonus"), Stats->GetCashEarned() >= 25 + Stats->GetLastRoundClearPayout().GetTotal());
		TestEqual(TEXT("Cash earned line"), Summary->GetCashEarnedText().ToString(), FormatSummaryCash(Stats->GetCashEarned()));
		TestEqual(TEXT("Total cash line"), Summary->GetTotalCashText().ToString(), FormatSummaryCash(Profile->GetCash()));
		TestEqual(TEXT("Run time line"), Summary->GetRunTimeText().ToString(), UGSRunSummaryWidget::FormatRunTime(S->RunTime).ToString());
		TestTrue(TEXT("A run seed"), GameState->GetRunSeed() > 0);
		TestEqual(TEXT("Seed line (no digit grouping)"), Summary->GetSeedText().ToString(), FString::FromInt(GameState->GetRunSeed()));

		// Upgrades: one icon per upgrade in pick order with its stacks, and the same as text. Weapons are listed apart.
		const UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		const TArray<FGSUpgradeStack> Stacks = Upgrades->GetUpgradeStacks().FilterByPredicate([](const FGSUpgradeStack& S)
		{
			return S.Upgrade->Category != EGSUpgradeCategory::Weapon;
		});
		TArray<FString> Expected;
		for (const FGSUpgradeStack& Stack : Stacks)
		{
			Expected.Add(FString::Printf(TEXT("%s x%d"), *Stack.Upgrade->DisplayName.ToString(), Stack.Stacks));
		}
		TArray<FString> ExpectedWeapons;
		for (const UGSUpgradeData* Weapon : Upgrades->GetOwnedWeapons())
		{
			ExpectedWeapons.Add(Weapon->DisplayName.ToString());
		}
		TestEqual(TEXT("Weapons line"), Summary->GetWeaponsText().ToString(), ExpectedWeapons.IsEmpty() ? FString(TEXT("None")) : FString::Join(ExpectedWeapons, TEXT(", ")));
		TestEqual(TEXT("Upgrades line"), Summary->GetUpgradesText().ToString(), FString::Join(Expected, TEXT(", ")));
		TestTrue(TEXT("Upgrades line has Damage"), Summary->GetUpgradesText().ToString().Contains(TEXT("Damage x")));
		TestTrue(TEXT("Upgrades line has Extra Jump"), Summary->GetUpgradesText().ToString().Contains(TEXT("Extra Jump x")));
		if (TestEqual(TEXT("One icon per upgrade"), Summary->GetUpgradeIconCount(), Stacks.Num()))
		{
			for (int32 Index = 0; Index < Stacks.Num(); ++Index)
			{
				const UGSUpgradeIconWidget* Icon = Summary->GetUpgradeIcon(Index);
				TestTrue(FString::Printf(TEXT("Icon %d: upgrade and stacks"), Index),
					Icon && Icon->GetUpgrade() == Stacks[Index].Upgrade && Icon->GetStacks() == Stacks[Index].Stacks);
			}
		}

		// Input: unlocked now; a held key's repeats and other keys don't return.
		TestFalse(TEXT("Input unlocked"), Summary->IsInputLocked());
		TestTrue(TEXT("Held R (repeat) consumed"), Summary->HandleReturnKey(EKeys::R, /*bIsRepeat*/ true));
		TestFalse(TEXT("Held R (repeat) does nothing"), Summary->IsReturnRequested());
		TestFalse(TEXT("Space isn't a return key"), Summary->HandleReturnKey(EKeys::SpaceBar));

		// The "Return to Market" button: fade out, then the hub.
		UButton* Button = Cast<UButton>(Summary->GetWidgetFromName(TEXT("ReturnButton")));
		if (!TestNotNull(TEXT("Return to Market button"), Button))
		{
			return;
		}
		Button->OnClicked.Broadcast();
		TestTrue(TEXT("Button asks to return"), Summary->IsReturnRequested());
		TestTrue(TEXT("The GameMode is returning to the hub"), GameMode->IsReturningToHub());
		TestFalse(TEXT("A second press does nothing"), Summary->TryReturnToHub());
		TestFalse(TEXT("A second request is refused"), GameMode->RequestReturnToHub(GetPC()));
		const APlayerCameraManager* Camera = GetPC()->PlayerCameraManager;
		TestTrue(TEXT("The camera fades to black"), Camera && Camera->bEnableFading && Camera->FadeColor == FLinearColor::Black && Camera->FadeAlpha.Y == 1.f);
	});
	WaitUntil(this, TEXT("run summary to fade out (or the hub to open)"), [S, OldWorld]()
	{
		if (GetPIEWorld() != OldWorld->Get())
		{
			return true;
		}
		const UGSRunSummaryWidget* Summary = GetRunSummary();
		S->bSawFadeOut = Summary && Summary->GetScreenOpacity() < 0.99f;
		return S->bSawFadeOut;
	}, 5.0);
	Step([this, S]() { TestTrue(TEXT("The summary fades out before the hub opens"), S->bSawFadeOut); });
	WaitForMapTravel(this, OldWorld, HubMap);

	// The hub: run state is gone (a new world), the cash is kept, game input is back.
	Step([this, S]()
	{
		GetMutableDefault<UEditorPerformanceSettings>()->bThrottleCPUWhenNotForeground = S->bOldThrottle;
		UGSProfileSubsystem* Profile = GetProfile();
		TestNotNull(TEXT("Return to Market opens the hub"), GetHubGameMode());
		TestNull(TEXT("No run in the hub"), GetGameMode());
		if (Profile)
		{
			TestEqual(TEXT("Cash kept in the hub"), Profile->GetCash(), S->Cash);
		}
		TestNull(TEXT("No run summary in the hub"), GetRunSummary());
		TestFalse(TEXT("Game input in the hub"), IsViewportIgnoringInput());
		TestFalse(TEXT("No cursor in the hub"), GetPC()->ShouldShowMouseCursor());
		const APlayerCameraManager* Camera = GetPC()->PlayerCameraManager;
		TestTrue(TEXT("The hub isn't faded"), Camera && Camera->FadeAmount < 0.01f);
		const UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		TestEqual(TEXT("No upgrades in the hub (run state rebuilt)"), Upgrades ? Upgrades->GetTotalPicks() : -1, 0);
		if (Profile)
		{
			Profile->ResetProfile(); // leave the test slot empty
		}
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Same seed, same run: waves, spawn positions (player still), upgrade offers and rerolls
// ---------------------------------------------------------------------------

namespace GSTests
{
	FString DescribeChoices(const UGSUpgradeComponent* Upgrades)
	{
		return FString::JoinBy(Upgrades->GetCurrentChoices(), TEXT("/"), [](const UGSUpgradeData* Upgrade) { return Upgrade->GetName(); });
	}

	/**
	 * Replay the start of a run with Seed, all in one frame so nothing else (enemy AI, drops) draws from the run stream
	 * in between. Rounds 1, 2, 5 and 8 (8 is bought from the budget): every enemy of the wave in spawn order with its
	 * spawn point (all batches run at once). Then a level-up offer, its 3 rerolls, the refused 4th, a pick and the next
	 * offer. Returns one line per item.
	 */
	TArray<FString> ReplayRunStart(int32 Seed)
	{
		TArray<FString> Lines;
		AGSGameMode* GameMode = GetGameMode();
		UGSWaveDirector* Director = GameMode->GetWaveDirector();
		AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		const UGSUpgradeComponent* Upgrades = PS->GetUpgradeComponent();

		GameMode->RestartRunWithSeed(Seed);
		for (const int32 Round : { 1, 2, 5, 8 })
		{
			GameMode->SetRound(Round);
			GameMode->AdvancePhase(); // Combat: the wave is bought and shuffled, the first batch telegraphs
			for (int32 Guard = 0; Guard < 100 && Director->GetSpawnQueue().Num() > 0; ++Guard)
			{
				Director->SpawnNextBatchNow();
			}
			Lines.Add(FString::Printf(TEXT("Round %d: %d enemies, %d unplaced"), Round, Director->GetTotal(), Director->GetSpawnQueue().Num()));
			for (const FGSPendingSpawn& Spawn : Director->GetPendingSpawns())
			{
				Lines.Add(FString::Printf(TEXT("  %s at (%.1f, %.1f)"), *GetNameSafe(Spawn.EnemyData), Spawn.Location.X, Spawn.Location.Y));
			}
		}
		GameMode->SetRound(1); // PreRound: the last wave stops

		GameMode->AwardXP(PS, static_cast<float>(PS->GetProgressionComponent()->GetXPForNextLevel()), false);
		Lines.Add(TEXT("Offer: ") + DescribeChoices(Upgrades));
		for (int32 Reroll = 1; Reroll <= 4; ++Reroll)
		{
			const bool bRerolled = GameMode->RerollUpgradeOffer(PS);
			Lines.Add(FString::Printf(TEXT("Reroll %d %s: %s"), Reroll, bRerolled ? TEXT("ok") : TEXT("refused"), *DescribeChoices(Upgrades)));
		}
		GameMode->ChooseUpgrade(PS, 0);
		GameMode->AwardXP(PS, static_cast<float>(PS->GetProgressionComponent()->GetXPForNextLevel()), false);
		Lines.Add(TEXT("Next offer: ") + DescribeChoices(Upgrades));

		GameMode->RestartRunWithSeed(Seed); // drops the open offer (unpauses), back to round 1
		return Lines;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSRunSameSeedTest, "BanditBaller.Run.SameSeedSameRun", GSTests::TestFlags)
bool FGSRunSameSeedTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		if (!TestNotNull(TEXT("AGSGameMode"), GameMode) || !TestNotNull(TEXT("Player"), GetPlayer()))
		{
			return;
		}
		GameMode->SetRoundTimerPaused(true);
		const FVector Start = GetPlayer()->GetActorLocation();

		const TArray<FString> A = ReplayRunStart(31337);
		const TArray<FString> B = ReplayRunStart(31337);
		const TArray<FString> C = ReplayRunStart(31338);
		AddInfo(TEXT("Seed 31337:\n") + FString::Join(A, TEXT("\n")));
		TestTrue(TEXT("The player stood still"), GetPlayer()->GetActorLocation().Equals(Start, 1.f));

		// Sanity: four waves fully placed, 3 rerolls then a refused 4th.
		TestEqual(TEXT("Lines recorded"), A.FilterByPredicate([](const FString& Line) { return Line.StartsWith(TEXT("Round ")); }).Num(), 4);
		TestTrue(TEXT("Every enemy got a spawn point"), A.FilterByPredicate([](const FString& Line) { return Line.StartsWith(TEXT("Round ")) && !Line.EndsWith(TEXT(" 0 unplaced")); }).IsEmpty());
		TestTrue(TEXT("Spawn points recorded"), A.FilterByPredicate([](const FString& Line) { return Line.StartsWith(TEXT("  ")); }).Num() >= 30);
		TestEqual(TEXT("Rerolls 1-3 ok"), A.FilterByPredicate([](const FString& Line) { return Line.StartsWith(TEXT("Reroll ")) && Line.Contains(TEXT(" ok: ")); }).Num(), 3);
		TestTrue(TEXT("Reroll 4 refused"), A.ContainsByPredicate([](const FString& Line) { return Line.StartsWith(TEXT("Reroll 4 refused")); }));

		// Same seed: the same run, line for line.
		int32 FirstDifference = INDEX_NONE;
		for (int32 Index = 0; Index < FMath::Max(A.Num(), B.Num()); ++Index)
		{
			if (!A.IsValidIndex(Index) || !B.IsValidIndex(Index) || A[Index] != B[Index])
			{
				FirstDifference = Index;
				break;
			}
		}
		TestEqual(FString::Printf(TEXT("Same seed: same waves, spawn points, offers and rerolls (first difference at line %d: '%s' vs '%s')"), FirstDifference,
			A.IsValidIndex(FirstDifference) ? *A[FirstDifference] : TEXT("-"), B.IsValidIndex(FirstDifference) ? *B[FirstDifference] : TEXT("-")), FirstDifference, static_cast<int32>(INDEX_NONE));
		TestFalse(TEXT("Another seed: another run"), A == C);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// A new run (death -> hub -> Start Run) starts at level 1 with no upgrades but keeps the cash
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSRunNewRunResetsTest, "BanditBaller.Run.NewRunResets", GSTests::TestFlags)
bool FGSRunNewRunResetsTest::RunTest(const FString& Parameters)
{
	struct FNewRunState
	{
		int32 Cash = 0;
		float BaseMaxHealth = 0.f;
		float BaseDamage = 0.f;
		float BaseMoveSpeed = 0.f;
		int32 RerollsPerRun = 0;
	};
	TSharedRef<FNewRunState> S = MakeShared<FNewRunState>();
	TSharedRef<TWeakObjectPtr<UWorld>> OldWorld = MakeShared<TWeakObjectPtr<UWorld>>();

	BeginPIE(this);
	Step([this, S, OldWorld]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSPlayerController* PC = GetPC();
		AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
		UGSProfileSubsystem* Profile = GetProfile();
		UAbilitySystemComponent* ASC = GetPlayerASC();
		if (!TestNotNull(TEXT("AGSGameMode"), GameMode) || !TestNotNull(TEXT("PlayerState"), PS) || !TestNotNull(TEXT("Profile"), Profile) || !TestNotNull(TEXT("ASC"), ASC))
		{
			return;
		}
		GameMode->SetRoundTimerPaused(true);
		Profile->ResetProfile();
		S->BaseMaxHealth = GetMaxHealth(ASC);
		S->BaseDamage = ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute());
		S->BaseMoveSpeed = ASC->GetNumericAttribute(UGSAttributeSet::GetMoveSpeedAttribute());
		S->RerollsPerRun = GameMode->GetUpgradePool() ? GameMode->GetUpgradePool()->RerollsPerRun : 0;

		// A run with progress: two level-ups (a reroll, both offers picked), more upgrade stacks, a kill and cash.
		UGSUpgradeComponent* Upgrades = PS->GetUpgradeComponent();
		GameMode->AwardXP(PS, static_cast<float>(GameMode->GetXPCurve()->GetTotalXPToReachLevel(3)), false);
		TestEqual(TEXT("Level 3"), PS->GetProgressionComponent()->GetLevel(), 3);
		TestTrue(TEXT("A reroll used"), GameMode->RerollUpgradeOffer(PS));
		// Pick a card that can't stop the killing blow below (a Floating Shield or Second Wind would, as designed).
		int32 Picked = 0;
		while (Upgrades->HasOpenOffer() && Picked < 10)
		{
			const TArray<FGSUpgradeChoice>& Offer = Upgrades->GetCurrentOffer();
			const int32 Safe = Offer.IndexOfByPredicate([](const FGSUpgradeChoice& C)
			{
				return !C.Upgrade->Effect->IsChildOf(UGSUpgradeEffect_FloatingShield::StaticClass())
					&& !C.Upgrade->Effect->IsChildOf(UGSUpgradeEffect_SecondWind::StaticClass());
			});
			GameMode->ChooseUpgrade(PS, FMath::Max(Safe, 0));
			++Picked;
		}
		TestEqual(TEXT("Both offers picked"), Picked, 2);
		UGSDebugLibrary::ExecuteDebugCommand(PC, TEXT("UpgradeGrant"), { TEXT("ExtraJump"), TEXT("2") });
		UGSDebugLibrary::ExecuteDebugCommand(PC, TEXT("UpgradeGrant"), { TEXT("Damage"), TEXT("2") });
		GameMode->AdvancePhase(); // Combat
		KillTestEnemyAsPlayer(this);
		GameMode->AwardCash(PC, 77, PC->GetPawn()->GetActorLocation());
		TestEqual(TEXT("6 upgrade picks"), Upgrades->GetTotalPicks(), 6);
		TestEqual(TEXT("A reroll spent"), Upgrades->GetRerollsLeft(), S->RerollsPerRun - 1);
		TestTrue(TEXT("Air jumps"), GetPlayer()->JumpMaxCount >= 3);
		TestEqual(TEXT("MaxHealth stacks"), GetMaxHealth(ASC), S->BaseMaxHealth + 20.f, 0.01f);
		TestEqual(TEXT("A kill"), GetGameState()->GetRunStats()->GetTotalKills(), 1);
		S->Cash = Profile->GetCash();
		TestEqual(TEXT("Cash earned"), S->Cash, 77);

		// The run ends.
		*OldWorld = GetPIEWorld();
		ApplyDamage(ASC, 100000.f);
		TestTrue(TEXT("Run over"), IsPhase(EGSRoundPhase::GameOver));
	});
	Step([]() { GetPC()->ServerRequestReturnToHub(); });
	WaitForMapTravel(this, OldWorld, HubMap);
	Step([this, S, OldWorld]()
	{
		AGSHubGameMode* Hub = GetHubGameMode();
		if (!TestNotNull(TEXT("Back in the hub"), Hub))
		{
			return;
		}
		TestEqual(TEXT("Cash in the hub"), GetProfile()->GetCash(), S->Cash);
		*OldWorld = GetPIEWorld();
		TestTrue(TEXT("Start a run from the hub (as the portal does)"), Hub->StartRun(GetPC()));
	});
	WaitForMapTravel(this, OldWorld, TownMap);
	Step([this, S]()
	{
		AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		UAbilitySystemComponent* ASC = GetPlayerASC();
		const AGSGameState* GameState = GetGameState();
		const UGSRunStats* Stats = GameState ? GameState->GetRunStats() : nullptr;
		if (!TestNotNull(TEXT("A run again"), GetGameMode()) || !TestNotNull(TEXT("Run stats"), Stats) || !PS || !ASC)
		{
			return;
		}
		const UGSProgressionComponent* Progression = PS->GetProgressionComponent();
		const UGSUpgradeComponent* Upgrades = PS->GetUpgradeComponent();
		TestEqual(TEXT("New run: level 1"), Progression->GetLevel(), 1);
		TestEqual(TEXT("New run: no XP"), Progression->GetTotalXP(), 0);
		TestEqual(TEXT("New run: no upgrades"), Upgrades->GetTotalPicks(), 0);
		TestFalse(TEXT("New run: no offer"), Upgrades->HasOpenOffer() || Upgrades->GetQueuedOffers() > 0);
		TestEqual(TEXT("New run: rerolls restored"), Upgrades->GetRerollsLeft(), S->RerollsPerRun);
		TestEqual(TEXT("New run: ExtraJumps 0"), ASC->GetNumericAttribute(UGSAttributeSet::GetExtraJumpsAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("New run: JumpMaxCount 1"), GetPlayer()->JumpMaxCount, 1);
		TestEqual(TEXT("New run: base Damage"), ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute()), S->BaseDamage, 0.001f);
		TestEqual(TEXT("New run: base MoveSpeed"), ASC->GetNumericAttribute(UGSAttributeSet::GetMoveSpeedAttribute()), S->BaseMoveSpeed, 0.01f);
		TestEqual(TEXT("New run: base MaxHealth"), GetMaxHealth(ASC), S->BaseMaxHealth, 0.01f);
		TestEqual(TEXT("New run: full health"), GetHealth(ASC), S->BaseMaxHealth, 0.01f);
		TestFalse(TEXT("New run: alive"), ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		TestEqual(TEXT("New run: round 1"), GameState->GetRoundNumber(), 1);
		TestEqual(TEXT("New run: no kills"), Stats->GetTotalKills(), 0);
		TestEqual(TEXT("New run: no cash earned"), Stats->GetCashEarned(), 0);
		TestEqual(TEXT("New run: best combo 0"), Stats->GetBestCombo(), 0);
		TestEqual(TEXT("New run: combo 0"), PS->GetComboComponent()->GetComboCount(), 0);
		TestEqual(TEXT("New run keeps the cash"), GetProfile()->GetCash(), S->Cash);
		if (const UGSHUDWidget* HUD = GetHUD())
		{
			TestEqual(TEXT("New run: HUD level 1"), HUD->GetDisplayedLevel(), 1);
			TestEqual(TEXT("New run: no upgrade icons"), HUD->GetUpgradeIconCount(), 0);
			TestEqual(TEXT("New run: HUD cash"), HUD->GetDisplayedCash(), S->Cash);
		}
		GetProfile()->ResetProfile(); // leave the test slot empty
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Flat drops: an enemy type drops the same cash and XP in round 1 and round 10
// ---------------------------------------------------------------------------

namespace GSTests
{
	struct FDropRecord
	{
		int32 Cash = 0;
		int32 CashPieces = 0;
		int32 XP = 0;
		int32 XPOrbs = 0;
		float MaxHealth = 0.f;
	};

	/** Spawn a frozen enemy of Type 8 m from the player, end the combo, kill it as the player and add up its drop. */
	FDropRecord KillAndMeasureDrop(FAutomationTestBase* Test, const TCHAR* Type)
	{
		FDropRecord Out;
		UGSCashPickupSubsystem* Pickups = GetCashSubsystem();
		Pickups->ReleaseAll();
		UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(Type);
		const FVector PlayerAt = GetPlayer()->GetActorLocation();
		AGSEnemyCharacter* Enemy = Data ? AGSEnemyCharacter::SpawnEnemy(GetPIEWorld(), Data, PlayerAt + GetPlayer()->GetActorForwardVector() * 800.f, PlayerAt) : nullptr;
		if (!Test->TestNotNull(FString::Printf(TEXT("Spawned a %s"), Type), Enemy))
		{
			return Out;
		}
		if (AAIController* AIC = Cast<AAIController>(Enemy->GetController()))
		{
			if (UBrainComponent* Brain = AIC->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("Drop test"));
			}
		}
		Out.MaxHealth = GetMaxHealth(Enemy->GetAbilitySystemComponent());
		if (UGSComboComponent* Combo = GetPlayerCombo())
		{
			Combo->EndCombo(EGSComboEndReason::Reset); // no combo multiplier on this kill
		}
		ApplyDamage(Enemy->GetAbilitySystemComponent(), 1000000.f, GetPlayerASC());
		for (const AGSCashPickup* Pickup : Pickups->GetActivePickups())
		{
			if (Pickup && Pickup->IsXPOrb())
			{
				Out.XP += Pickup->GetValue();
				++Out.XPOrbs;
			}
			else if (Pickup && Pickup->IsCash()) // a health orb (a chance per kill) isn't part of the flat drop
			{
				Out.Cash += Pickup->GetValue();
				++Out.CashPieces;
			}
		}
		Pickups->ReleaseAll();
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSFlatDropsTest, "BanditBaller.Economy.FlatDropsPerRound", GSTests::TestFlags)
bool FGSFlatDropsTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		if (!TestNotNull(TEXT("AGSGameMode"), GameMode))
		{
			return;
		}
		GameMode->SetRoundTimerPaused(true);
		for (const TCHAR* Type : { TEXT("MeleeBandit"), TEXT("RangedBandit") })
		{
			const UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(Type);
			if (!TestNotNull(FString::Printf(TEXT("%s data"), Type), Data))
			{
				continue;
			}
			GameMode->SetRound(1);
			const FDropRecord Round1 = KillAndMeasureDrop(this, Type);
			GameMode->SetRound(10);
			const FDropRecord Round10 = KillAndMeasureDrop(this, Type);
			AddInfo(FString::Printf(TEXT("%s: round 1 HP %.0f, $%d in %d pieces, %d XP | round 10 HP %.0f, $%d in %d pieces, %d XP"), Type,
				Round1.MaxHealth, Round1.Cash, Round1.CashPieces, Round1.XP, Round10.MaxHealth, Round10.Cash, Round10.CashPieces, Round10.XP));

			TestTrue(FString::Printf(TEXT("%s: round 10 is tougher (round scaling applies)"), Type), Round10.MaxHealth > Round1.MaxHealth);
			TestEqual(FString::Printf(TEXT("%s: round 1 drops CashValue"), Type), Round1.Cash, Data->CashValue);
			TestEqual(FString::Printf(TEXT("%s: round 10 drops the same cash"), Type), Round10.Cash, Round1.Cash);
			TestEqual(FString::Printf(TEXT("%s: same cash pieces"), Type), Round10.CashPieces, Round1.CashPieces);
			TestEqual(FString::Printf(TEXT("%s: one XP orb in round 1"), Type), Round1.XPOrbs, 1);
			TestEqual(FString::Printf(TEXT("%s: one XP orb in round 10"), Type), Round10.XPOrbs, 1);
			TestEqual(FString::Printf(TEXT("%s: round 1 orb is XPValue"), Type), Round1.XP, Data->XPValue);
			TestEqual(FString::Printf(TEXT("%s: round 10 orb is the same XP"), Type), Round10.XP, Round1.XP);
		}
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Health orbs (Prompt 35b)
// ---------------------------------------------------------------------------

namespace GSTests
{
	AGSCashPickup* FindActiveHealthOrb()
	{
		for (AGSCashPickup* Pickup : GetCashSubsystem()->GetActivePickups())
		{
			if (Pickup && Pickup->IsHealthOrb())
			{
				return Pickup;
			}
		}
		return nullptr;
	}

	int32 CountMagnetizedHealthOrbs()
	{
		int32 Count = 0;
		for (const AGSCashPickup* Pickup : GetCashSubsystem()->GetActivePickups())
		{
			Count += (Pickup && Pickup->IsHealthOrb() && Pickup->IsMagnetized()) ? 1 : 0;
		}
		return Count;
	}

	/** A frozen MeleeBandit at Location (no AI, no movement), for blast and kill-credit checks. */
	AGSEnemyCharacter* SpawnFrozenMeleeAt(FAutomationTestBase* Test, const FVector& Location)
	{
		UGSEnemyData* Data = UGSDebugLibrary::FindEnemyData(TEXT("MeleeBandit"));
		AGSEnemyCharacter* Enemy = Data ? AGSEnemyCharacter::SpawnEnemy(GetPIEWorld(), Data, Location, GetPlayer()->GetActorLocation()) : nullptr;
		if (!Test->TestNotNull(TEXT("Spawned a frozen MeleeBandit"), Enemy))
		{
			return nullptr;
		}
		if (AAIController* AIC = Cast<AAIController>(Enemy->GetController()))
		{
			if (UBrainComponent* Brain = AIC->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("Health orb test"));
			}
			AIC->StopMovement();
		}
		Enemy->GetCharacterMovement()->DisableMovement();
		return Enemy;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSHealthOrbsTest, "BanditBaller.Economy.HealthOrbs", GSTests::TestFlags)
bool FGSHealthOrbsTest::RunTest(const FString& Parameters)
{
	struct FHealState
	{
		FVector PlayerHome = FVector(0.f, 0.f, 100.f);
		FVector EnemyAt = FVector(1000.f, 0.f, 0.f);
		FVector DropOrigin = FVector::ZeroVector;
		int32 CashValue = 0;
		int32 HealAmount = 0;
		int32 DroppedBefore = 0;
		int32 CollectedBefore = 0;
	};
	TSharedRef<FHealState> S = MakeShared<FHealState>();

	BeginPIE(this);

	// Settings, then chance 0 never drops and chance 1 always drops (enemies killed by the player, out of reach).
	Step([this, S]()
	{
		AGSGameMode* GameMode = GetGameMode();
		const UGSCashSettings* Settings = GameMode->GetCashSettings();
		if (!TestNotNull(TEXT("Pickup settings (DA_CashSettings)"), Settings))
		{
			return;
		}
		SpeedUpWorld(4.f); // tosses, magnet flights and the waits run in game time
		GameMode->SetRoundTimerPaused(true);
		GameMode->RestartRunWithSeed(351);
		RemoveAllEnemies(GetPIEWorld());
		GetPlayer()->TeleportTo(S->PlayerHome, FRotator::ZeroRotator);
		// The kills' XP orbs must not level the player up (a level-up pauses the game and heals).
		GetPlayerASC()->SetNumericAttributeBase(UGSAttributeSet::GetXPGainAttribute(), 0.f);

		TestEqual(TEXT("DropChance 20%"), Settings->HealthOrbDropChance, 0.2f, 0.0001f);
		TestEqual(TEXT("Drop chance read from the settings"), GameMode->GetHealthOrbDropChance(), 0.2f, 0.0001f);
		TestEqual(TEXT("HealAmount 15"), Settings->HealthOrbHealAmount, 15);
		TestEqual(TEXT("Health orbs never expire by default"), Settings->HealthOrbLifetime, 0.f);
		TestTrue(TEXT("Health orb look: mesh + material"), Settings->HealthOrbLook.Mesh && Settings->HealthOrbLook.Material);
		TestTrue(TEXT("Its own material, not the XP orb's"), Settings->HealthOrbLook.Material != Settings->XPOrbLook.Material);
		TestTrue(TEXT("Larger than the XP orb"), Settings->HealthOrbLook.Scale.X > Settings->XPOrbLook.Scale.X);
		TestNotNull(TEXT("Placeholder pickup sound"), Settings->HealthOrbSound.Get());
		const UGSUpgradeData* Magnet = UGSDebugLibrary::FindUpgradeData(TEXT("Magnet"));
		TestTrue(TEXT("Magnet card mentions health orbs"), Magnet && Magnet->GetFormattedDescription().ToString().Contains(TEXT("health")));
		S->HealAmount = Settings->HealthOrbHealAmount;

		UGSCashPickupSubsystem* Pickups = GetCashSubsystem();
		S->DroppedBefore = Pickups->GetNumHealthOrbsDropped();
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		for (int32 Index = 0; Index < 10; ++Index)
		{
			KillEnemyForCash(this, S->EnemyAt, S->DropOrigin, S->CashValue);
		}
		TestEqual(TEXT("Chance 0: 10 kills, no health orb"), Pickups->GetNumHealthOrbsDropped(), S->DroppedBefore);
		TestEqual(TEXT("Chance 0: none on the ground"), Pickups->GetNumActiveOfKind(EGSPickupKind::Health), 0);

		GameMode->SetHealthOrbDropChanceOverride(1.f);
		for (int32 Index = 0; Index < 5; ++Index)
		{
			KillEnemyForCash(this, S->EnemyAt, S->DropOrigin, S->CashValue);
		}
		TestEqual(TEXT("Chance 1: 5 kills, 5 health orbs"), Pickups->GetNumHealthOrbsDropped(), S->DroppedBefore + 5);
		TestEqual(TEXT("Chance 1: 5 on the ground"), Pickups->GetNumActiveOfKind(EGSPickupKind::Health), 5);

		// One orb for the next checks.
		Pickups->ReleaseAll();
		KillEnemyForCash(this, S->EnemyAt, S->DropOrigin, S->CashValue);
		const AGSCashPickup* Orb = FindActiveHealthOrb();
		if (TestNotNull(TEXT("Health orb"), Orb))
		{
			TestEqual(TEXT("Orb worth HealAmount"), Orb->GetValue(), S->HealAmount);
			TestEqual(TEXT("Orb has no expiry"), Orb->GetExpireAtServerTime(), 0.0);
			TestTrue(TEXT("Orb uses the health orb mesh"), Orb->GetMesh()->GetStaticMesh() == Settings->HealthOrbLook.Mesh);
			TestTrue(TEXT("Orb uses M_HealthOrb"), Orb->GetMesh()->GetMaterial(0) == Settings->HealthOrbLook.Material);
			TestFalse(TEXT("Orb is visible"), Orb->IsHidden());
		}
	});
	WaitWorldSeconds(this, 0.6); // the toss lands

	// Full health: standing on it, the orb isn't attracted or collected.
	Step([this, S]()
	{
		const AGSCashPickup* Orb = FindActiveHealthOrb();
		TestEqual(TEXT("Player at full health"), GetHealth(GetPlayerASC()), GetMaxHealth(GetPlayerASC()));
		if (Orb)
		{
			GetPlayer()->TeleportTo(Orb->GetActorLocation() + FVector(-60.f, 0.f, 60.f), FRotator::ZeroRotator);
		}
		S->CollectedBefore = GetCashSubsystem()->GetNumHealthOrbsCollected();
	});
	WaitWorldSeconds(this, 1.5);
	Step([this, S]()
	{
		TestEqual(TEXT("Full health: the orb stays"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Health), 1);
		TestEqual(TEXT("Full health: not attracted"), CountMagnetizedHealthOrbs(), 0);
		TestEqual(TEXT("Full health: not collected"), GetCashSubsystem()->GetNumHealthOrbsCollected(), S->CollectedBefore);

		// Hurt: 100 -> 60, and the orb comes in.
		ApplyDamage(GetPlayerASC(), 40.f);
		TestEqual(TEXT("Hurt to 60"), GetHealth(GetPlayerASC()), GetMaxHealth(GetPlayerASC()) - 40.f, 0.01f);
	});
	WaitUntil(this, TEXT("the orb to be collected once hurt"), []() { return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Health) == 0; }, 10.0);
	Step([this, S]()
	{
		UAbilitySystemComponent* ASC = GetPlayerASC();
		TestEqual(TEXT("Healed HealAmount"), GetHealth(ASC), GetMaxHealth(ASC) - 40.f + S->HealAmount, 0.01f);
		TestEqual(TEXT("Counted as collected"), GetCashSubsystem()->GetNumHealthOrbsCollected(), S->CollectedBefore + 1);
		const UGSRunStats* Stats = GetGameState()->GetRunStats();
		TestEqual(TEXT("Run stats: 1 health orb"), Stats->GetHealthOrbsCollected(), 1);
		TestEqual(TEXT("Run stats: healed HealAmount"), Stats->GetHealthHealedByOrbs(), static_cast<float>(S->HealAmount), 0.01f);
		const UGSHUDWidget* HUD = GetPC()->GetGSHUD();
		TestTrue(TEXT("Green +N popup"), HUD && HUD->GetActiveHealthPopupTotal() >= S->HealAmount);

		// gs.HealthOrb.Spawn 2 at 75/100: 15 + 10, never above MaxHealth.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("HealthOrbSpawn"), { TEXT("2") });
		TestEqual(TEXT("gs.HealthOrb.Spawn 2 drops 2"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Health), 2);
	});
	WaitUntil(this, TEXT("the spawned orbs to be collected"), []() { return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Health) == 0; }, 10.0);
	Step([this, S]()
	{
		UAbilitySystemComponent* ASC = GetPlayerASC();
		TestEqual(TEXT("Never above MaxHealth"), GetHealth(ASC), GetMaxHealth(ASC), 0.01f);
		const UGSRunStats* Stats = GetGameState()->GetRunStats();
		TestEqual(TEXT("Run stats: 3 health orbs"), Stats->GetHealthOrbsCollected(), 3);
		TestEqual(TEXT("Run stats: only the health actually healed (40)"), Stats->GetHealthHealedByOrbs(), 40.f, 0.01f);

		// At full health a spawned orb waits on the ground.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("HealthOrbSpawn"), {});
	});
	WaitWorldSeconds(this, 1.5);
	Step([this, S]()
	{
		TestEqual(TEXT("Full health: the spawned orb waits"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Health), 1);
		TestEqual(TEXT("Full health: not attracted"), CountMagnetizedHealthOrbs(), 0);

		// The RoundClear vacuum collects it anyway; the heal over MaxHealth is lost.
		GetGameMode()->SetPhase(EGSRoundPhase::RoundClear);
		TestEqual(TEXT("RoundClear: the orb flies to the player"), CountMagnetizedHealthOrbs(), 1);
	});
	WaitUntil(this, TEXT("the vacuumed health orb to arrive"), []() { return GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Health) == 0; }, 10.0);
	Step([this, S]()
	{
		UAbilitySystemComponent* ASC = GetPlayerASC();
		TestEqual(TEXT("Vacuum: still at MaxHealth"), GetHealth(ASC), GetMaxHealth(ASC), 0.01f);
		const UGSRunStats* Stats = GetGameState()->GetRunStats();
		TestEqual(TEXT("Vacuum: collected (4 orbs)"), Stats->GetHealthOrbsCollected(), 4);
		TestEqual(TEXT("Vacuum: nothing healed over MaxHealth"), Stats->GetHealthHealedByOrbs(), 40.f, 0.01f);

		// Kill credit: a blast kill (Powder Keg) drops one too; a kill that isn't the player's doesn't.
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRound(1);
		GameMode->SetRoundTimerPaused(true);
		GetPlayer()->TeleportTo(S->PlayerHome, FRotator::ZeroRotator);
		UGSCashPickupSubsystem* Pickups = GetCashSubsystem();
		Pickups->ReleaseAll();

		AGSEnemyCharacter* Other = SpawnFrozenMeleeAt(this, FVector(1000.f, 800.f, 0.f));
		if (Other)
		{
			ApplyDamage(Other->GetAbilitySystemComponent(), 100000.f); // its own damage: not the player's kill
			TestTrue(TEXT("Non-player kill died"), Other->GetHealthComponent()->IsDead());
		}
		TestEqual(TEXT("A kill that isn't the player's drops no health orb"), Pickups->GetNumActiveOfKind(EGSPickupKind::Health), 0);

		// Powder Keg x34: every kill explodes (100%), so the shot kill's blast surely kills the neighbour.
		TestEqual(TEXT("Powder Keg x34"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("PowderKeg"), 34), 34);
		AGSEnemyCharacter* Neighbour = SpawnFrozenMeleeAt(this, S->EnemyAt + FVector(0.f, 150.f, 0.f));
		if (Neighbour)
		{
			Neighbour->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 20.f);
		}
		KillEnemyForCash(this, S->EnemyAt, S->DropOrigin, S->CashValue);
		TestTrue(TEXT("The blast killed the neighbour"), Neighbour && Neighbour->GetHealthComponent()->IsDead());
		TestEqual(TEXT("Shot kill + blast kill: 2 health orbs"), Pickups->GetNumActiveOfKind(EGSPickupKind::Health), 2);

		// A new run clears health orbs on the ground and the run stats.
		GameMode->RestartRunWithSeed(352);
		TestEqual(TEXT("New run clears health orbs"), Pickups->GetNumActiveOfKind(EGSPickupKind::Health), 0);
		TestEqual(TEXT("New run: 0 orbs collected"), GetGameState()->GetRunStats()->GetHealthOrbsCollected(), 0);
		TestEqual(TEXT("New run: 0 healed"), GetGameState()->GetRunStats()->GetHealthHealedByOrbs(), 0.f);

		// A dead player collects nothing.
		GetPlayerASC()->SetNumericAttributeBase(UGSAttributeSet::GetXPGainAttribute(), 0.f);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("HealthOrbSpawn"), {});
		ApplyDamage(GetPlayerASC(), 100000.f);
		TestTrue(TEXT("Player is dead"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
	});
	WaitWorldSeconds(this, 1.5);
	Step([this]()
	{
		TestEqual(TEXT("Dead player: the orb stays"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Health), 1);
		TestEqual(TEXT("Dead player: nothing collected"), GetGameState()->GetRunStats()->GetHealthOrbsCollected(), 0);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Floating Shield (Prompt 36): shields block whole hits in the damage path, each recharges on its own, dash first
// ---------------------------------------------------------------------------

namespace GSTests
{
	UGSShieldComponent* GetPlayerShields()
	{
		const AGSPlayerState* PS = GetPC() ? GetPC()->GetPlayerState<AGSPlayerState>() : nullptr;
		return PS ? PS->GetShieldComponent() : nullptr;
	}

	UGSFloatingShieldsComponent* GetShieldVisuals()
	{
		const AGSCharacter* Player = GetPlayer();
		return Player ? Player->GetFloatingShields() : nullptr;
	}

	/** An enemy of Type at Location facing the player, AI and movement stopped (its attacks fire straight ahead). */
	AGSEnemyCharacter* SpawnFrozenEnemyFacingPlayer(const TCHAR* Type, const FVector& Location)
	{
		AGSEnemyCharacter* Enemy = AGSEnemyCharacter::SpawnEnemy(GetPIEWorld(), UGSDebugLibrary::FindEnemyData(Type), Location,
			GetPlayer()->GetActorLocation());
		if (!Enemy)
		{
			return nullptr;
		}
		if (AAIController* AIC = Cast<AAIController>(Enemy->GetController()))
		{
			if (UBrainComponent* Brain = AIC->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("Shield test"));
			}
			AIC->StopMovement();
		}
		Enemy->GetCharacterMovement()->DisableMovement();
		const FVector ToPlayer = (GetPlayer()->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
		Enemy->SetActorRotation(ToPlayer.Rotation());
		return Enemy;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSFloatingShieldTest, "BanditBaller.Upgrades.FloatingShield", GSTests::TestFlags)
bool FGSFloatingShieldTest::RunTest(const FString& Parameters)
{
	struct FShieldState
	{
		TWeakObjectPtr<AGSEnemyCharacter> Attacker;
		float Health = 0.f;
		float DamageTakenBefore = 0.f;
		double Block1Time = 0.0;
		double Block2Time = 0.0;
		int32 BlockedBefore = 0;
	};
	TSharedRef<FShieldState> S = MakeShared<FShieldState>();
	const FVector PlayerHome(0.f, 0.f, 100.f);
	auto Now = []() { return GetPIEWorld() ? GetPIEWorld()->GetTimeSeconds() : 0.0; };
	auto AttackerASC = [S]() { return S->Attacker.IsValid() ? S->Attacker->GetAbilitySystemComponent() : nullptr; };

	BeginPIE(this);

	// Data; no shields: a hit takes Health. Then two shields (console).
	Step([this, S, PlayerHome, AttackerASC]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		GetPlayer()->TeleportTo(PlayerHome, FRotator::ZeroRotator);

		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("FloatingShield"));
		if (TestNotNull(TEXT("DA_Upgrade_FloatingShield exists"), Upgrade))
		{
			TestTrue(TEXT("Floating Shield is an Ability"), Upgrade->Category == EGSUpgradeCategory::Ability);
			TestTrue(TEXT("Effect is GE_Upgrade_FloatingShield"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_FloatingShield::StaticClass()));
			TestTrue(FString::Printf(TEXT("Card says +1 shield: %s"), *Upgrade->FormatValueText(Upgrade->BaseValue).ToString()),
				Upgrade->FormatValueText(Upgrade->BaseValue).ToString().Contains(TEXT("+1")));
			const UGSUpgradePool* Pool = GameMode->GetUpgradePool();
			TestTrue(TEXT("Floating Shield is in DA_UpgradePool"), Pool && Pool->Upgrades.Contains(Upgrade));
		}
		const UGSCombatSettings* Combat = GameMode->GetCombatSettings();
		TestTrue(TEXT("ShieldCooldown 10 s"), Combat && FMath::IsNearlyEqual(Combat->ShieldCooldown, 10.f));

		UGSShieldComponent* Shields = GetPlayerShields();
		UGSFloatingShieldsComponent* Visuals = GetShieldVisuals();
		if (!TestNotNull(TEXT("Shield component on the PlayerState"), Shields) || !TestNotNull(TEXT("Shield visuals on the character"), Visuals))
		{
			return;
		}
		TestEqual(TEXT("Shields start at 0"), GetAttr(UGSAttributeSet::GetShieldsAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("No shields"), Shields->GetNumShields(), 0);
		TestEqual(TEXT("No shield meshes"), Visuals->GetNumShieldMeshes(), 0);

		S->Attacker = SpawnFrozenBandit(FVector(-600.f, 0.f, 0.f));
		const float Before = GetHealth(GetPlayerASC());
		ApplyDamage(GetPlayerASC(), 10.f, AttackerASC());
		TestEqual(TEXT("No shield: the hit takes Health"), GetHealth(GetPlayerASC()), Before - 10.f, 0.01f);
		TestEqual(TEXT("No shield: nothing blocked"), Shields->GetHitsBlocked(), 0);

		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingShield"), TEXT("2") });
		TestEqual(TEXT("Shields 2"), GetAttr(UGSAttributeSet::GetShieldsAttribute()), 2.f, 0.001f);
		TestEqual(TEXT("Two shields"), Shields->GetNumShields(), 2);
		TestEqual(TEXT("Both ready"), Shields->GetNumReady(), 2);
	});
	WaitFrames(this, 2);

	// The visuals: two discs at chest height, evenly spaced, no collision.
	Step([this]()
	{
		const UGSFloatingShieldsComponent* Visuals = GetShieldVisuals();
		TestEqual(TEXT("Two shield meshes"), Visuals->GetNumShieldMeshes(), 2);
		const FVector Center = GetPlayer()->GetActorLocation();
		TArray<float> Yaws;
		for (int32 i = 0; i < Visuals->GetNumShieldMeshes(); ++i)
		{
			const UStaticMeshComponent* Mesh = Visuals->GetShieldMesh(i);
			if (!TestNotNull(FString::Printf(TEXT("Shield mesh %d"), i), Mesh))
			{
				return;
			}
			TestNotNull(TEXT("Placeholder mesh set (BP_GSCharacter)"), Mesh->GetStaticMesh().Get());
			TestTrue(TEXT("Shown while ready"), Visuals->IsShieldShown(i));
			TestEqual(TEXT("No collision"), static_cast<int32>(Mesh->GetCollisionEnabled()), static_cast<int32>(ECollisionEnabled::NoCollision));
			const FVector Offset = Mesh->GetComponentLocation() - Center;
			TestEqual(TEXT("On the orbit"), static_cast<float>(Offset.Size2D()), Visuals->GetOrbitRadius(), 1.f);
			TestEqual(TEXT("At chest height"), static_cast<float>(Offset.Z), Visuals->GetHeight(), 1.f);
			Yaws.Add(FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X)));
		}
		if (Yaws.Num() == 2)
		{
			const float Apart = FMath::Abs(FMath::FindDeltaAngleDegrees(Yaws[0], Yaws[1]));
			TestEqual(TEXT("Two shields 180 degrees apart"), Apart, 180.f, 1.f);
		}

		// Dash invulnerability comes first: the hit is blocked without using a shield.
		TapInput(GSGameplayTags::Ability_Dash);
	});
	WaitUntil(this, TEXT("dash i-frames"), []() { return GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Invulnerable); });
	Step([this, S, AttackerASC]()
	{
		const float Before = GetHealth(GetPlayerASC());
		ApplyDamage(GetPlayerASC(), 25.f, AttackerASC());
		TestEqual(TEXT("Dash i-frames: no Health lost"), GetHealth(GetPlayerASC()), Before, 0.01f);
		TestEqual(TEXT("Dash i-frames: no shield used"), GetPlayerShields()->GetNumReady(), 2);
		TestEqual(TEXT("Dash i-frames: nothing blocked by a shield"), GetPlayerShields()->GetHitsBlocked(), 0);
	});
	WaitUntil(this, TEXT("dash i-frames to end"), []() { return !GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Invulnerable); });

	// Block 1: the whole hit is cancelled; one shield goes. A blocked hit isn't damage taken (combo, stats).
	Step([this, S, Now, AttackerASC]()
	{
		UGSShieldComponent* Shields = GetPlayerShields();
		KillTestEnemyAsPlayer(this); // start a combo
		UGSComboComponent* Combo = GetPlayerCombo();
		const int32 ComboBefore = Combo->GetComboCount();
		TestTrue(TEXT("A combo is running"), ComboBefore > 0);
		S->Health = GetHealth(GetPlayerASC());
		S->DamageTakenBefore = GetGameState()->GetRunStats()->GetDamageTaken();

		ApplyDamage(GetPlayerASC(), 25.f, AttackerASC());
		S->Block1Time = Now();
		TestEqual(TEXT("Block 1: no Health lost"), GetHealth(GetPlayerASC()), S->Health, 0.01f);
		TestEqual(TEXT("Block 1: one shield left"), Shields->GetNumReady(), 1);
		TestFalse(TEXT("Block 1: the first shield went"), Shields->IsShieldReady(0));
		TestEqual(TEXT("Block 1: back in 10 s"), Shields->GetCooldownRemaining(0), 10.f, 0.01f);
		TestEqual(TEXT("Block 1: counted"), Shields->GetHitsBlocked(), 1);
		TestEqual(TEXT("Block 1: no damage taken in run stats"), GetGameState()->GetRunStats()->GetDamageTaken(), S->DamageTakenBefore, 0.01f);
		TestEqual(TEXT("Block 1: the combo goes on"), Combo->GetComboCount(), ComboBefore);
		TestEqual(TEXT("Block 1: the block flash was shown"), GetShieldVisuals()->GetBlocksShown(), 1);
	});
	WaitWorldSeconds(this, 0.5);
	Step([this]()
	{
		const UGSFloatingShieldsComponent* Visuals = GetShieldVisuals();
		TestFalse(TEXT("The blocked shield is gone"), Visuals->IsShieldShown(0));
		TestTrue(TEXT("The other shield is still there"), Visuals->IsShieldShown(1));
	});
	WaitWorldSeconds(this, 2.0);

	// Block 2, then a third hit with no shield ready takes Health.
	Step([this, S, Now, AttackerASC]()
	{
		UGSShieldComponent* Shields = GetPlayerShields();
		ApplyDamage(GetPlayerASC(), 25.f, AttackerASC());
		S->Block2Time = Now();
		TestEqual(TEXT("Block 2: no Health lost"), GetHealth(GetPlayerASC()), S->Health, 0.01f);
		TestEqual(TEXT("Block 2: no shield left"), Shields->GetNumReady(), 0);
		TestTrue(TEXT("Each shield has its own cooldown"), Shields->GetCooldownRemaining(0) < Shields->GetCooldownRemaining(1) - 1.5f);

		ApplyDamage(GetPlayerASC(), 25.f, AttackerASC());
		TestEqual(TEXT("No shield ready: the hit takes Health"), GetHealth(GetPlayerASC()), S->Health - 25.f, 0.01f);
		TestEqual(TEXT("No shield ready: still 2 blocks"), Shields->GetHitsBlocked(), 2);
		TestEqual(TEXT("Run stats: 25 damage taken"), GetGameState()->GetRunStats()->GetDamageTaken(), S->DamageTakenBefore + 25.f, 0.01f);
	});

	// Each shield comes back 10 s after its own block.
	WaitUntil(this, TEXT("the first shield to come back"), []() { return GetPlayerShields()->GetNumReady() >= 1; }, 15.0);
	Step([this, S, Now]()
	{
		UGSShieldComponent* Shields = GetPlayerShields();
		const double Elapsed = Now() - S->Block1Time;
		AddInfo(FString::Printf(TEXT("[shield] shield 1 back %.2f s after its block"), Elapsed));
		TestTrue(FString::Printf(TEXT("Shield 1 back 10 s after its block (%.2f)"), Elapsed), FMath::Abs(Elapsed - 10.0) < 0.5);
		TestTrue(TEXT("Shield 1 ready"), Shields->IsShieldReady(0));
		TestFalse(TEXT("Shield 2 still recharging"), Shields->IsShieldReady(1));
		TestEqual(TEXT("The return was shown"), GetShieldVisuals()->GetReturnsShown(), 1);
	});
	WaitFrames(this, 2);
	Step([this]() { TestTrue(TEXT("The returned shield is visible again"), GetShieldVisuals()->IsShieldShown(0)); });
	WaitUntil(this, TEXT("the second shield to come back"), []() { return GetPlayerShields()->GetNumReady() >= 2; }, 15.0);
	Step([this, S, Now]()
	{
		const double Elapsed = Now() - S->Block2Time;
		TestTrue(FString::Printf(TEXT("Shield 2 back 10 s after its own block (%.2f)"), Elapsed), FMath::Abs(Elapsed - 10.0) < 0.5);

		// A pick while a shield recharges: the new one is ready at once; three shields 120 degrees apart.
		ApplyDamage(GetPlayerASC(), 25.f, GetPlayerASC());
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingShield") });
		UGSShieldComponent* Shields = GetPlayerShields();
		TestEqual(TEXT("Three shields"), Shields->GetNumShields(), 3);
		TestEqual(TEXT("The new one is ready at once (2 of 3)"), Shields->GetNumReady(), 2);
	});
	WaitFrames(this, 2);

	// Real attacks: a melee bandit's swing and a ranged bandit's burst are both blocked.
	Step([this, S]()
	{
		const UGSFloatingShieldsComponent* Visuals = GetShieldVisuals();
		TestEqual(TEXT("Three shield meshes"), Visuals->GetNumShieldMeshes(), 3);
		if (Visuals->GetNumShieldMeshes() == 3)
		{
			const FVector Center = GetPlayer()->GetActorLocation();
			const FVector A = Visuals->GetShieldMesh(0)->GetComponentLocation() - Center;
			const FVector B = Visuals->GetShieldMesh(1)->GetComponentLocation() - Center;
			const float Apart = FMath::Abs(FMath::FindDeltaAngleDegrees(FMath::RadiansToDegrees(FMath::Atan2(A.Y, A.X)), FMath::RadiansToDegrees(FMath::Atan2(B.Y, B.X))));
			TestEqual(TEXT("Three shields 120 degrees apart"), Apart, 120.f, 1.f);
		}

		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingShield"), TEXT("5") });
		GetPlayerShields()->ResetShields(); // all 8 ready
		S->Health = GetHealth(GetPlayerASC());
		S->BlockedBefore = GetPlayerShields()->GetHitsBlocked();
		S->Attacker = SpawnFrozenEnemyFacingPlayer(TEXT("MeleeBandit"), GetPlayer()->GetActorLocation() + FVector(130.f, 0.f, 0.f));
		TestTrue(TEXT("Melee bandit swings"), S->Attacker.IsValid()
			&& S->Attacker->GetAbilitySystemComponent()->TryActivateAbilitiesByTag(FGameplayTagContainer(GSGameplayTags::Ability_Attack_Melee)));
	});
	WaitUntil(this, TEXT("the melee hit to be blocked"), [S]() { return GetPlayerShields()->GetHitsBlocked() > S->BlockedBefore; }, 10.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Melee hit blocked: no Health lost"), GetHealth(GetPlayerASC()), S->Health, 0.01f);
		RemoveAllEnemies(GetPIEWorld());
		S->BlockedBefore = GetPlayerShields()->GetHitsBlocked();
		S->Attacker = SpawnFrozenEnemyFacingPlayer(TEXT("RangedBandit"), GetPlayer()->GetActorLocation() + FVector(600.f, 0.f, 0.f));
		TestTrue(TEXT("Ranged bandit fires"), S->Attacker.IsValid()
			&& S->Attacker->GetAbilitySystemComponent()->TryActivateAbilitiesByTag(FGameplayTagContainer(GSGameplayTags::Ability_Attack_Ranged)));
	});
	WaitUntil(this, TEXT("a shot to be blocked"), [S]() { return GetPlayerShields()->GetHitsBlocked() > S->BlockedBefore; }, 10.0);
	WaitWorldSeconds(this, 2.0); // the rest of the burst
	Step([this, S]()
	{
		const int32 Shots = GetPlayerShields()->GetHitsBlocked() - S->BlockedBefore;
		AddInfo(FString::Printf(TEXT("[shield] %d shot(s) of the burst blocked"), Shots));
		TestTrue(TEXT("Shots blocked (each hit uses a shield)"), Shots >= 1 && Shots <= 4);
		TestEqual(TEXT("Shots blocked: no Health lost"), GetHealth(GetPlayerASC()), S->Health, 0.01f);
		RemoveAllEnemies(GetPIEWorld());

		// A new run: no shields, no meshes, no recharges.
		GetGameMode()->RestartRunWithSeed(36);
		TestEqual(TEXT("New run: Shields 0"), GetAttr(UGSAttributeSet::GetShieldsAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("New run: no shields"), GetPlayerShields()->GetNumShields(), 0);
		TestEqual(TEXT("New run: nothing blocked yet"), GetPlayerShields()->GetHitsBlocked(), 0);
	});
	WaitFrames(this, 2);
	Step([this]()
	{
		TestEqual(TEXT("New run: no shield meshes"), GetShieldVisuals()->GetNumShieldMeshes(), 0);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Second Wind (Prompt 37): a killing blow uses one up instead (30% Health, 2 s invulnerable, one stack gone)
// ---------------------------------------------------------------------------

namespace GSTests
{
	UGSSecondWindComponent* GetPlayerSecondWind()
	{
		const AGSPlayerState* PS = GetPC() ? GetPC()->GetPlayerState<AGSPlayerState>() : nullptr;
		return PS ? PS->GetSecondWindComponent() : nullptr;
	}

	bool IsPlayerInvulnerable()
	{
		return GetPlayerASC() && GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Invulnerable);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSSecondWindTest, "BanditBaller.Upgrades.SecondWind", GSTests::TestFlags)
bool FGSSecondWindTest::RunTest(const FString& Parameters)
{
	struct FWindState
	{
		TWeakObjectPtr<AGSEnemyCharacter> Attacker;
		TObjectPtr<const UGSUpgradeData> Upgrade;
		double SaveTime = 0.0;
		float Restored = 0.f;
	};
	TSharedRef<FWindState> S = MakeShared<FWindState>();
	auto Now = []() { return GetPIEWorld() ? GetPIEWorld()->GetTimeSeconds() : 0.0; };
	auto AttackerASC = [S]() { return S->Attacker.IsValid() ? S->Attacker->GetAbilitySystemComponent() : nullptr; };
	auto Stacks = [S]()
	{
		const AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		return PS ? PS->GetUpgradeComponent()->GetStacks(S->Upgrade) : -1;
	};

	BeginPIE(this);

	// Data; two Second Winds; a killing blow uses one: 30% Health, invulnerable, one stack gone, feedback.
	Step([this, S, Now, AttackerASC, Stacks]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		GetPlayer()->TeleportTo(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);

		S->Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("SecondWind"));
		if (!TestNotNull(TEXT("DA_Upgrade_SecondWind exists"), S->Upgrade.Get()))
		{
			return;
		}
		TestTrue(TEXT("Second Wind is a Legendary ability"), S->Upgrade->Category == EGSUpgradeCategory::Ability && S->Upgrade->bLegendaryAbility);
		TestTrue(TEXT("Effect is GE_Upgrade_SecondWind"), S->Upgrade->Effect && S->Upgrade->Effect->IsChildOf(UGSUpgradeEffect_SecondWind::StaticClass()));
		TestTrue(FString::Printf(TEXT("Card says +1: %s"), *S->Upgrade->FormatValueText(S->Upgrade->BaseValue).ToString()),
			S->Upgrade->FormatValueText(S->Upgrade->BaseValue).ToString().Contains(TEXT("+1")));
		const UGSUpgradePool* Pool = GameMode->GetUpgradePool();
		TestTrue(TEXT("Second Wind is in DA_UpgradePool"), Pool && Pool->Upgrades.Contains(S->Upgrade));
		const UGSCombatSettings* Combat = GameMode->GetCombatSettings();
		TestTrue(TEXT("Back at 30% Health"), Combat && FMath::IsNearlyEqual(Combat->SecondWindHealthFraction, 0.3f));
		TestTrue(TEXT("Invulnerable for 2 s"), Combat && FMath::IsNearlyEqual(Combat->SecondWindInvulnerableTime, 2.f));

		UGSSecondWindComponent* SecondWind = GetPlayerSecondWind();
		if (!TestNotNull(TEXT("Second Wind component on the PlayerState"), SecondWind))
		{
			return;
		}
		TestEqual(TEXT("SecondWinds starts at 0"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 0.f, 0.001f);
		TestFalse(TEXT("No Second Wind: can't save"), SecondWind->CanSave());

		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("SecondWind"), TEXT("2") });
		TestEqual(TEXT("SecondWinds 2"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 2.f, 0.001f);
		TestEqual(TEXT("Two stacks (Owned: 2)"), Stacks(), 2);

		S->Attacker = SpawnFrozenBandit(FVector(-600.f, 0.f, 0.f));
		KillTestEnemyAsPlayer(this); // start a combo
		TestTrue(TEXT("A combo is running"), GetPlayerCombo()->GetComboCount() > 0);
		const float DamageTakenBefore = GetGameState()->GetRunStats()->GetDamageTaken();
		const float MaxHealth = GetMaxHealth(GetPlayerASC());
		const float HealthBefore = GetHealth(GetPlayerASC());
		S->Restored = FMath::RoundToFloat(MaxHealth * 0.3f); // whole health points
		UGSHUDWidget* HUD = GetPC()->GetGSHUD();
		// The flash fades in real time; game time runs x4 here, so shorten this HUD's flash (not the asset).
		SetFloatProperty(HUD, TEXT("SecondWindFlashDuration"), 0.3f);

		ApplyDamage(GetPlayerASC(), 9999.f, AttackerASC());
		S->SaveTime = Now();
		TestFalse(TEXT("Saved: not dead"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		TestTrue(TEXT("Saved: still in the run"), GetGameState()->GetRoundPhase() != EGSRoundPhase::GameOver);
		TestEqual(TEXT("Saved: Health = 30% of MaxHealth"), GetHealth(GetPlayerASC()), S->Restored, 0.01f);
		TestTrue(TEXT("Saved: invulnerable"), IsPlayerInvulnerable());
		const TArray<TPair<float, float>> Invulnerability = GetPlayerASC()->GetActiveEffectsTimeRemainingAndDuration(
			FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(GSGameplayTags::State_Invulnerable)));
		TestTrue(TEXT("The invulnerability lasts exactly 2 s"), Invulnerability.Num() == 1 && FMath::IsNearlyEqual(Invulnerability[0].Value, 2.f, 0.001f));
		TestEqual(TEXT("One Second Wind used: SecondWinds 1"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 1.f, 0.001f);
		TestEqual(TEXT("One stack removed (Owned: 1)"), Stacks(), 1);
		TestEqual(TEXT("Used once this run"), SecondWind->GetUsesThisRun(), 1);
		TestEqual(TEXT("The hit counts as damage taken (the Health it took)"), GetGameState()->GetRunStats()->GetDamageTaken(), DamageTakenBefore + HealthBefore, 0.01f);
		TestEqual(TEXT("The hit ended the combo"), GetPlayerCombo()->GetComboCount(), 0);
		if (TestNotNull(TEXT("HUD"), HUD))
		{
			TestTrue(TEXT("Screen flash"), HUD->IsSecondWindFlashShowing() && HUD->GetSecondWindFlashAlpha() > 0.5f);
			TestEqual(TEXT("Sound played once"), HUD->GetSecondWindSoundsPlayed(), 1);
			TestEqual(TEXT("HUD shows whole Health (30, not 31)"), FMath::CeilToInt(HUD->GetDisplayedHealth()), FMath::RoundToInt(S->Restored));
			TestTrue(TEXT("SECOND WIND banner"), HUD->IsSecondWindBannerShowing());
			TestEqual(TEXT("Banner title"), HUD->GetRoundBannerTitle().ToString(), FString(TEXT("SECOND WIND")));
			TestEqual(TEXT("Banner subtitle"), HUD->GetRoundBannerSubtitle().ToString(), FString(TEXT("1 left")));
			const UGSUpgradeIconWidget* Icon = HUD->FindUpgradeIcon(S->Upgrade);
			TestTrue(TEXT("The HUD icon shows 1 left"), Icon && Icon->GetStacks() == 1);
		}

		// Invulnerable: another killing blow does nothing.
		ApplyDamage(GetPlayerASC(), 9999.f, AttackerASC());
		TestEqual(TEXT("Invulnerable: no Health lost"), GetHealth(GetPlayerASC()), S->Restored, 0.01f);
		TestEqual(TEXT("Invulnerable: no Second Wind used"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 1.f, 0.001f);
	});
	WaitUntil(this, TEXT("the invulnerability to end"), []() { return !IsPlayerInvulnerable(); }, 10.0);

	// A hit that doesn't kill uses nothing; a ready shield comes first; then the last Second Wind.
	Step([this, S, Now, AttackerASC, Stacks]()
	{
		const double Elapsed = Now() - S->SaveTime;
		AddInfo(FString::Printf(TEXT("[secondwind] invulnerable for %.2f s"), Elapsed));
		// Sampled once a frame, so a long frame at x4 can overshoot; it must not end early.
		TestTrue(FString::Printf(TEXT("Invulnerability didn't end early (%.2f s)"), Elapsed), Elapsed >= 1.95);
		TestFalse(TEXT("The flash is over"), GetPC()->GetGSHUD()->IsSecondWindFlashShowing());

		ApplyDamage(GetPlayerASC(), 10.f, AttackerASC());
		TestEqual(TEXT("A hit that doesn't kill takes Health"), GetHealth(GetPlayerASC()), S->Restored - 10.f, 0.01f);
		TestEqual(TEXT("... and uses no Second Wind"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 1.f, 0.001f);

		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingShield") });
		ApplyDamage(GetPlayerASC(), 9999.f, AttackerASC());
		TestEqual(TEXT("Shield first: no Health lost"), GetHealth(GetPlayerASC()), S->Restored - 10.f, 0.01f);
		TestEqual(TEXT("Shield first: the shield is used"), GetPlayerShields()->GetNumReady(), 0);
		TestEqual(TEXT("Shield first: no Second Wind used"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 1.f, 0.001f);

		ApplyDamage(GetPlayerASC(), 9999.f, AttackerASC());
		TestFalse(TEXT("Last Second Wind: not dead"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		TestEqual(TEXT("Last Second Wind: Health = 30%"), GetHealth(GetPlayerASC()), S->Restored, 0.01f);
		TestEqual(TEXT("None left"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("No stacks left (Owned: 0)"), Stacks(), 0);
		const UGSHUDWidget* HUD = GetPC()->GetGSHUD();
		TestEqual(TEXT("Banner: that was the last"), HUD->GetRoundBannerSubtitle().ToString(), FString(TEXT("THAT WAS YOUR LAST")));
		TestNull(TEXT("The HUD icon is gone"), HUD->FindUpgradeIcon(S->Upgrade));
		TestEqual(TEXT("Two sounds in all"), HUD->GetSecondWindSoundsPlayed(), 2);
	});
	WaitUntil(this, TEXT("the invulnerability to end"), []() { return !IsPlayerInvulnerable(); }, 10.0);

	// Picking it again adds one; a new run clears it; without one, a killing blow kills.
	Step([this, S, AttackerASC, Stacks]()
	{
		TestEqual(TEXT("Picked again"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("SecondWind"), 1), 1);
		TestEqual(TEXT("Picked again: SecondWinds 1"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 1.f, 0.001f);
		TestEqual(TEXT("Picked again: Owned 1"), Stacks(), 1);

		RemoveAllEnemies(GetPIEWorld());
		GetGameMode()->RestartRunWithSeed(37);
		TestEqual(TEXT("New run: SecondWinds 0"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("New run: no uses"), GetPlayerSecondWind()->GetUsesThisRun(), 0);
		TestEqual(TEXT("New run: Owned 0"), Stacks(), 0);

		ApplyDamage(GetPlayerASC(), 9999.f);
		TestTrue(TEXT("No Second Wind: a killing blow kills"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		TestTrue(TEXT("No Second Wind: game over"), GetGameState()->GetRoundPhase() == EGSRoundPhase::GameOver);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Floating Gun, part 1 (Prompt 38): a gun over the right shoulder shoots the nearest enemy with pooled projectiles
// ---------------------------------------------------------------------------

namespace GSTests
{
	AGSFloatingGun* GetFloatingGun()
	{
		const AGSCharacter* Player = GetPlayer();
		const UGSFloatingGunComponent* Guns = Player ? Player->GetFloatingGuns() : nullptr;
		return Guns ? Guns->GetGun(0) : nullptr;
	}

	UGSProjectileSubsystem* GetProjectiles()
	{
		return GetPIEWorld() ? GetPIEWorld()->GetSubsystem<UGSProjectileSubsystem>() : nullptr;
	}

	/** A frozen MeleeBandit with a lot of Health (the gun can shoot it for a while). */
	AGSEnemyCharacter* SpawnToughBandit(const FVector& Location)
	{
		AGSEnemyCharacter* Bandit = SpawnFrozenBandit(Location);
		if (Bandit)
		{
			Bandit->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetMaxHealthAttribute(), 100000.f);
			Bandit->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 100000.f);
		}
		return Bandit;
	}

	AStaticMeshActor* SpawnTestWall(const FVector& Location, const FVector& Scale)
	{
		AStaticMeshActor* Wall = GetPIEWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
		if (Wall)
		{
			Wall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			Wall->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			Wall->GetStaticMeshComponent()->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Wall->SetActorScale3D(Scale);
		}
		return Wall;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSFloatingGunTest, "BanditBaller.Upgrades.FloatingGun", GSTests::TestFlags)
bool FGSFloatingGunTest::RunTest(const FString& Parameters)
{
	struct FGunState
	{
		TWeakObjectPtr<AGSEnemyCharacter> Bandit;
		TWeakObjectPtr<AGSEnemyCharacter> Near;
		TWeakObjectPtr<AStaticMeshActor> Wall;
		double WindowEnd = 0.0;
		int32 ScheduleChecks = 0;
		int32 ScheduleMisses = 0;
		int32 ShotsBefore = 0;
		int32 HitsBefore = 0;
		float BanditHealthBefore = 0.f;
		int32 CritNumbersBefore = 0;
		FActiveGameplayEffectHandle SafeZone;
		int32 Config = 0;
		float WorstDistance = 1.f;
		// Kill credit.
		int32 KillsBefore = 0;
		int32 XPOrbsBefore = 0;
		int32 HealthOrbsBefore = 0;
		int32 BlastsBefore = 0;
		float PlayerHealthBefore = 0.f;
	};
	TSharedRef<FGunState> S = MakeShared<FGunState>();
	auto Now = []() { return GetPIEWorld() ? GetPIEWorld()->GetTimeSeconds() : 0.0; };
	auto Health = [](const TWeakObjectPtr<AGSEnemyCharacter>& Bandit) { return Bandit.IsValid() ? GetHealth(Bandit->GetAbilitySystemComponent()) : -1.f; };
	const FVector Home(0.f, 0.f, 100.f);

	BeginPIE(this);

	// Data; no gun at level 0; one gun at level 1.
	Step([this, Home]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		GetPlayer()->TeleportTo(Home, FRotator::ZeroRotator);
		GetPC()->SetControlRotation(FRotator::ZeroRotator);

		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("FloatingGun"));
		if (TestNotNull(TEXT("DA_Upgrade_FloatingGun exists"), Upgrade))
		{
			TestTrue(TEXT("Floating Gun is a Weapon"), Upgrade->Category == EGSUpgradeCategory::Weapon);
			TestTrue(TEXT("Effect is GE_Upgrade_FloatingGun"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_FloatingGun::StaticClass()));
			const UGSUpgradePool* Pool = GameMode->GetUpgradePool();
			TestTrue(TEXT("Floating Gun is in DA_UpgradePool"), Pool && Pool->Upgrades.Contains(Upgrade));
		}
		const UGSFloatingGunSettings* Settings = GameMode->GetFloatingGunSettings();
		if (TestNotNull(TEXT("DA_FloatingGunSettings on the GameMode"), Settings))
		{
			TestEqual(TEXT("8 damage per hit"), Settings->Damage, 8.f);
			TestEqual(TEXT("1.5 shots per second"), Settings->ShotsPerSecond, 1.5f);
			TestEqual(TEXT("20 m range"), Settings->Range, 2000.f);
			TestEqual(TEXT("60 m/s projectiles"), Settings->ProjectileSpeed, 6000.f);
			TestNotNull(TEXT("Damage effect set"), Settings->DamageEffect.Get());
			TestNotNull(TEXT("Gun mesh set"), Settings->GunMesh.Get());
			TestNotNull(TEXT("Projectile mesh set"), Settings->ProjectileMesh.Get());
		}
		TestEqual(TEXT("FloatingGunLevel starts at 0"), GetAttr(UGSAttributeSet::GetFloatingGunLevelAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("No gun at level 0"), GetPlayer()->GetFloatingGuns()->GetGuns().Num(), 0);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingGun") });
		TestEqual(TEXT("FloatingGunLevel 1"), GetAttr(UGSAttributeSet::GetFloatingGunLevelAttribute()), 1.f, 0.001f);
	});
	WaitWorldSeconds(this, 0.5);

	// Placement: over the right shoulder, above the head; no collision; nothing to shoot yet.
	Step([this, S, Health]()
	{
		const AGSFloatingGun* Gun = GetFloatingGun();
		TestEqual(TEXT("Level 1: one gun"), GetPlayer()->GetFloatingGuns()->GetGuns().Num(), 1);
		if (!TestNotNull(TEXT("The gun"), Gun))
		{
			return;
		}
		TestEqual(TEXT("Right shoulder"), Gun->GetSide(), 1.f);
		TestEqual(TEXT("No collision"), static_cast<int32>(Gun->GetMesh()->GetCollisionEnabled()), static_cast<int32>(ECollisionEnabled::NoCollision));
		const FVector Center = GetPlayer()->GetActorLocation();
		const float HeadTop = Center.Z + GetPlayer()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector Rest = Gun->GetRestLocation();
		AddInfo(FString::Printf(TEXT("[gun] rest offset from the capsule center: %s (head top +%.0f)"), *(Rest - Center).ToCompactString(), HeadTop - Center.Z));
		TestTrue(TEXT("To the right of the player (camera yaw 0: +Y)"), Rest.Y - Center.Y > 30.f);
		TestTrue(TEXT("Higher than the top of the head"), Rest.Z > HeadTop);
		TestTrue(TEXT("It has followed to where it should be"), Gun->GetActorLocation().Equals(Rest + Gun->GetCrosshairAvoidOffset(), 5.f));
		TestNull(TEXT("No enemies: no target"), Gun->GetTarget());
		TestEqual(TEXT("No enemies: no shots"), Gun->GetShotsFired(), 0);

		// A tough frozen bandit 10 m ahead.
		S->Bandit = SpawnToughBandit(FVector(1000.f, 0.f, 0.f));
		S->BanditHealthBefore = Health(S->Bandit);
		S->HitsBefore = GetProjectiles() ? GetProjectiles()->GetNumHitTargets() : 0;
	});
	WaitUntil(this, TEXT("the first shot"), []() { return GetFloatingGun() && GetFloatingGun()->GetShotsFired() > 0; }, 10.0);

	// The schedule: in every frame, shots in the burst == 1 + floor(elapsed / interval), for 4 s of game time.
	Step([this, S, Now]()
	{
		TestTrue(TEXT("It targets the bandit"), GetFloatingGun()->GetTarget() == S->Bandit.Get());
		S->WindowEnd = Now() + 4.0;
	});
	WaitUntil(this, TEXT("4 s of shooting"), [this, S, Now]()
	{
		const AGSFloatingGun* Gun = GetFloatingGun();
		const double Interval = GetGameMode()->GetFloatingGunSettings()->GetShotInterval();
		const int32 Expected = 1 + FMath::FloorToInt((Now() - Gun->GetBurstStartTime()) / Interval + 1e-6);
		++S->ScheduleChecks;
		if (Gun->GetBurstShots() != Expected)
		{
			++S->ScheduleMisses;
			AddError(FString::Printf(TEXT("Schedule: %d shots at %.3f s into the burst, expected %d"), Gun->GetBurstShots(), Now() - Gun->GetBurstStartTime(), Expected));
		}
		return Now() >= S->WindowEnd;
	}, 30.0);
	Step([this, S, Health]()
	{
		const AGSFloatingGun* Gun = GetFloatingGun();
		AddInfo(FString::Printf(TEXT("[gun] %d shots in the burst, %d frames checked against the schedule"), Gun->GetBurstShots(), S->ScheduleChecks));
		TestTrue(TEXT("About 1.5 shots a second (at least 6 in 4 s)"), Gun->GetBurstShots() >= 6);
		TestEqual(TEXT("Every frame on schedule"), S->ScheduleMisses, 0);
		const int32 Hits = GetProjectiles()->GetNumHitTargets() - S->HitsBefore;
		const float Dealt = S->BanditHealthBefore - Health(S->Bandit);
		AddInfo(FString::Printf(TEXT("[gun] %d hits, %.0f damage"), Hits, Dealt));
		TestTrue(TEXT("Its projectiles hit the bandit"), Hits >= 5);
		TestEqual(TEXT("8 damage per hit (Damage 1, no crits)"), Dealt, Hits * 8.f, 0.01f);
	});
	// The damage is set when a shot is fired (the crit is rolled when it hits), so grant with no shot in the air.
	WaitUntil(this, TEXT("no projectile in flight"), []() { return GetProjectiles()->GetNumActive() == 0; }, 10.0);
	Step([this, S]()
	{
		// Damage x the player's Damage (x1.5) and Loaded Dice (20 stacks: every hit critical, x2) -> 24.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Damage"), TEXT("5") });
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("LoadedDice"), TEXT("20") });
		S->CritNumbersBefore = GetPC()->GetGSHUD()->GetCriticalDamageNumbersAdded();
	});
	WaitUntil(this, TEXT("a critical hit from the gun"), [S]() { return GetPC()->GetGSHUD()->GetCriticalDamageNumbersAdded() > S->CritNumbersBefore; }, 10.0);
	Step([this, S, Home]()
	{
		const UGSHUDWidget* HUD = GetPC()->GetGSHUD();
		TestEqual(TEXT("Damage x1.5 and a crit: 8 x 1.5 x 2 = 24"), HUD->GetLastDamageNumber(), 24.f, 0.01f);
		TestTrue(TEXT("Shown as a critical number"), HUD->WasLastDamageNumberCritical());

		// Out of range: 25 m away, no target.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandit = SpawnToughBandit(FVector(2500.f, 0.f, 0.f));
		S->ShotsBefore = GetFloatingGun()->GetShotsFired();
	});
	WaitWorldSeconds(this, 1.0);
	Step([this, S]()
	{
		TestNull(TEXT("25 m: out of range, no target"), GetFloatingGun()->GetTarget());
		TestEqual(TEXT("25 m: no shots"), GetFloatingGun()->GetShotsFired(), S->ShotsBefore);

		// No clear line: a wall between the gun and a bandit 10 m away.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandit = SpawnToughBandit(FVector(1000.f, 0.f, 0.f));
		S->Wall = SpawnTestWall(FVector(500.f, 0.f, 200.f), FVector(0.2f, 3.f, 4.f));
		S->ShotsBefore = GetFloatingGun()->GetShotsFired();
	});
	WaitWorldSeconds(this, 1.0);
	Step([this, S]()
	{
		TestNull(TEXT("Behind a wall: no target"), GetFloatingGun()->GetTarget());
		TestEqual(TEXT("Behind a wall: no shots"), GetFloatingGun()->GetShotsFired(), S->ShotsBefore);
		if (S->Wall.IsValid())
		{
			S->Wall->Destroy();
		}
		// A nearer bandit (6 m, off to the side) is the one it picks.
		S->Near = SpawnToughBandit(FVector(600.f, 300.f, 0.f));
	});
	WaitWorldSeconds(this, 0.5);
	Step([this, S, Health]()
	{
		TestTrue(TEXT("The nearest bandit is the target"), GetFloatingGun()->GetTarget() == S->Near.Get());
		TestTrue(TEXT("Wall gone: it shoots again"), GetFloatingGun()->GetShotsFired() > S->ShotsBefore);

		// Projectile rules, fired by hand: through the player into a bandit; into a wall; into the sky (expires).
		RemoveAllEnemies(GetPIEWorld());
		S->Bandit = SpawnToughBandit(FVector(600.f, 0.f, 0.f));
		S->BanditHealthBefore = Health(S->Bandit);
		const UGSFloatingGunSettings* Settings = GetGameMode()->GetFloatingGunSettings();
		FGSProjectileLaunch Launch;
		Launch.Start = GetPlayer()->GetActorLocation() - FVector(300.f, 0.f, 0.f);
		Launch.Direction = FVector(1.f, 0.f, 0.f);
		Launch.Speed = Settings->ProjectileSpeed;
		Launch.Lifetime = Settings->ProjectileLifetime;
		Launch.Radius = Settings->ProjectileRadius;
		Launch.Damage = 7.f;
		Launch.SourceASC = GetPlayerASC();
		Launch.DamageEffect = Settings->DamageEffect;
		Launch.Mesh = Settings->ProjectileMesh;
		S->HitsBefore = GetProjectiles()->GetNumHitTargets();
		S->ShotsBefore = GetProjectiles()->GetNumFired();
		GetProjectiles()->FireProjectile(Launch);
		// Keep the gun from shooting meanwhile: it sits in a safe zone.
		const TSubclassOf<UGameplayEffect> SafeZoneClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/BanditBaller/Abilities/GE_SafeZone.GE_SafeZone_C"));
		if (TestNotNull(TEXT("GE_SafeZone"), SafeZoneClass.Get()))
		{
			S->SafeZone = GetPlayerASC()->ApplyGameplayEffectToSelf(SafeZoneClass->GetDefaultObject<UGameplayEffect>(), 1.f, GetPlayerASC()->MakeEffectContext());
		}
	});
	WaitUntil(this, TEXT("the hand-fired projectile to land"), []() { return GetProjectiles()->GetNumActive() == 0; }, 10.0);
	Step([this, S, Health]()
	{
		TestEqual(TEXT("Through the player into the bandit: one target hit"), GetProjectiles()->GetNumHitTargets(), S->HitsBefore + 1);
		TestEqual(TEXT("... for its damage (7, Loaded Dice x2 = 14)"), S->BanditHealthBefore - Health(S->Bandit), 14.f, 0.01f);
		TestEqual(TEXT("Safe zone: the gun fired nothing meanwhile"), GetProjectiles()->GetNumFired(), S->ShotsBefore + 1);

		// Into a wall: stops there, the bandit behind it untouched.
		S->Wall = SpawnTestWall(FVector(300.f, 0.f, 150.f), FVector(0.2f, 3.f, 3.f));
		S->BanditHealthBefore = Health(S->Bandit);
		S->HitsBefore = GetProjectiles()->GetNumHitWorld();
		const UGSFloatingGunSettings* Settings = GetGameMode()->GetFloatingGunSettings();
		FGSProjectileLaunch Launch;
		Launch.Start = FVector(0.f, 0.f, 150.f);
		Launch.Direction = FVector(1.f, 0.f, 0.f);
		Launch.Speed = Settings->ProjectileSpeed;
		Launch.Lifetime = Settings->ProjectileLifetime;
		Launch.Radius = Settings->ProjectileRadius;
		Launch.Damage = 7.f;
		Launch.SourceASC = GetPlayerASC();
		Launch.DamageEffect = Settings->DamageEffect;
		GetProjectiles()->FireProjectile(Launch);
	});
	WaitUntil(this, TEXT("the projectile at the wall"), []() { return GetProjectiles()->GetNumActive() == 0; }, 10.0);
	Step([this, S, Health]()
	{
		TestEqual(TEXT("World geometry stops it"), GetProjectiles()->GetNumHitWorld(), S->HitsBefore + 1);
		TestEqual(TEXT("The bandit behind the wall is untouched"), Health(S->Bandit), S->BanditHealthBefore, 0.01f);
		if (S->Wall.IsValid())
		{
			S->Wall->Destroy();
		}

		// Straight up: nothing to hit, it expires.
		S->HitsBefore = GetProjectiles()->GetNumExpired();
		const UGSFloatingGunSettings* Settings = GetGameMode()->GetFloatingGunSettings();
		FGSProjectileLaunch Launch;
		Launch.Start = GetPlayer()->GetActorLocation() + FVector(0.f, 0.f, 300.f);
		Launch.Direction = FVector(0.f, 0.f, 1.f);
		Launch.Speed = Settings->ProjectileSpeed;
		Launch.Lifetime = Settings->ProjectileLifetime;
		Launch.SourceASC = GetPlayerASC();
		Launch.DamageEffect = Settings->DamageEffect;
		GetProjectiles()->FireProjectile(Launch);
		S->ShotsBefore = GetProjectiles()->GetNumCreated();
	});
	WaitUntil(this, TEXT("the projectile to expire"), []() { return GetProjectiles()->GetNumActive() == 0; }, 10.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Into the sky: it expires"), GetProjectiles()->GetNumExpired(), S->HitsBefore + 1);
		TestEqual(TEXT("Pooled: no projectile created beyond the prewarmed ones"), GetProjectiles()->GetNumCreated(), GetGameMode()->GetFloatingGunSettings()->ProjectilePoolPrewarm);
		TestTrue(TEXT("Safe zone: the gun has no target"), GetFloatingGun()->GetTarget() == nullptr);

		// Leave the safe zone. Kill credit: a weak bandit, Leech, a sure health orb, Powder Keg at 100%.
		GetPlayerASC()->RemoveActiveGameplayEffect(S->SafeZone);
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Leech") });
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("PowderKeg"), TEXT("34") });
		GetGameMode()->SetHealthOrbDropChanceOverride(1.f);
		ApplyDamage(GetPlayerASC(), 10.f);
		S->PlayerHealthBefore = GetHealth(GetPlayerASC());
		GetPlayerCombo()->EndCombo(EGSComboEndReason::Reset);
		S->KillsBefore = GetGameState()->GetRunStats()->GetTotalKills();
		S->XPOrbsBefore = GetCashSubsystem()->GetNumXPOrbsDropped();
		S->HealthOrbsBefore = GetCashSubsystem()->GetNumHealthOrbsDropped();
		S->BlastsBefore = GetGameMode()->GetPowderKegBlasts();
		S->Bandit = SpawnFrozenBandit(FVector(800.f, 0.f, 0.f));
		if (S->Bandit.IsValid())
		{
			S->Bandit->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 5.f);
		}
	});
	WaitUntil(this, TEXT("the gun to kill the bandit"), [S]() { return !S->Bandit.IsValid() || S->Bandit->GetHealthComponent()->IsDead(); }, 10.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Gun kill: counted in run stats"), GetGameState()->GetRunStats()->GetTotalKills(), S->KillsBefore + 1);
		TestEqual(TEXT("Gun kill: counts for the combo"), GetPlayerCombo()->GetComboCount(), 1);
		TestEqual(TEXT("Gun kill: an XP orb"), GetCashSubsystem()->GetNumXPOrbsDropped(), S->XPOrbsBefore + 1);
		TestEqual(TEXT("Gun kill: rolls the health orb chance (1: one orb)"), GetCashSubsystem()->GetNumHealthOrbsDropped(), S->HealthOrbsBefore + 1);
		TestTrue(TEXT("Gun kill: a cash drop"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) > 0);
		TestEqual(TEXT("Gun kill: Leech heals"), GetHealth(GetPlayerASC()), S->PlayerHealthBefore + 1.f, 0.01f);
		TestEqual(TEXT("Gun kill: Powder Keg explodes"), GetGameMode()->GetPowderKegBlasts(), S->BlastsBefore + 1);
		GetGameMode()->SetHealthOrbDropChanceOverride(0.f);

		// Paused: no shots (a tough bandit in range).
		RemoveAllEnemies(GetPIEWorld());
		S->Bandit = SpawnToughBandit(FVector(800.f, 0.f, 0.f));
	});
	WaitUntil(this, TEXT("shooting the next bandit"), [S]() { return GetFloatingGun()->GetTarget() == S->Bandit.Get(); }, 10.0);
	Step([this, S]()
	{
		S->ShotsBefore = GetFloatingGun()->GetShotsFired();
		UGameplayStatics::SetGamePaused(GetPIEWorld(), true);
	});
	WaitRealSeconds(this, 1.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Paused: no shots"), GetFloatingGun()->GetShotsFired(), S->ShotsBefore);
		UGameplayStatics::SetGamePaused(GetPIEWorld(), false);

		// The crosshair: both shoulders, hip and ADS, looking from steeply down to up.
		RemoveAllEnemies(GetPIEWorld());
		S->Config = 0;
	});
	for (int32 Config = 0; Config < 24; ++Config)
	{
		const float Pitch = TArray<float>{ -70.f, -50.f, -30.f, 0.f, 25.f, 50.f }[Config % 6];
		const bool bAim = (Config / 6) % 2 == 1;
		const bool bLeft = Config >= 12;
		Step([this, Pitch, bAim, bLeft]()
		{
			AGSCharacter* Player = GetPlayer();
			if ((Player->GetShoulderSide() < 0.f) != bLeft)
			{
				Player->SwapShoulderForDebug();
			}
			Player->SetAimingForDebug(bAim);
			GetPC()->SetControlRotation(FRotator(Pitch, 0.f, 0.f));
		});
		WaitWorldSeconds(this, 1.0); // the camera blends in, the gun follows and slides off the crosshair
		Step([this, S, Pitch, bAim, bLeft]()
		{
			const AGSFloatingGun* Gun = GetFloatingGun();
			const float Distance = Gun->GetCrosshairDistance();
			const float Radius = GetGameMode()->GetFloatingGunSettings()->CrosshairClearRadius;
			AddInfo(FString::Printf(TEXT("[gun] %s shoulder, %s, pitch %+.0f: %.3f of the screen height from the crosshair (clear radius %.2f), pushed %.0f cm"),
				bLeft ? TEXT("left") : TEXT("right"), bAim ? TEXT("ADS") : TEXT("hip"), Pitch, Distance, Radius, Gun->GetCrosshairAvoidOffset().Size()));
			if (Distance >= 0.f)
			{
				S->WorstDistance = FMath::Min(S->WorstDistance, Distance);
				TestTrue(FString::Printf(TEXT("Off the crosshair (%s, %s, pitch %+.0f): %.3f"), bLeft ? TEXT("left") : TEXT("right"), bAim ? TEXT("ADS") : TEXT("hip"), Pitch, Distance),
					Distance >= Radius * 0.9f);
			}
		});
	}
	Step([this, S]()
	{
		AddInfo(FString::Printf(TEXT("[gun] closest to the crosshair in any view: %.3f of the screen height"), S->WorstDistance));
		AGSCharacter* Player = GetPlayer();
		Player->SetAimingForDebug(false);
		if (Player->GetShoulderSide() < 0.f)
		{
			Player->SwapShoulderForDebug();
		}
		GetPC()->SetControlRotation(FRotator::ZeroRotator);

		// Dead: no shots.
		S->Bandit = SpawnToughBandit(FVector(800.f, 0.f, 0.f));
	});
	WaitUntil(this, TEXT("shooting before death"), [S]() { return GetFloatingGun()->GetTarget() == S->Bandit.Get(); }, 10.0);
	Step([this, S]()
	{
		ApplyDamage(GetPlayerASC(), 100000.f);
		TestTrue(TEXT("The player is dead"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		S->ShotsBefore = GetFloatingGun()->GetShotsFired();
	});
	WaitWorldSeconds(this, 2.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Dead: no shots"), GetFloatingGun()->GetShotsFired(), S->ShotsBefore);

		// A new run: level 0, the gun is gone.
		RemoveAllEnemies(GetPIEWorld());
		GetGameMode()->RestartRunWithSeed(38);
		TestEqual(TEXT("New run: FloatingGunLevel 0"), GetAttr(UGSAttributeSet::GetFloatingGunLevelAttribute()), 0.f, 0.001f);
		TestEqual(TEXT("New run: no projectiles in flight"), GetProjectiles()->GetNumActive(), 0);
	});
	WaitFrames(this, 2);
	Step([this]()
	{
		TestEqual(TEXT("New run: no gun"), GetPlayer()->GetFloatingGuns()->GetGuns().Num(), 0);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Floating Gun, part 2 (Prompt 39): the second gun, separate targets, the level 3+ bonus and the card text
// ---------------------------------------------------------------------------

namespace GSTests
{
	AGSFloatingGun* GetFloatingGunAt(int32 Index)
	{
		const AGSCharacter* Player = GetPlayer();
		const UGSFloatingGunComponent* Guns = Player ? Player->GetFloatingGuns() : nullptr;
		return Guns ? Guns->GetGun(Index) : nullptr;
	}

	int32 CountLiveFloatingGuns()
	{
		int32 Count = 0;
		for (TActorIterator<AGSFloatingGun> It(GetPIEWorld()); It; ++It)
		{
			Count += IsValid(*It) ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSFloatingGunLevelsTest, "BanditBaller.Upgrades.FloatingGunLevels", GSTests::TestFlags)
bool FGSFloatingGunLevelsTest::RunTest(const FString& Parameters)
{
	struct FLevelsState
	{
		TWeakObjectPtr<AGSEnemyCharacter> A;
		TWeakObjectPtr<AGSEnemyCharacter> B;
		float HealthA = 0.f;
		float HealthB = 0.f;
		int32 Shots[2] = { 0, 0 };
		int32 HitsBefore = 0;
		double WindowStart = 0.0;
		double WindowEnd = 0.0;
		double GrantAt = 0.0;
		bool bGranted = false;
		int32 ScheduleChecks = 0;
		int32 ScheduleMisses = 0;
		bool bRetimed[2] = { false, false };
	};
	TSharedRef<FLevelsState> S = MakeShared<FLevelsState>();
	auto Now = []() { return GetPIEWorld() ? GetPIEWorld()->GetTimeSeconds() : 0.0; };
	auto Health = [](const TWeakObjectPtr<AGSEnemyCharacter>& Bandit) { return Bandit.IsValid() ? GetHealth(Bandit->GetAbilitySystemComponent()) : -1.f; };
	const FVector Home(0.f, 0.f, 100.f);

	BeginPIE(this);

	// Data, the level rules and the card text for each next pick.
	Step([this, Home]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		GetPlayer()->TeleportTo(Home, FRotator::ZeroRotator);
		GetPC()->SetControlRotation(FRotator::ZeroRotator);

		const UGSFloatingGunSettings* Settings = GameMode->GetFloatingGunSettings();
		if (!TestNotNull(TEXT("DA_FloatingGunSettings on the GameMode"), Settings))
		{
			return;
		}
		TestEqual(TEXT("+10% damage per point of power"), Settings->DamageBonusPerPower, 0.1f, 0.0001f);
		TestEqual(TEXT("+10% fire rate per point of power"), Settings->FireRateBonusPerPower, 0.1f, 0.0001f);
		const int32 Guns[] = { 0, 1, 2, 2, 2, 2, 2 };
		for (int32 Picks = 0; Picks <= 6; ++Picks)
		{
			TestEqual(FString::Printf(TEXT("%d pick(s): %d gun(s)"), Picks, Guns[Picks]), UGSFloatingGunSettings::GetGunCount(Picks), Guns[Picks]);
		}
		TestEqual(TEXT("Power 0: no bonus"), Settings->GetDamageMultiplier(0.f), 1.f, 0.0001f);
		TestEqual(TEXT("Power 1: x1.1 damage"), Settings->GetDamageMultiplier(1.f), 1.1f, 0.0001f);
		TestEqual(TEXT("Power 1.5 (a Rare pick): x1.15 damage"), Settings->GetDamageMultiplier(1.5f), 1.15f, 0.0001f);
		TestEqual(TEXT("Power 3: x1.3 damage (added up, not compounded)"), Settings->GetDamageMultiplier(3.f), 1.3f, 0.0001f);
		TestEqual(TEXT("Power 3: x1.3 fire rate"), Settings->GetFireRateMultiplier(3.f), 1.3f, 0.0001f);
		TestEqual(TEXT("Power 3: 1.95 shots a second"), 1.0 / Settings->GetShotInterval(3.f), 1.95, 0.0001);

		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("FloatingGun"));
		if (!TestNotNull(TEXT("DA_Upgrade_FloatingGun"), Upgrade))
		{
			return;
		}
		TestEqual(TEXT("Three card texts (the last repeats)"), Upgrade->DescriptionsByOwnedStacks.Num(), 3);
		const FString Text0 = Upgrade->GetFormattedDescription(0).ToString();
		const FString Text1 = Upgrade->GetFormattedDescription(1).ToString();
		const FString Text2 = Upgrade->GetFormattedDescription(2).ToString();
		const FString Text9 = Upgrade->GetFormattedDescription(9).ToString();
		AddInfo(FString::Printf(TEXT("[gun] cards: \"%s\" / \"%s\" / \"%s\""), *Text0, *Text1, *Text2));
		TestTrue(TEXT("None owned: \"Adds a floating gun\""), Text0.StartsWith(TEXT("Adds a floating gun")));
		TestTrue(TEXT("One owned: \"Adds a second floating gun\""), Text1.StartsWith(TEXT("Adds a second floating gun")));
		TestFalse(TEXT("Two owned: a text without numbers (the pick line has them)"), Text2.IsEmpty() || Text2.Contains(TEXT("%")));
		// The pick line shows power x DisplayScale as the bonus, so DisplayScale must be the bonus per power in percent.
		TestEqual(TEXT("DisplayScale = DamageBonusPerPower in percent"), Upgrade->GetDisplayScale(), Settings->DamageBonusPerPower * 100.f, 0.0001f);
		const FString Bonus = FString::Printf(TEXT("+%.0f%% floating gun damage and fire rate"), Settings->DamageBonusPerPower * 100.f);
		TestEqual(TEXT("One point of power reads +10% (matches DA_FloatingGunSettings)"), Upgrade->FormatValueText(1.f).ToString(), Bonus);
		TestEqual(TEXT("Damage and fire rate bonus are the same (one number on the card)"), Settings->FireRateBonusPerPower, Settings->DamageBonusPerPower, 0.0001f);
		TestEqual(TEXT("Nine owned: the last text again"), Text9, Text2);

		// The real card: the description for the next pick, the pick line only when the pick adds power.
		const TSubclassOf<UGSUpgradeCardWidget> CardClass = LoadClass<UGSUpgradeCardWidget>(nullptr, TEXT("/Game/BanditBaller/UI/WBP_UpgradeCard.WBP_UpgradeCard_C"));
		UGSUpgradeCardWidget* Card = CardClass ? CreateWidget<UGSUpgradeCardWidget>(GetPC(), CardClass) : nullptr;
		if (TestNotNull(TEXT("WBP_UpgradeCard"), Card))
		{
			const UGSUpgradePool* Pool = GameMode->GetUpgradePool();
			const FString Texts[] = { Text0, Text1, Text2, Text2 };
			for (int32 Owned = 0; Owned < 4; ++Owned)
			{
				const float Common = Pool->GetPickValue(Upgrade, EGSUpgradeRarity::Common, Owned);
				Card->SetChoice(Upgrade, EGSUpgradeRarity::Common, Common, Owned, FMath::Max(Owned - 2, 0), 0);
				TestEqual(FString::Printf(TEXT("Card with %d owned: description"), Owned), Card->GetDescriptionText().ToString(), Texts[Owned]);
				TestEqual(FString::Printf(TEXT("Card with %d owned: Common pick line"), Owned), Card->GetPickText().ToString(), Owned < 2 ? FString() : Bonus);
				TestEqual(FString::Printf(TEXT("Card with %d owned: category"), Owned), Card->GetCategoryText().ToString(), FString(Owned == 0 ? TEXT("NEW WEAPON") : TEXT("WEAPON UPGRADE")));
			}
			Card->SetChoice(Upgrade, EGSUpgradeRarity::Rare, Pool->GetPickValue(Upgrade, EGSUpgradeRarity::Rare, 0), 0, 0.f, 0);
			TestEqual(TEXT("A Rare first pick: the gun and +5%"), Card->GetPickText().ToString(), FString(TEXT("+5% floating gun damage and fire rate")));
			TestEqual(TEXT("Nothing owned: Now: none"), Card->GetNowText().ToString(), FString(TEXT("Now: none")));
		}

		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingGun") });
	});
	WaitWorldSeconds(this, 0.5);

	// Level 1: one gun. Level 2: a second one over the left shoulder, at the same height.
	Step([this]()
	{
		TestEqual(TEXT("Level 1: one gun"), GetPlayer()->GetFloatingGuns()->GetGuns().Num(), 1);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingGun") });
	});
	WaitWorldSeconds(this, 0.5);
	Step([this, S]()
	{
		TestEqual(TEXT("Level 2"), GetAttr(UGSAttributeSet::GetFloatingGunLevelAttribute()), 2.f, 0.001f);
		TestEqual(TEXT("Level 2: two guns"), GetPlayer()->GetFloatingGuns()->GetGuns().Num(), 2);
		const AGSFloatingGun* Right = GetFloatingGunAt(0);
		const AGSFloatingGun* Left = GetFloatingGunAt(1);
		if (!TestNotNull(TEXT("Right gun"), Right) || !TestNotNull(TEXT("Left gun"), Left))
		{
			return;
		}
		TestEqual(TEXT("The first gun stays on the right"), Right->GetSide(), 1.f);
		TestEqual(TEXT("The second gun is on the left"), Left->GetSide(), -1.f);
		const FVector Center = GetPlayer()->GetActorLocation();
		const float HeadTop = Center.Z + GetPlayer()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		AddInfo(FString::Printf(TEXT("[gun] rest offsets: right %s, left %s"), *(Right->GetRestLocation() - Center).ToCompactString(),
			*(Left->GetRestLocation() - Center).ToCompactString()));
		TestTrue(TEXT("Left gun: to the left of the player (camera yaw 0: -Y)"), Left->GetRestLocation().Y - Center.Y < -30.f);
		TestEqual(TEXT("Left gun: at the same height as the right one"), Left->GetRestLocation().Z, Right->GetRestLocation().Z, 0.5);
		TestTrue(TEXT("Left gun: above the head"), Left->GetRestLocation().Z > HeadTop);
		TestEqual(TEXT("Left gun: mirrored (same distance out)"), Left->GetRestLocation().Y - Center.Y, -(Right->GetRestLocation().Y - Center.Y), 0.5);
		TestEqual(TEXT("Left gun: no collision"), static_cast<int32>(Left->GetMesh()->GetCollisionEnabled()), static_cast<int32>(ECollisionEnabled::NoCollision));
		TestEqual(TEXT("Level 2: no bonus yet"), Left->GetDamageMultiplier(), 1.f, 0.0001f);
		TestEqual(TEXT("Two Common picks give the two guns and no power"), GetAttr(UGSAttributeSet::GetFloatingGunPowerAttribute()), 0.f, 0.0001f);

		// One enemy in range: both guns shoot it.
		S->A = SpawnToughBandit(FVector(1000.f, 0.f, 0.f));
	});
	WaitUntil(this, TEXT("both guns shooting"), []()
	{
		return GetFloatingGunAt(0)->GetShotsFired() > 0 && GetFloatingGunAt(1)->GetShotsFired() > 0;
	}, 10.0);
	Step([this, S]()
	{
		TestTrue(TEXT("One enemy: the right gun shoots it"), GetFloatingGunAt(0)->GetTarget() == S->A.Get());
		TestTrue(TEXT("One enemy: the left gun shoots it too"), GetFloatingGunAt(1)->GetTarget() == S->A.Get());

		// A second enemy in range: the guns split up.
		S->B = SpawnToughBandit(FVector(1000.f, 400.f, 0.f));
	});
	WaitWorldSeconds(this, 0.5);
	Step([this, S, Now, Health]()
	{
		const AGSEnemyCharacter* TargetRight = GetFloatingGunAt(0)->GetTarget();
		const AGSEnemyCharacter* TargetLeft = GetFloatingGunAt(1)->GetTarget();
		AddInfo(FString::Printf(TEXT("[gun] two enemies: right gun -> %s, left gun -> %s"), *GetNameSafe(TargetRight), *GetNameSafe(TargetLeft)));
		TestTrue(TEXT("Two enemies: different targets"), TargetRight && TargetLeft && TargetRight != TargetLeft);
		TestTrue(TEXT("Two enemies: both are shot at"), (TargetRight == S->A.Get() || TargetRight == S->B.Get()) && (TargetLeft == S->A.Get() || TargetLeft == S->B.Get()));
		// Each gun alone: one gun's damage per second at level 2.
		S->HealthA = Health(S->A);
		S->HealthB = Health(S->B);
		S->Shots[0] = GetFloatingGunAt(0)->GetShotsFired();
		S->Shots[1] = GetFloatingGunAt(1)->GetShotsFired();
		S->WindowStart = Now();
	});
	WaitWorldSeconds(this, 4.0);
	Step([this, S, Now, Health]()
	{
		const double Seconds = Now() - S->WindowStart;
		const float DpsA = (S->HealthA - Health(S->A)) / Seconds;
		const float DpsB = (S->HealthB - Health(S->B)) / Seconds;
		AddInfo(FString::Printf(TEXT("[gun] level 2, one gun per bandit over %.2f s: %.1f and %.1f damage per second (%d and %d shots); expected 8 x 1.5 = 12"),
			Seconds, DpsA, DpsB, GetFloatingGunAt(0)->GetShotsFired() - S->Shots[0], GetFloatingGunAt(1)->GetShotsFired() - S->Shots[1]));
		TestTrue(TEXT("Still on different targets"), GetFloatingGunAt(0)->GetTarget() != GetFloatingGunAt(1)->GetTarget());
		TestTrue(TEXT("Bandit A takes one gun's damage (about 12 a second)"), DpsA > 8.f && DpsA < 16.f);
		TestTrue(TEXT("Bandit B takes one gun's damage (about 12 a second)"), DpsB > 8.f && DpsB < 16.f);

		// B dies: both guns go back to A.
		ApplyDamage(S->B->GetAbilitySystemComponent(), 1000000.f);
	});
	WaitWorldSeconds(this, 0.5);
	Step([this, S, Now]()
	{
		TestTrue(TEXT("B dead: the right gun shoots A"), GetFloatingGunAt(0)->GetTarget() == S->A.Get());
		TestTrue(TEXT("B dead: the left gun shoots A"), GetFloatingGunAt(1)->GetTarget() == S->A.Get());
		// The schedule through a level pick: 3 more levels (to 5) 1 s into a 3 s window.
		S->WindowEnd = Now() + 3.0;
		S->GrantAt = Now() + 1.0;
	});
	WaitUntil(this, TEXT("both guns on schedule through the level 5 pick"), [this, S, Now]()
	{
		for (int32 Index = 0; Index < 2; ++Index)
		{
			const AGSFloatingGun* Gun = GetFloatingGunAt(Index);
			const int32 Expected = 1 + FMath::FloorToInt((Now() - Gun->GetBurstStartTime()) / Gun->GetBurstInterval() + 1e-6);
			++S->ScheduleChecks;
			if (Gun->GetBurstShots() != Expected)
			{
				++S->ScheduleMisses;
				AddError(FString::Printf(TEXT("Gun %d schedule: %d shots at %.3f s into the burst (interval %.4f), expected %d"), Index,
					Gun->GetBurstShots(), Now() - Gun->GetBurstStartTime(), Gun->GetBurstInterval(), Expected));
			}
			if (S->bGranted && FMath::IsNearlyEqual(Gun->GetBurstInterval(), GetGameMode()->GetFloatingGunSettings()->GetShotInterval(3.f), 1e-9))
			{
				S->bRetimed[Index] = true;
			}
		}
		if (!S->bGranted && Now() >= S->GrantAt)
		{
			UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingGun"), TEXT("3") });
			S->bGranted = true;
		}
		return Now() >= S->WindowEnd;
	}, 30.0);
	Step([this, S, Now, Health]()
	{
		TestEqual(TEXT("Level 5"), GetAttr(UGSAttributeSet::GetFloatingGunLevelAttribute()), 5.f, 0.001f);
		TestEqual(TEXT("Level 5: power 3 (three Common picks after the guns)"), GetAttr(UGSAttributeSet::GetFloatingGunPowerAttribute()), 3.f, 0.0001f);
		TestEqual(TEXT("Level 5: still two guns (never more)"), GetPlayer()->GetFloatingGuns()->GetGuns().Num(), 2);
		TestEqual(TEXT("Level 5: two gun actors in the world"), CountLiveFloatingGuns(), 2);
		TestEqual(TEXT("Every frame on schedule, before and after the pick"), S->ScheduleMisses, 0);
		AddInfo(FString::Printf(TEXT("[gun] %d gun-frames checked against the schedule"), S->ScheduleChecks));
		for (int32 Index = 0; Index < 2; ++Index)
		{
			const AGSFloatingGun* Gun = GetFloatingGunAt(Index);
			TestTrue(FString::Printf(TEXT("Gun %d: the schedule switched to the level 5 interval"), Index), S->bRetimed[Index]);
			TestEqual(FString::Printf(TEXT("Gun %d: 1.95 shots a second"), Index), 1.0 / Gun->GetShotInterval(), 1.95, 0.0001);
			TestEqual(FString::Printf(TEXT("Gun %d: x1.3 damage"), Index), Gun->GetDamageMultiplier(), 1.3f, 0.0001f);
		}
		// Damage at level 5: every hit 8 x 1.3 = 10.4 (Damage 1, no crits).
		S->HealthA = Health(S->A);
		S->HitsBefore = GetProjectiles()->GetNumHitTargets();
		S->WindowStart = Now();
	});
	WaitWorldSeconds(this, 4.0);
	Step([this, S, Now, Health]()
	{
		const double Seconds = Now() - S->WindowStart;
		const int32 Hits = GetProjectiles()->GetNumHitTargets() - S->HitsBefore;
		const float Dealt = S->HealthA - Health(S->A);
		const float Dps = Dealt / Seconds;
		TestTrue(TEXT("Level 5: both guns hit"), Hits >= 12);
		TestEqual(TEXT("Level 5: 10.4 per hit (8 x 1.3)"), Dealt, Hits * 10.4f, 0.01f * FMath::Max(Hits, 1));
		TestTrue(TEXT("Level 5: about 40.6 damage a second from both guns"), Dps > 34.f && Dps < 47.f);

		// Next to the rifle (held fire, Damage 1).
		const UGSWeaponComponent* Weapons = GetPlayer()->GetWeaponComponent();
		const UGSWeaponData* Rifle = Weapons ? Weapons->GetCurrentWeaponData() : nullptr;
		if (TestNotNull(TEXT("The rifle"), Rifle))
		{
			const float RifleBurst = Rifle->Damage * Rifle->FireRateRPM / 60.f;
			const float RifleSustained = Rifle->Damage * Rifle->MagazineSize / (Rifle->MagazineSize * Rifle->GetRefireInterval(1.f) + Rifle->ReloadTime);
			const UGSFloatingGunSettings* Settings = GetGameMode()->GetFloatingGunSettings();
			AddInfo(FString::Printf(TEXT("[gun] damage per second at Damage 1, no crits: one gun level 1 %.1f, level 5 %.1f (measured %.1f per gun over %.2f s, %d hits); rifle %.0f while firing, %.0f with reloads"),
				Settings->Damage / Settings->GetShotInterval(0.f), Settings->Damage * Settings->GetDamageMultiplier(3.f) / Settings->GetShotInterval(3.f),
				Dps * 0.5f, Seconds, Hits, RifleBurst, RifleSustained));
		}

		// A new run removes the guns.
		RemoveAllEnemies(GetPIEWorld());
		GetGameMode()->RestartRunWithSeed(39);
		TestEqual(TEXT("New run: FloatingGunLevel 0"), GetAttr(UGSAttributeSet::GetFloatingGunLevelAttribute()), 0.f, 0.001f);
	});
	WaitFrames(this, 2);
	Step([this]()
	{
		TestEqual(TEXT("New run: no guns"), GetPlayer()->GetFloatingGuns()->GetGuns().Num(), 0);
		TestEqual(TEXT("New run: no gun actors left"), CountLiveFloatingGuns(), 0);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}
// ---------------------------------------------------------------------------
// Upgrade picks (M2c, Prompt 41): category data, a value per pick by rarity, totals on one effect, the weapon-pick rule
// ---------------------------------------------------------------------------

namespace GSTests
{
	/** What one upgrade's data says, and what a Common pick gave before Prompt 41 (one GE stack of BaseValue). */
	struct FPickExpect
	{
		const TCHAR* Name;
		EGSUpgradeCategory Category;
		bool bLegendary;
		bool bWhole;
		float BaseValue; // = the old per-stack value
		bool bPercent;
		FGameplayAttribute Attribute;
	};

	TArray<FPickExpect> GetPickExpectations()
	{
		using A = UGSAttributeSet;
		const EGSUpgradeCategory Stat = EGSUpgradeCategory::StatBoost;
		const EGSUpgradeCategory Ability = EGSUpgradeCategory::Ability;
		return {
			{ TEXT("Damage"), Stat, false, false, 0.10f, true, A::GetDamageAttribute() },
			{ TEXT("FireRate"), Stat, false, false, 0.08f, true, A::GetFireRateAttribute() },
			{ TEXT("ReloadSpeed"), Stat, false, false, 0.10f, true, A::GetReloadSpeedAttribute() },
			{ TEXT("MoveSpeed"), Stat, false, false, 0.08f, true, A::GetMoveSpeedAttribute() },
			{ TEXT("Magnet"), Stat, false, false, 0.15f, true, A::GetPickupRadiusAttribute() },
			{ TEXT("BiggerMags"), Stat, false, false, 0.15f, true, A::GetMagazineSizeMultiplierAttribute() },
			{ TEXT("LoadedDice"), Stat, false, false, 0.05f, false, A::GetCritChanceAttribute() },
			{ TEXT("ExtraJump"), Ability, false, true, 1.f, false, A::GetExtraJumpsAttribute() },
			{ TEXT("DashCharge"), Ability, false, true, 1.f, false, A::GetExtraDashChargesAttribute() },
			{ TEXT("FloatingShield"), Ability, false, true, 1.f, false, A::GetShieldsAttribute() },
			{ TEXT("PiercingRounds"), Ability, false, true, 1.f, false, A::GetPierceAttribute() },
			{ TEXT("Leech"), Ability, false, true, 1.f, false, A::GetHealthOnKillAttribute() },
			{ TEXT("PowderKeg"), Ability, false, false, 1.f, false, A::GetPowderKegAttribute() },
			{ TEXT("SecondWind"), Ability, true, false, 1.f, false, A::GetSecondWindsAttribute() },
			{ TEXT("FloatingGun"), EGSUpgradeCategory::Weapon, false, false, 1.f, false, A::GetFloatingGunPowerAttribute() },
			{ TEXT("RazorCards"), EGSUpgradeCategory::Weapon, false, false, 1.f, false, A::GetRazorCardsPowerAttribute() },
			{ TEXT("BarbedWire"), EGSUpgradeCategory::Weapon, false, false, 1.f, false, A::GetBarbedWirePowerAttribute() },
			{ TEXT("Dynamite"), EGSUpgradeCategory::Weapon, false, false, 1.f, false, A::GetDynamitePowerAttribute() },
			{ TEXT("LuckyCoin"), EGSUpgradeCategory::Weapon, false, false, 1.f, false, A::GetLuckyCoinPowerAttribute() },
			{ TEXT("Molotov"), EGSUpgradeCategory::Weapon, false, false, 1.f, false, A::GetMolotovPowerAttribute() },
			{ TEXT("Tomahawk"), EGSUpgradeCategory::Weapon, false, false, 1.f, false, A::GetTomahawkPowerAttribute() },
		};
	}

	/** The attribute after a total value: base x (1 + total) for percent upgrades, base + total otherwise. */
	float ExpectedAfter(const FPickExpect& E, float Base, float Total)
	{
		return E.bPercent ? Base * (1.f + Total) : Base + Total;
	}

	const float PickMultipliers[] = { 1.f, 1.5f, 2.f, 3.f };
	const float PickWholeUnits[] = { 1.f, 0.f, 2.f, 3.f };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSUpgradePickValuesTest, "BanditBaller.Upgrades.PickValues", GSTests::TestFlags)
bool FGSUpgradePickValuesTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	// Data: the pool's multipliers, every upgrade's category and Common value.
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		const UGSUpgradePool* Pool = GameMode->GetUpgradePool();
		if (!TestNotNull(TEXT("DA_UpgradePool"), Pool))
		{
			return;
		}
		for (const EGSUpgradeRarity Rarity : UGSUpgradePool::GetAllRarities())
		{
			const int32 R = static_cast<int32>(Rarity);
			TestEqual(FString::Printf(TEXT("%s multiplier"), *UGSUpgradeData::GetRarityName(Rarity)), Pool->GetRarityValueMultiplier(Rarity), PickMultipliers[R], 0.0001f);
			const float* Whole = Pool->WholeUnitValues.Find(Rarity);
			TestEqual(FString::Printf(TEXT("%s whole-unit value"), *UGSUpgradeData::GetRarityName(Rarity)), Whole ? *Whole : 0.f, PickWholeUnits[R], 0.0001f);
		}

		TArray<const UGSUpgradeData*> Seen;
		for (const GSTests::FPickExpect& E : GSTests::GetPickExpectations())
		{
			const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(E.Name);
			if (!TestNotNull(FString::Printf(TEXT("DA_Upgrade_%s"), E.Name), Upgrade))
			{
				continue;
			}
			Seen.Add(Upgrade);
			TestTrue(FString::Printf(TEXT("%s: category"), E.Name), Upgrade->Category == E.Category);
			TestEqual(FString::Printf(TEXT("%s: Legendary ability"), E.Name), Upgrade->bLegendaryAbility, E.bLegendary);
			TestEqual(FString::Printf(TEXT("%s: whole units"), E.Name), Upgrade->bWholeUnits, E.bWhole);
			TestEqual(FString::Printf(TEXT("%s: BaseValue (one Common pick = one pick before Prompt 41)"), E.Name), Upgrade->BaseValue, E.BaseValue, 0.0001f);
			TestEqual(FString::Printf(TEXT("%s: AppearanceWeight 1"), E.Name), Upgrade->AppearanceWeight, 1.f, 0.0001f);
			TestEqual(FString::Printf(TEXT("%s: percent upgrade"), E.Name), Upgrade->IsPercentUpgrade(), E.bPercent);
			TestTrue(FString::Printf(TEXT("%s: in DA_UpgradePool"), E.Name), Pool->Upgrades.Contains(Upgrade));
		}
		TestEqual(TEXT("All 21 upgrades checked"), Seen.Num(), 21);
		TestEqual(TEXT("Every pool upgrade checked"), Pool->Upgrades.Num(), Seen.Num());
		const UGSUpgradeData* Gun = UGSDebugLibrary::FindUpgradeData(TEXT("FloatingGun"));
		TestEqual(TEXT("Floating Gun: its first two picks give the guns"), Gun ? Gun->WeaponGivingPicks : 0, 2);

		// Card numbers are unchanged (they show BaseValue).
		const UGSUpgradeData* Damage = UGSDebugLibrary::FindUpgradeData(TEXT("Damage"));
		TestEqual(TEXT("Damage card: 10 per pick"), Damage ? Damage->GetPerStackValue() : 0.f, 10.f, 0.001f);
		TestEqual(TEXT("FormatValue: +25% damage"), Damage ? Damage->FormatValue(0.25f) : FString(), FString(TEXT("+25% damage")));

		// What every upgrade's card line reads for a Common pick and for its Legendary pick (plurals included).
		struct FText2 { const TCHAR* Name; const TCHAR* Common; const TCHAR* Legendary; };
		for (const FText2& E : {
			FText2{ TEXT("Damage"), TEXT("+10% damage"), TEXT("+30% damage") },
			FText2{ TEXT("FireRate"), TEXT("+8% fire rate"), TEXT("+24% fire rate") },
			FText2{ TEXT("ReloadSpeed"), TEXT("+10% reload speed"), TEXT("+30% reload speed") },
			FText2{ TEXT("MoveSpeed"), TEXT("+8% move speed"), TEXT("+24% move speed") },
			FText2{ TEXT("Magnet"), TEXT("+15% pickup radius"), TEXT("+45% pickup radius") },
			FText2{ TEXT("BiggerMags"), TEXT("+15% magazine size"), TEXT("+45% magazine size") },
			FText2{ TEXT("LoadedDice"), TEXT("+5% critical hit chance"), TEXT("+15% critical hit chance") },
			FText2{ TEXT("ExtraJump"), TEXT("+1 air jump"), TEXT("+3 air jumps") },
			FText2{ TEXT("DashCharge"), TEXT("+1 dash charge"), TEXT("+3 dash charges") },
			FText2{ TEXT("FloatingShield"), TEXT("+1 floating shield"), TEXT("+3 floating shields") },
			FText2{ TEXT("PiercingRounds"), TEXT("+1 enemy pierced per shot"), TEXT("+3 enemies pierced per shot") },
			FText2{ TEXT("Leech"), TEXT("+1 health per kill"), TEXT("+3 health per kill") },
			FText2{ TEXT("PowderKeg"), TEXT("+3% chance that a kill explodes"), TEXT("+9% chance that a kill explodes") },
			FText2{ TEXT("SecondWind"), TEXT("+1 Second Wind"), TEXT("+1 Second Wind") },
			FText2{ TEXT("FloatingGun"), TEXT("+10% floating gun damage and fire rate"), TEXT("+30% floating gun damage and fire rate") },
			FText2{ TEXT("RazorCards"), TEXT("+10% Razor Cards damage and fire rate"), TEXT("+30% Razor Cards damage and fire rate") },
			FText2{ TEXT("BarbedWire"), TEXT("+10% Barbed Wire damage, +5% size"), TEXT("+30% Barbed Wire damage, +15% size") },
			FText2{ TEXT("Dynamite"), TEXT("+10% Dynamite damage, +5% blast size"), TEXT("+30% Dynamite damage, +15% blast size") },
			FText2{ TEXT("LuckyCoin"), TEXT("+10% Lucky Coin damage"), TEXT("+30% Lucky Coin damage") },
			FText2{ TEXT("Molotov"), TEXT("+10% Molotov burn damage, +5% patch size"), TEXT("+30% Molotov burn damage, +15% patch size") },
			FText2{ TEXT("Tomahawk"), TEXT("+10% Tomahawk damage"), TEXT("+30% Tomahawk damage") } })
		{
			const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(E.Name);
			if (!Upgrade)
			{
				continue;
			}
			// Picks after the weapon's guns, so the Floating Gun line shows its full multiplier.
			const int32 Before = Upgrade->Category == EGSUpgradeCategory::Weapon ? Upgrade->WeaponGivingPicks : 0;
			TestEqual(FString::Printf(TEXT("%s Common card line"), E.Name), Upgrade->FormatValueText(Pool->GetPickValue(Upgrade, EGSUpgradeRarity::Common, Before)).ToString(), FString(E.Common));
			TestEqual(FString::Printf(TEXT("%s Legendary card line"), E.Name), Upgrade->FormatValueText(Pool->GetPickValue(Upgrade, EGSUpgradeRarity::Legendary, Before)).ToString(), FString(E.Legendary));
			TestFalse(FString::Printf(TEXT("%s description has no numbers"), E.Name),
				Upgrade->GetFormattedDescription(Before).ToString().Contains(TEXT("%")) || Upgrade->GetFormattedDescription(Before).ToString().Contains(TEXT("+")));
		}
		const UGSUpgradeData* Magnet = UGSDebugLibrary::FindUpgradeData(TEXT("Magnet"));
		TestEqual(TEXT("A Rare Magnet pick: +22.5%"), Magnet ? Magnet->FormatValueText(0.225f).ToString() : FString(), FString(TEXT("+22.5% pickup radius")));
		EGSUpgradeRarity Parsed = EGSUpgradeRarity::Common;
		TestTrue(TEXT("ParseRarity epic"), UGSUpgradeData::ParseRarity(TEXT("epic"), Parsed) && Parsed == EGSUpgradeRarity::Epic);
		TestTrue(TEXT("ParseRarity L"), UGSUpgradeData::ParseRarity(TEXT("L"), Parsed) && Parsed == EGSUpgradeRarity::Legendary);
		TestFalse(TEXT("ParseRarity Mythic"), UGSUpgradeData::ParseRarity(TEXT("Mythic"), Parsed));
	});

	// One pick of every rarity for each stat boost and ability: the values add up on one effect.
	Step([this]()
	{
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		for (const GSTests::FPickExpect& E : GSTests::GetPickExpectations())
		{
			const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(E.Name);
			if (!Upgrade || E.bLegendary || E.Category == EGSUpgradeCategory::Weapon)
			{
				continue;
			}
			const float Base = GetAttr(E.Attribute);
			const float Tolerance = FMath::Max(0.001f, Base * 0.0001f);
			float Total = 0.f;
			int32 Picks = 0;
			for (const EGSUpgradeRarity Rarity : UGSUpgradePool::GetAllRarities())
			{
				const int32 R = static_cast<int32>(Rarity);
				const FString What = FString::Printf(TEXT("%s %s pick"), E.Name, *UGSUpgradeData::GetRarityName(Rarity));
				if (E.bWhole && Rarity == EGSUpgradeRarity::Rare)
				{
					AddExpectedMessage(TEXT("has no Rare version"), EAutomationExpectedErrorFlags::Contains, 1);
					TestFalse(What + TEXT(": refused (whole units have no Rare)"), Upgrades->GrantUpgrade(Upgrade, Rarity, TEXT("Test")));
					TestEqual(What + TEXT(": attribute unchanged"), GetAttr(E.Attribute), ExpectedAfter(E, Base, Total), Tolerance);
					continue;
				}
				const float Value = E.BaseValue * (E.bWhole ? PickWholeUnits[R] : PickMultipliers[R]);
				TestTrue(What + TEXT(": granted"), Upgrades->GrantUpgrade(Upgrade, Rarity, TEXT("Test")));
				Total += Value;
				++Picks;
				TestEqual(What + TEXT(": attribute"), GetAttr(E.Attribute), ExpectedAfter(E, Base, Total), Tolerance);
				if (Rarity == EGSUpgradeRarity::Common)
				{
					// The rule for this prompt: a Common pick gives exactly what a pick gave before.
					TestEqual(What + TEXT(": the same as one pick before Prompt 41"), GetAttr(E.Attribute),
						E.bPercent ? Base * (1.f + E.BaseValue) : Base + E.BaseValue, Tolerance);
				}
			}
			TestEqual(FString::Printf(TEXT("%s: picks"), E.Name), Upgrades->GetStacks(Upgrade), Picks);
			TestEqual(FString::Printf(TEXT("%s: total value"), E.Name), Upgrades->GetTotalValue(Upgrade), Total, 0.0001f);
			TestEqual(FString::Printf(TEXT("%s: one active effect"), E.Name), GetActiveUpgradeEffectCount(Upgrade), 1);
			const FGSUpgradeStack* Entry = Upgrades->FindEntry(Upgrade);
			TestEqual(FString::Printf(TEXT("%s: every pick recorded"), E.Name), Entry ? Entry->Picks.Num() : -1, Picks);
		}

		// What the totals mean in play.
		const AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		TestEqual(TEXT("Floating shields follow (1 + 2 + 3)"), PS->GetShieldComponent()->GetNumShields(), 6);
		TestEqual(TEXT("Air jumps follow"), GetPlayer()->JumpMaxCount, 7);
		TestEqual(TEXT("Powder Keg 7.5 stacks: 22.5% chance"), GetGameMode()->GetPowderKegChance(PS), 0.225f, 0.0001f);
		TestEqual(TEXT("Loaded Dice: 5 + 7.5 + 10 + 15 = 37.5%"), GetAttr(UGSAttributeSet::GetCritChanceAttribute()), 0.375f, 0.0001f);

		// The console: gs.Upgrade.Grant <Name> [Count] [Rarity].
		const UGSUpgradeData* Damage = UGSDebugLibrary::FindUpgradeData(TEXT("Damage"));
		const float DamageBefore = GetAttr(UGSAttributeSet::GetDamageAttribute());
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Damage"), TEXT("2"), TEXT("Epic") });
		TestEqual(TEXT("gs.Upgrade.Grant Damage 2 Epic: +40%"), GetAttr(UGSAttributeSet::GetDamageAttribute()), DamageBefore + 0.4f, 0.0001f);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Damage"), TEXT("Legendary") });
		TestEqual(TEXT("gs.Upgrade.Grant Damage Legendary: +30% more"), GetAttr(UGSAttributeSet::GetDamageAttribute()), DamageBefore + 0.7f, 0.0001f);
		TestEqual(TEXT("Damage picks"), Upgrades->GetStacks(Damage), 7);
		AddExpectedMessage(TEXT("has no Rare version"), EAutomationExpectedErrorFlags::Contains, 1);
		TestEqual(TEXT("gs.Upgrade.Grant ExtraJump 1 Rare: refused"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("ExtraJump"), 1, EGSUpgradeRarity::Rare), 0);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeList"), {});
	});

	// Floating Gun: picks 1 and 2 give the guns (multiplier - 1 power each), later picks the multiplier.
	Step([this]()
	{
		const UGSUpgradeData* Gun = UGSDebugLibrary::FindUpgradeData(TEXT("FloatingGun"));
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		TestTrue(TEXT("Floating Gun Rare first pick"), Upgrades->GrantUpgrade(Gun, EGSUpgradeRarity::Rare, TEXT("Test")));
		TestEqual(TEXT("Rare first pick: one pick"), GetAttr(UGSAttributeSet::GetFloatingGunLevelAttribute()), 1.f, 0.0001f);
		TestEqual(TEXT("Rare first pick: power 0.5"), GetAttr(UGSAttributeSet::GetFloatingGunPowerAttribute()), 0.5f, 0.0001f);
		TestTrue(TEXT("Floating Gun Legendary second pick"), Upgrades->GrantUpgrade(Gun, EGSUpgradeRarity::Legendary, TEXT("Test")));
		TestEqual(TEXT("Legendary second pick (the second gun): power 0.5 + 2"), GetAttr(UGSAttributeSet::GetFloatingGunPowerAttribute()), 2.5f, 0.0001f);
		TestTrue(TEXT("Floating Gun Common third pick"), Upgrades->GrantUpgrade(Gun, EGSUpgradeRarity::Common, TEXT("Test")));
		TestEqual(TEXT("Common third pick: +1 power"), GetAttr(UGSAttributeSet::GetFloatingGunPowerAttribute()), 3.5f, 0.0001f);
		TestTrue(TEXT("Floating Gun Epic fourth pick"), Upgrades->GrantUpgrade(Gun, EGSUpgradeRarity::Epic, TEXT("Test")));
		TestEqual(TEXT("Epic fourth pick: +2 power"), GetAttr(UGSAttributeSet::GetFloatingGunPowerAttribute()), 5.5f, 0.0001f);
		TestEqual(TEXT("Four picks"), GetAttr(UGSAttributeSet::GetFloatingGunLevelAttribute()), 4.f, 0.0001f);
		TestEqual(TEXT("Floating Gun total value = power"), Upgrades->GetTotalValue(Gun), 5.5f, 0.0001f);
	});
	WaitFrames(this, 2);
	Step([this]()
	{
		TestEqual(TEXT("Two guns"), GetPlayer()->GetFloatingGuns()->GetGuns().Num(), 2);
		const AGSFloatingGun* Right = GetFloatingGunAt(0);
		if (TestNotNull(TEXT("A gun"), Right))
		{
			TestEqual(TEXT("Power 5.5: x1.55 damage"), Right->GetDamageMultiplier(), 1.55f, 0.0001f);
			TestEqual(TEXT("Power 5.5: 1.5 x 1.55 shots a second"), 1.0 / Right->GetShotInterval(), 1.5 * 1.55, 0.0001);
		}

		// Second Wind: no rarities, one charge per pick, and a used one takes one pick back. Ready shields would block
		// the killing blow first, so this runs in a fresh run.
		GetGameMode()->RestartRunWithSeed(41);
		const UGSUpgradeData* SecondWind = UGSDebugLibrary::FindUpgradeData(TEXT("SecondWind"));
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		TestTrue(TEXT("Second Wind Common pick"), Upgrades->GrantUpgrade(SecondWind, EGSUpgradeRarity::Common, TEXT("Test")));
		TestTrue(TEXT("Second Wind Legendary pick"), Upgrades->GrantUpgrade(SecondWind, EGSUpgradeRarity::Legendary, TEXT("Test")));
		TestTrue(TEXT("Second Wind Rare pick (no rarities: allowed)"), Upgrades->GrantUpgrade(SecondWind, EGSUpgradeRarity::Rare, TEXT("Test")));
		TestEqual(TEXT("Three picks = three charges, whatever the rarity"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 3.f, 0.0001f);
		ApplyDamage(GetPlayerASC(), 99999.f);
		TestFalse(TEXT("The killing blow used a Second Wind"), GetPlayer()->GetHealthComponent()->IsDead());
		TestEqual(TEXT("One charge used: 2 left"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 2.f, 0.0001f);
		TestEqual(TEXT("One pick taken back"), Upgrades->GetStacks(SecondWind), 2);
		TestEqual(TEXT("Total value 2"), Upgrades->GetTotalValue(SecondWind), 2.f, 0.0001f);
		TestEqual(TEXT("Still one active effect"), GetActiveUpgradeEffectCount(SecondWind), 1);

		// A new run removes everything.
		GetGameMode()->RestartRunWithSeed(42);
		TestEqual(TEXT("New run: no picks"), Upgrades->GetTotalPicks(), 0);
		TestEqual(TEXT("New run: no Second Winds"), GetAttr(UGSAttributeSet::GetSecondWindsAttribute()), 0.f, 0.0001f);
		TestEqual(TEXT("New run: no effects left"), GetActiveUpgradeEffectCount(SecondWind), 0);
		TestEqual(TEXT("New run: Floating Gun power 0"), GetAttr(UGSAttributeSet::GetFloatingGunPowerAttribute()), 0.f, 0.0001f);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Weapon slots (M2c, Prompt 43): MaxWeapons, the first offer's weapon card, the card label, HUD slots, run summary
// ---------------------------------------------------------------------------

namespace GSTests
{
	bool OfferHasWeapon(const TArray<FGSUpgradeChoice>& Offer)
	{
		return Offer.ContainsByPredicate([](const FGSUpgradeChoice& C) { return C.Upgrade->Category == EGSUpgradeCategory::Weapon; });
	}

	/** Index of the first card that isn't a weapon (0 if they all are). */
	int32 FirstNonWeaponCard(const TArray<FGSUpgradeChoice>& Offer)
	{
		return FMath::Max(Offer.IndexOfByPredicate([](const FGSUpgradeChoice& C) { return C.Upgrade->Category != EGSUpgradeCategory::Weapon; }), 0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSWeaponSlotsTest, "BanditBaller.Upgrades.WeaponSlots", GSTests::TestFlags)
bool FGSWeaponSlotsTest::RunTest(const FString& Parameters)
{
	struct FWeaponState
	{
		TStrongObjectPtr<UGSUpgradePool> TestPool;
		TStrongObjectPtr<UGSUpgradeData> TestWeapon;
	};
	TSharedRef<FWeaponState> S = MakeShared<FWeaponState>();
	auto GetPS = []() { return GetPC() ? GetPC()->GetPlayerState<AGSPlayerState>() : nullptr; };

	BeginPIE(this);

	// The roll rules on test pools (two test weapons, three stat boosts).
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		const UGSUpgradePool* Pool = GameMode->GetUpgradePool();
		if (!TestNotNull(TEXT("DA_UpgradePool"), Pool))
		{
			return;
		}
		TestEqual(TEXT("MaxWeapons 3"), Pool->MaxWeapons, 3);

		UGSUpgradeData* W1 = MakeTestUpgrade(TEXT("TestWeapon_1"), EGSUpgradeCategory::Weapon);
		UGSUpgradeData* W2 = MakeTestUpgrade(TEXT("TestWeapon_2"), EGSUpgradeCategory::Weapon);
		UGSUpgradePool* Test = MakeTestPool({ W1, W2, MakeTestUpgrade(TEXT("TestStat_1")), MakeTestUpgrade(TEXT("TestStat_2")), MakeTestUpgrade(TEXT("TestStat_3")) });
		Test->MaxWeapons = 1;
		auto NoneOwned = [](const UGSUpgradeData*) { return 0; };
		auto OwnsW1 = [W1](const UGSUpgradeData* U) { return U == W1 ? 1 : 0; };
		FRandomStream Stream(43);

		auto Seen = [&Stream, Test](TFunctionRef<int32(const UGSUpgradeData*)> GetStacks, const UGSUpgradeData* Weapon, bool bWeaponCard)
		{
			int32 Count = 0;
			for (int32 i = 0; i < 400; ++i)
			{
				Count += OfferContains(Test->RollOffer(Stream, GetStacks, bWeaponCard), Weapon) ? 1 : 0;
			}
			return Count;
		};
		TestTrue(TEXT("MaxWeapons 1, none owned: weapon 1 is offered"), Seen(NoneOwned, W1, false) > 0);
		TestTrue(TEXT("MaxWeapons 1, none owned: weapon 2 is offered"), Seen(NoneOwned, W2, false) > 0);
		TestEqual(TEXT("MaxWeapons 1, weapon 1 owned: weapon 2 is never offered"), Seen(OwnsW1, W2, false), 0);
		TestTrue(TEXT("MaxWeapons 1, weapon 1 owned: weapon 1 is still offered (an upgrade)"), Seen(OwnsW1, W1, false) > 0);
		TestFalse(TEXT("CanOfferWeapon: no free slot for weapon 2"), Test->CanOfferWeapon(W2, OwnsW1));
		TestTrue(TEXT("CanOfferWeapon: the owned one"), Test->CanOfferWeapon(W1, OwnsW1));
		Test->MaxWeapons = 2;
		TestTrue(TEXT("MaxWeapons 2, weapon 1 owned: weapon 2 is offered again"), Seen(OwnsW1, W2, false) > 0);
		TestEqual(TEXT("Owned weapons counted"), Test->CountOwnedWeapons(OwnsW1), 1);

		// The first offer's weapon card: always there, its rarity rolled, anywhere in the offer.
		Test->MaxWeapons = 1;
		bool bAlways = true, bFull = true;
		int32 AtIndex[3] = {};
		int32 Rare = 0, Common = 0;
		for (int32 i = 0; i < 400; ++i)
		{
			const TArray<FGSUpgradeChoice> Offer = Test->RollOffer(Stream, NoneOwned, true);
			bAlways &= OfferHasWeapon(Offer);
			bFull &= Offer.Num() == 3 && AllDifferent(Offer);
			const int32 Index = Offer.IndexOfByPredicate([](const FGSUpgradeChoice& C) { return C.Upgrade->Category == EGSUpgradeCategory::Weapon; });
			if (Offer.IsValidIndex(Index))
			{
				++AtIndex[FMath::Min(Index, 2)];
				Rare += Offer[Index].Rarity == EGSUpgradeRarity::Rare ? 1 : 0;
				Common += Offer[Index].Rarity == EGSUpgradeRarity::Common ? 1 : 0;
			}
		}
		TestTrue(TEXT("Weapon card in every first offer"), bAlways);
		TestTrue(TEXT("First offers still have 3 different cards"), bFull);
		TestTrue(TEXT("The weapon card can be on any of the 3 cards"), AtIndex[0] > 0 && AtIndex[1] > 0 && AtIndex[2] > 0);
		AddInfo(FString::Printf(TEXT("Forced weapon card rarity over 400: %d Common, %d Rare"), Common, Rare));
		TestTrue(TEXT("Its rarity is rolled as usual (Common and Rare both come up)"), Common > 0 && Rare > 0);
		bool bOwnedOnly = true;
		for (int32 i = 0; i < 200; ++i)
		{
			const TArray<FGSUpgradeChoice> Offer = Test->RollOffer(Stream, OwnsW1, true);
			bOwnedOnly &= OfferContains(Offer, W1) && !OfferContains(Offer, W2);
		}
		TestTrue(TEXT("Slots full: the first offer's weapon card is the owned weapon"), bOwnedOnly);
		Test->MaxWeapons = 0;
		const TArray<FGSUpgradeChoice> NoWeapons = Test->RollOffer(Stream, NoneOwned, true);
		TestTrue(TEXT("MaxWeapons 0: no weapon card, still 3 cards"), !OfferHasWeapon(NoWeapons) && NoWeapons.Num() == 3);

		// The card label.
		const UGSUpgradeData* Gun = UGSDebugLibrary::FindUpgradeData(TEXT("FloatingGun"));
		TestEqual(TEXT("Not owned: NEW WEAPON"), UGSUpgradeCardWidget::GetCategoryLabel(Gun, 0).ToString(), FString(TEXT("NEW WEAPON")));
		TestEqual(TEXT("Owned: WEAPON UPGRADE"), UGSUpgradeCardWidget::GetCategoryLabel(Gun, 1).ToString(), FString(TEXT("WEAPON UPGRADE")));
		TestEqual(TEXT("Not a weapon: its category"), UGSUpgradeCardWidget::GetCategoryLabel(UGSDebugLibrary::FindUpgradeData(TEXT("Damage")), 0).ToString(), FString(TEXT("STAT BOOST")));
	});

	// The real first offer of a run: a weapon card, kept by rerolls, gone after the first pick; back on a new run.
	Step([this, GetPS]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSPlayerState* PS = GetPS();
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		GameMode->RestartRunWithSeed(4301);
		TestTrue(TEXT("A new run: the first offer is pending"), Upgrades->IsFirstOfferPending());
		GameMode->AwardXP(PS, 100.f, false);
		TestTrue(TEXT("First offer has a weapon card (any of the three weapons)"), OfferHasWeapon(Upgrades->GetCurrentOffer()));
		bool bRerollsKeepIt = true;
		for (int32 i = 0; i < 3; ++i)
		{
			GameMode->RerollUpgradeOffer(PS);
			bRerollsKeepIt &= OfferHasWeapon(Upgrades->GetCurrentOffer());
		}
		TestTrue(TEXT("Rerolls of the first offer keep a weapon card"), bRerollsKeepIt);
		TestTrue(TEXT("Pick a non-weapon card"), GameMode->ChooseUpgrade(PS, FirstNonWeaponCard(Upgrades->GetCurrentOffer())));
		TestFalse(TEXT("After the first pick no more guaranteed weapon cards"), Upgrades->IsFirstOfferPending());
		// Later offers: a weapon card only by chance, at the rate the pool gives three cards drawn without a weapon. The
		// rate is worked out for each offer from the weapons that can be offered then: an all-weapon offer makes the
		// player pick a weapon, and with MaxWeapons owned only those are offered. (Legendary ability cards are left out
		// of the estimate: they only make a weapon-free offer a little likelier.)
		const UGSUpgradePool* RealPool = GameMode->GetUpgradePool();
		int32 NonWeapons = 0;
		TArray<const UGSUpgradeData*> Weapons;
		for (const UGSUpgradeData* Upgrade : RealPool->Upgrades)
		{
			if (Upgrade->bLegendaryAbility)
			{
				continue;
			}
			if (Upgrade->Category == EGSUpgradeCategory::Weapon)
			{
				Weapons.Add(Upgrade);
			}
			else
			{
				++NonWeapons;
			}
		}
		constexpr int32 NumLater = 200;
		int32 Without = 0;
		double Expected = 0.0;
		for (int32 i = 0; i < NumLater; ++i)
		{
			int32 Offerable = 0;
			for (const UGSUpgradeData* Weapon : Weapons)
			{
				Offerable += RealPool->CanOfferWeapon(Weapon, [Upgrades](const UGSUpgradeData* U) { return Upgrades->GetStacks(U); }) ? 1 : 0;
			}
			double NoWeapon = 1.0;
			for (int32 Card = 0; Card < 3; ++Card)
			{
				NoWeapon *= static_cast<double>(NonWeapons - Card) / (NonWeapons + Offerable - Card);
			}
			Expected += NoWeapon / NumLater;
			GameMode->QueueUpgradeOfferForDebug(PS);
			Without += OfferHasWeapon(Upgrades->GetCurrentOffer()) ? 0 : 1;
			GameMode->ChooseUpgrade(PS, FirstNonWeaponCard(Upgrades->GetCurrentOffer()));
		}
		const double Share = static_cast<double>(Without) / NumLater;
		AddInfo(FString::Printf(TEXT("Later offers without a weapon card: %d of %d (%.1f%%; expected about %.1f%%)"), Without, NumLater, 100.0 * Share, 100.0 * Expected));
		TestTrue(TEXT("Later offers are rolled as usual (the pool's weapon-free share, no guarantee)"), FMath::Abs(Share - Expected) < 0.1);
		GameMode->RestartRunWithSeed(4302);
		TestTrue(TEXT("A new run: pending again"), Upgrades->IsFirstOfferPending());
	});

	// The limit in the real offer flow: a copy of the pool with a second (test) weapon and MaxWeapons 1.
	Step([this, S, GetPS]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSPlayerState* PS = GetPS();
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		S->TestPool.Reset(DuplicateObject<UGSUpgradePool>(GameMode->GetUpgradePool(), GetTransientPackage()));
		S->TestWeapon.Reset(MakeTestUpgrade(TEXT("TestWeapon_Real"), EGSUpgradeCategory::Weapon));
		S->TestWeapon->DisplayName = FText::FromString(TEXT("Test Weapon"));
		// Exactly two weapons here (the Floating Gun and the test weapon): the other weapons stay out of this copy.
		S->TestPool->Upgrades.Remove(UGSDebugLibrary::FindUpgradeData(TEXT("RazorCards")));
		S->TestPool->Upgrades.Remove(UGSDebugLibrary::FindUpgradeData(TEXT("BarbedWire")));
		S->TestPool->Upgrades.Remove(UGSDebugLibrary::FindUpgradeData(TEXT("Dynamite")));
		S->TestPool->Upgrades.Remove(UGSDebugLibrary::FindUpgradeData(TEXT("LuckyCoin")));
		S->TestPool->Upgrades.Remove(UGSDebugLibrary::FindUpgradeData(TEXT("Molotov")));
		S->TestPool->Upgrades.Remove(UGSDebugLibrary::FindUpgradeData(TEXT("Tomahawk")));
		S->TestPool->Upgrades.Add(S->TestWeapon.Get());
		S->TestPool->MaxWeapons = 1;
		GameMode->SetUpgradePoolOverride(S->TestPool.Get());
		GameMode->RestartRunWithSeed(4303);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingGun") });
		TestEqual(TEXT("One weapon slot (MaxWeapons 1)"), GetHUD()->GetWeaponSlotCount(), 1);

		const UGSUpgradeData* Gun = UGSDebugLibrary::FindUpgradeData(TEXT("FloatingGun"));
		bool bTestWeaponSeen = false;
		int32 GunSeen = 0;
		for (int32 i = 0; i < 30; ++i)
		{
			GameMode->QueueUpgradeOfferForDebug(PS);
			const TArray<FGSUpgradeChoice> Offer = Upgrades->GetCurrentOffer();
			if (i == 0)
			{
				TestTrue(TEXT("First offer, slots full: its weapon card is the owned Floating Gun"), OfferContains(Offer, Gun));
			}
			bTestWeaponSeen |= OfferContains(Offer, S->TestWeapon.Get());
			GunSeen += OfferContains(Offer, Gun) ? 1 : 0;
			if (GameMode->RerollUpgradeOffer(PS))
			{
				bTestWeaponSeen |= OfferContains(Upgrades->GetCurrentOffer(), S->TestWeapon.Get());
			}
			GameMode->ChooseUpgrade(PS, FirstNonWeaponCard(Upgrades->GetCurrentOffer()));
		}
		TestFalse(TEXT("MaxWeapons 1 and a weapon owned: a new weapon is never offered (offers and rerolls)"), bTestWeaponSeen);
		TestTrue(TEXT("The owned weapon is still offered, as an upgrade"), GunSeen > 1);

		// A free slot: the second weapon can be offered and picked.
		S->TestPool->MaxWeapons = 3;
		bool bPicked = false;
		for (int32 i = 0; i < 60 && !bPicked; ++i)
		{
			GameMode->QueueUpgradeOfferForDebug(PS);
			const TArray<FGSUpgradeChoice> Offer = Upgrades->GetCurrentOffer();
			const int32 Index = Offer.IndexOfByPredicate([S](const FGSUpgradeChoice& C) { return C.Upgrade == S->TestWeapon.Get(); });
			bPicked = Index != INDEX_NONE;
			GameMode->ChooseUpgrade(PS, bPicked ? Index : FirstNonWeaponCard(Offer));
		}
		TestTrue(TEXT("MaxWeapons 3: the second weapon is offered and picked"), bPicked);
		UGSHUDWidget* HUD = GetHUD();
		TestEqual(TEXT("Three weapon slots"), HUD->GetWeaponSlotCount(), 3);
		TestTrue(TEXT("Slot 1: the Floating Gun (picked first)"), HUD->GetWeaponInSlot(0) == Gun);
		TestTrue(TEXT("Slot 2: the test weapon"), HUD->GetWeaponInSlot(1) == S->TestWeapon.Get());
		TestNull(TEXT("Slot 3: empty"), HUD->GetWeaponInSlot(2));
		TestNull(TEXT("No weapon icon in the upgrade row"), HUD->FindUpgradeIcon(Gun));
		TestNull(TEXT("No test weapon icon in the upgrade row"), HUD->FindUpgradeIcon(S->TestWeapon.Get()));
		GameMode->SetUpgradePoolOverride(nullptr);
	});

	// HUD slots with the real pool: empty, filled by a pick, icon only; a new run empties them.
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		GameMode->RestartRunWithSeed(4304);
		UGSHUDWidget* HUD = GetHUD();
		const UGSUpgradeData* Gun = UGSDebugLibrary::FindUpgradeData(TEXT("FloatingGun"));
		const UGSUpgradeData* Damage = UGSDebugLibrary::FindUpgradeData(TEXT("Damage"));
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Damage") });
		TestEqual(TEXT("Three weapon slots"), HUD->GetWeaponSlotCount(), 3);
		bool bAllEmpty = true;
		for (int32 Index = 0; Index < HUD->GetWeaponSlotCount(); ++Index)
		{
			const UGSUpgradeIconWidget* WeaponSlot = HUD->GetWeaponSlot(Index);
			bAllEmpty &= WeaponSlot && WeaponSlot->IsWeaponSlot() && !WeaponSlot->GetUpgrade() && !WeaponSlot->IsIconShown();
		}
		TestTrue(TEXT("No weapon: all three slots empty, no icon"), bAllEmpty);
		TestEqual(TEXT("The Damage pick is in the upgrade row"), HUD->GetUpgradeIconCount(), 1);

		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingGun"), TEXT("2"), TEXT("Epic") });
		const UGSUpgradeIconWidget* First = HUD->GetWeaponSlot(0);
		TestTrue(TEXT("Slot 1 shows the Floating Gun"), First && First->GetUpgrade() == Gun && First->IsIconShown());
		TestTrue(TEXT("Slots 2 and 3 still empty"), !HUD->GetWeaponInSlot(1) && !HUD->GetWeaponInSlot(2));
		TestTrue(TEXT("Icon only: no pick count shown"), First && First->GetStacks() == 0);
		TestNull(TEXT("The Floating Gun isn't in the upgrade row"), HUD->FindUpgradeIcon(Gun));
		TestEqual(TEXT("Upgrade row: still just Damage"), HUD->GetUpgradeIconCount(), 1);
		TestTrue(TEXT("Damage icon still there"), HUD->FindUpgradeIcon(Damage) != nullptr);

		GameMode->RestartRunWithSeed(4305);
		TestTrue(TEXT("New run: slots empty"), !HUD->GetWeaponInSlot(0) && !HUD->GetWeaponInSlot(1) && !HUD->GetWeaponInSlot(2));
		TestEqual(TEXT("New run: still three slots"), HUD->GetWeaponSlotCount(), 3);

		// The run summary lists the weapons held.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("FloatingGun") });
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("Damage") });
		ApplyDamage(GetPlayerASC(), 100000.f);
	});
	WaitUntil(this, TEXT("the run summary"), []() { return GetRunSummary() != nullptr; }, 10.0);
	Step([this]()
	{
		UGSRunSummaryWidget* Summary = GetRunSummary();
		if (!TestNotNull(TEXT("Run summary"), Summary))
		{
			return;
		}
		TestEqual(TEXT("Summary: weapons held"), Summary->GetWeaponsText().ToString(), FString(TEXT("Floating Gun")));
		TestEqual(TEXT("Summary: upgrades without the weapon"), Summary->GetUpgradesText().ToString(), FString(TEXT("Damage x1")));
		TestEqual(TEXT("Summary: one upgrade icon (no weapon icon)"), Summary->GetUpgradeIconCount(), 1);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Move Speed: the slide and the dash scale with the speed ratio (Prompt 44b)
// ---------------------------------------------------------------------------

namespace GSTests
{
	struct FMoveSpeedTrial
	{
		// Slide
		float SlideRunSpeed = 0.f;
		float SlideStartSpeed = 0.f;
		float SlideEndSpeed = -1.f;
		float SlideRatio = 0.f;
		// Dash
		FVector DashStart = FVector::ZeroVector;
		double DashStartTime = 0.0;
		FVector DashEnd = FVector::ZeroVector;
		double DashEndTime = 0.0;
		float DashExitSpeed = -1.f;
		float DashRatio = 0.f;
	};

	struct FMoveSpeedState
	{
		TWeakObjectPtr<const UGSSlideSettings> Slide;
		TWeakObjectPtr<const UGSDashSettings> Dash;
		float BaseWalk = 0.f;
		FVector LaneStart = FVector::ZeroVector;
		FVector LaneDir = FVector::ForwardVector;
		bool bLaneFound = false;
		FMoveSpeedTrial Trials[2]; // [0] x1, [1] x2
		int32 Current = 0;
		FDelegateHandle SlideHandle;
		FDelegateHandle DashHandle;
	};

	void SetMoveSpeedBase(float Value)
	{
		if (UGSAbilitySystemComponent* ASC = GetPlayerASC())
		{
			ASC->SetNumericAttributeBase(UGSAttributeSet::GetMoveSpeedAttribute(), Value);
		}
	}

	/** A straight, flat stretch of town with no cover for Length cm (capsule sweep), for the slide and dash runs. */
	bool FindClearLane(float Length, FVector& OutStart, FVector& OutDir)
	{
		UWorld* World = GetPIEWorld();
		const AGSCharacter* Player = GetPlayer();
		if (!World || !Player)
		{
			return false;
		}
		const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
		const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius() + 20.f, Capsule->GetScaledCapsuleHalfHeight());
		const float Z = Capsule->GetScaledCapsuleHalfHeight() + 5.f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSMoveSpeedLane), false, Player);
		// The ring road (77-89 m out) and the side streets are kept clear of cover.
		const TPair<FVector, FVector> Candidates[] = {
			{ FVector(8300.f, -3000.f, Z), FVector(0.f, 1.f, 0.f) },
			{ FVector(-8300.f, -3000.f, Z), FVector(0.f, 1.f, 0.f) },
			{ FVector(-3000.f, 8300.f, Z), FVector(1.f, 0.f, 0.f) },
			{ FVector(-3000.f, -8300.f, Z), FVector(1.f, 0.f, 0.f) },
		};
		for (const TPair<FVector, FVector>& Candidate : Candidates)
		{
			FHitResult Hit;
			if (!World->SweepSingleByChannel(Hit, Candidate.Key, Candidate.Key + Candidate.Value * Length, FQuat::Identity, ECC_Pawn, Shape, Params))
			{
				OutStart = Candidate.Key;
				OutDir = Candidate.Value;
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSMoveSpeedSlideDashTest, "BanditBaller.Upgrades.MoveSpeedSlideDash", GSTests::TestFlags)
bool FGSMoveSpeedSlideDashTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	TSharedRef<FMoveSpeedState> S = MakeShared<FMoveSpeedState>();

	// Settings, the ratio rule, a clear lane. Normal game speed (the dash time is measured), editor throttling off.
	Step([this, S]()
	{
		SpeedUpWorld(1.f);
		GetGameMode()->SetRoundTimerPaused(true);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		AGSCharacter* Player = GetPlayer();
		const UGSGameplayAbility_Slide* SlideAbility = UGSGameplayAbility_Slide::FindGrantedSlide(GetPlayerASC());
		const UGSGameplayAbility_Dash* DashAbility = UGSGameplayAbility_Dash::FindGrantedDash(GetPlayerASC());
		if (!TestNotNull(TEXT("GA_Slide granted"), SlideAbility) || !TestNotNull(TEXT("GA_Dash granted"), DashAbility)
			|| !TestNotNull(TEXT("Slide settings"), SlideAbility->GetSettings()) || !TestNotNull(TEXT("Dash settings"), DashAbility->GetSettings())
			|| !TestNotNull(TEXT("Movement settings"), Player->GetMovementSettings()))
		{
			return;
		}
		S->Slide = SlideAbility->GetSettings();
		S->Dash = DashAbility->GetSettings();
		S->BaseWalk = Player->GetMovementSettings()->MaxWalkSpeed;
		TestEqual(TEXT("MoveSpeed starts at the base walk speed"), GetAttr(UGSAttributeSet::GetMoveSpeedAttribute()), S->BaseWalk, 0.01f);
		TestEqual(TEXT("Base Move Speed: ratio exactly 1"), Player->GetMoveSpeedRatio(), 1.f);

		// The settings' scaled getters are the plain values at 1 and scale linearly.
		const UGSSlideSettings* Slide = S->Slide.Get();
		const UGSDashSettings* Dash = S->Dash.Get();
		TestEqual(TEXT("Slide boost at x1"), Slide->GetSpeedBoost(1.f), Slide->SpeedBoost);
		TestEqual(TEXT("Slide cap at x1"), Slide->GetMaxSpeed(1.f), Slide->MaxSpeed);
		TestEqual(TEXT("Slide end speed at x1"), Slide->GetEndSpeed(1.f), Slide->EndSpeed);
		TestEqual(TEXT("Dash distance at x1"), Dash->GetDistance(1.f), Dash->Distance);
		TestEqual(TEXT("Dash exit speed at x1"), Dash->GetExitSpeed(1.f), Dash->ExitSpeed);
		TestEqual(TEXT("Dash speed at x2 is twice"), Dash->GetDashSpeed(2.f), Dash->GetDashSpeed() * 2.f, 0.01f);

		// Never below 1: a slower player keeps the base slide and dash.
		SetMoveSpeedBase(S->BaseWalk * 0.5f);
		TestEqual(TEXT("Half Move Speed: ratio stays 1"), Player->GetMoveSpeedRatio(), 1.f);
		SetMoveSpeedBase(S->BaseWalk * 2.f);
		TestEqual(TEXT("Move Speed x2: ratio 2"), Player->GetMoveSpeedRatio(), 2.f, 0.0001f);
		SetMoveSpeedBase(S->BaseWalk);

		// The Move Speed upgrade raises it too (Common +8%).
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("MoveSpeed") });
		TestTrue(TEXT("A Move Speed pick raises the ratio above 1"), Player->GetMoveSpeedRatio() > 1.05f);
		GetGameMode()->RestartRunWithSeed(4401);
		GetGameMode()->SetRoundTimerPaused(true);
		TestEqual(TEXT("A new run: ratio 1 again"), Player->GetMoveSpeedRatio(), 1.f);

		const float LaneLength = FMath::Max(Dash->Distance * 2.f, Slide->MaxSpeed * 2.f * Slide->Duration) + 800.f;
		S->bLaneFound = FindClearLane(LaneLength, S->LaneStart, S->LaneDir);
		TestTrue(FString::Printf(TEXT("A clear %.0f m lane in the town"), LaneLength / 100.f), S->bLaneFound);

		// Record the slide's end speed and the dash's end (position, time, exit speed) when they finish.
		S->SlideHandle = Player->OnSlideFinished.AddLambda([S]()
		{
			S->Trials[S->Current].SlideEndSpeed = GetPlayer()->GetCharacterMovement()->Velocity.Size2D();
		});
		S->DashHandle = Player->OnDashFinished.AddLambda([S]()
		{
			FMoveSpeedTrial& T = S->Trials[S->Current];
			T.DashEnd = GetPlayer()->GetActorLocation();
			T.DashEndTime = GetPIEWorld()->GetTimeSeconds();
			T.DashExitSpeed = GetPlayer()->GetCharacterMovement()->Velocity.Size2D();
		});
	});

	// One slide and one dash at x1, then at x2 (MoveSpeed base value; MaxWalkSpeed follows).
	for (int32 TrialIndex = 0; TrialIndex < 2; ++TrialIndex)
	{
		const float Ratio = TrialIndex == 0 ? 1.f : 2.f;
		Step([S, TrialIndex, Ratio]()
		{
			S->Current = TrialIndex;
			SetMoveSpeedBase(S->BaseWalk * Ratio);
			GetPlayer()->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			GetPlayer()->TeleportTo(S->LaneStart, S->LaneDir.Rotation());
		});
		WaitFrames(this, 2);
		WaitUntil(this, TEXT("standing on the lane before the slide"), []() { return GetPlayer()->GetCharacterMovement()->IsMovingOnGround(); }, 5.0);
		Step([this, S, TrialIndex]()
		{
			// A full run, then the slide.
			FMoveSpeedTrial& T = S->Trials[TrialIndex];
			UCharacterMovementComponent* CMC = GetPlayer()->GetCharacterMovement();
			TestTrue(TEXT("On the ground before the slide"), CMC->IsMovingOnGround());
			T.SlideRunSpeed = CMC->MaxWalkSpeed;
			CMC->Velocity = S->LaneDir * T.SlideRunSpeed;
			if (const UGSSlideSettings* Slide = S->Slide.Get())
			{
				GetPlayer()->StartSlide(*Slide);
			}
			T.SlideStartSpeed = GetPlayer()->GetSlideStartSpeed();
			T.SlideRatio = GetPlayer()->GetSlideSpeedRatio();
			TestTrue(TEXT("Sliding"), GetPlayer()->IsSliding());
		});
		WaitUntil(this, TEXT("the slide to end"), []() { return !GetPlayer()->IsSliding(); }, 5.0);
		Step([S]()
		{
			GetPlayer()->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			GetPlayer()->TeleportTo(S->LaneStart, S->LaneDir.Rotation());
		});
		WaitFrames(this, 2);
		WaitUntil(this, TEXT("standing on the lane before the dash"), []() { return GetPlayer()->GetCharacterMovement()->IsMovingOnGround(); }, 5.0);
		Step([S, TrialIndex]()
		{
			FMoveSpeedTrial& T = S->Trials[TrialIndex];
			AGSCharacter* Player = GetPlayer();
			Player->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			T.DashStart = Player->GetActorLocation();
			T.DashStartTime = GetPIEWorld()->GetTimeSeconds();
			if (const UGSDashSettings* Dash = S->Dash.Get())
			{
				Player->StartDash(S->LaneDir, *Dash);
			}
			T.DashRatio = Player->GetDashSpeedRatio();
		});
		WaitUntil(this, TEXT("the dash to end"), []() { return !GetPlayer()->IsDashing(); }, 5.0);
	}

	Step([this, S]()
	{
		const UGSSlideSettings* Slide = S->Slide.Get();
		const UGSDashSettings* Dash = S->Dash.Get();
		if (!Slide || !Dash)
		{
			return;
		}
		for (int32 TrialIndex = 0; TrialIndex < 2; ++TrialIndex)
		{
			const FMoveSpeedTrial& T = S->Trials[TrialIndex];
			const float R = TrialIndex == 0 ? 1.f : 2.f;
			const FString Tag = FString::Printf(TEXT("x%.0f"), R);
			TestEqual(Tag + TEXT(": the slide kept ratio"), T.SlideRatio, R, 0.0001f);
			TestEqual(Tag + TEXT(": the dash kept ratio"), T.DashRatio, R, 0.0001f);

			// Slide: start = min(run + boost x R, cap x R), end = EndSpeed x R.
			const float ExpectedStart = FMath::Min(T.SlideRunSpeed + Slide->SpeedBoost * R, Slide->MaxSpeed * R);
			TestEqual(Tag + TEXT(": slide start speed"), T.SlideStartSpeed, ExpectedStart, 0.5f);
			TestTrue(Tag + TEXT(": the slide starts faster than running"), T.SlideStartSpeed > T.SlideRunSpeed);
			TestEqual(Tag + TEXT(": slide end speed = EndSpeed x ratio"), T.SlideEndSpeed, Slide->EndSpeed * R, 0.5f);

			// Dash: Distance x R in Duration; exit = min(ExitSpeed, dash speed) x R = the running speed.
			const float Traveled = FVector::Dist2D(T.DashStart, T.DashEnd);
			const double Time = T.DashEndTime - T.DashStartTime;
			AddInfo(FString::Printf(TEXT("%s: slide %.0f -> %.0f -> %.0f cm/s; dash %.1f cm in %.3f s, exit %.0f cm/s"), *Tag,
				T.SlideRunSpeed, T.SlideStartSpeed, T.SlideEndSpeed, Traveled, Time, T.DashExitSpeed));
			TestEqual(Tag + TEXT(": dash distance = Distance x ratio"), Traveled, Dash->Distance * R, 3.f);
			TestTrue(FString::Printf(TEXT("%s: the dash takes its Duration (%.3f s for %.3f s)"), *Tag, Time, Dash->Duration),
				Time >= Dash->Duration - 0.02 && Time <= Dash->Duration + 0.06);
			TestEqual(Tag + TEXT(": dash exit speed"), T.DashExitSpeed, FMath::Min(Dash->ExitSpeed, Dash->GetDashSpeed()) * R, 0.5f);
			TestEqual(Tag + TEXT(": the dash ends at the running speed"), T.DashExitSpeed, S->BaseWalk * R, 0.5f);
		}

		// x1 is exactly the old slide and dash.
		const FMoveSpeedTrial& Base = S->Trials[0];
		TestEqual(TEXT("x1: slide start as before (min(run + SpeedBoost, MaxSpeed))"), Base.SlideStartSpeed, FMath::Min(Base.SlideRunSpeed + Slide->SpeedBoost, Slide->MaxSpeed));
		TestEqual(TEXT("x1: slide end as before (EndSpeed)"), Base.SlideEndSpeed, Slide->EndSpeed, 0.5f);
		TestEqual(TEXT("x1: dash exit as before (min(ExitSpeed, dash speed))"), Base.DashExitSpeed, FMath::Min(Dash->ExitSpeed, Dash->GetDashSpeed()), 0.5f);

		// x2: twice the distance in the same time.
		const FMoveSpeedTrial& Fast = S->Trials[1];
		const float BaseDist = FVector::Dist2D(Base.DashStart, Base.DashEnd);
		const float FastDist = FVector::Dist2D(Fast.DashStart, Fast.DashEnd);
		TestEqual(TEXT("x2: the dash covers twice the distance"), FastDist, BaseDist * 2.f, 5.f);
		TestTrue(TEXT("x2: in the same time (within 0.05 s)"),
			FMath::Abs((Fast.DashEndTime - Fast.DashStartTime) - (Base.DashEndTime - Base.DashStartTime)) <= 0.05);
		TestEqual(TEXT("x2: the slide ends at twice the base EndSpeed"), Fast.SlideEndSpeed, Slide->EndSpeed * 2.f, 0.5f);

		// Move Speed x33 into the east boundary wall.
		AGSMapBoundary* Boundary = GetBoundary();
		if (!TestNotNull(TEXT("Boundary"), Boundary))
		{
			return;
		}
		SetMoveSpeedBase(S->BaseWalk * 33.f);
		TestEqual(TEXT("x33: ratio 33"), GetPlayer()->GetMoveSpeedRatio(), 33.f, 0.001f);
		const FBox Bounds = Boundary->GetPlayBounds();
		GetPlayer()->TeleportTo(FVector(Bounds.Max.X - 300.f, Bounds.GetCenter().Y - 600.f, Boundary->GetGroundZ() + 120.f), FRotator::ZeroRotator);
	});
	WaitFrames(this, 3);
	Step([S]()
	{
		GetPlayer()->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		if (const UGSDashSettings* Dash = S->Dash.Get())
		{
			GetPlayer()->StartDash(FVector(1.f, 0.f, 0.f), *Dash);
		}
	});
	WaitUntil(this, TEXT("the x33 dash into the wall to end"), []() { return !GetPlayer()->IsDashing(); }, 5.0);
	WaitFrames(this, 3);
	Step([this, S]()
	{
		const FVector Location = GetPlayer()->GetActorLocation();
		AddInfo(FString::Printf(TEXT("x33 dash (%.0f cm) into the east wall stopped at x = %.1f (wall at %.0f)"),
			S->Dash.IsValid() ? S->Dash->GetDistance(33.f) : 0.f, Location.X, GetBoundary()->GetPlayBounds().Max.X));
		TestTrue(TEXT("x33: the dash stopped inside the boundary"), GetBoundary()->IsInside(Location));

		// And a slide from a x33 run into the same wall.
		const FBox Bounds = GetBoundary()->GetPlayBounds();
		GetPlayer()->TeleportTo(FVector(Bounds.Max.X - 400.f, Bounds.GetCenter().Y - 1200.f, GetBoundary()->GetGroundZ() + 120.f), FRotator::ZeroRotator);
	});
	WaitFrames(this, 3);
	Step([S]()
	{
		UCharacterMovementComponent* CMC = GetPlayer()->GetCharacterMovement();
		CMC->Velocity = FVector(CMC->MaxWalkSpeed, 0.f, 0.f);
		if (const UGSSlideSettings* Slide = S->Slide.Get())
		{
			GetPlayer()->StartSlide(*Slide);
		}
	});
	WaitUntil(this, TEXT("the x33 slide to end"), []() { return !GetPlayer()->IsSliding(); }, 5.0);
	WaitFrames(this, 3);
	Step([this, S]()
	{
		const FVector Location = GetPlayer()->GetActorLocation();
		AddInfo(FString::Printf(TEXT("x33 slide (start %.0f cm/s) into the east wall stopped at x = %.1f"), GetPlayer()->GetSlideStartSpeed(), Location.X));
		TestTrue(TEXT("x33: the slide stopped inside the boundary"), GetBoundary()->IsInside(Location));

		SetMoveSpeedBase(S->BaseWalk);
		GetPlayer()->OnSlideFinished.Remove(S->SlideHandle);
		GetPlayer()->OnDashFinished.Remove(S->DashHandle);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

namespace GSTests
{
	/** One piece of town cover and a clear approach to it: from Start along Dir, the first thing a capsule meets is it. */
	struct FCoverTarget
	{
		FString Kind;
		TWeakObjectPtr<AActor> Actor;
		FVector Start = FVector::ZeroVector;
		FVector Dir = FVector::ForwardVector;
		float FaceDistance = 0.f; // capsule sweep distance from Start to the cover's face
	};

	/** Finds the cover piece of each kind (by size) with a clear 6 m run-up from one of the four axis directions. */
	TArray<FCoverTarget> FindCoverTargets()
	{
		TArray<FCoverTarget> Targets;
		UWorld* World = GetPIEWorld();
		const AGSCharacter* Player = GetPlayer();
		if (!World || !Player)
		{
			return Targets;
		}
		const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
		const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSMoveSpeedCover), false, Player);

		struct FKind { const TCHAR* Name; float MinHeight; float MaxHeight; float MaxThin; FName Tag; };
		const FKind Kinds[] = {
			{ TEXT("crate (1.5 m)"), 130.f, 170.f, 1e9f, NAME_None },
			{ TEXT("low wall (1.8 m)"), 170.f, 200.f, 120.f, NAME_None },
			{ TEXT("shack (4 m)"), 380.f, 420.f, 1e9f, NAME_None },
			{ TEXT("tower (12 m)"), 1150.f, 1250.f, 1e9f, NAME_None },
			{ TEXT("silo (east landmark)"), 0.f, 1e9f, 1e9f, FName(TEXT("Landmark_East")) },
		};
		const FVector Dirs[] = { FVector(1.f, 0.f, 0.f), FVector(-1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), FVector(0.f, -1.f, 0.f) };
		for (const FKind& Kind : Kinds)
		{
			bool bFound = false;
			for (TActorIterator<AStaticMeshActor> It(World); It && !bFound; ++It)
			{
				AStaticMeshActor* Actor = *It;
				FVector Origin, Extent;
				Actor->GetActorBounds(true, Origin, Extent);
				const float Height = Origin.Z + Extent.Z; // from the ground (Z 0) to the top
				const bool bMatches = Kind.Tag.IsNone()
					? (Height >= Kind.MinHeight && Height <= Kind.MaxHeight && FMath::Min(Extent.X, Extent.Y) * 2.f <= Kind.MaxThin
						&& Origin.Z - Extent.Z < 5.f && FVector2D(Origin).Size() > 9000.f) // standing on the ground, in the outskirts
					: Actor->ActorHasTag(Kind.Tag) && Height > 2000.f;
				if (!bMatches)
				{
					continue;
				}
				for (const FVector& Dir : Dirs)
				{
					const float Half = FMath::Abs(Dir.X) > 0.5f ? Extent.X : Extent.Y;
					const FVector Center(Origin.X, Origin.Y, HalfHeight + 5.f);
					const FVector Start = Center - Dir * (Half + 600.f);
					FHitResult Hit;
					// The run-up spot itself is clear, and the first thing on the way is this piece.
					if (World->OverlapAnyTestByChannel(Start, FQuat::Identity, ECC_Pawn, Shape, Params)
						|| !World->SweepSingleByChannel(Hit, Start, Center, FQuat::Identity, ECC_Pawn, Shape, Params)
						|| Hit.GetActor() != Actor || Hit.Distance < 300.f)
					{
						continue;
					}
					FCoverTarget Target;
					Target.Kind = Kind.Name;
					Target.Actor = Actor;
					Target.Start = Start;
					Target.Dir = Dir;
					Target.FaceDistance = Hit.Distance;
					Targets.Add(Target);
					bFound = true;
					break;
				}
			}
		}
		return Targets;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSMoveSpeedCoverTest, "BanditBaller.Upgrades.MoveSpeedCover", GSTests::TestFlags)
bool FGSMoveSpeedCoverTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		TArray<FCoverTarget> Targets;
		TWeakObjectPtr<const UGSSlideSettings> Slide;
		TWeakObjectPtr<const UGSDashSettings> Dash;
		float BaseWalk = 0.f;
		TArray<FVector> Path; // the player's location every frame of the current run
	};
	TSharedRef<FState> S = MakeShared<FState>();

	Step([this, S]()
	{
		SpeedUpWorld(3.f); // movement x3: longer frame steps, which only makes the wall checks harder
		GetGameMode()->SetRoundTimerPaused(true);
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		const UGSGameplayAbility_Slide* Slide = UGSGameplayAbility_Slide::FindGrantedSlide(GetPlayerASC());
		const UGSGameplayAbility_Dash* Dash = UGSGameplayAbility_Dash::FindGrantedDash(GetPlayerASC());
		S->Slide = Slide ? Slide->GetSettings() : nullptr;
		S->Dash = Dash ? Dash->GetSettings() : nullptr;
		S->BaseWalk = GetPlayer()->GetMovementSettings() ? GetPlayer()->GetMovementSettings()->MaxWalkSpeed : 0.f;
		TestTrue(TEXT("Slide and dash settings"), S->Slide.IsValid() && S->Dash.IsValid() && S->BaseWalk > 0.f);
		S->Targets = FindCoverTargets();
		TestEqual(TEXT("A crate, a low wall, a shack, a tower and the silo with a clear run-up"), S->Targets.Num(), 5);
	});

	// For each piece: a dash from 6 m out, then a slide from a full run, at Move Speed x1, x5 and x33. The player must
	// never overlap the piece, stays on the ground, and only ends up behind it by going round it.
	const float Ratios[] = { 1.f, 5.f, 33.f };
	for (const float Ratio : Ratios)
	for (int32 Index = 0; Index < 5; ++Index)
	{
		for (int32 Move = 0; Move < 2; ++Move)
		{
			Step([S, Index, Ratio]()
			{
				SetMoveSpeedBase(S->BaseWalk * Ratio);
				S->Path.Reset();
				if (S->Targets.IsValidIndex(Index))
				{
					GetPlayer()->GetCharacterMovement()->Velocity = FVector::ZeroVector;
					GetPlayer()->TeleportTo(S->Targets[Index].Start, S->Targets[Index].Dir.Rotation());
				}
			});
			WaitFrames(this, 2);
			WaitUntil(this, TEXT("standing at the run-up"), []() { return GetPlayer()->GetCharacterMovement()->IsMovingOnGround(); }, 5.0);
			Step([S, Index, Move]()
			{
				if (!S->Targets.IsValidIndex(Index) || !S->Slide.IsValid() || !S->Dash.IsValid())
				{
					return;
				}
				const FCoverTarget& T = S->Targets[Index];
				UCharacterMovementComponent* CMC = GetPlayer()->GetCharacterMovement();
				if (Move == 0)
				{
					CMC->Velocity = FVector::ZeroVector;
					GetPlayer()->StartDash(T.Dir, *S->Dash.Get());
				}
				else
				{
					CMC->Velocity = T.Dir * CMC->MaxWalkSpeed;
					GetPlayer()->StartSlide(*S->Slide.Get());
				}
			});
			// Sample the path every frame until the move is over and the player has stopped (at most 200 frames).
			Step([S]() { S->Path.Add(GetPlayer()->GetActorLocation()); });
			WaitUntil(this, TEXT("the dash / slide to end and the player to stop"), [S]()
			{
				S->Path.Add(GetPlayer()->GetActorLocation());
				const bool bMoving = GetPlayer()->IsDashing() || GetPlayer()->IsSliding() || GetPlayer()->GetVelocity().Size2D() > 10.f;
				return (!bMoving && S->Path.Num() > 3) || S->Path.Num() > 200;
			}, 10.0);
			Step([this, S, Index, Move, Ratio]()
			{
				if (!S->Targets.IsValidIndex(Index) || !S->Targets[Index].Actor.IsValid() || S->Path.Num() < 2)
				{
					return;
				}
				const FCoverTarget& T = S->Targets[Index];
				const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(T.Actor.Get());
				const UStaticMeshComponent* Comp = MeshActor ? MeshActor->GetStaticMeshComponent() : nullptr;
				const UStaticMesh* Mesh = Comp ? Comp->GetStaticMesh() : nullptr;
				if (!TestNotNull(TEXT("Cover mesh"), Mesh))
				{
					return;
				}
				// Work in the piece's own frame (cover is rotated), in cm: the mesh box scaled by the component.
				const FTransform Xf = Comp->GetComponentTransform();
				const FBox LocalBox = Mesh->GetBoundingBox();
				const FVector Center = LocalBox.GetCenter() * Xf.GetScale3D();
				const FVector Half = LocalBox.GetExtent() * Xf.GetScale3D().GetAbs();
				const bool bRound = Mesh->GetName().Contains(TEXT("Cylinder"));
				// The local axis facing the run-up, and the side axis.
				const float DotX = FVector::DotProduct(Xf.GetUnitAxis(EAxis::X), T.Dir);
				const float DotY = FVector::DotProduct(Xf.GetUnitAxis(EAxis::Y), T.Dir);
				const int32 AxisA = FMath::Abs(DotX) >= FMath::Abs(DotY) ? 0 : 1;
				const int32 AxisB = 1 - AxisA;
				const float SignA = (AxisA == 0 ? DotX : DotY) >= 0.f ? 1.f : -1.f;
				const float Radius = GetPlayer()->GetCapsuleComponent()->GetScaledCapsuleRadius();

				// Wrong = a frame where the capsule overlaps the piece. Sweeps can't skip a piece without a frame inside it
				// unless the player ends up behind it; then it must have been out to its side at some point (went round).
				const float HalfAlong = bRound ? Half.X : Half[AxisA];
				const float HalfSide = bRound ? Half.X : Half[AxisB];
				int32 WrongFrames = 0;
				FString WrongSamples;
				float MaxAlong = -1e9f, MaxSide = 0.f, MaxZ = -1e9f;
				for (int32 i = 0; i < S->Path.Num(); ++i)
				{
					const FVector& P = S->Path[i];
					const FVector L = Xf.InverseTransformPositionNoScale(P) - Center;
					const float A = SignA * L[AxisA];
					const float B = L[AxisB];
					MaxAlong = FMath::Max(MaxAlong, A);
					MaxSide = FMath::Max(MaxSide, FMath::Abs(B));
					MaxZ = FMath::Max(MaxZ, static_cast<float>(P.Z));
					const float Gap = bRound
						? FVector2D(L.X, L.Y).Size() - Half.X
						: FVector2D(FMath::Max(FMath::Abs(A) - HalfAlong, 0.f), FMath::Max(FMath::Abs(B) - HalfSide, 0.f)).Size();
					const bool bWrong = Gap < Radius - 2.f && P.Z < 150.f;
					WrongFrames += bWrong ? 1 : 0;
					if (bWrong && WrongFrames <= 6)
					{
						WrongSamples += FString::Printf(TEXT(" [frame %d: %.0f along, %.0f side, Z %.0f]"), i, A, B, P.Z);
					}
				}
				const FVector LEnd = Xf.InverseTransformPositionNoScale(S->Path.Last()) - Center;
				const FString What = FString::Printf(TEXT("x%.0f %s into the %s (%s, yaw %.0f)"), Ratio, Move == 0 ? TEXT("dash") : TEXT("slide"), *T.Kind,
					*GetNameSafe(T.Actor.Get()), Xf.Rotator().Yaw);
				AddInfo(FString::Printf(TEXT("%s: in its frame contact at %.0f, furthest %.0f along / %.0f to the side (half width %.0f), top Z %.0f, end (%.0f, %.0f), %d frames, %d wrong%s"),
					*What, -(bRound ? Half.X : Half[AxisA]) - Radius, MaxAlong, MaxSide, bRound ? Half.X : Half[AxisB], MaxZ, SignA * LEnd[AxisA], LEnd[AxisB], S->Path.Num(), WrongFrames, *WrongSamples));
				TestEqual(What + TEXT(": frames overlapping it"), WrongFrames, 0);
				const bool bEndedBehind = SignA * LEnd[AxisA] > HalfAlong;
				TestTrue(What + TEXT(": if it ended behind it, it went round it"), !bEndedBehind || MaxSide >= HalfSide + Radius - 2.f);
				TestTrue(What + TEXT(": stayed on the ground (didn't go over it)"), MaxZ < 150.f);
			});
		}
	}

	Step([S]()
	{
		SetMoveSpeedBase(S->BaseWalk);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Endless levels (Prompt 45): level 100 through gs.XP.Add, rewards, offers and the HUD at 3-digit levels
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSEndlessLevelsTest, "BanditBaller.Progression.EndlessLevels", GSTests::TestFlags)
bool FGSEndlessLevelsTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		AGSPlayerState* PS = GetPC() ? GetPC()->GetPlayerState<AGSPlayerState>() : nullptr;
		UGSProgressionComponent* Progression = PS ? PS->GetProgressionComponent() : nullptr;
		UGSUpgradeComponent* Upgrades = PS ? PS->GetUpgradeComponent() : nullptr;
		const UGSXPCurve* Curve = GameMode ? GameMode->GetXPCurve() : nullptr;
		UGSHUDWidget* HUD = GetHUD();
		UGSRunStats* Stats = GetGameState() ? GetGameState()->GetRunStats() : nullptr;
		if (!TestNotNull(TEXT("Progression"), Progression) || !TestNotNull(TEXT("Upgrades"), Upgrades) || !TestNotNull(TEXT("XP curve"), Curve)
			|| !TestNotNull(TEXT("HUD"), HUD) || !TestNotNull(TEXT("Run stats"), Stats))
		{
			return;
		}
		GameMode->SetRoundTimerPaused(true);
		GameMode->RestartRunWithSeed(4501);
		GameMode->SetRoundTimerPaused(true);
		const float BaseMaxHealth = GetAttr(UGSAttributeSet::GetMaxHealthAttribute());

		// Level 100 needs 99 level-ups: 99 x 100 + 30 x (0 + 1 + ... + 98) = 155,430 XP in total.
		const int64 Total100 = Curve->GetTotalXPToReachLevel(100);
		TestEqual(TEXT("Total XP to level 100"), Total100, static_cast<int64>(155430));

		// One level first, to measure the MaxHealth reward per level.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("XPAdd"), { FString::FromInt(Curve->GetXPToReachLevel(2)) });
		TestEqual(TEXT("gs.XP.Add 100: level 2"), Progression->GetLevel(), 2);
		const float PerLevel = GetAttr(UGSAttributeSet::GetMaxHealthAttribute()) - BaseMaxHealth;
		TestTrue(FString::Printf(TEXT("A level-up adds MaxHealth (%.0f)"), PerLevel), PerLevel > 0.f);

		// The rest in one gs.XP.Add: 98 more levels at once.
		ApplyDamage(GetPlayerASC(), 50.f);
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("XPAdd"), { FString::Printf(TEXT("%lld"), Total100 - Curve->GetXPToReachLevel(2)) });
		TestEqual(TEXT("Level 100"), Progression->GetLevel(), 100);
		TestEqual(TEXT("0 XP into level 100"), Progression->GetCurrentXP(), 0);
		TestEqual(TEXT("Level 101 needs 100 + 30 x 99 = 3,070"), Progression->GetXPForNextLevel(), 3070);
		TestEqual(TEXT("Total XP"), static_cast<int64>(Progression->GetTotalXP()), Total100);
		TestEqual(TEXT("Run stats: level reached 100"), Stats->GetLevelReached(), 100);

		// Rewards at every level: +MaxHealth per level gained, and a full heal.
		TestEqual(TEXT("MaxHealth = base + 99 rewards"), GetAttr(UGSAttributeSet::GetMaxHealthAttribute()), BaseMaxHealth + 99.f * PerLevel, 0.01f);
		TestEqual(TEXT("Full heal at the new MaxHealth"), GetAttr(UGSAttributeSet::GetHealthAttribute()), GetAttr(UGSAttributeSet::GetMaxHealthAttribute()), 0.01f);

		// HUD with a 3-digit level.
		TestEqual(TEXT("HUD level 100"), HUD->GetDisplayedLevel(), 100);
		TestEqual(TEXT("HUD level label"), HUD->GetLevelLabelText(), FString(TEXT("LV 100")));
		TestEqual(TEXT("HUD XP text"), HUD->GetXPLabelText(), FString(TEXT("0 / 3,070 XP")));
		TestEqual(TEXT("HUD XP bar empty"), HUD->GetDisplayedXPPercent(), 0.f, 0.0001f);
		TestEqual(TEXT("HUD health follows MaxHealth"), HUD->GetDisplayedMaxHealth(), BaseMaxHealth + 99.f * PerLevel, 0.01f);

		// Offers: one per level gained (99): one open, 98 queued, the game paused. Taken one at a time, each opens the
		// next with 3 different cards; the first one of the run has a weapon card; no reroll is used.
		TestTrue(TEXT("An offer is open"), Upgrades->HasOpenOffer());
		TestEqual(TEXT("98 offers queued behind it"), Upgrades->GetQueuedOffers(), 98);
		TestTrue(TEXT("Paused for the choice"), IsWorldPaused());
		const TArray<FGSUpgradeChoice> First = Upgrades->GetCurrentOffer();
		TestTrue(TEXT("The run's first offer has a weapon card"),
			First.ContainsByPredicate([](const FGSUpgradeChoice& C) { return C.Upgrade && C.Upgrade->Category == EGSUpgradeCategory::Weapon; }));
		const int32 PicksBefore = Upgrades->GetTotalPicks();
		const int32 Rerolls = Upgrades->GetRerollsLeft();
		bool bQueueOk = true;
		int32 Taken = 0;
		while (Upgrades->HasOpenOffer() && Taken < 200)
		{
			const TArray<UGSUpgradeData*> Choices = Upgrades->GetCurrentChoices();
			bQueueOk &= Choices.Num() == 3 && Choices[0] != Choices[1] && Choices[0] != Choices[2] && Choices[1] != Choices[2];
			bQueueOk &= Upgrades->GetQueuedOffers() == 98 - Taken;
			bQueueOk &= GameMode->ChooseUpgrade(PS, Taken % 3);
			++Taken;
		}
		TestEqual(TEXT("99 offers taken"), Taken, 99);
		TestTrue(TEXT("Each offer had 3 different cards and the queue went down by one per pick"), bQueueOk);
		TestEqual(TEXT("99 picks granted"), Upgrades->GetTotalPicks() - PicksBefore, 99);
		TestEqual(TEXT("No reroll used"), Upgrades->GetRerollsLeft(), Rerolls);
		TestFalse(TEXT("No offer left"), Upgrades->HasOpenOffer());
		TestFalse(TEXT("Unpaused after the last pick"), IsWorldPaused());

		// Half way to 101: the bar is half full.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("XPAdd"), { TEXT("1535") });
		TestEqual(TEXT("Still level 100"), Progression->GetLevel(), 100);
		TestEqual(TEXT("HUD XP text half way"), HUD->GetXPLabelText(), FString(TEXT("1,535 / 3,070 XP")));
		TestEqual(TEXT("HUD XP bar half"), HUD->GetDisplayedXPPercent(), 0.5f, 0.0001f);

		// One more level: 101, one offer.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("XPAdd"), { TEXT("1535") });
		TestEqual(TEXT("Level 101"), Progression->GetLevel(), 101);
		TestEqual(TEXT("HUD level label 101"), HUD->GetLevelLabelText(), FString(TEXT("LV 101")));
		TestEqual(TEXT("HUD XP text at 101"), HUD->GetXPLabelText(), FString(TEXT("0 / 3,100 XP")));
		TestTrue(TEXT("Level 101: an offer"), Upgrades->HasOpenOffer());
		TestEqual(TEXT("Level 101: nothing queued"), Upgrades->GetQueuedOffers(), 0);
		PickAllUpgradeOffers();

		// gs.XP.SetLevel past the old cap.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("XPSetLevel"), { TEXT("250") });
		TestEqual(TEXT("SetLevel 250"), Progression->GetLevel(), 250);
		TestEqual(TEXT("SetLevel: 0 XP into it"), Progression->GetCurrentXP(), 0);
		TestEqual(TEXT("Level 251 needs 100 + 30 x 249"), Progression->GetXPForNextLevel(), 7570);
		TestEqual(TEXT("HUD label LV 250"), HUD->GetLevelLabelText(), FString(TEXT("LV 250")));
		PickAllUpgradeOffers();

		// A new run starts at level 1 again.
		GameMode->RestartRunWithSeed(4502);
		TestEqual(TEXT("New run: level 1"), Progression->GetLevel(), 1);
		TestEqual(TEXT("New run: HUD label"), HUD->GetLevelLabelText(), FString(TEXT("LV 1")));
		TestEqual(TEXT("New run: HUD XP text"), HUD->GetXPLabelText(), FString(TEXT("0 / 100 XP")));
		TestEqual(TEXT("New run: base MaxHealth"), GetAttr(UGSAttributeSet::GetMaxHealthAttribute()), BaseMaxHealth, 0.01f);
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Razor Cards (Prompt 46): homing cards thrown at the nearest enemy
// ---------------------------------------------------------------------------

namespace GSTests
{
	UGSRazorCardsComponent* GetRazorCards()
	{
		const AGSCharacter* Player = GetPlayer();
		return Player ? Player->GetRazorCards() : nullptr;
	}

	struct FRazorState
	{
		FVector Lane = FVector::ZeroVector; // lane start
		FVector Dir = FVector::ForwardVector;
		FVector Side = FVector::RightVector;
		FVector PlayerSpot = FVector::ZeroVector;
		/** PlayerSpot on the ground (Z 0): where bandits are spawned from (SpawnEnemy adds the capsule half-height). */
		FVector Ground = FVector::ZeroVector;
		TWeakObjectPtr<AGSEnemyCharacter> Bandit;
		TWeakObjectPtr<AStaticMeshActor> Wall;
		TWeakObjectPtr<AGSProjectile> Card;
		FVector CardDir = FVector::ZeroVector;
		int32 ThrownBefore = 0;
		int32 HitsBefore = 0;
		int32 WorldBefore = 0;
		int32 ExpiredBefore = 0;
		float HealthBefore = 0.f;
		double WindowStart = 0.0;
		bool bScheduleOk = true;
		int32 ScheduleFrames = 0;
		FVector BanditStart = FVector::ZeroVector;
		float PlayerHealthBefore = 0.f;
		int32 KillsBefore = 0;
		int32 XPOrbsBefore = 0;
		int32 HealthOrbsBefore = 0;
	};

	/** Every card thrown in the current burst is on its exact schedule (1 + floor(elapsed / interval) cards by now). */
	bool RazorCardsOnSchedule()
	{
		const UGSRazorCardsComponent* Cards = GetRazorCards();
		if (!Cards || Cards->GetBurstShots() == 0)
		{
			return true;
		}
		const double Elapsed = GetPIEWorld()->GetTimeSeconds() - Cards->GetBurstStartTime();
		const int32 Expected = 1 + FMath::FloorToInt(Elapsed / Cards->GetBurstInterval() + 1e-6);
		return FMath::Abs(Cards->GetBurstShots() - Expected) <= (Cards->GetBurstShots() == 1 ? 1 : 0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSRazorCardsTest, "BanditBaller.Upgrades.RazorCards", GSTests::TestFlags)
bool FGSRazorCardsTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	TSharedRef<FRazorState> S = MakeShared<FRazorState>();

	// Data, a clear lane, the player at its side. Game time x4.
	Step([this, S]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const UGSRazorCardsSettings* Settings = GameMode->GetRazorCardsSettings();
		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("RazorCards"));
		if (!TestNotNull(TEXT("DA_RazorCardsSettings on BP_GSGameMode"), Settings) || !TestNotNull(TEXT("DA_Upgrade_RazorCards"), Upgrade)
			|| !TestNotNull(TEXT("Razor Cards component on the character"), GetRazorCards()))
		{
			return;
		}
		TestEqual(TEXT("6 damage per card"), Settings->Damage, 6.f);
		TestEqual(TEXT("2 cards a second"), Settings->CardsPerSecond, 2.f);
		TestEqual(TEXT("25 m range"), Settings->Range, 2500.f);
		TestEqual(TEXT("30 m/s"), Settings->CardSpeed, 3000.f);
		TestTrue(TEXT("Homing turns"), Settings->HomingTurnRate >= 360.f);
		TestEqual(TEXT("+10% damage per power"), Settings->DamageBonusPerPower, 0.1f);
		TestEqual(TEXT("+10% fire rate per power"), Settings->FireRateBonusPerPower, 0.1f);
		TestNotNull(TEXT("Damage effect (GE_Damage)"), Settings->DamageEffect.Get());
		TestTrue(TEXT("A card mesh and material"), Settings->CardMesh && Settings->CardMaterial);
		TestTrue(TEXT("A thin card"), Settings->CardScale.Z < Settings->CardScale.X * 0.1f);
		TestTrue(TEXT("Category Weapon"), Upgrade->Category == EGSUpgradeCategory::Weapon);
		TestEqual(TEXT("The first pick gives the weapon"), Upgrade->WeaponGivingPicks, 1);
		TestTrue(TEXT("Effect GE_Upgrade_RazorCards"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_RazorCards::StaticClass()));
		TestTrue(TEXT("In DA_UpgradePool"), GameMode->GetUpgradePool() && GameMode->GetUpgradePool()->Upgrades.Contains(Upgrade));
		TestEqual(TEXT("Card text, none owned"), Upgrade->GetFormattedDescription(0).ToString(), FString(TEXT("New weapon: throws homing cards at the nearest enemy.")));
		TestEqual(TEXT("Card line of a later Common pick"), Upgrade->FormatValueText(GameMode->GetUpgradePool()->GetPickValue(Upgrade, EGSUpgradeRarity::Common, 1)).ToString(),
			FString(TEXT("+10% Razor Cards damage and fire rate")));
		TestEqual(TEXT("The card's +10% matches the settings"), Upgrade->GetDisplayScale() / 100.f, Settings->DamageBonusPerPower, 0.0001f);

		if (!TestTrue(TEXT("A clear lane in the town"), FindClearLane(2600.f, S->Lane, S->Dir)))
		{
			return;
		}
		S->Side = FVector(-S->Dir.Y, S->Dir.X, 0.f);
		S->PlayerSpot = S->Lane + S->Dir * 1300.f + S->Side * 400.f;
		S->Ground = FVector(S->PlayerSpot.X, S->PlayerSpot.Y, 0.f);
		GetPlayer()->TeleportTo(S->PlayerSpot, (-S->Side).Rotation());
	});
	WaitFrames(this, 3);

	// The projectile's homing: a card thrown at a bandit whose target dies flies on straight.
	Step([this, S]()
	{
		S->Bandit = SpawnFrozenBandit(S->Ground + S->Dir * 1000.f);
		const UGSRazorCardsSettings* Settings = GetGameMode()->GetRazorCardsSettings();
		if (!TestTrue(TEXT("Bandit for the homing check"), S->Bandit.IsValid()) || !Settings)
		{
			return;
		}
		FGSProjectileLaunch Launch;
		Launch.Start = S->PlayerSpot + FVector(0.f, 0.f, 40.f);
		Launch.Direction = (S->Dir + S->Side * 0.6f).GetSafeNormal(); // off to the side: homing has to turn it
		Launch.Speed = Settings->CardSpeed;
		Launch.Lifetime = Settings->CardLifetime;
		Launch.Radius = Settings->CardRadius;
		Launch.HomingTarget = S->Bandit.Get();
		Launch.HomingTurnRate = Settings->HomingTurnRate;
		S->CardDir = Launch.Direction;
		S->Card = GetProjectiles()->FireProjectile(Launch);
		TestTrue(TEXT("Manual card launched"), S->Card.IsValid());
	});
	WaitFrames(this, 2);
	Step([this, S]()
	{
		if (!S->Card.IsValid() || !S->Bandit.IsValid())
		{
			return;
		}
		TestTrue(TEXT("Homing on the bandit"), S->Card->GetHomingTarget() == S->Bandit.Get());
		const FVector Now = S->Card->GetVelocity().GetSafeNormal();
		const FVector ToBandit = (S->Bandit->GetActorLocation() - S->Card->GetActorLocation()).GetSafeNormal();
		TestTrue(TEXT("It turned toward the bandit"), FVector::DotProduct(Now, ToBandit) > FVector::DotProduct(S->CardDir, ToBandit));
		ApplyDamage(S->Bandit->GetAbilitySystemComponent(), 100000.f, GetPlayerASC());
	});
	WaitFrames(this, 2);
	Step([this, S]()
	{
		if (S->Card.IsValid() && S->Card->IsActiveProjectile())
		{
			TestNull(TEXT("Target dead: no homing target"), S->Card->GetHomingTarget());
			S->CardDir = S->Card->GetVelocity().GetSafeNormal();
		}
	});
	WaitFrames(this, 2);
	Step([this, S]()
	{
		if (S->Card.IsValid() && S->Card->IsActiveProjectile())
		{
			TestTrue(TEXT("Target dead: it flies on straight"), FVector::DotProduct(S->Card->GetVelocity().GetSafeNormal(), S->CardDir) > 0.99999f);
		}
		RemoveAllEnemies(GetPIEWorld());
	});
	WaitUntil(this, TEXT("the manual card to end"), []() { return GetProjectiles()->GetNumActive() == 0; }, 10.0);

	// The weapon: no target, no cards (nothing in range, out of range, behind a wall).
	Step([this, S]()
	{
		GetPlayer()->TeleportTo(S->PlayerSpot, (-S->Side).Rotation());
		TestEqual(TEXT("gs.Upgrade.Grant RazorCards"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("RazorCards"), 1), 1);
		const UGSRazorCardsComponent* Cards = GetRazorCards();
		TestEqual(TEXT("Owned: level 1"), Cards->GetLevel(), 1);
		TestEqual(TEXT("A Common first pick: power 0"), Cards->GetPower(), 0.f);
		TestEqual(TEXT("2 cards a second"), Cards->GetThrowInterval(), 0.5, 1e-6);
		TestEqual(TEXT("x1 damage"), Cards->GetDamageMultiplier(), 1.f);
		S->ThrownBefore = Cards->GetCardsThrown();
	});
	WaitWorldSeconds(this, 1.0);
	Step([this, S]()
	{
		TestEqual(TEXT("No enemy: no cards"), GetRazorCards()->GetCardsThrown(), S->ThrownBefore);
		TestNull(TEXT("No enemy: no target"), GetRazorCards()->GetTarget());
		S->Bandit = SpawnToughBandit(S->Ground + FVector(0.f, 0.f, 2700.f)); // 27 m up (frozen, it floats): out of range
	});
	WaitWorldSeconds(this, 1.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Out of range: no cards"), GetRazorCards()->GetCardsThrown(), S->ThrownBefore);
		RemoveAllEnemies(GetPIEWorld());
		// A wall across the line (thin along the side axis), and a bandit straight across the lane behind it.
		S->Wall = SpawnTestWall(S->PlayerSpot - S->Side * 300.f, FMath::Abs(S->Side.X) > 0.5f ? FVector(0.5f, 4.f, 4.f) : FVector(4.f, 0.5f, 4.f));
		S->Bandit = SpawnToughBandit(S->Ground - S->Side * 600.f);
	});
	WaitWorldSeconds(this, 1.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Behind a wall: no cards"), GetRazorCards()->GetCardsThrown(), S->ThrownBefore);
		TestNull(TEXT("Behind a wall: no target"), GetRazorCards()->GetTarget());
		RemoveAllEnemies(GetPIEWorld());
		if (S->Wall.IsValid())
		{
			S->Wall->Destroy();
		}

		// A target in range: cards on the exact schedule, every one a hit for 6.
		S->Bandit = SpawnToughBandit(S->Ground - S->Side * 800.f);
		S->ThrownBefore = GetRazorCards()->GetCardsThrown();
		S->HitsBefore = GetProjectiles()->GetNumHitTargets();
		S->HealthBefore = S->Bandit.IsValid() ? GetHealth(S->Bandit->GetAbilitySystemComponent()) : 0.f;
		S->bScheduleOk = true;
		S->ScheduleFrames = 0;
		S->WindowStart = GetPIEWorld()->GetTimeSeconds();
	});
	WaitUntil(this, TEXT("2 s of cards at power 0"), [S]()
	{
		S->bScheduleOk &= RazorCardsOnSchedule();
		++S->ScheduleFrames;
		return GetPIEWorld()->GetTimeSeconds() - S->WindowStart >= 2.0;
	}, 10.0);
	WaitUntil(this, TEXT("cards in flight to land"), []() { return GetProjectiles()->GetNumActive() == 0; }, 10.0);
	Step([this, S]()
	{
		const int32 Thrown = GetRazorCards()->GetCardsThrown() - S->ThrownBefore;
		const int32 Hits = GetProjectiles()->GetNumHitTargets() - S->HitsBefore;
		const float Lost = S->HealthBefore - GetHealth(S->Bandit->GetAbilitySystemComponent());
		AddInfo(FString::Printf(TEXT("Power 0: %d cards in 2 s, %d hits, %.1f damage (%d frames on schedule: %s)"), Thrown, Hits, Lost, S->ScheduleFrames, S->bScheduleOk ? TEXT("yes") : TEXT("no")));
		TestTrue(TEXT("It throws when it has a target"), GetRazorCards()->GetTarget() == S->Bandit.Get());
		TestTrue(TEXT("About 4 cards in 2 s (2 a second)"), Thrown >= 4 && Thrown <= 5);
		TestTrue(TEXT("Every frame on the exact schedule"), S->bScheduleOk);
		TestEqual(TEXT("Every card hit"), Hits, Thrown);
		TestEqual(TEXT("6 damage per card"), Hits > 0 ? Lost / Hits : 0.f, 6.f, 0.01f);

		// Power 1 (a Common pick after the first): +10% damage and fire rate.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("RazorCards"), 1);
		TestEqual(TEXT("Power 1"), GetRazorCards()->GetPower(), 1.f);
		TestEqual(TEXT("Power 1: x1.1 damage"), GetRazorCards()->GetDamageMultiplier(), 1.1f, 0.0001f);
		TestEqual(TEXT("Power 1: 2.2 cards a second"), GetRazorCards()->GetThrowInterval(), 1.0 / 2.2, 1e-6);
		S->ThrownBefore = GetRazorCards()->GetCardsThrown();
		S->HitsBefore = GetProjectiles()->GetNumHitTargets();
		S->HealthBefore = GetHealth(S->Bandit->GetAbilitySystemComponent());
		S->bScheduleOk = true;
		S->WindowStart = GetPIEWorld()->GetTimeSeconds();
	});
	WaitUntil(this, TEXT("2 s of cards at power 1"), [S]()
	{
		S->bScheduleOk &= RazorCardsOnSchedule();
		return GetPIEWorld()->GetTimeSeconds() - S->WindowStart >= 2.0;
	}, 10.0);
	WaitUntil(this, TEXT("cards in flight to land"), []() { return GetProjectiles()->GetNumActive() == 0; }, 10.0);
	Step([this, S]()
	{
		const int32 Hits = GetProjectiles()->GetNumHitTargets() - S->HitsBefore;
		const float Lost = S->HealthBefore - GetHealth(S->Bandit->GetAbilitySystemComponent());
		TestTrue(TEXT("Power 1: on schedule, at the new interval"), S->bScheduleOk && FMath::IsNearlyEqual(GetRazorCards()->GetBurstInterval(), 1.0 / 2.2, 1e-6));
		TestEqual(TEXT("Power 1: 6.6 per card"), Hits > 0 ? Lost / Hits : 0.f, 6.6f, 0.01f);

		// An Epic pick (+2 power): power 3, x1.3 damage and fire rate.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("RazorCards"), 1, EGSUpgradeRarity::Epic);
		TestEqual(TEXT("Power 3"), GetRazorCards()->GetPower(), 3.f);
		TestEqual(TEXT("Power 3: 2.6 cards a second"), GetRazorCards()->GetThrowInterval(), 1.0 / 2.6, 1e-6);
		S->ThrownBefore = GetRazorCards()->GetCardsThrown();
		S->HitsBefore = GetProjectiles()->GetNumHitTargets();
		S->HealthBefore = GetHealth(S->Bandit->GetAbilitySystemComponent());
		S->WindowStart = GetPIEWorld()->GetTimeSeconds();
	});
	WaitWorldSeconds(this, 2.0);
	WaitUntil(this, TEXT("cards in flight to land"), []() { return GetProjectiles()->GetNumActive() == 0; }, 10.0);
	Step([this, S]()
	{
		const int32 Thrown = GetRazorCards()->GetCardsThrown() - S->ThrownBefore;
		const int32 Hits = GetProjectiles()->GetNumHitTargets() - S->HitsBefore;
		const float Lost = S->HealthBefore - GetHealth(S->Bandit->GetAbilitySystemComponent());
		AddInfo(FString::Printf(TEXT("Power 3: %d cards in 2 s, %.1f per card, %.1f damage a second"), Thrown, Hits > 0 ? Lost / Hits : 0.f, Lost / 2.f));
		TestEqual(TEXT("Power 3: 7.8 per card"), Hits > 0 ? Lost / Hits : 0.f, 7.8f, 0.01f);
		TestTrue(TEXT("Power 3: about 5 cards in 2 s"), Thrown >= 5 && Thrown <= 6);

		// A moving target: a bandit running down the lane past the player (4 m to the side).
		RemoveAllEnemies(GetPIEWorld());
		S->BanditStart = FVector(S->Lane.X, S->Lane.Y, 0.f) + S->Dir * 600.f;
		S->Bandit = SpawnFrozenBandit(S->BanditStart, S->BanditStart + S->Dir * 1000.f); // AI off, movement on
		if (S->Bandit.IsValid())
		{
			S->Bandit->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetMaxHealthAttribute(), 100000.f);
			S->Bandit->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 100000.f);
		}
		S->ThrownBefore = GetRazorCards()->GetCardsThrown();
		S->HitsBefore = GetProjectiles()->GetNumHitTargets();
		S->WorldBefore = GetProjectiles()->GetNumHitWorld();
		S->ExpiredBefore = GetProjectiles()->GetNumExpired();
		S->WindowStart = GetPIEWorld()->GetTimeSeconds();
	});
	WaitUntil(this, TEXT("3 s of cards at a running bandit"), [S]()
	{
		if (S->Bandit.IsValid())
		{
			S->Bandit->AddMovementInput(S->Dir, 1.f);
		}
		return GetPIEWorld()->GetTimeSeconds() - S->WindowStart >= 3.0;
	}, 15.0);
	WaitUntil(this, TEXT("cards in flight to land"), [S]()
	{
		if (S->Bandit.IsValid())
		{
			S->Bandit->AddMovementInput(S->Dir, 1.f);
		}
		return GetProjectiles()->GetNumActive() == 0;
	}, 10.0);
	Step([this, S]()
	{
		const int32 Thrown = GetRazorCards()->GetCardsThrown() - S->ThrownBefore;
		const int32 Hits = GetProjectiles()->GetNumHitTargets() - S->HitsBefore;
		const float Moved = S->Bandit.IsValid() ? FVector::Dist2D(S->Bandit->GetActorLocation(), S->BanditStart) : 0.f;
		const float Speed = S->Bandit.IsValid() ? S->Bandit->GetVelocity().Size2D() : 0.f;
		AddInfo(FString::Printf(TEXT("Moving target: ran %.0f cm (%.0f cm/s); %d cards, %d hits, %d hit the world, %d expired"), Moved, Speed, Thrown, Hits,
			GetProjectiles()->GetNumHitWorld() - S->WorldBefore, GetProjectiles()->GetNumExpired() - S->ExpiredBefore));
		TestTrue(TEXT("The bandit ran (over 10 m)"), Moved > 1000.f);
		TestTrue(TEXT("Cards were thrown at it"), Thrown >= 6);
		TestEqual(TEXT("Every card hit the running bandit"), Hits, Thrown);
		TestEqual(TEXT("No card hit the world"), GetProjectiles()->GetNumHitWorld() - S->WorldBefore, 0);
		TestEqual(TEXT("No card expired"), GetProjectiles()->GetNumExpired() - S->ExpiredBefore, 0);

		// Its kill is the player's: a weak bandit, Leech, a sure health orb.
		RemoveAllEnemies(GetPIEWorld());
		GetPlayer()->TeleportTo(S->PlayerSpot, (-S->Side).Rotation());
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Leech"), 1);
		GetGameMode()->SetHealthOrbDropChanceOverride(1.f);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		ApplyDamage(GetPlayerASC(), 10.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		S->PlayerHealthBefore = GetHealth(GetPlayerASC());
		GetPlayerCombo()->EndCombo(EGSComboEndReason::Reset);
		S->KillsBefore = GetGameState()->GetRunStats()->GetTotalKills();
		S->XPOrbsBefore = GetCashSubsystem()->GetNumXPOrbsDropped();
		S->HealthOrbsBefore = GetCashSubsystem()->GetNumHealthOrbsDropped();
		S->Bandit = SpawnFrozenBandit(S->Ground - S->Side * 800.f);
		if (S->Bandit.IsValid())
		{
			S->Bandit->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 5.f);
		}
	});
	WaitUntil(this, TEXT("a card to kill the bandit"), [S]() { return !S->Bandit.IsValid() || S->Bandit->GetHealthComponent()->IsDead(); }, 10.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Card kill: counted in run stats"), GetGameState()->GetRunStats()->GetTotalKills(), S->KillsBefore + 1);
		TestEqual(TEXT("Card kill: counts for the combo"), GetPlayerCombo()->GetComboCount(), 1);
		TestEqual(TEXT("Card kill: an XP orb"), GetCashSubsystem()->GetNumXPOrbsDropped(), S->XPOrbsBefore + 1);
		TestEqual(TEXT("Card kill: the health orb chance (1: one orb)"), GetCashSubsystem()->GetNumHealthOrbsDropped(), S->HealthOrbsBefore + 1);
		TestTrue(TEXT("Card kill: a cash drop"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) > 0);
		TestEqual(TEXT("Card kill: Leech heals"), GetHealth(GetPlayerASC()), S->PlayerHealthBefore + 1.f, 0.01f);
		GetGameMode()->SetHealthOrbDropChanceOverride(0.f);

		// Paused: no cards.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandit = SpawnToughBandit(S->Ground - S->Side * 800.f);
	});
	WaitUntil(this, TEXT("throwing at the next bandit"), [S]() { return GetRazorCards()->GetTarget() == S->Bandit.Get(); }, 10.0);
	Step([S]()
	{
		S->ThrownBefore = GetRazorCards()->GetCardsThrown();
		UGameplayStatics::SetGamePaused(GetPIEWorld(), true);
	});
	WaitRealSeconds(this, 0.5);
	Step([this, S]()
	{
		TestEqual(TEXT("Paused: no cards"), GetRazorCards()->GetCardsThrown(), S->ThrownBefore);
		UGameplayStatics::SetGamePaused(GetPIEWorld(), false);

		// A new run removes it.
		GetGameMode()->RestartRunWithSeed(4601);
		GetGameMode()->SetRoundTimerPaused(true);
		TestEqual(TEXT("New run: level 0"), GetRazorCards()->GetLevel(), 0);
		TestEqual(TEXT("New run: power 0"), GetRazorCards()->GetPower(), 0.f);
		TestEqual(TEXT("New run: no cards in flight"), GetProjectiles()->GetNumActive(), 0);
		GetPlayer()->TeleportTo(S->PlayerSpot, (-S->Side).Rotation());
		RemoveAllEnemies(GetPIEWorld());
		S->Bandit = SpawnToughBandit(S->Ground - S->Side * 800.f);
		S->ThrownBefore = GetRazorCards()->GetCardsThrown();
	});
	WaitWorldSeconds(this, 1.0);
	Step([this, S]()
	{
		TestEqual(TEXT("New run: no cards at a bandit in range"), GetRazorCards()->GetCardsThrown(), S->ThrownBefore);
		TestNull(TEXT("New run: no target"), GetRazorCards()->GetTarget());
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Barbed Wire (Prompt 47): a ring on the ground that hurts every enemy inside it on a fixed tick
// ---------------------------------------------------------------------------

namespace GSTests
{
	UGSBarbedWireComponent* GetBarbedWire()
	{
		const AGSCharacter* Player = GetPlayer();
		return Player ? Player->GetBarbedWire() : nullptr;
	}

	UGSDynamiteComponent* GetDynamite()
	{
		const AGSCharacter* Player = GetPlayer();
		return Player ? Player->GetDynamite() : nullptr;
	}

	UGSLuckyCoinComponent* GetLuckyCoin()
	{
		const AGSCharacter* Player = GetPlayer();
		return Player ? Player->GetLuckyCoin() : nullptr;
	}

	UGSMolotovComponent* GetMolotov()
	{
		const AGSCharacter* Player = GetPlayer();
		return Player ? Player->GetMolotov() : nullptr;
	}

	UGSTomahawkComponent* GetTomahawk()
	{
		const AGSCharacter* Player = GetPlayer();
		return Player ? Player->GetTomahawk() : nullptr;
	}

	struct FWireState
	{
		FVector PlayerSpot = FVector::ZeroVector;
		FVector Ground = FVector::ZeroVector;
		FVector Dir = FVector::ForwardVector;
		FVector Side = FVector::RightVector;
		TArray<TWeakObjectPtr<AGSEnemyCharacter>> Bandits; // [0] 2 m, [1] 3.3 m, [2] 3.8 m, [3] above
		TArray<float> HealthBefore;
		int32 TicksBefore = 0;
		bool bScheduleOk = true;
		int32 Frames = 0;
		TWeakObjectPtr<AGSEnemyCharacter> Weak;
		float PlayerHealthBefore = 0.f;
		int32 KillsBefore = 0;
		int32 XPOrbsBefore = 0;
		int32 HealthOrbsBefore = 0;
	};

	/** Every tick of the current run of ticks is on its exact schedule (1 + floor(elapsed / interval) ticks by now). */
	bool BarbedWireOnSchedule()
	{
		const UGSBarbedWireComponent* Wire = GetBarbedWire();
		const UGSBarbedWireSettings* Settings = Wire ? Wire->GetSettings() : nullptr;
		if (!Settings || Wire->GetScheduledTicks() == 0)
		{
			return true;
		}
		const double Elapsed = GetPIEWorld()->GetTimeSeconds() - Wire->GetFirstTickTime();
		return Wire->GetScheduledTicks() == 1 + FMath::FloorToInt(Elapsed / Settings->TickInterval + 1e-6);
	}

	void RecordWireHealth(FWireState& S)
	{
		S.HealthBefore.Reset();
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S.Bandits)
		{
			S.HealthBefore.Add(Bandit.IsValid() ? GetHealth(Bandit->GetAbilitySystemComponent()) : 0.f);
		}
		S.TicksBefore = GetBarbedWire()->GetTicks();
	}

	float WireHealthLost(const FWireState& S, int32 Index)
	{
		const TWeakObjectPtr<AGSEnemyCharacter>& Bandit = S.Bandits[Index];
		return Bandit.IsValid() ? S.HealthBefore[Index] - GetHealth(Bandit->GetAbilitySystemComponent()) : -1.f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSBarbedWireTest, "BanditBaller.Upgrades.BarbedWire", GSTests::TestFlags)
bool FGSBarbedWireTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	TSharedRef<FWireState> S = MakeShared<FWireState>();

	// Data, a clear lane, the player at its side. Game time x4.
	Step([this, S]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const UGSBarbedWireSettings* Settings = GameMode->GetBarbedWireSettings();
		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("BarbedWire"));
		if (!TestNotNull(TEXT("DA_BarbedWireSettings on BP_GSGameMode"), Settings) || !TestNotNull(TEXT("DA_Upgrade_BarbedWire"), Upgrade)
			|| !TestNotNull(TEXT("Barbed Wire component on the character"), GetBarbedWire()))
		{
			return;
		}
		TestEqual(TEXT("3.5 m radius"), Settings->Radius, 350.f);
		TestEqual(TEXT("5 damage per tick"), Settings->Damage, 5.f);
		TestEqual(TEXT("A tick every 0.5 s"), Settings->TickInterval, 0.5f);
		TestEqual(TEXT("+10% damage per power"), Settings->DamageBonusPerPower, 0.1f);
		TestEqual(TEXT("+5% radius per power"), Settings->RadiusBonusPerPower, 0.05f);
		TestNotNull(TEXT("Damage effect (GE_Damage)"), Settings->DamageEffect.Get());
		TestTrue(TEXT("Ring segment mesh and material"), Settings->SegmentMesh && Settings->SegmentMaterial);
		TestNotNull(TEXT("A hit effect"), Settings->HitEffect.Get());
		TestTrue(TEXT("Category Weapon"), Upgrade->Category == EGSUpgradeCategory::Weapon);
		TestEqual(TEXT("The first pick gives the weapon"), Upgrade->WeaponGivingPicks, 1);
		TestTrue(TEXT("Effect GE_Upgrade_BarbedWire"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_BarbedWire::StaticClass()));
		TestTrue(TEXT("In DA_UpgradePool"), GameMode->GetUpgradePool() && GameMode->GetUpgradePool()->Upgrades.Contains(Upgrade));
		TestEqual(TEXT("Card text, none owned"), Upgrade->GetFormattedDescription(0).ToString(), FString(TEXT("New weapon: a ring around you that hurts every enemy inside it.")));
		TestEqual(TEXT("Card line of a later Common pick"), Upgrade->FormatValueText(GameMode->GetUpgradePool()->GetPickValue(Upgrade, EGSUpgradeRarity::Common, 1)).ToString(),
			FString(TEXT("+10% Barbed Wire damage, +5% size")));
		TestEqual(TEXT("Card line of a Rare later pick (halves)"), Upgrade->FormatValueText(1.5f).ToString(), FString(TEXT("+15% Barbed Wire damage, +7.5% size")));
		TestEqual(TEXT("The card's +10% damage matches the settings"), Upgrade->GetDisplayScale() / 100.f, Settings->DamageBonusPerPower, 0.0001f);
		TestEqual(TEXT("The card's +5% size matches the settings"), Upgrade->SecondaryDisplayScale / 100.f, Settings->RadiusBonusPerPower, 0.0001f);

		FVector Lane;
		if (!TestTrue(TEXT("A clear lane in the town"), FindClearLane(2600.f, Lane, S->Dir)))
		{
			return;
		}
		S->Side = FVector(-S->Dir.Y, S->Dir.X, 0.f);
		S->PlayerSpot = Lane + S->Dir * 1300.f + S->Side * 200.f;
		S->Ground = FVector(S->PlayerSpot.X, S->PlayerSpot.Y, 0.f);
		GetPlayer()->TeleportTo(S->PlayerSpot, S->Dir.Rotation());
		TestFalse(TEXT("Not owned: no ring"), GetBarbedWire()->IsRingShown());
	});
	WaitFrames(this, 3);

	// Granted: the ring at the feet, no collision; four tough frozen bandits: two inside, one just outside, one above.
	Step([this, S]()
	{
		TestEqual(TEXT("gs.Upgrade.Grant BarbedWire"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("BarbedWire"), 1), 1);
		UGSBarbedWireComponent* Wire = GetBarbedWire();
		TestEqual(TEXT("Owned: level 1"), Wire->GetLevel(), 1);
		TestEqual(TEXT("A Common first pick: power 0"), Wire->GetPower(), 0.f);
		TestEqual(TEXT("Radius 350"), Wire->GetRadius(), 350.f, 0.01f);
		TestEqual(TEXT("5 per tick"), Wire->GetTickDamage(), 5.f, 0.0001f);
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Side * 200.f));
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 330.f));
		S->Bandits.Add(SpawnToughBandit(S->Ground - S->Dir * 380.f));
		S->Bandits.Add(SpawnToughBandit(S->Ground - S->Dir * 150.f + FVector(0.f, 0.f, 300.f))); // 1.5 m away, 3 m up: floats
		for (int32 Index = 0; Index < S->Bandits.Num(); ++Index)
		{
			TestTrue(FString::Printf(TEXT("Bandit %d spawned"), Index), S->Bandits[Index].IsValid());
		}
	});
	WaitFrames(this, 2);
	Step([this, S]()
	{
		UGSBarbedWireComponent* Wire = GetBarbedWire();
		const UGSBarbedWireSettings* Settings = Wire->GetSettings();
		TestTrue(TEXT("The ring is shown"), Wire->IsRingShown());
		TestEqual(TEXT("One instance per segment"), Wire->GetRing() ? Wire->GetRing()->GetInstanceCount() : 0, Settings->RingSegments);
		TestEqual(TEXT("Drawn at the radius"), Wire->GetShownRadius(), 350.f, 0.1f);
		TestTrue(TEXT("The ring has no collision"), Wire->GetRing() && Wire->GetRing()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		const FVector Feet = GetPlayer()->GetActorLocation() - FVector(0.f, 0.f, GetPlayer()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		TestTrue(TEXT("The ring lies at the feet"), Wire->GetRing() && FVector::Dist(Wire->GetRing()->GetComponentLocation(), Feet) < Settings->HeightAboveFloor + 1.f);
		const FString Names[] = { TEXT("2 m"), TEXT("3.3 m"), TEXT("3.8 m"), TEXT("3 m up") };
		const bool Expect[] = { true, true, false, false };
		for (int32 Index = 0; Index < S->Bandits.Num(); ++Index)
		{
			const AGSEnemyCharacter* Bandit = S->Bandits[Index].Get();
			const FVector Delta = Bandit ? Bandit->GetActorLocation() - GetPlayer()->GetActorLocation() : FVector::ZeroVector;
			AddInfo(FString::Printf(TEXT("Bandit %s: %.0f cm flat, %.0f cm up"), *Names[Index], Delta.Size2D(), Delta.Z));
			TestEqual(FString::Printf(TEXT("Bandit %s inside?"), *Names[Index]), Wire->IsInside(Bandit), Expect[Index]);
		}
		RecordWireHealth(*S);
		S->bScheduleOk = true;
		S->Frames = 0;
	});
	WaitUntil(this, TEXT("4 ticks at power 0"), [S]()
	{
		S->bScheduleOk &= BarbedWireOnSchedule();
		++S->Frames;
		return GetBarbedWire()->GetTicks() >= S->TicksBefore + 4;
	}, 10.0);
	Step([this, S]()
	{
		UGSBarbedWireComponent* Wire = GetBarbedWire();
		const int32 Ticks = Wire->GetTicks() - S->TicksBefore;
		TestTrue(FString::Printf(TEXT("Every frame on the exact schedule (%d frames)"), S->Frames), S->bScheduleOk);
		TestEqual(TEXT("Each tick hits the two inside"), Wire->GetLastTickHits(), 2);
		TestEqual(TEXT("2 m: 5 per tick"), WireHealthLost(*S, 0), 5.f * Ticks, 0.01f);
		TestEqual(TEXT("3.3 m: 5 per tick"), WireHealthLost(*S, 1), 5.f * Ticks, 0.01f);
		TestEqual(TEXT("3.8 m (outside): nothing"), WireHealthLost(*S, 2), 0.f, 0.01f);
		TestEqual(TEXT("3 m up (outside): nothing"), WireHealthLost(*S, 3), 0.f, 0.01f);

		// Power 1: +10% damage, +5% radius (367.5 cm: the 3.8 m bandit is still outside).
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("BarbedWire"), 1);
		TestEqual(TEXT("Power 1"), Wire->GetPower(), 1.f);
		TestEqual(TEXT("Power 1: radius 367.5"), Wire->GetRadius(), 367.5f, 0.01f);
		TestEqual(TEXT("Power 1: 5.5 per tick"), Wire->GetTickDamage(), 5.5f, 0.0001f);
		TestFalse(TEXT("Power 1: 3.8 m still outside"), Wire->IsInside(S->Bandits[2].Get()));
		RecordWireHealth(*S);
	});
	WaitUntil(this, TEXT("3 ticks at power 1"), [S]() { return GetBarbedWire()->GetTicks() >= S->TicksBefore + 3; }, 10.0);
	Step([this, S]()
	{
		UGSBarbedWireComponent* Wire = GetBarbedWire();
		const int32 Ticks = Wire->GetTicks() - S->TicksBefore;
		TestEqual(TEXT("The ring is redrawn at the new radius"), Wire->GetShownRadius(), 367.5f, 0.1f);
		TestEqual(TEXT("Power 1: 5.5 per tick inside"), WireHealthLost(*S, 0), 5.5f * Ticks, 0.01f);
		TestEqual(TEXT("Power 1: still nothing at 3.8 m"), WireHealthLost(*S, 2), 0.f, 0.01f);

		// An Epic pick (+2 power): power 3, x1.3 damage, radius 402.5 cm: now the 3.8 m bandit is inside.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("BarbedWire"), 1, EGSUpgradeRarity::Epic);
		TestEqual(TEXT("Power 3"), Wire->GetPower(), 3.f);
		TestEqual(TEXT("Power 3: radius 402.5"), Wire->GetRadius(), 402.5f, 0.01f);
		TestEqual(TEXT("Power 3: 6.5 per tick"), Wire->GetTickDamage(), 6.5f, 0.0001f);
		TestTrue(TEXT("Power 3: the 3.8 m bandit is inside the bigger ring"), Wire->IsInside(S->Bandits[2].Get()));
		RecordWireHealth(*S);
	});
	WaitUntil(this, TEXT("3 ticks at power 3"), [S]() { return GetBarbedWire()->GetTicks() >= S->TicksBefore + 3; }, 10.0);
	Step([this, S]()
	{
		UGSBarbedWireComponent* Wire = GetBarbedWire();
		const int32 Ticks = Wire->GetTicks() - S->TicksBefore;
		TestEqual(TEXT("Power 3: three inside each tick"), Wire->GetLastTickHits(), 3);
		TestEqual(TEXT("Power 3: 6.5 per tick at 2 m"), WireHealthLost(*S, 0), 6.5f * Ticks, 0.01f);
		TestEqual(TEXT("Power 3: 6.5 per tick at 3.8 m"), WireHealthLost(*S, 2), 6.5f * Ticks, 0.01f);
		TestEqual(TEXT("Power 3: still nothing 3 m up"), WireHealthLost(*S, 3), 0.f, 0.01f);
		TestEqual(TEXT("The ring is redrawn at 402.5"), Wire->GetShownRadius(), 402.5f, 0.1f);

		// Its kill is the player's: a weak bandit inside, Leech, a sure health orb.
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Leech"), 1);
		GetGameMode()->SetHealthOrbDropChanceOverride(1.f);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		ApplyDamage(GetPlayerASC(), 10.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		S->PlayerHealthBefore = GetHealth(GetPlayerASC());
		GetPlayerCombo()->EndCombo(EGSComboEndReason::Reset);
		S->KillsBefore = GetGameState()->GetRunStats()->GetTotalKills();
		S->XPOrbsBefore = GetCashSubsystem()->GetNumXPOrbsDropped();
		S->HealthOrbsBefore = GetCashSubsystem()->GetNumHealthOrbsDropped();
		S->Weak = SpawnFrozenBandit(S->Ground + S->Side * 200.f);
		if (S->Weak.IsValid())
		{
			S->Weak->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 3.f);
		}
	});
	WaitUntil(this, TEXT("the ring to kill the bandit"), [S]() { return !S->Weak.IsValid() || S->Weak->GetHealthComponent()->IsDead(); }, 10.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Ring kill: counted in run stats"), GetGameState()->GetRunStats()->GetTotalKills(), S->KillsBefore + 1);
		TestEqual(TEXT("Ring kill: counts for the combo"), GetPlayerCombo()->GetComboCount(), 1);
		TestEqual(TEXT("Ring kill: an XP orb"), GetCashSubsystem()->GetNumXPOrbsDropped(), S->XPOrbsBefore + 1);
		TestEqual(TEXT("Ring kill: the health orb chance (1: one orb)"), GetCashSubsystem()->GetNumHealthOrbsDropped(), S->HealthOrbsBefore + 1);
		TestTrue(TEXT("Ring kill: a cash drop"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) > 0);
		TestEqual(TEXT("Ring kill: Leech heals"), GetHealth(GetPlayerASC()), S->PlayerHealthBefore + 1.f, 0.01f);
		GetGameMode()->SetHealthOrbDropChanceOverride(0.f);

		// Paused: no ticks.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Side * 200.f));
		S->TicksBefore = GetBarbedWire()->GetTicks();
		UGameplayStatics::SetGamePaused(GetPIEWorld(), true);
	});
	WaitRealSeconds(this, 0.5);
	Step([this, S]()
	{
		TestEqual(TEXT("Paused: no ticks"), GetBarbedWire()->GetTicks(), S->TicksBefore);
		UGameplayStatics::SetGamePaused(GetPIEWorld(), false);

		// A new run removes it.
		GetGameMode()->RestartRunWithSeed(4701);
		GetGameMode()->SetRoundTimerPaused(true);
		GetPlayer()->TeleportTo(S->PlayerSpot, S->Dir.Rotation());
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Side * 200.f));
		RecordWireHealth(*S);
	});
	WaitWorldSeconds(this, 1.5);
	Step([this, S]()
	{
		UGSBarbedWireComponent* Wire = GetBarbedWire();
		TestEqual(TEXT("New run: level 0"), Wire->GetLevel(), 0);
		TestEqual(TEXT("New run: power 0"), Wire->GetPower(), 0.f);
		TestFalse(TEXT("New run: no ring"), Wire->IsRingShown());
		TestEqual(TEXT("New run: no ticks"), Wire->GetTicks(), S->TicksBefore);
		TestEqual(TEXT("New run: a bandit next to the player takes nothing"), WireHealthLost(*S, 0), 0.f, 0.01f);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// M2b + M2c checkpoint (Prompt 48): the rules the other tests didn't cover yet
// ---------------------------------------------------------------------------

namespace GSTests
{
	/** Every attribute of the player's set (IncomingDamage, a meta attribute, left out) and its current value. */
	TMap<FString, float> SnapshotPlayerAttributes()
	{
		TMap<FString, float> Values;
		TArray<FGameplayAttribute> Attributes;
		UAttributeSet::GetAttributesFromSetClass(UGSAttributeSet::StaticClass(), Attributes);
		for (const FGameplayAttribute& Attribute : Attributes)
		{
			if (Attribute != UGSAttributeSet::GetIncomingDamageAttribute())
			{
				Values.Add(Attribute.GetName(), GetAttr(Attribute));
			}
		}
		return Values;
	}
}

// Weapon power for every rarity, then every upgrade at every rarity followed by a new run: everything back to the start.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSCheckpointPicksAndNewRunTest, "BanditBaller.Checkpoint.WeaponPicksAndNewRun", GSTests::TestFlags)
bool FGSCheckpointPicksAndNewRunTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);

	struct FState
	{
		TMap<FString, float> Base;
		int32 BaseRerolls = 0;
		int32 BaseDashCharges = 0;
	};
	TSharedRef<FState> S = MakeShared<FState>();

	// Weapon picks: Common, Rare, Epic, Legendary in turn; the giving picks add the multiplier - 1, later ones the multiplier.
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		struct FWeapon { const TCHAR* Name; FGameplayAttribute Power; float Expected[4]; };
		const FWeapon Weapons[] = {
			{ TEXT("FloatingGun"), UGSAttributeSet::GetFloatingGunPowerAttribute(), { 0.f, 0.5f, 2.5f, 5.5f } }, // two giving picks
			{ TEXT("RazorCards"), UGSAttributeSet::GetRazorCardsPowerAttribute(), { 0.f, 1.5f, 3.5f, 6.5f } },
			{ TEXT("Dynamite"), UGSAttributeSet::GetDynamitePowerAttribute(), { 0.f, 1.5f, 3.5f, 6.5f } },
			{ TEXT("LuckyCoin"), UGSAttributeSet::GetLuckyCoinPowerAttribute(), { 0.f, 1.5f, 3.5f, 6.5f } },
			{ TEXT("Molotov"), UGSAttributeSet::GetMolotovPowerAttribute(), { 0.f, 1.5f, 3.5f, 6.5f } },
			{ TEXT("Tomahawk"), UGSAttributeSet::GetTomahawkPowerAttribute(), { 0.f, 1.5f, 3.5f, 6.5f } },
			{ TEXT("BarbedWire"), UGSAttributeSet::GetBarbedWirePowerAttribute(), { 0.f, 1.5f, 3.5f, 6.5f } },
		};
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		int32 Seed = 4800;
		for (const FWeapon& W : Weapons)
		{
			GameMode->RestartRunWithSeed(Seed++);
			GameMode->SetRoundTimerPaused(true);
			const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(W.Name);
			if (!TestNotNull(W.Name, Upgrade))
			{
				continue;
			}
			for (const EGSUpgradeRarity Rarity : UGSUpgradePool::GetAllRarities())
			{
				const int32 R = static_cast<int32>(Rarity);
				TestTrue(FString::Printf(TEXT("%s: %s pick granted"), W.Name, *UGSUpgradeData::GetRarityName(Rarity)), Upgrades->GrantUpgrade(Upgrade, Rarity, TEXT("Test")));
				TestEqual(FString::Printf(TEXT("%s: power after the %s pick"), W.Name, *UGSUpgradeData::GetRarityName(Rarity)), GetAttr(W.Power), W.Expected[R], 0.0001f);
			}
		}
		// The last run holds Barbed Wire at power 6.5: x1.65 damage, radius 350 x 1.325.
		TestEqual(TEXT("Barbed Wire at power 6.5: x1.65 damage"), GetBarbedWire()->GetDamageMultiplier(), 1.65f, 0.0001f);
		TestEqual(TEXT("Barbed Wire at power 6.5: radius 463.75"), GetBarbedWire()->GetRadius(), 463.75f, 0.01f);
	});

	// A fresh run's starting values, then every upgrade at every rarity (whole units skip Rare), level-ups, a reroll.
	Step([this, S]()
	{
		AGSGameMode* GameMode = GetGameMode();
		GameMode->RestartRunWithSeed(4810);
		GameMode->SetRoundTimerPaused(true);
		S->Base = SnapshotPlayerAttributes();
		S->BaseRerolls = GetPlayerUpgrades()->GetRerollsLeft();
		S->BaseDashCharges = GetDashMaxCharges();
		TestEqual(TEXT("A new run starts with 1 dash charge"), S->BaseDashCharges, 1);

		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		int32 Granted = 0;
		for (const UGSUpgradeData* Upgrade : GameMode->GetUpgradePool()->Upgrades)
		{
			for (const EGSUpgradeRarity Rarity : UGSUpgradePool::GetAllRarities())
			{
				if (Upgrade->bWholeUnits && Rarity == EGSUpgradeRarity::Rare)
				{
					continue;
				}
				Granted += Upgrades->GrantUpgrade(Upgrade, Rarity, TEXT("Test")) ? 1 : 0;
			}
		}
		AddInfo(FString::Printf(TEXT("%d picks granted across %d upgrades"), Granted, GameMode->GetUpgradePool()->Upgrades.Num()));
		AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		GameMode->AwardXP(PS, static_cast<float>(GameMode->GetXPCurve()->GetTotalXPToReachLevel(4)), false);
		TestTrue(TEXT("A reroll used"), GameMode->RerollUpgradeOffer(PS));
		PickAllUpgradeOffers();
		TestTrue(TEXT("Damage raised"), GetAttr(UGSAttributeSet::GetDamageAttribute()) > S->Base[TEXT("Damage")]);
		TestTrue(TEXT("Air jumps"), GetAttr(UGSAttributeSet::GetExtraJumpsAttribute()) > 0.f);
		TestTrue(TEXT("MaxHealth raised by the level-ups"), GetAttr(UGSAttributeSet::GetMaxHealthAttribute()) > S->Base[TEXT("MaxHealth")]);
	});
	WaitFrames(this, 3);
	Step([this]()
	{
		TestEqual(TEXT("Before the new run: two floating guns"), GetPlayer()->GetFloatingGuns()->GetGuns().Num(), 2);
		TestTrue(TEXT("Before the new run: the Barbed Wire ring"), GetBarbedWire()->IsRingShown());
		TestTrue(TEXT("Before the new run: Razor Cards owned"), GetRazorCards()->GetLevel() > 0);
		TestTrue(TEXT("Before the new run: Dynamite owned"), GetDynamite()->GetLevel() > 0);
		TestTrue(TEXT("Before the new run: Lucky Coin owned"), GetLuckyCoin()->GetLevel() > 0);
		TestTrue(TEXT("Before the new run: Molotov owned"), GetMolotov()->GetLevel() > 0);
		TestTrue(TEXT("Before the new run: Tomahawk owned"), GetTomahawk()->GetLevel() > 0);
		TestTrue(TEXT("Before the new run: shields"), GetPC()->GetPlayerState<AGSPlayerState>()->GetShieldComponent()->GetNumShields() > 0);
		GetGameMode()->RestartRunWithSeed(4811);
		GetGameMode()->SetRoundTimerPaused(true);
	});
	WaitFrames(this, 3);
	Step([this, S]()
	{
		// Every attribute back to the fresh run's value.
		const TMap<FString, float> After = SnapshotPlayerAttributes();
		for (const TPair<FString, float>& Entry : S->Base)
		{
			const float* Now = After.Find(Entry.Key);
			TestTrue(FString::Printf(TEXT("New run: %s back to %.3f (now %.3f)"), *Entry.Key, Entry.Value, Now ? *Now : -1.f),
				Now && FMath::IsNearlyEqual(*Now, Entry.Value, 0.001f));
		}
		AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		TestEqual(TEXT("New run: no picks"), Upgrades->GetTotalPicks(), 0);
		TestFalse(TEXT("New run: no offer"), Upgrades->HasOpenOffer());
		TestEqual(TEXT("New run: rerolls refilled"), Upgrades->GetRerollsLeft(), S->BaseRerolls);
		TestEqual(TEXT("New run: no weapons"), Upgrades->GetOwnedWeapons().Num(), 0);
		TestTrue(TEXT("New run: the first offer's weapon card is pending again"), Upgrades->IsFirstOfferPending());
		TestEqual(TEXT("New run: level 1"), PS->GetProgressionComponent()->GetLevel(), 1);
		TestEqual(TEXT("New run: 1 dash charge"), GetDashMaxCharges(), S->BaseDashCharges);
		TestEqual(TEXT("New run: no shields"), PS->GetShieldComponent()->GetNumShields(), 0);
		TestEqual(TEXT("New run: no floating guns"), GetPlayer()->GetFloatingGuns()->GetGuns().Num(), 0);
		TestFalse(TEXT("New run: no ring"), GetBarbedWire()->IsRingShown());
		TestEqual(TEXT("New run: Razor Cards gone"), GetRazorCards()->GetLevel(), 0);
		TestEqual(TEXT("New run: Dynamite gone"), GetDynamite()->GetLevel(), 0);
		TestEqual(TEXT("New run: Lucky Coin gone"), GetLuckyCoin()->GetLevel(), 0);
		TestEqual(TEXT("New run: Molotov gone"), GetMolotov()->GetLevel(), 0);
		TestEqual(TEXT("New run: Tomahawk gone"), GetTomahawk()->GetLevel(), 0);
		TestEqual(TEXT("New run: nothing thrown is left in play"), GetPIEWorld()->GetSubsystem<UGSThrownSubsystem>()->GetActive().Num(), 0);
		TestEqual(TEXT("New run: no combo"), PS->GetComboComponent()->GetComboCount(), 0);
		TestEqual(TEXT("New run: no Powder Keg blasts"), GetGameMode()->GetPowderKegBlasts(), 0);
		TestEqual(TEXT("New run: no damage dealt"), GetGameState()->GetRunStats()->GetDamageDealt(), 0.f);
		const UGSHUDWidget* HUD = GetHUD();
		TestTrue(TEXT("New run: HUD weapon slots empty"), HUD && !HUD->GetWeaponInSlot(0) && !HUD->GetWeaponInSlot(1) && !HUD->GetWeaponInSlot(2));
		TestEqual(TEXT("New run: no upgrade icons"), HUD ? HUD->GetUpgradeIconCount() : -1, 0);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
	});
	EndPIE();
	return true;
}

// MaxWeapons with all three real weapons owned (plus a fourth test weapon in a pool copy), the HUD slots, the auto-pick.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSCheckpointWeaponLimitTest, "BanditBaller.Checkpoint.WeaponLimitAllThree", GSTests::TestFlags)
bool FGSCheckpointWeaponLimitTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	TSharedRef<TStrongObjectPtr<UGSUpgradePool>> Pool = MakeShared<TStrongObjectPtr<UGSUpgradePool>>();
	TSharedRef<TStrongObjectPtr<UGSUpgradeData>> Extra = MakeShared<TStrongObjectPtr<UGSUpgradeData>>();

	// The real pool has more weapons than slots: with three owned, no other real weapon is offered (offers and rerolls).
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		GameMode->RestartRunWithSeed(4819);
		GameMode->SetRoundTimerPaused(true);
		AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		const UGSUpgradePool* RealPool = GameMode->GetUpgradePool();
		TArray<const UGSUpgradeData*> Owned, Unowned;
		for (const UGSUpgradeData* Upgrade : RealPool->Upgrades)
		{
			if (Upgrade->Category == EGSUpgradeCategory::Weapon)
			{
				(Owned.Num() < RealPool->MaxWeapons ? Owned : Unowned).Add(Upgrade);
			}
		}
		if (!TestTrue(TEXT("The real pool has more weapons than MaxWeapons"), Unowned.Num() > 0))
		{
			return;
		}
		for (const UGSUpgradeData* Weapon : Owned)
		{
			Upgrades->GrantUpgrade(Weapon, EGSUpgradeRarity::Common, TEXT("Test"));
		}
		TestEqual(TEXT("Real pool: three weapons owned"), Upgrades->GetOwnedWeapons().Num(), RealPool->MaxWeapons);
		bool bUnownedSeen = false;
		int32 OwnedSeen = 0, Rerolls = 0;
		auto Look = [&](const TArray<FGSUpgradeChoice>& Offer)
		{
			for (const FGSUpgradeChoice& Choice : Offer)
			{
				bUnownedSeen |= Unowned.Contains(Choice.Upgrade);
				OwnedSeen += Owned.Contains(Choice.Upgrade) ? 1 : 0;
			}
		};
		for (int32 i = 0; i < 60; ++i)
		{
			GameMode->QueueUpgradeOfferForDebug(PS);
			Look(Upgrades->GetCurrentOffer());
			if (GameMode->RerollUpgradeOffer(PS))
			{
				++Rerolls;
				Look(Upgrades->GetCurrentOffer());
			}
			GameMode->ChooseUpgrade(PS, FirstNonWeaponCard(Upgrades->GetCurrentOffer()));
		}
		AddInfo(FString::Printf(TEXT("Real pool: %d unowned weapon(s); owned weapon cards in 60 offers + %d rerolls: %d"), Unowned.Num(), Rerolls, OwnedSeen));
		TestFalse(TEXT("Real pool, three weapons owned: an unowned weapon is never offered (offers and rerolls)"), bUnownedSeen);
		TestTrue(TEXT("Real pool: the owned weapons are still offered"), OwnedSeen > 0);
		TestEqual(TEXT("Real pool: still three weapons"), Upgrades->GetOwnedWeapons().Num(), RealPool->MaxWeapons);
	});

	Step([this, Pool, Extra]()
	{
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		AGSPlayerState* PS = GetPC()->GetPlayerState<AGSPlayerState>();
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		Pool->Reset(DuplicateObject<UGSUpgradePool>(GameMode->GetUpgradePool(), GetTransientPackage()));
		Extra->Reset(MakeTestUpgrade(TEXT("TestWeapon_Fourth"), EGSUpgradeCategory::Weapon));
		Pool->Get()->Upgrades.Add(Extra->Get());
		Pool->Get()->MaxWeapons = 3;
		GameMode->SetUpgradePoolOverride(Pool->Get());
		GameMode->RestartRunWithSeed(4820);
		GameMode->SetRoundTimerPaused(true);

		const UGSUpgradeData* Wire = UGSDebugLibrary::FindUpgradeData(TEXT("BarbedWire"));
		const UGSUpgradeData* Cards = UGSDebugLibrary::FindUpgradeData(TEXT("RazorCards"));
		const UGSUpgradeData* Gun = UGSDebugLibrary::FindUpgradeData(TEXT("FloatingGun"));
		for (const TCHAR* Name : { TEXT("BarbedWire"), TEXT("RazorCards"), TEXT("FloatingGun") })
		{
			UGSDebugLibrary::GrantUpgradeTo(GetPC(), Name, 1);
		}
		const UGSHUDWidget* HUD = GetHUD();
		TestTrue(TEXT("HUD slots in pick order: Barbed Wire, Razor Cards, Floating Gun"),
			HUD && HUD->GetWeaponInSlot(0) == Wire && HUD->GetWeaponInSlot(1) == Cards && HUD->GetWeaponInSlot(2) == Gun);
		TestTrue(TEXT("No weapon in the upgrade row"), HUD && !HUD->FindUpgradeIcon(Wire) && !HUD->FindUpgradeIcon(Cards) && !HUD->FindUpgradeIcon(Gun));
		TestFalse(TEXT("Slots full: the fourth weapon can't be offered"), Pool->Get()->CanOfferWeapon(Extra->Get(), [Upgrades](const UGSUpgradeData* U) { return Upgrades->GetStacks(U); }));

		// 60 offers, each rerolled once: the fourth weapon never; the owned ones still.
		bool bFourthSeen = false;
		int32 OwnedSeen = 0;
		auto Look = [&](const TArray<FGSUpgradeChoice>& Offer)
		{
			for (const FGSUpgradeChoice& Choice : Offer)
			{
				bFourthSeen |= Choice.Upgrade == Extra->Get();
				OwnedSeen += (Choice.Upgrade == Wire || Choice.Upgrade == Cards || Choice.Upgrade == Gun) ? 1 : 0;
			}
		};
		for (int32 i = 0; i < 60; ++i)
		{
			GameMode->QueueUpgradeOfferForDebug(PS);
			Look(Upgrades->GetCurrentOffer());
			if (GameMode->RerollUpgradeOffer(PS))
			{
				Look(Upgrades->GetCurrentOffer());
			}
			GameMode->ChooseUpgrade(PS, FirstNonWeaponCard(Upgrades->GetCurrentOffer()));
		}
		AddInfo(FString::Printf(TEXT("Owned weapon cards in 60 offers + rerolls: %d"), OwnedSeen));
		TestFalse(TEXT("Three weapons owned: an unowned weapon is never offered (offers and rerolls)"), bFourthSeen);
		TestTrue(TEXT("The owned weapons are still offered"), OwnedSeen > 0);
		TestEqual(TEXT("Still three weapons"), Upgrades->GetOwnedWeapons().Num(), 3);

		// The debug auto-pick policy (gs.Debug.AutoPickUpgrades).
		const UGSUpgradeData* Damage = UGSDebugLibrary::FindUpgradeData(TEXT("Damage"));
		auto Card = [](const UGSUpgradeData* U, EGSUpgradeRarity R) { FGSUpgradeChoice C; C.Upgrade = const_cast<UGSUpgradeData*>(U); C.Rarity = R; return C; };
		const TArray<FGSUpgradeChoice> WithWeapons = { Card(Damage, EGSUpgradeRarity::Epic), Card(Extra->Get(), EGSUpgradeRarity::Common), Card(Cards, EGSUpgradeRarity::Common) };
		TestEqual(TEXT("Auto-pick, slots full: the owned weapon"), UGSDebugLibrary::ChooseUpgradeForAutoPick(WithWeapons, Upgrades, 3, true), 2);
		TestEqual(TEXT("Auto-pick, a free slot: the new weapon"), UGSDebugLibrary::ChooseUpgradeForAutoPick(WithWeapons, Upgrades, 4, true), 1);
		const TArray<FGSUpgradeChoice> Plain = { Card(Damage, EGSUpgradeRarity::Common), Card(UGSDebugLibrary::FindUpgradeData(TEXT("Leech")), EGSUpgradeRarity::Common) };
		TestEqual(TEXT("Auto-pick, all Common: reroll"), UGSDebugLibrary::ChooseUpgradeForAutoPick(Plain, Upgrades, 3, true), INDEX_NONE);
		TestEqual(TEXT("Auto-pick, all Common, no rerolls: the first"), UGSDebugLibrary::ChooseUpgradeForAutoPick(Plain, Upgrades, 3, false), 0);
		const TArray<FGSUpgradeChoice> Mixed = { Card(Damage, EGSUpgradeRarity::Common), Card(UGSDebugLibrary::FindUpgradeData(TEXT("Leech")), EGSUpgradeRarity::Legendary) };
		TestEqual(TEXT("Auto-pick: the rarest"), UGSDebugLibrary::ChooseUpgradeForAutoPick(Mixed, Upgrades, 3, true), 1);

		// One more slot: now the fourth weapon can be offered.
		Pool->Get()->MaxWeapons = 4;
		bool bOffered = false;
		for (int32 i = 0; i < 80 && !bOffered; ++i)
		{
			GameMode->QueueUpgradeOfferForDebug(PS);
			Look(Upgrades->GetCurrentOffer());
			bOffered = bFourthSeen;
			GameMode->ChooseUpgrade(PS, FirstNonWeaponCard(Upgrades->GetCurrentOffer()));
		}
		TestTrue(TEXT("MaxWeapons 4: the fourth weapon is offered"), bOffered);
		GameMode->SetUpgradePoolOverride(nullptr);
	});
	EndPIE();
	return true;
}

// Powder Keg at 100%: a chain along a line of bandits ends once nobody is left in reach; the player is never hurt.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSPowderKegChainEndsTest, "BanditBaller.Upgrades.PowderKegChainEnds", GSTests::TestFlags)
bool FGSPowderKegChainEndsTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	struct FState
	{
		TArray<TWeakObjectPtr<AGSEnemyCharacter>> Bandits;
		int32 BlastsBefore = 0;
		float PlayerHealth = 0.f;
	};
	TSharedRef<FState> S = MakeShared<FState>();
	Step([this, S]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		FVector Lane, Dir;
		if (!TestTrue(TEXT("A clear lane"), FindClearLane(2600.f, Lane, Dir)))
		{
			return;
		}
		GetPlayer()->TeleportTo(Lane, Dir.Rotation());
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("PowderKeg"), 34); // 102%: every kill explodes
		const FVector Ground(Lane.X, Lane.Y, 0.f);
		for (int32 Index = 0; Index < 8; ++Index)
		{
			// 2.5 m apart: each blast (3 m) reaches the next bandit only (6 m when one is double size).
			AGSEnemyCharacter* Bandit = SpawnFrozenBandit(Ground + Dir * (600.f + 250.f * Index));
			if (Bandit)
			{
				Bandit->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 10.f);
			}
			S->Bandits.Add(Bandit);
		}
		S->BlastsBefore = GameMode->GetPowderKegBlasts();
		S->PlayerHealth = GetHealth(GetPlayerASC());
	});
	WaitFrames(this, 2);
	Step([S]()
	{
		if (S->Bandits.Num() > 0 && S->Bandits[0].IsValid())
		{
			ApplyDamage(S->Bandits[0]->GetAbilitySystemComponent(), 1000.f, GetPlayerASC());
		}
	});
	WaitUntil(this, TEXT("the chain to run down the line and end"), [S]()
	{
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S->Bandits)
		{
			if (Bandit.IsValid() && !Bandit->GetHealthComponent()->IsDead())
			{
				return false;
			}
		}
		return GetGameMode()->GetPendingPowderKegs() == 0;
	}, 10.0);
	WaitWorldSeconds(this, 1.0);
	Step([this, S]()
	{
		int32 Dead = 0;
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S->Bandits)
		{
			Dead += (!Bandit.IsValid() || Bandit->GetHealthComponent()->IsDead()) ? 1 : 0;
		}
		TestEqual(TEXT("The chain killed the whole line"), Dead, 8);
		TestEqual(TEXT("Each death exploded once: 8 blasts"), GetGameMode()->GetPowderKegBlasts() - S->BlastsBefore, 8);
		TestEqual(TEXT("The chain ended: nothing pending"), GetGameMode()->GetPendingPowderKegs(), 0);
		TestEqual(TEXT("The blasts never hurt the player (no god mode)"), GetHealth(GetPlayerASC()), S->PlayerHealth, 0.01f);
		TestTrue(TEXT("Run stats: Powder Keg damage recorded"), GetGameState()->GetRunStats()->GetDamageDealtBy(EGSDamageSource::PowderKeg) > 0.f);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// Health orbs from every kind of player kill at the same chance: the player's own kills, each weapon, Powder Keg.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSHealthOrbsAllSourcesTest, "BanditBaller.Economy.HealthOrbsAllSources", GSTests::TestFlags)
bool FGSHealthOrbsAllSourcesTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	struct FState
	{
		FVector Spot = FVector::ZeroVector;
		FVector Ground = FVector::ZeroVector;
		FVector Dir = FVector::ForwardVector;
		FVector Side = FVector::RightVector;
		TArray<TWeakObjectPtr<AGSEnemyCharacter>> Bandits;
		int32 Seed = 4830;
	};
	TSharedRef<FState> S = MakeShared<FState>();
	Step([this, S]()
	{
		SpeedUpWorld(4.f);
		GetGameMode()->SetRoundTimerPaused(true);
		FVector Lane;
		if (!TestTrue(TEXT("A clear lane"), FindClearLane(2600.f, Lane, S->Dir)))
		{
			return;
		}
		S->Side = FVector(-S->Dir.Y, S->Dir.X, 0.f);
		S->Spot = Lane + S->Dir * 1300.f + S->Side * 200.f;
		S->Ground = FVector(S->Spot.X, S->Spot.Y, 0.f);
	});

	enum class ESource : uint8 { Own, FloatingGun, RazorCards, BarbedWire, PowderKeg };
	const ESource Sources[] = { ESource::Own, ESource::FloatingGun, ESource::RazorCards, ESource::BarbedWire, ESource::PowderKeg };
	const TCHAR* SourceNames[] = { TEXT("own kill"), TEXT("Floating Gun"), TEXT("Razor Cards"), TEXT("Barbed Wire"), TEXT("Powder Keg") };
	const EGSDamageSource StatSources[] = { EGSDamageSource::Other, EGSDamageSource::FloatingGun, EGSDamageSource::RazorCards, EGSDamageSource::BarbedWire, EGSDamageSource::PowderKeg };
	for (const float Chance : { 1.f, 0.f })
	{
		for (int32 SourceIndex = 0; SourceIndex < 5; ++SourceIndex)
		{
			const ESource Source = Sources[SourceIndex];
			Step([S, Source, Chance]()
			{
				// A fresh run: nothing owned, run stats at 0.
				AGSGameMode* GameMode = GetGameMode();
				GameMode->RestartRunWithSeed(S->Seed++);
				GameMode->SetRoundTimerPaused(true);
				GameMode->SetHealthOrbDropChanceOverride(Chance);
				UGSDebugLibrary::SetGodMode(GetPC(), true);
				RemoveAllEnemies(GetPIEWorld());
				GetPlayer()->TeleportTo(S->Spot, S->Dir.Rotation());
				S->Bandits.Reset();
				const FVector Near = S->Ground - S->Side * 200.f; // inside the ring, in range of everything
				switch (Source)
				{
				case ESource::FloatingGun: UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("FloatingGun"), 1); break;
				case ESource::RazorCards: UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("RazorCards"), 1); break;
				case ESource::BarbedWire: UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("BarbedWire"), 1); break;
				case ESource::PowderKeg: UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("PowderKeg"), 34); break;
				default: break;
				}
				S->Bandits.Add(SpawnFrozenBandit(Near));
				if (Source == ESource::PowderKeg)
				{
					S->Bandits.Add(SpawnFrozenBandit(Near + S->Dir * 150.f)); // the blast's victim
				}
				for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S->Bandits)
				{
					if (Bandit.IsValid())
					{
						Bandit->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 3.f);
					}
				}
				if ((Source == ESource::Own || Source == ESource::PowderKeg) && S->Bandits[0].IsValid())
				{
					ApplyDamage(S->Bandits[0]->GetAbilitySystemComponent(), 100.f, GetPlayerASC()); // the player's own kill
				}
			});
			WaitUntil(this, FString::Printf(TEXT("%s kills (chance %.0f)"), SourceNames[SourceIndex], Chance), [S]()
			{
				for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S->Bandits)
				{
					if (Bandit.IsValid() && !Bandit->GetHealthComponent()->IsDead())
					{
						return false;
					}
				}
				return GetGameMode()->GetPendingPowderKegs() == 0;
			}, 15.0);
			Step([this, S, SourceIndex, Chance, SourceNames, StatSources]()
			{
				const UGSRunStats* Stats = GetGameState()->GetRunStats();
				const int32 Kills = Stats->GetTotalKills();
				const FString What = FString::Printf(TEXT("%s, chance %.0f"), SourceNames[SourceIndex], Chance);
				TestEqual(What + TEXT(": kills"), Kills, S->Bandits.Num());
				TestEqual(What + TEXT(": health orbs dropped"), Stats->GetHealthOrbsDropped(), Chance >= 1.f ? Kills : 0);
				TestTrue(What + TEXT(": damage recorded for its source"), Stats->GetDamageDealtBy(StatSources[SourceIndex]) > 0.f);
			});
		}
	}

	// The same chance in bulk: 0.3 for the player's own kills and for one Barbed Wire tick through a crowd.
	TSharedRef<int32> OwnKills = MakeShared<int32>(0), OwnOrbs = MakeShared<int32>(0);
	Step([S, OwnKills, OwnOrbs]()
	{
		AGSGameMode* GameMode = GetGameMode();
		GameMode->RestartRunWithSeed(4850);
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.3f);
		RemoveAllEnemies(GetPIEWorld());
		GetPlayer()->TeleportTo(S->Spot, S->Dir.Rotation());
		for (int32 Index = 0; Index < 60; ++Index)
		{
			if (AGSEnemyCharacter* Bandit = SpawnFrozenBandit(S->Ground + S->Dir * 3000.f + FVector((Index % 10) * 120.f, (Index / 10) * 120.f, 0.f)))
			{
				ApplyDamage(Bandit->GetAbilitySystemComponent(), 1000.f, GetPlayerASC());
			}
		}
		*OwnKills = GetGameState()->GetRunStats()->GetTotalKills();
		*OwnOrbs = GetGameState()->GetRunStats()->GetHealthOrbsDropped();
		RemoveAllEnemies(GetPIEWorld());

		// A crowd inside a big ring (5 Legendary picks: radius 5.95 m), weak enough for one tick.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("BarbedWire"), 5, EGSUpgradeRarity::Legendary);
		S->Bandits.Reset();
		for (const float Radius : { 150.f, 300.f, 450.f })
		{
			const int32 Count = FMath::FloorToInt(2.f * PI * Radius / 110.f);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const float Angle = 2.f * PI * Index / Count;
				AGSEnemyCharacter* Bandit = SpawnFrozenBandit(S->Ground + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.f));
				if (Bandit)
				{
					Bandit->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 3.f);
				}
				S->Bandits.Add(Bandit);
			}
		}
	});
	WaitWorldSeconds(this, 1.2);
	Step([this, S, OwnKills, OwnOrbs]()
	{
		const UGSRunStats* Stats = GetGameState()->GetRunStats();
		const int32 RingKills = Stats->GetTotalKills() - *OwnKills;
		const int32 RingOrbs = Stats->GetHealthOrbsDropped() - *OwnOrbs;
		const double OwnRate = *OwnKills > 0 ? static_cast<double>(*OwnOrbs) / *OwnKills : 0.0;
		const double RingRate = RingKills > 0 ? static_cast<double>(RingOrbs) / RingKills : 0.0;
		AddInfo(FString::Printf(TEXT("Chance 0.3: own kills %d orbs in %d (%.2f); Barbed Wire %d orbs in %d (%.2f)"), *OwnOrbs, *OwnKills, OwnRate, RingOrbs, RingKills, RingRate));
		TestTrue(TEXT("60 own kills"), *OwnKills == 60);
		TestTrue(TEXT("The ring killed most of the crowd"), RingKills >= S->Bandits.Num() * 3 / 4);
		TestTrue(TEXT("Own kills drop at about 0.3"), OwnRate > 0.15 && OwnRate < 0.45);
		TestTrue(TEXT("Ring kills drop at about 0.3"), RingRate > 0.15 && RingRate < 0.45);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Dynamite (Prompt 49): lobs a stick at the biggest group in range; it explodes where it lands
// ---------------------------------------------------------------------------

namespace GSTests
{
	struct FDynamiteState
	{
		FVector PlayerSpot = FVector::ZeroVector;
		FVector Ground = FVector::ZeroVector;
		FVector Dir = FVector::ForwardVector;
		FVector Side = FVector::RightVector;
		TArray<TWeakObjectPtr<AGSEnemyCharacter>> Bandits;
		TArray<float> HealthBefore;
		int32 ThrowsBefore = 0;
		int32 BlastsBefore = 0;
		int32 NumbersBefore = 0;
		int32 CritNumbersBefore = 0;
		float SourceDamageBefore = 0.f;
		float PlayerHealthBefore = 0.f;
		float FlownBefore = 0.f;
		double InRangeAt = 0.0;
		bool bScheduleOk = true;
		int32 Frames = 0;
		TWeakObjectPtr<AGSEnemyCharacter> Weak;
		int32 KillsBefore = 0;
		int32 XPOrbsBefore = 0;
		int32 HealthOrbsBefore = 0;
		/** The test's own copy of DA_DynamiteSettings (the GameMode override): a shorter interval, LeadFactor per step. */
		TWeakObjectPtr<UGSDynamiteSettings> TestSettings;
	};

	/**
	 * Speed (Prompt 52b): the weapon tests run on a copy of their settings asset with shorter intervals, set as the
	 * GameMode's override. The asset's own values are still checked first; every fixed wait shrinks by the same factor,
	 * so each wait still covers the same number of intervals.
	 */
	template <typename TSettings>
	TSettings* MakeWeaponSettingsCopy(const TSettings* Asset)
	{
		return Asset ? DuplicateObject<TSettings>(Asset, GetTransientPackage()) : nullptr;
	}

	/** Dynamite throws every 1 s in the test instead of 4 s. */
	constexpr float DynamiteTestInterval = 1.f;
	constexpr double DynamiteTimeScale = DynamiteTestInterval / 4.0;

	/** The lead runs: a bandit running straight down the lane at this steady speed (0.6 s of flight = 4.8 m). */
	constexpr float LeadRunnerSpeed = 800.f;

	/** A tough bandit with AI off and movement on, at Location, facing along the lane, with MaxWalkSpeed LeadRunnerSpeed. */
	AGSEnemyCharacter* SpawnLeadRunner(const FVector& Location, const FVector& Dir)
	{
		AGSEnemyCharacter* Runner = SpawnFrozenBandit(Location, Location + Dir * 3000.f);
		if (Runner)
		{
			Runner->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetMaxHealthAttribute(), 100000.f);
			Runner->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 100000.f);
			Runner->GetCharacterMovement()->MaxWalkSpeed = LeadRunnerSpeed;
		}
		return Runner;
	}

	UGSThrownSubsystem* GetThrown()
	{
		return GetPIEWorld() ? GetPIEWorld()->GetSubsystem<UGSThrownSubsystem>() : nullptr;
	}

	int32 NumSticksInFlight()
	{
		const UGSThrownSubsystem* Thrown = GetThrown();
		return Thrown ? Thrown->GetNumActive(AGSLobbedThrow::StaticClass()) : 0;
	}

	const AGSLobbedThrow* FindStickInFlight()
	{
		const UGSThrownSubsystem* Thrown = GetThrown();
		for (const AGSThrownActor* Actor : Thrown ? Thrown->GetActive() : TArray<TObjectPtr<AGSThrownActor>>())
		{
			if (const AGSLobbedThrow* Stick = Cast<AGSLobbedThrow>(Actor))
			{
				return Stick;
			}
		}
		return nullptr;
	}

	/** Every throw of the current run of throws is on its exact schedule (1 + floor(elapsed / interval) throws by now). */
	bool DynamiteOnSchedule()
	{
		const UGSDynamiteComponent* Dynamite = GetDynamite();
		if (!Dynamite || Dynamite->GetBurstShots() == 0)
		{
			return true;
		}
		const double Elapsed = GetPIEWorld()->GetTimeSeconds() - Dynamite->GetBurstStartTime();
		return Dynamite->GetBurstShots() == 1 + FMath::FloorToInt(Elapsed / Dynamite->GetThrowInterval() + 1e-6);
	}

	void RecordDynamite(FDynamiteState& S)
	{
		S.HealthBefore.Reset();
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S.Bandits)
		{
			S.HealthBefore.Add(Bandit.IsValid() ? GetHealth(Bandit->GetAbilitySystemComponent()) : 0.f);
		}
		S.ThrowsBefore = GetDynamite()->GetThrows();
		S.BlastsBefore = GetDynamite()->GetBlasts();
		const UGSHUDWidget* HUD = GetHUD();
		S.NumbersBefore = HUD ? HUD->GetDamageNumbersAdded() : 0;
		S.CritNumbersBefore = HUD ? HUD->GetCriticalDamageNumbersAdded() : 0;
		S.SourceDamageBefore = GetGameState()->GetRunStats()->GetDamageDealtBy(EGSDamageSource::Dynamite);
		S.PlayerHealthBefore = GetHealth(GetPlayerASC());
	}

	float DynamiteLost(const FDynamiteState& S, int32 Index)
	{
		const TWeakObjectPtr<AGSEnemyCharacter>& Bandit = S.Bandits[Index];
		return Bandit.IsValid() ? S.HealthBefore[Index] - GetHealth(Bandit->GetAbilitySystemComponent()) : -1.f;
	}

	/** Enqueue: wait until no stick is in the air (so the next blasts all come from throws made after a change). */
	void WaitNoStickInFlight(FAutomationTestBase* Test)
	{
		WaitUntil(Test, TEXT("no stick in the air"), []() { return NumSticksInFlight() == 0; }, 10.0);
	}

	/** Enqueue: wait for Count more blasts after the last RecordDynamite. */
	void WaitBlasts(FAutomationTestBase* Test, TSharedRef<FDynamiteState> S, int32 Count, bool bCheckSchedule = false)
	{
		WaitUntil(Test, FString::Printf(TEXT("%d blast(s)"), Count), [S, Count, bCheckSchedule]()
		{
			if (bCheckSchedule)
			{
				S->bScheduleOk &= DynamiteOnSchedule();
				++S->Frames;
			}
			return GetDynamite()->GetBlasts() >= S->BlastsBefore + Count;
		}, 30.0);
	}

	/**
	 * Enqueue one lead run: the player in a safe zone (no throws) while a bandit 2 m ahead speeds up to a steady
	 * LeadRunnerSpeed straight down the lane; then the safe zone goes, one stick is thrown at it with LeadFactor, and the
	 * blast is checked against where the runner is when the stick lands.
	 */
	void EnqueueDynamiteLeadRun(FAutomationTestBase* Test, TSharedRef<FDynamiteState> S, float LeadFactor)
	{
		Step([S, LeadFactor]()
		{
			GetPlayerASC()->AddLooseGameplayTag(GSGameplayTags::State_SafeZone);
			RemoveAllEnemies(GetPIEWorld());
			S->Bandits.Reset();
			if (S->TestSettings.IsValid())
			{
				S->TestSettings->LeadFactor = LeadFactor;
			}
		});
		WaitNoStickInFlight(Test);
		Step([S]() { S->Bandits.Add(SpawnLeadRunner(S->Ground + S->Dir * 200.f, S->Dir)); });
		WaitUntil(Test, TEXT("the runner at a steady speed"), [S]()
		{
			AGSEnemyCharacter* Runner = S->Bandits.Num() > 0 ? S->Bandits[0].Get() : nullptr;
			if (!Runner)
			{
				return true;
			}
			Runner->AddMovementInput(S->Dir, 1.f);
			return Runner->GetVelocity().Size2D() >= LeadRunnerSpeed * 0.99f;
		}, 10.0);
		Step([S]()
		{
			RecordDynamite(*S);
			GetPlayerASC()->RemoveLooseGameplayTag(GSGameplayTags::State_SafeZone);
		});
		WaitUntil(Test, TEXT("the blast at the runner"), [S]()
		{
			if (AGSEnemyCharacter* Runner = S->Bandits.Num() > 0 ? S->Bandits[0].Get() : nullptr)
			{
				Runner->AddMovementInput(S->Dir, 1.f);
			}
			return GetDynamite()->GetBlasts() > S->BlastsBefore;
		}, 10.0);
		Step([Test, S, LeadFactor]()
		{
			UGSDynamiteComponent* Dynamite = GetDynamite();
			const AGSEnemyCharacter* Runner = S->Bandits.Num() > 0 ? S->Bandits[0].Get() : nullptr;
			if (!Test->TestNotNull(TEXT("Lead: the runner"), Runner))
			{
				return;
			}
			const FString Tag = FString::Printf(TEXT("LeadFactor %.0f"), LeadFactor);
			const float Away = FVector::Dist(Runner->GetActorLocation(), Dynamite->GetLastBlastCenter());
			Test->AddInfo(FString::Printf(TEXT("%s: runner at %.0f cm/s, %.0f cm from the blast center when it went off (radius %.0f); the landing %.0f cm %s the runner"),
				*Tag, Runner->GetVelocity().Size2D(), Away, Dynamite->GetLastBlastRadius(), FVector::Dist2D(Dynamite->GetLastLanding(), Runner->GetActorLocation()),
				FVector::DotProduct(Dynamite->GetLastLanding() - Runner->GetActorLocation(), S->Dir) < 0.f ? TEXT("behind") : TEXT("in front of")));
			Test->TestEqual(Tag + TEXT(": one throw"), Dynamite->GetThrows(), S->ThrowsBefore + 1);
			Test->TestEqual(Tag + TEXT(": thrown at the runner"), Dynamite->GetLastTarget(), S->Bandits[0].Get());
			Test->TestTrue(Tag + (LeadFactor > 0.f ? TEXT(": the landing was led") : TEXT(": the landing was not led")), Dynamite->WasLastThrowLed() == (LeadFactor > 0.f));
			const float Feet = Runner->GetActorLocation().Z - Runner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			Test->TestTrue(Tag + TEXT(": the stick landed on the ground"), FMath::Abs(Dynamite->GetLastLanding().Z - Feet) < 5.f);
			if (LeadFactor > 0.f)
			{
				Test->TestTrue(Tag + TEXT(": the moving runner is inside the blast"), Away <= Dynamite->GetLastBlastRadius());
				Test->TestTrue(Tag + TEXT(": counted as a target hit"), Dynamite->DidLastBlastHitTarget());
				Test->TestEqual(Tag + TEXT(": the runner takes the blast"), DynamiteLost(*S, 0), 40.f, 0.01f);
			}
			else
			{
				Test->TestTrue(Tag + TEXT(": the moving runner is outside the blast"), Away > Dynamite->GetLastBlastRadius());
				Test->TestFalse(Tag + TEXT(": counted as a miss"), Dynamite->DidLastBlastHitTarget());
				Test->TestEqual(Tag + TEXT(": the runner takes nothing"), DynamiteLost(*S, 0), 0.f, 0.01f);
			}
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSDynamiteTest, "BanditBaller.Upgrades.Dynamite", GSTests::TestFlags)
bool FGSDynamiteTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	TSharedRef<FDynamiteState> S = MakeShared<FDynamiteState>();

	// Data, a clear lane, the player near its start. Game time x4.
	Step([this, S]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const UGSDynamiteSettings* Settings = GameMode->GetDynamiteSettings();
		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("Dynamite"));
		if (!TestNotNull(TEXT("DA_DynamiteSettings on BP_GSGameMode"), Settings) || !TestNotNull(TEXT("DA_Upgrade_Dynamite"), Upgrade)
			|| !TestNotNull(TEXT("Dynamite component on the character"), GetDynamite()))
		{
			return;
		}
		TestEqual(TEXT("40 damage"), Settings->Damage, 40.f);
		TestEqual(TEXT("3 m blast radius"), Settings->BlastRadius, 300.f);
		TestEqual(TEXT("A throw every 4 s"), Settings->ThrowInterval, 4.f);
		TestEqual(TEXT("20 m range"), Settings->Range, 2000.f);
		TestEqual(TEXT("0.6 s flight"), Settings->FlightTime, 0.6f);
		TestEqual(TEXT("+10% damage per power"), Settings->DamageBonusPerPower, 0.1f);
		TestEqual(TEXT("+5% blast radius per power"), Settings->RadiusBonusPerPower, 0.05f);
		TestNotNull(TEXT("Damage effect (GE_Damage)"), Settings->DamageEffect.Get());
		TestTrue(TEXT("Stick mesh and material"), Settings->StickMesh && Settings->StickMaterial);
		TestNotNull(TEXT("An explosion effect"), Settings->BlastEffect.Get());
		TestTrue(TEXT("Category Weapon"), Upgrade->Category == EGSUpgradeCategory::Weapon);
		TestEqual(TEXT("The first pick gives the weapon"), Upgrade->WeaponGivingPicks, 1);
		TestTrue(TEXT("Effect GE_Upgrade_Dynamite"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_Dynamite::StaticClass()));
		TestTrue(TEXT("In DA_UpgradePool"), GameMode->GetUpgradePool() && GameMode->GetUpgradePool()->Upgrades.Contains(Upgrade));
		TestEqual(TEXT("Card text, none owned"), Upgrade->GetFormattedDescription(0).ToString(), FString(TEXT("New weapon: lobs dynamite at the biggest group of enemies nearby.")));
		TestEqual(TEXT("Card text, owned"), Upgrade->GetFormattedDescription(1).ToString(), FString(TEXT("Makes Dynamite stronger and its blast bigger.")));
		TestEqual(TEXT("Card line of a later Common pick"), Upgrade->FormatValueText(GameMode->GetUpgradePool()->GetPickValue(Upgrade, EGSUpgradeRarity::Common, 1)).ToString(),
			FString(TEXT("+10% Dynamite damage, +5% blast size")));
		TestEqual(TEXT("Card line of a Rare later pick (halves)"), Upgrade->FormatValueText(1.5f).ToString(), FString(TEXT("+15% Dynamite damage, +7.5% blast size")));
		TestEqual(TEXT("The card's +10% damage matches the settings"), Upgrade->GetDisplayScale() / 100.f, Settings->DamageBonusPerPower, 0.0001f);
		TestEqual(TEXT("The card's +5% blast size matches the settings"), Upgrade->SecondaryDisplayScale / 100.f, Settings->RadiusBonusPerPower, 0.0001f);
		TestEqual(TEXT("LeadFactor 1: it aims where the target will be"), Settings->LeadFactor, 1.f);

		// From here on the test runs on a copy with a 1 s interval (the asset's 4 s is checked above).
		UGSDynamiteSettings* Copy = MakeWeaponSettingsCopy(Settings);
		if (!TestNotNull(TEXT("A test copy of DA_DynamiteSettings"), Copy))
		{
			return;
		}
		Copy->ThrowInterval = DynamiteTestInterval;
		GameMode->SetDynamiteSettingsOverride(Copy);
		S->TestSettings = Copy;
		TestEqual(TEXT("The test copy is in use"), GetDynamite()->GetThrowInterval(), static_cast<double>(DynamiteTestInterval), 1e-6);

		FVector Lane;
		if (!TestTrue(TEXT("A clear lane in the town"), FindClearLane(2600.f, Lane, S->Dir)))
		{
			return;
		}
		S->Side = FVector(-S->Dir.Y, S->Dir.X, 0.f);
		S->PlayerSpot = Lane + S->Dir * 400.f;
		S->Ground = FVector(S->PlayerSpot.X, S->PlayerSpot.Y, 0.f);
		GetPlayer()->TeleportTo(S->PlayerSpot, S->Dir.Rotation());
		RemoveAllEnemies(GetPIEWorld());
	});
	WaitFrames(this, 3);

	// Granted, but the only bandit is 21.5 m away: nothing is thrown and nothing is banked.
	Step([this, S]()
	{
		TestEqual(TEXT("gs.Upgrade.Grant Dynamite"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Dynamite"), 1), 1);
		UGSDynamiteComponent* Dynamite = GetDynamite();
		TestEqual(TEXT("Owned: level 1"), Dynamite->GetLevel(), 1);
		TestEqual(TEXT("A Common first pick: power 0"), Dynamite->GetPower(), 0.f);
		TestEqual(TEXT("Blast radius 300"), Dynamite->GetBlastRadius(), 300.f, 0.01f);
		TestEqual(TEXT("40 per blast"), Dynamite->GetBlastDamage(), 40.f, 0.0001f);
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 2150.f));
		RecordDynamite(*S);
	});
	WaitWorldSeconds(this, 9.0 * DynamiteTimeScale); // 2.25 throw intervals
	Step([this, S]()
	{
		UGSDynamiteComponent* Dynamite = GetDynamite();
		TestTrue(TEXT("It could throw (owned, alive, no safe zone)"), Dynamite->CanThrow());
		TestNull(TEXT("No target out of range"), Dynamite->FindTarget());
		TestEqual(TEXT("Out of range: no throws in 2.25 intervals"), Dynamite->GetThrows(), S->ThrowsBefore);
		// The bandit walks into range: the first throw comes at once (nothing banked, nothing owed).
		AGSEnemyCharacter* Bandit = S->Bandits[0].Get();
		if (TestNotNull(TEXT("Bandit"), Bandit))
		{
			const FVector At = S->Ground + S->Dir * 1000.f;
			Bandit->SetActorLocation(FVector(At.X, At.Y, Bandit->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
		}
		S->InRangeAt = GetPIEWorld()->GetTimeSeconds();
	});
	WaitUntil(this, TEXT("the first throw"), [S]() { return GetDynamite()->GetThrows() > S->ThrowsBefore; }, 10.0);
	Step([this, S]()
	{
		UGSDynamiteComponent* Dynamite = GetDynamite();
		TestEqual(TEXT("One throw (nothing banked)"), Dynamite->GetThrows(), S->ThrowsBefore + 1);
		const double Delay = Dynamite->GetBurstStartTime() - S->InRangeAt;
		AddInfo(FString::Printf(TEXT("First throw %.3f s after the bandit came into range"), Delay));
		TestTrue(TEXT("The first throw comes at once"), Delay >= -0.001 && Delay < 0.25);
		TestTrue(TEXT("A stick is in the air"), NumSticksInFlight() == 1);
		TestEqual(TEXT("It was thrown at the bandit"), Dynamite->GetLastTarget(), S->Bandits[0].Get());

		// Targeting: a lone bandit 5 m out and a group of three 14 m out: the group, its nearest member.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 500.f));                     // [0] alone
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 1400.f));                    // [1] group, nearest
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 1400.f + S->Side * 120.f));  // [2] group
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 1400.f - S->Side * 120.f));  // [3] group
	});
	WaitFrames(this, 2);
	Step([this, S]()
	{
		UGSDynamiteComponent* Dynamite = GetDynamite();
		TestEqual(TEXT("The lone bandit has no neighbours"), Dynamite->CountNeighbours(S->Bandits[0].Get()), 0);
		TestEqual(TEXT("Each in the group has two"), Dynamite->CountNeighbours(S->Bandits[1].Get()), 2);
		TestEqual(TEXT("It picks the group's nearest member over the nearer lone bandit"), Dynamite->FindTarget(), S->Bandits[1].Get());

		// A tie (two lone bandits): the nearest.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 1100.f));
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 600.f));
	});
	WaitFrames(this, 2);
	Step([this, S]()
	{
		TestEqual(TEXT("A tie goes to the nearest"), GetDynamite()->FindTarget(), S->Bandits[1].Get());

		// Damage per second at power 0: one bandit, then a group of five.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 300.f));
	});
	WaitNoStickInFlight(this);
	Step([S]() { RecordDynamite(*S); });
	WaitBlasts(this, S, 3);
	Step([this, S]()
	{
		const int32 Blasts = GetDynamite()->GetBlasts() - S->BlastsBefore;
		const float Lost = DynamiteLost(*S, 0);
		TestEqual(TEXT("One bandit: 40 per blast"), Lost, 40.f * Blasts, 0.01f);
		// DPS at the asset's interval (4 s); the damage per blast is the same in the test copy.
		const double AssetInterval = DynamiteTestInterval / DynamiteTimeScale;
		AddInfo(FString::Printf(TEXT("DPS vs one enemy: %.1f (%.0f in %d blasts, one every %.1f s)"), Lost / (Blasts * AssetInterval), Lost, Blasts, AssetInterval));
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		const FVector Center = S->Ground + S->Dir * 700.f;
		for (const FVector& Offset : { FVector::ZeroVector, S->Dir * 120.f, S->Dir * -120.f, S->Side * 120.f, S->Side * -120.f })
		{
			S->Bandits.Add(SpawnToughBandit(Center + Offset));
		}
	});
	WaitNoStickInFlight(this);
	Step([S]() { RecordDynamite(*S); });
	WaitBlasts(this, S, 3);
	Step([this, S]()
	{
		const int32 Blasts = GetDynamite()->GetBlasts() - S->BlastsBefore;
		float Lost = 0.f;
		for (int32 Index = 0; Index < S->Bandits.Num(); ++Index)
		{
			TestEqual(FString::Printf(TEXT("Group of five: bandit %d takes 40 per blast"), Index), DynamiteLost(*S, Index), 40.f * Blasts, 0.01f);
			Lost += DynamiteLost(*S, Index);
		}
		TestEqual(TEXT("Group of five: every blast hits all five"), GetDynamite()->GetLastBlastHits(), 5);
		AddInfo(FString::Printf(TEXT("DPS vs a group of five: %.1f (%.0f in %d blasts)"), Lost / (Blasts * (DynamiteTestInterval / DynamiteTimeScale)), Lost, Blasts));

		// The blast: T 2 m in front of the player, A 2.5 m beside it (inside), B 3.4 m further on (outside at 3 m).
		// The player (god mode off) is 2 m from the blast center.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		const FVector T = S->Ground + S->Dir * 200.f;
		S->Bandits.Add(SpawnToughBandit(T));                       // [0] T, the target
		S->Bandits.Add(SpawnToughBandit(T + S->Side * 250.f));     // [1] A
		S->Bandits.Add(SpawnToughBandit(T + S->Dir * 340.f));      // [2] B
		UGSDebugLibrary::SetGodMode(GetPC(), false);
	});
	WaitNoStickInFlight(this);
	Step([S]()
	{
		RecordDynamite(*S);
		S->bScheduleOk = true;
		S->Frames = 0;
	});
	WaitBlasts(this, S, 2, true);
	Step([this, S]()
	{
		UGSDynamiteComponent* Dynamite = GetDynamite();
		const int32 Blasts = Dynamite->GetBlasts() - S->BlastsBefore;
		TestTrue(FString::Printf(TEXT("Every frame on the exact schedule (the test's %.0f s interval; %d frames)"), Dynamite->GetThrowInterval(), S->Frames), S->bScheduleOk);
		TestEqual(TEXT("Target T"), Dynamite->GetLastTarget(), S->Bandits[0].Get());
		const AGSEnemyCharacter* T = S->Bandits[0].Get();
		if (T)
		{
			// LeadFactor 1 (the default) with a standing target: exactly the old landing point.
			TestEqual(TEXT("LeadFactor 1 in this part"), Dynamite->GetSettings()->LeadFactor, 1.f);
			TestFalse(TEXT("A standing target: the landing isn't led"), Dynamite->WasLastThrowLed());
			TestTrue(TEXT("The blast is centered on where T stood"), Dynamite->GetLastBlastCenter().Equals(T->GetActorLocation(), 1.f));
			const float Feet = T->GetActorLocation().Z - T->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			TestTrue(TEXT("The stick landed at T's feet"), FVector::Dist2D(Dynamite->GetLastLanding(), T->GetActorLocation()) < 1.f
				&& FMath::IsNearlyEqual(Dynamite->GetLastLanding().Z, Feet, 1.f));
			const FGSLobAim Led = AGSLobbedThrow::AimAt(T, 0.6f, 1.f);
			const FGSLobAim Old = AGSLobbedThrow::AimAt(T, 0.6f, 0.f);
			TestTrue(TEXT("A standing target: LeadFactor 1 aims exactly where LeadFactor 0 does"), Led.Landing == Old.Landing && Led.BodyCenter == Old.BodyCenter && !Led.bLed);
		}
		TestEqual(TEXT("Two hit per blast"), Dynamite->GetLastBlastHits(), 2);
		TestEqual(TEXT("T: 40 per blast"), DynamiteLost(*S, 0), 40.f * Blasts, 0.01f);
		TestEqual(TEXT("A (2.5 m): 40 per blast"), DynamiteLost(*S, 1), 40.f * Blasts, 0.01f);
		TestEqual(TEXT("B (3.4 m, outside): nothing"), DynamiteLost(*S, 2), 0.f, 0.01f);
		TestEqual(TEXT("The player 2 m from the blast: unhurt"), GetHealth(GetPlayerASC()), S->PlayerHealthBefore, 0.01f);
		TestEqual(TEXT("A damage number per enemy hit"), GetHUD()->GetDamageNumbersAdded(), S->NumbersBefore + 2 * Blasts);
		TestEqual(TEXT("Run stats count it as Dynamite damage"), GetGameState()->GetRunStats()->GetDamageDealtBy(EGSDamageSource::Dynamite) - S->SourceDamageBefore,
			80.f * Blasts, 0.01f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		// The aim on its own (Prompt 52b): a frozen bandit given a velocity by hand.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		AGSEnemyCharacter* Moving = SpawnFrozenBandit(S->Ground + S->Dir * 600.f);
		if (!TestNotNull(TEXT("Aim: a bandit"), Moving))
		{
			return;
		}
		UCharacterMovementComponent* CMC = Moving->GetCharacterMovement();
		const float HalfHeight = Moving->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector Feet = Moving->GetActorLocation() - FVector(0.f, 0.f, HalfHeight);
		CMC->Velocity = S->Dir * 800.f + FVector(0.f, 0.f, 300.f);
		const FGSLobAim Full = AGSLobbedThrow::AimAt(Moving, 0.6f, 1.f);
		TestTrue(TEXT("Aim: led"), Full.bLed);
		TestTrue(FString::Printf(TEXT("Aim: 800 cm/s x 0.6 s = 480 cm ahead, flat (%.1f cm)"), FVector::Dist2D(Full.Landing, Feet + S->Dir * 480.f)),
			FVector::Dist2D(Full.Landing, Feet + S->Dir * 480.f) < 1.f);
		TestTrue(TEXT("Aim: on the ground there"), FMath::Abs(Full.Landing.Z - Feet.Z) < 5.f);
		TestTrue(TEXT("Aim: the blast center is the body over the landing"), Full.BodyCenter.Equals(Full.Landing + FVector(0.f, 0.f, HalfHeight), 0.01f));
		const FGSLobAim Half = AGSLobbedThrow::AimAt(Moving, 0.6f, 0.5f);
		TestTrue(TEXT("Aim: LeadFactor 0.5 = 240 cm ahead"), FVector::Dist2D(Half.Landing, Feet + S->Dir * 240.f) < 1.f);
		const FGSLobAim None = AGSLobbedThrow::AimAt(Moving, 0.6f, 0.f);
		TestTrue(TEXT("Aim: LeadFactor 0 = the old landing point"), !None.bLed && None.Landing == Feet && None.BodyCenter == Moving->GetActorLocation());
		CMC->Velocity = FVector(0.f, 0.f, -900.f);
		TestTrue(TEXT("Aim: only vertical speed = no lead"), !AGSLobbedThrow::AimAt(Moving, 0.6f, 1.f).bLed);
		// Off the map: running out over the edge of the town's ground (300 x 300 m, so the edge is at 150 m).
		const FVector EdgeSpot(14900.f, Moving->GetActorLocation().Y, Moving->GetActorLocation().Z);
		Moving->SetActorLocation(EdgeSpot, false, nullptr, ETeleportType::TeleportPhysics);
		CMC->Velocity = FVector(2000.f, 0.f, 0.f); // 12 m out past the edge
		const FGSLobAim Off = AGSLobbedThrow::AimAt(Moving, 0.6f, 1.f);
		TestTrue(TEXT("Aim: no ground ahead (off the map) = where it stands now"), !Off.bLed && Off.BodyCenter == Moving->GetActorLocation()
			&& Off.Landing == Moving->GetActorLocation() - FVector(0.f, 0.f, HalfHeight));
		RemoveAllEnemies(GetPIEWorld());
	});
	// A bandit running straight down the lane: inside the blast with LeadFactor 1, outside with 0.
	EnqueueDynamiteLeadRun(this, S, 1.f);
	EnqueueDynamiteLeadRun(this, S, 0.f);
	Step([this, S]()
	{
		if (S->TestSettings.IsValid())
		{
			S->TestSettings->LeadFactor = 1.f;
		}
		// T, A and B again for the power checks.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		const FVector T = S->Ground + S->Dir * 200.f;
		S->Bandits.Add(SpawnToughBandit(T));                       // [0] T, the target
		S->Bandits.Add(SpawnToughBandit(T + S->Side * 250.f));     // [1] A
		S->Bandits.Add(SpawnToughBandit(T + S->Dir * 340.f));      // [2] B
		UGSDynamiteComponent* Dynamite = GetDynamite();

		// Power 1: +10% damage, +5% radius (315 cm: B is still outside).
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Dynamite"), 1);
		TestEqual(TEXT("Power 1"), Dynamite->GetPower(), 1.f);
		TestEqual(TEXT("Power 1: x1.1 damage"), Dynamite->GetDamageMultiplier(), 1.1f, 0.0001f);
		TestEqual(TEXT("Power 1: 44 per blast"), Dynamite->GetBlastDamage(), 44.f, 0.0001f);
		TestEqual(TEXT("Power 1: radius 315"), Dynamite->GetBlastRadius(), 315.f, 0.01f);
	});
	WaitNoStickInFlight(this);
	Step([S]() { RecordDynamite(*S); });
	WaitBlasts(this, S, 1);
	Step([this, S]()
	{
		const int32 Blasts = GetDynamite()->GetBlasts() - S->BlastsBefore;
		TestEqual(TEXT("Power 1: T takes 44 per blast"), DynamiteLost(*S, 0), 44.f * Blasts, 0.01f);
		TestEqual(TEXT("Power 1: B still outside"), DynamiteLost(*S, 2), 0.f, 0.01f);
		TestEqual(TEXT("Power 1: the blast radius"), GetDynamite()->GetLastBlastRadius(), 315.f, 0.01f);

		// An Epic pick (+2): power 3, x1.3 damage, radius 345 cm: B is inside now.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Dynamite"), 1, EGSUpgradeRarity::Epic);
		TestEqual(TEXT("Power 3"), GetDynamite()->GetPower(), 3.f);
		TestEqual(TEXT("Power 3: 52 per blast"), GetDynamite()->GetBlastDamage(), 52.f, 0.0001f);
		TestEqual(TEXT("Power 3: radius 345"), GetDynamite()->GetBlastRadius(), 345.f, 0.01f);
	});
	WaitNoStickInFlight(this);
	Step([S]() { RecordDynamite(*S); });
	WaitBlasts(this, S, 1);
	Step([this, S]()
	{
		const int32 Blasts = GetDynamite()->GetBlasts() - S->BlastsBefore;
		TestEqual(TEXT("Power 3: three hit"), GetDynamite()->GetLastBlastHits(), 3);
		TestEqual(TEXT("Power 3: T takes 52"), DynamiteLost(*S, 0), 52.f * Blasts, 0.01f);
		TestEqual(TEXT("Power 3: B (3.4 m) is inside the bigger blast"), DynamiteLost(*S, 2), 52.f * Blasts, 0.01f);

		// Every hit rolls its own crit: at 100% (Loaded Dice x20) every enemy hit is a critical hit.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("LoadedDice"), 20);
	});
	WaitNoStickInFlight(this);
	Step([S]() { RecordDynamite(*S); });
	WaitBlasts(this, S, 1);
	Step([this, S]()
	{
		const int32 Blasts = GetDynamite()->GetBlasts() - S->BlastsBefore;
		TestEqual(TEXT("Crits: T takes 104 (52 x2)"), DynamiteLost(*S, 0), 104.f * Blasts, 0.01f);
		TestEqual(TEXT("Crits: a critical number for each of the three hit"), GetHUD()->GetCriticalDamageNumbersAdded(), S->CritNumbersBefore + 3 * Blasts);

		// Its kill is the player's: a weak bandit, Leech, a sure health orb.
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Leech"), 1);
		GetGameMode()->SetHealthOrbDropChanceOverride(1.f);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		ApplyDamage(GetPlayerASC(), 10.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		S->PlayerHealthBefore = GetHealth(GetPlayerASC());
		GetPlayerCombo()->EndCombo(EGSComboEndReason::Reset);
		S->KillsBefore = GetGameState()->GetRunStats()->GetTotalKills();
		S->XPOrbsBefore = GetCashSubsystem()->GetNumXPOrbsDropped();
		S->HealthOrbsBefore = GetCashSubsystem()->GetNumHealthOrbsDropped();
		S->Weak = SpawnFrozenBandit(S->Ground + S->Dir * 300.f);
		if (S->Weak.IsValid())
		{
			S->Weak->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 3.f);
		}
	});
	WaitUntil(this, TEXT("a blast to kill the bandit"), [S]() { return !S->Weak.IsValid() || S->Weak->GetHealthComponent()->IsDead(); }, 20.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Dynamite kill: counted in run stats"), GetGameState()->GetRunStats()->GetTotalKills(), S->KillsBefore + 1);
		TestEqual(TEXT("Dynamite kill: counts for the combo"), GetPlayerCombo()->GetComboCount(), 1);
		TestEqual(TEXT("Dynamite kill: an XP orb"), GetCashSubsystem()->GetNumXPOrbsDropped(), S->XPOrbsBefore + 1);
		TestEqual(TEXT("Dynamite kill: the health orb chance (1: one orb)"), GetCashSubsystem()->GetNumHealthOrbsDropped(), S->HealthOrbsBefore + 1);
		TestTrue(TEXT("Dynamite kill: a cash drop"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) > 0);
		TestEqual(TEXT("Dynamite kill: Leech heals"), GetHealth(GetPlayerASC()), S->PlayerHealthBefore + 1.f, 0.01f);
		GetGameMode()->SetHealthOrbDropChanceOverride(0.f);

		// Paused with a stick in the air: it stays put, nothing is thrown.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 600.f));
	});
	WaitUntil(this, TEXT("a stick in the air"), []() { return NumSticksInFlight() > 0; }, 20.0);
	Step([S]()
	{
		RecordDynamite(*S);
		const AGSLobbedThrow* Stick = FindStickInFlight();
		S->FlownBefore = Stick ? Stick->GetFlown() : -1.f;
		UGameplayStatics::SetGamePaused(GetPIEWorld(), true);
	});
	WaitRealSeconds(this, 1.0);
	Step([this, S]()
	{
		const AGSLobbedThrow* Stick = FindStickInFlight();
		TestEqual(TEXT("Paused: no throws"), GetDynamite()->GetThrows(), S->ThrowsBefore);
		TestEqual(TEXT("Paused: no blast"), GetDynamite()->GetBlasts(), S->BlastsBefore);
		TestTrue(TEXT("Paused: the stick doesn't fly on"), Stick && FMath::IsNearlyEqual(Stick->GetFlown(), S->FlownBefore));
		UGameplayStatics::SetGamePaused(GetPIEWorld(), false);

		// A safe zone: no throws, and none banked.
		GetPlayerASC()->AddLooseGameplayTag(GSGameplayTags::State_SafeZone);
	});
	WaitNoStickInFlight(this);
	Step([S]() { RecordDynamite(*S); });
	WaitWorldSeconds(this, 9.0 * DynamiteTimeScale);
	Step([this, S]()
	{
		TestFalse(TEXT("Safe zone: it can't throw"), GetDynamite()->CanThrow());
		TestEqual(TEXT("Safe zone: no throws in 2.25 intervals"), GetDynamite()->GetThrows(), S->ThrowsBefore);
		GetPlayerASC()->RemoveLooseGameplayTag(GSGameplayTags::State_SafeZone);
		S->InRangeAt = GetPIEWorld()->GetTimeSeconds();
	});
	WaitUntil(this, TEXT("a throw after the safe zone"), [S]() { return GetDynamite()->GetThrows() > S->ThrowsBefore; }, 10.0);
	Step([this, S]()
	{
		TestEqual(TEXT("After the safe zone: one throw (nothing banked)"), GetDynamite()->GetThrows(), S->ThrowsBefore + 1);
		TestTrue(TEXT("After the safe zone: it comes at once"), GetPIEWorld()->GetTimeSeconds() - S->InRangeAt < 0.3);

		// A new run with a stick in the air: the stick is gone and never explodes, the weapon is gone.
		TestTrue(TEXT("A stick is in the air before the new run"), NumSticksInFlight() > 0);
		GetGameMode()->RestartRunWithSeed(4901);
		GetGameMode()->SetRoundTimerPaused(true);
		GetPlayer()->TeleportTo(S->PlayerSpot, S->Dir.Rotation());
		TestEqual(TEXT("New run: no stick in the air"), NumSticksInFlight(), 0);
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 600.f));
		RecordDynamite(*S);
	});
	WaitWorldSeconds(this, 6.0 * DynamiteTimeScale);
	Step([this, S]()
	{
		UGSDynamiteComponent* Dynamite = GetDynamite();
		TestEqual(TEXT("New run: level 0"), Dynamite->GetLevel(), 0);
		TestEqual(TEXT("New run: power 0"), Dynamite->GetPower(), 0.f);
		TestEqual(TEXT("New run: the stick never exploded"), Dynamite->GetBlasts(), S->BlastsBefore);
		TestEqual(TEXT("New run: no throws"), Dynamite->GetThrows(), S->ThrowsBefore);
		TestEqual(TEXT("New run: a bandit in range takes nothing"), DynamiteLost(*S, 0), 0.f, 0.01f);

		// Dead: no throws.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Dynamite"), 1);
	});
	WaitUntil(this, TEXT("a throw in the new run"), [S]() { return GetDynamite()->GetThrows() > S->ThrowsBefore; }, 10.0);
	Step([this, S]()
	{
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		ApplyDamage(GetPlayerASC(), 100000.f);
		TestTrue(TEXT("The player is dead"), GetPlayerASC()->HasMatchingGameplayTag(GSGameplayTags::State_Dead));
		S->ThrowsBefore = GetDynamite()->GetThrows();
	});
	WaitWorldSeconds(this, 9.0 * DynamiteTimeScale);
	Step([this, S]()
	{
		TestFalse(TEXT("Dead: it can't throw"), GetDynamite()->CanThrow());
		TestEqual(TEXT("Dead: no throws in 2.25 intervals"), GetDynamite()->GetThrows(), S->ThrowsBefore);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		GetGameMode()->SetDynamiteSettingsOverride(nullptr);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Lucky Coin (Prompt 50): a coin that bounces from enemy to enemy, each once per flip
// ---------------------------------------------------------------------------

namespace GSTests
{
	struct FCoinState
	{
		FVector PlayerSpot = FVector::ZeroVector;
		FVector Ground = FVector::ZeroVector;
		FVector Dir = FVector::ForwardVector;
		FVector Side = FVector::RightVector;
		TArray<TWeakObjectPtr<AGSEnemyCharacter>> Bandits;
		TArray<float> HealthBefore;
		int32 FlipsBefore = 0;
		int32 DoneBefore = 0;
		float SourceDamageBefore = 0.f;
		double SpawnedAt = 0.0;
		bool bScheduleOk = true;
		int32 Frames = 0;
		TWeakObjectPtr<AGSEnemyCharacter> Weak;
		float PlayerHealthBefore = 0.f;
		int32 KillsBefore = 0;
		int32 XPOrbsBefore = 0;
		int32 HealthOrbsBefore = 0;
	};

	/**
	 * The Lucky Coin test's copy flips every 1.25 s instead of 1.5 s: a five-enemy flip (6 m, then four 4 m bounces) takes
	 * about 0.9-1.1 s, so this still leaves a moment with no coin in the air between flips. Fixed waits scale with it.
	 */
	constexpr float LuckyCoinTestInterval = 1.25f;
	constexpr double LuckyCoinTimeScale = LuckyCoinTestInterval / 1.5;

	bool LuckyCoinOnSchedule()
	{
		const UGSLuckyCoinComponent* Coin = GetLuckyCoin();
		if (!Coin || Coin->GetBurstShots() == 0)
		{
			return true;
		}
		const double Elapsed = GetPIEWorld()->GetTimeSeconds() - Coin->GetBurstStartTime();
		return Coin->GetBurstShots() == 1 + FMath::FloorToInt(Elapsed / Coin->GetFlipInterval() + 1e-6);
	}

	void RecordCoin(FCoinState& S)
	{
		S.HealthBefore.Reset();
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S.Bandits)
		{
			S.HealthBefore.Add(Bandit.IsValid() ? GetHealth(Bandit->GetAbilitySystemComponent()) : 0.f);
		}
		S.FlipsBefore = GetLuckyCoin()->GetFlips();
		S.DoneBefore = GetLuckyCoin()->GetFlipsDone();
		S.SourceDamageBefore = GetGameState()->GetRunStats()->GetDamageDealtBy(EGSDamageSource::LuckyCoin);
	}

	float CoinLost(const FCoinState& S, int32 Index)
	{
		const TWeakObjectPtr<AGSEnemyCharacter>& Bandit = S.Bandits[Index];
		return Bandit.IsValid() ? S.HealthBefore[Index] - GetHealth(Bandit->GetAbilitySystemComponent()) : -1.f;
	}

	/** The last finished flip hit exactly these bandits (by index into S.Bandits), in this order, each once. */
	bool LastFlipHitExactly(const FCoinState& S, std::initializer_list<int32> Indices)
	{
		const TArray<TWeakObjectPtr<AActor>>& Hits = GetLuckyCoin()->GetLastFlipHits();
		if (Hits.Num() != static_cast<int32>(Indices.size()))
		{
			return false;
		}
		int32 Slot = 0;
		for (const int32 Index : Indices)
		{
			if (Hits[Slot++].Get() != S.Bandits[Index].Get())
			{
				return false;
			}
		}
		return true;
	}

	/** Enqueue: wait until no coin is in the air, then record. */
	void WaitCoinsDownAndRecord(FAutomationTestBase* Test, TSharedRef<FCoinState> S)
	{
		WaitUntil(Test, TEXT("no coin in the air"), []() { return GetLuckyCoin()->GetFlipsInAir() == 0; }, 10.0);
		Step([S]() { RecordCoin(*S); });
	}

	/** Enqueue: wait for Count more finished flips after the last RecordCoin. */
	void WaitFlipsDone(FAutomationTestBase* Test, TSharedRef<FCoinState> S, int32 Count, bool bCheckSchedule = false)
	{
		WaitUntil(Test, FString::Printf(TEXT("%d finished flip(s)"), Count), [S, Count, bCheckSchedule]()
		{
			if (bCheckSchedule)
			{
				S->bScheduleOk &= LuckyCoinOnSchedule();
				++S->Frames;
			}
			return GetLuckyCoin()->GetFlipsDone() >= S->DoneBefore + Count;
		}, 20.0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSLuckyCoinTest, "BanditBaller.Upgrades.LuckyCoin", GSTests::TestFlags)
bool FGSLuckyCoinTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	TSharedRef<FCoinState> S = MakeShared<FCoinState>();

	// Data, a clear lane, the player near its start. Game time x4.
	Step([this, S]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const UGSLuckyCoinSettings* Settings = GameMode->GetLuckyCoinSettings();
		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("LuckyCoin"));
		if (!TestNotNull(TEXT("DA_LuckyCoinSettings on BP_GSGameMode"), Settings) || !TestNotNull(TEXT("DA_Upgrade_LuckyCoin"), Upgrade)
			|| !TestNotNull(TEXT("Lucky Coin component on the character"), GetLuckyCoin()))
		{
			return;
		}
		TestEqual(TEXT("10 damage per hit"), Settings->Damage, 10.f);
		TestEqual(TEXT("A flip every 1.5 s"), Settings->FlipInterval, 1.5f);
		TestEqual(TEXT("20 m range"), Settings->Range, 2000.f);
		TestEqual(TEXT("8 m bounce range"), Settings->BounceRange, 800.f);
		TestEqual(TEXT("3 bounces"), Settings->BaseBounces, 3);
		TestEqual(TEXT("+1 bounce per 2 full power"), Settings->PowerPerBounce, 2.f);
		TestEqual(TEXT("+10% damage per power"), Settings->DamageBonusPerPower, 0.1f);
		TestEqual(TEXT("Bounces at power 0 / 1 / 1.9 / 2 / 3.5 / 4"), FString::Printf(TEXT("%d %d %d %d %d %d"), Settings->GetBounces(0.f), Settings->GetBounces(1.f),
			Settings->GetBounces(1.9f), Settings->GetBounces(2.f), Settings->GetBounces(3.5f), Settings->GetBounces(4.f)), FString(TEXT("3 3 3 4 4 5")));
		TestNotNull(TEXT("Damage effect (GE_Damage)"), Settings->DamageEffect.Get());
		TestTrue(TEXT("Coin mesh and material"), Settings->CoinMesh && Settings->CoinMaterial);
		TestTrue(TEXT("Category Weapon"), Upgrade->Category == EGSUpgradeCategory::Weapon);
		TestEqual(TEXT("The first pick gives the weapon"), Upgrade->WeaponGivingPicks, 1);
		TestTrue(TEXT("Effect GE_Upgrade_LuckyCoin"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_LuckyCoin::StaticClass()));
		TestTrue(TEXT("In DA_UpgradePool"), GameMode->GetUpgradePool() && GameMode->GetUpgradePool()->Upgrades.Contains(Upgrade));
		TestEqual(TEXT("Card text, none owned"), Upgrade->GetFormattedDescription(0).ToString(), FString(TEXT("New weapon: flips a coin that bounces from enemy to enemy.")));
		TestEqual(TEXT("Card text, owned"), Upgrade->GetFormattedDescription(1).ToString(), FString(TEXT("Makes the Lucky Coin stronger and, in time, adds bounces.")));
		TestEqual(TEXT("Card line of a later Common pick"), Upgrade->FormatValueText(GameMode->GetUpgradePool()->GetPickValue(Upgrade, EGSUpgradeRarity::Common, 1)).ToString(),
			FString(TEXT("+10% Lucky Coin damage")));
		TestEqual(TEXT("The card's +10% matches the settings"), Upgrade->GetDisplayScale() / 100.f, Settings->DamageBonusPerPower, 0.0001f);

		// From here on the test runs on a copy that flips every 1.25 s (the asset's 1.5 s is checked above).
		UGSLuckyCoinSettings* Copy = MakeWeaponSettingsCopy(Settings);
		if (!TestNotNull(TEXT("A test copy of DA_LuckyCoinSettings"), Copy))
		{
			return;
		}
		Copy->FlipInterval = LuckyCoinTestInterval;
		GameMode->SetLuckyCoinSettingsOverride(Copy);
		TestEqual(TEXT("The test copy is in use"), GetLuckyCoin()->GetFlipInterval(), static_cast<double>(LuckyCoinTestInterval), 1e-6);

		FVector Lane;
		if (!TestTrue(TEXT("A clear lane in the town"), FindClearLane(2600.f, Lane, S->Dir)))
		{
			return;
		}
		S->Side = FVector(-S->Dir.Y, S->Dir.X, 0.f);
		S->PlayerSpot = Lane + S->Dir * 100.f;
		S->Ground = FVector(S->PlayerSpot.X, S->PlayerSpot.Y, 0.f);
		GetPlayer()->TeleportTo(S->PlayerSpot, S->Dir.Rotation());
		RemoveAllEnemies(GetPIEWorld());
		TestEqual(TEXT("gs.Upgrade.Grant LuckyCoin"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("LuckyCoin"), 1), 1);
		TestEqual(TEXT("Owned: level 1"), GetLuckyCoin()->GetLevel(), 1);
		TestEqual(TEXT("Power 0: 3 bounces"), GetLuckyCoin()->GetBounces(), 3);
		TestEqual(TEXT("Power 0: 10 per hit"), GetLuckyCoin()->GetHitDamage(), 10.f, 0.0001f);
		S->Bandits.Reset();
		RecordCoin(*S);
	});
	// No enemy: nothing is flipped and nothing banked.
	WaitWorldSeconds(this, 2.0 * LuckyCoinTimeScale); // 1.33 flip intervals
	Step([this, S]()
	{
		TestEqual(TEXT("No target: no flips in 1.33 intervals"), GetLuckyCoin()->GetFlips(), S->FlipsBefore);
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 600.f));
		S->SpawnedAt = GetPIEWorld()->GetTimeSeconds();
		RecordCoin(*S);
	});
	WaitUntil(this, TEXT("the first flip"), [S]() { return GetLuckyCoin()->GetFlips() > S->FlipsBefore; }, 10.0);
	Step([this, S]()
	{
		const double Delay = GetPIEWorld()->GetTimeSeconds() - S->SpawnedAt;
		TestEqual(TEXT("One flip (nothing banked)"), GetLuckyCoin()->GetFlips(), S->FlipsBefore + 1);
		TestTrue(FString::Printf(TEXT("The first flip comes at once (%.3f s)"), Delay), Delay < 0.3);
	});

	// One bandit: one hit per flip (nobody to bounce to). DPS against one enemy.
	WaitCoinsDownAndRecord(this, S);
	WaitFlipsDone(this, S, 3);
	Step([this, S]()
	{
		const int32 Done = GetLuckyCoin()->GetFlipsDone() - S->DoneBefore;
		TestTrue(TEXT("One bandit: the flip hit it once"), LastFlipHitExactly(*S, { 0 }));
		TestEqual(TEXT("One bandit: 10 per flip"), CoinLost(*S, 0), 10.f * Done, 0.01f);
		// DPS at the asset's interval (1.5 s); the damage per flip is the same in the test copy.
		const double AssetInterval = LuckyCoinTestInterval / LuckyCoinTimeScale;
		AddInfo(FString::Printf(TEXT("DPS vs one enemy: %.1f (%.0f in %d flips, one every %.1f s)"), CoinLost(*S, 0) / (Done * AssetInterval),
			CoinLost(*S, 0), Done, AssetInterval));

		// Five in a row, 4 m apart: a flip hits the first four in order, each once; the fifth is left (3 bounces).
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		for (int32 Index = 0; Index < 5; ++Index)
		{
			S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * (600.f + 400.f * Index)));
		}
	});
	WaitCoinsDownAndRecord(this, S);
	Step([S]() { S->bScheduleOk = true; S->Frames = 0; });
	WaitFlipsDone(this, S, 3, true);
	Step([this, S]()
	{
		UGSLuckyCoinComponent* Coin = GetLuckyCoin();
		const int32 Done = Coin->GetFlipsDone() - S->DoneBefore;
		TestTrue(FString::Printf(TEXT("Every frame on the exact schedule (the test's %.2f s interval; %d frames)"), Coin->GetFlipInterval(), S->Frames), S->bScheduleOk);
		TestTrue(TEXT("Five in a row: the flip hit the first four, in order, each once"), LastFlipHitExactly(*S, { 0, 1, 2, 3 }));
		float Lost = 0.f;
		for (int32 Index = 0; Index < 4; ++Index)
		{
			TestEqual(FString::Printf(TEXT("Five in a row: bandit %d took 10 per flip (once each)"), Index), CoinLost(*S, Index), 10.f * Done, 0.01f);
			Lost += CoinLost(*S, Index);
		}
		TestEqual(TEXT("Five in a row: the fifth is out of bounces"), CoinLost(*S, 4), 0.f, 0.01f);
		TestEqual(TEXT("Run stats count it as Lucky Coin damage"), GetGameState()->GetRunStats()->GetDamageDealtBy(EGSDamageSource::LuckyCoin) - S->SourceDamageBefore, Lost, 0.01f);
		AddInfo(FString::Printf(TEXT("DPS vs a group of five (4 m apart): %.1f (%.0f in %d flips)"), Lost / (Done * (LuckyCoinTestInterval / LuckyCoinTimeScale)), Lost, Done));

		// Power 1: x1.1, still 3 bounces.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("LuckyCoin"), 1);
		TestEqual(TEXT("Power 1"), Coin->GetPower(), 1.f);
		TestEqual(TEXT("Power 1: 11 per hit"), Coin->GetHitDamage(), 11.f, 0.0001f);
		TestEqual(TEXT("Power 1: still 3 bounces"), Coin->GetBounces(), 3);
	});
	WaitCoinsDownAndRecord(this, S);
	WaitFlipsDone(this, S, 1);
	Step([this, S]()
	{
		UGSLuckyCoinComponent* Coin = GetLuckyCoin();
		TestTrue(TEXT("Power 1: four hit"), LastFlipHitExactly(*S, { 0, 1, 2, 3 }));
		TestEqual(TEXT("Power 1: 11 per hit"), Coin->GetLastFlipDamage(), 11.f, 0.0001f);
		TestEqual(TEXT("Power 1: the first took 11"), CoinLost(*S, 0), 11.f, 0.01f);

		// Power 2: 4 bounces, all five hit.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("LuckyCoin"), 1);
		TestEqual(TEXT("Power 2: 4 bounces"), Coin->GetBounces(), 4);
	});
	WaitCoinsDownAndRecord(this, S);
	WaitFlipsDone(this, S, 1);
	Step([this, S]()
	{
		UGSLuckyCoinComponent* Coin = GetLuckyCoin();
		TestTrue(TEXT("Power 2: all five hit, each once"), LastFlipHitExactly(*S, { 0, 1, 2, 3, 4 }));
		TestEqual(TEXT("Power 2: the fifth took 12"), CoinLost(*S, 4), 12.f, 0.01f);

		// A Rare pick (+1.5): power 3.5, x1.35, still 4 bounces (only full 2-point steps count).
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("LuckyCoin"), 1, EGSUpgradeRarity::Rare);
		TestEqual(TEXT("Power 3.5"), Coin->GetPower(), 3.5f);
		TestEqual(TEXT("Power 3.5: 13.5 per hit"), Coin->GetHitDamage(), 13.5f, 0.0001f);
		TestEqual(TEXT("Power 3.5: 4 bounces"), Coin->GetBounces(), 4);
	});
	WaitCoinsDownAndRecord(this, S);
	WaitFlipsDone(this, S, 1);
	Step([this, S]()
	{
		TestTrue(TEXT("Power 3.5: five hit"), LastFlipHitExactly(*S, { 0, 1, 2, 3, 4 }));
		TestEqual(TEXT("Power 3.5: 13.5 each"), CoinLost(*S, 2), 13.5f, 0.01f);

		// Out of bounce range: two bandits 12 m apart: each flip stops after the first.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 600.f));
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 1800.f));
	});
	WaitCoinsDownAndRecord(this, S);
	WaitFlipsDone(this, S, 2);
	Step([this, S]()
	{
		TestTrue(TEXT("12 m apart: the flip stops after the first"), LastFlipHitExactly(*S, { 0 }));
		TestEqual(TEXT("12 m apart: the far one is never hit"), CoinLost(*S, 1), 0.f, 0.01f);

		// Its kill is the player's: a weak bandit, Leech, a sure health orb.
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Leech"), 1);
		GetGameMode()->SetHealthOrbDropChanceOverride(1.f);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		ApplyDamage(GetPlayerASC(), 10.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		S->PlayerHealthBefore = GetHealth(GetPlayerASC());
		GetPlayerCombo()->EndCombo(EGSComboEndReason::Reset);
		S->KillsBefore = GetGameState()->GetRunStats()->GetTotalKills();
		S->XPOrbsBefore = GetCashSubsystem()->GetNumXPOrbsDropped();
		S->HealthOrbsBefore = GetCashSubsystem()->GetNumHealthOrbsDropped();
		S->Weak = SpawnFrozenBandit(S->Ground + S->Dir * 600.f);
		if (S->Weak.IsValid())
		{
			S->Weak->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 3.f);
		}
	});
	WaitUntil(this, TEXT("a coin to kill the bandit"), [S]() { return !S->Weak.IsValid() || S->Weak->GetHealthComponent()->IsDead(); }, 10.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Coin kill: counted in run stats"), GetGameState()->GetRunStats()->GetTotalKills(), S->KillsBefore + 1);
		TestEqual(TEXT("Coin kill: counts for the combo"), GetPlayerCombo()->GetComboCount(), 1);
		TestEqual(TEXT("Coin kill: an XP orb"), GetCashSubsystem()->GetNumXPOrbsDropped(), S->XPOrbsBefore + 1);
		TestEqual(TEXT("Coin kill: the health orb chance (1: one orb)"), GetCashSubsystem()->GetNumHealthOrbsDropped(), S->HealthOrbsBefore + 1);
		TestTrue(TEXT("Coin kill: a cash drop"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) > 0);
		TestEqual(TEXT("Coin kill: Leech heals"), GetHealth(GetPlayerASC()), S->PlayerHealthBefore + 1.f, 0.01f);
		GetGameMode()->SetHealthOrbDropChanceOverride(0.f);

		// Paused: no flips.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 600.f));
	});
	WaitCoinsDownAndRecord(this, S);
	Step([]() { UGameplayStatics::SetGamePaused(GetPIEWorld(), true); });
	WaitRealSeconds(this, 1.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Paused: no flips"), GetLuckyCoin()->GetFlips(), S->FlipsBefore);
		UGameplayStatics::SetGamePaused(GetPIEWorld(), false);
	});

	// A new run with a coin in the air: the coin is gone, the weapon is gone.
	WaitUntil(this, TEXT("a coin in the air"), []() { return GetLuckyCoin()->GetFlipsInAir() > 0; }, 10.0);
	Step([this, S]()
	{
		GetGameMode()->RestartRunWithSeed(5001);
		GetGameMode()->SetRoundTimerPaused(true);
		GetPlayer()->TeleportTo(S->PlayerSpot, S->Dir.Rotation());
		TestEqual(TEXT("New run: no coin in the air"), GetPIEWorld()->GetSubsystem<UGSProjectileSubsystem>()->GetNumActive(), 0);
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 600.f));
		RecordCoin(*S);
	});
	WaitWorldSeconds(this, 2.0 * LuckyCoinTimeScale);
	Step([this, S]()
	{
		UGSLuckyCoinComponent* Coin = GetLuckyCoin();
		TestEqual(TEXT("New run: level 0"), Coin->GetLevel(), 0);
		TestEqual(TEXT("New run: power 0"), Coin->GetPower(), 0.f);
		TestEqual(TEXT("New run: no flips"), Coin->GetFlips(), S->FlipsBefore);
		TestEqual(TEXT("New run: nothing in the air"), Coin->GetFlipsInAir(), 0);
		TestEqual(TEXT("New run: a bandit in range takes nothing"), CoinLost(*S, 0), 0.f, 0.01f);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		GetGameMode()->SetLuckyCoinSettingsOverride(nullptr);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Molotov (Prompt 51): a bottle lobbed at the nearest enemy leaves a patch of fire that burns on a fixed tick
// ---------------------------------------------------------------------------

namespace GSTests
{
	struct FMolotovState
	{
		FVector PlayerSpot = FVector::ZeroVector;
		FVector Ground = FVector::ZeroVector;
		FVector Dir = FVector::ForwardVector;
		FVector Side = FVector::RightVector;
		TArray<TWeakObjectPtr<AGSEnemyCharacter>> Bandits;
		TArray<float> HealthBefore;
		int32 ThrowsBefore = 0;
		int32 PatchesBefore = 0;
		int32 BurnsBefore = 0;
		float PlayerHealthBefore = 0.f;
		float SourceDamageBefore = 0.f;
		double InRangeAt = 0.0;
		float AgeBefore = 0.f;
		int32 PatchBurnsBefore = 0;
		bool bScheduleOk = true;
		int32 Frames = 0;
		TWeakObjectPtr<AGSEnemyCharacter> Weak;
		int32 KillsBefore = 0;
		int32 XPOrbsBefore = 0;
		int32 HealthOrbsBefore = 0;
		int32 TargetHitsBefore = 0;
		/** The test's own copy of DA_MolotovSettings (the GameMode override): shorter times, LeadFactor per step. */
		TWeakObjectPtr<UGSMolotovSettings> TestSettings;
	};

	/**
	 * The Molotov test's copy: a throw every 2.75 s (5 s), a burn every 0.25 s (0.5 s) for 2 s (4 s), so still 8 burns a
	 * patch and still a gap between patches (0.6 s flight + 2 s of fire < 2.75 s). Fixed waits scale with them.
	 */
	constexpr float MolotovTestThrowInterval = 2.75f;
	constexpr float MolotovTestBurnInterval = 0.25f;
	constexpr float MolotovTestPatchDuration = 2.f;
	constexpr double MolotovThrowTimeScale = MolotovTestThrowInterval / 5.0;
	constexpr double MolotovBurnTimeScale = MolotovTestBurnInterval / 0.5;

	int32 NumThrownActive()
	{
		const UGSThrownSubsystem* Thrown = GetThrown();
		return Thrown ? Thrown->GetActive().Num() : 0;
	}

	AGSFirePatch* FindActivePatch()
	{
		const TArray<AGSFirePatch*> Patches = GetMolotov()->GetActivePatches();
		return Patches.Num() > 0 ? Patches[0] : nullptr;
	}

	bool MolotovOnSchedule()
	{
		const UGSMolotovComponent* Molotov = GetMolotov();
		if (!Molotov || Molotov->GetBurstShots() == 0)
		{
			return true;
		}
		const double Elapsed = GetPIEWorld()->GetTimeSeconds() - Molotov->GetBurstStartTime();
		return Molotov->GetBurstShots() == 1 + FMath::FloorToInt(Elapsed / Molotov->GetThrowInterval() + 1e-6);
	}

	void RecordMolotov(FMolotovState& S)
	{
		S.HealthBefore.Reset();
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S.Bandits)
		{
			S.HealthBefore.Add(Bandit.IsValid() ? GetHealth(Bandit->GetAbilitySystemComponent()) : 0.f);
		}
		S.ThrowsBefore = GetMolotov()->GetThrows();
		S.PatchesBefore = GetMolotov()->GetPatchesLit();
		S.BurnsBefore = GetMolotov()->GetBurns();
		S.PlayerHealthBefore = GetHealth(GetPlayerASC());
		S.SourceDamageBefore = GetGameState()->GetRunStats()->GetDamageDealtBy(EGSDamageSource::Molotov);
	}

	float MolotovLost(const FMolotovState& S, int32 Index)
	{
		const TWeakObjectPtr<AGSEnemyCharacter>& Bandit = S.Bandits[Index];
		return Bandit.IsValid() ? S.HealthBefore[Index] - GetHealth(Bandit->GetAbilitySystemComponent()) : -1.f;
	}

	/** Enqueue: wait until nothing is in the air or burning (a gap between patches), then record. */
	void WaitMolotovIdleAndRecord(FAutomationTestBase* Test, TSharedRef<FMolotovState> S)
	{
		WaitUntil(Test, TEXT("no bottle or patch"), []() { return NumThrownActive() == 0; }, 20.0);
		Step([S]() { RecordMolotov(*S); });
	}

	/** Enqueue: wait for Count patches lit after the last record to burn out. */
	void WaitPatchesDone(FAutomationTestBase* Test, TSharedRef<FMolotovState> S, int32 Count, bool bCheckSchedule = false)
	{
		WaitUntil(Test, FString::Printf(TEXT("%d patch(es) burned out"), Count), [S, Count, bCheckSchedule]()
		{
			if (bCheckSchedule)
			{
				S->bScheduleOk &= MolotovOnSchedule();
				++S->Frames;
			}
			return GetMolotov()->GetPatchesLit() >= S->PatchesBefore + Count && GetMolotov()->GetActivePatches().Num() == 0;
		}, 40.0);
	}

	/**
	 * Enqueue one lead run: the player in a safe zone (no throws) while a bandit 2 m ahead speeds up to a steady
	 * LeadRunnerSpeed straight down the lane; then the safe zone goes, one bottle is thrown at it with LeadFactor, and the
	 * patch's first burn is checked against where the runner is when the bottle lands.
	 */
	void EnqueueMolotovLeadRun(FAutomationTestBase* Test, TSharedRef<FMolotovState> S, float LeadFactor)
	{
		Step([S, LeadFactor]()
		{
			GetPlayerASC()->AddLooseGameplayTag(GSGameplayTags::State_SafeZone);
			RemoveAllEnemies(GetPIEWorld());
			S->Bandits.Reset();
			if (S->TestSettings.IsValid())
			{
				S->TestSettings->LeadFactor = LeadFactor;
			}
		});
		WaitUntil(Test, TEXT("no bottle or patch"), []() { return NumThrownActive() == 0; }, 20.0);
		Step([S]() { S->Bandits.Add(SpawnLeadRunner(S->Ground + S->Dir * 200.f, S->Dir)); });
		WaitUntil(Test, TEXT("the runner at a steady speed"), [S]()
		{
			AGSEnemyCharacter* Runner = S->Bandits.Num() > 0 ? S->Bandits[0].Get() : nullptr;
			if (!Runner)
			{
				return true;
			}
			Runner->AddMovementInput(S->Dir, 1.f);
			return Runner->GetVelocity().Size2D() >= LeadRunnerSpeed * 0.99f;
		}, 10.0);
		Step([S]()
		{
			RecordMolotov(*S);
			S->TargetHitsBefore = GetMolotov()->GetFirstBurnTargetHits();
			GetPlayerASC()->RemoveLooseGameplayTag(GSGameplayTags::State_SafeZone);
		});
		WaitUntil(Test, TEXT("the patch under the runner"), [S]()
		{
			if (AGSEnemyCharacter* Runner = S->Bandits.Num() > 0 ? S->Bandits[0].Get() : nullptr)
			{
				Runner->AddMovementInput(S->Dir, 1.f);
			}
			return GetMolotov()->GetPatchesLit() > S->PatchesBefore;
		}, 10.0);
		Step([Test, S, LeadFactor]()
		{
			UGSMolotovComponent* Molotov = GetMolotov();
			const AGSEnemyCharacter* Runner = S->Bandits.Num() > 0 ? S->Bandits[0].Get() : nullptr;
			const AGSFirePatch* Patch = FindActivePatch();
			if (!Test->TestNotNull(TEXT("Lead: the runner"), Runner) || !Test->TestNotNull(TEXT("Lead: the patch"), Patch))
			{
				return;
			}
			const FString Tag = FString::Printf(TEXT("LeadFactor %.0f"), LeadFactor);
			// The first burn came when the bottle landed, in the frame the patch was lit (this one); later burns may have
			// followed in a long frame, so the first burn is read from the hit flag and the runner's damage.
			const float Away = FVector::Dist2D(Runner->GetActorLocation(), Patch->GetParams().Center);
			Test->AddInfo(FString::Printf(TEXT("%s: runner at %.0f cm/s, %.0f cm from the patch center at the first burn (radius %.0f); %d burn(s) so far"),
				*Tag, Runner->GetVelocity().Size2D(), Away, Patch->GetParams().Radius, Patch->GetBurns()));
			Test->TestEqual(Tag + TEXT(": one throw"), Molotov->GetThrows(), S->ThrowsBefore + 1);
			Test->TestEqual(Tag + TEXT(": thrown at the runner"), Molotov->GetLastTarget(), S->Bandits[0].Get());
			Test->TestTrue(Tag + (LeadFactor > 0.f ? TEXT(": the landing was led") : TEXT(": the landing was not led")), Molotov->WasLastThrowLed() == (LeadFactor > 0.f));
			const float Feet = Runner->GetActorLocation().Z - Runner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			Test->TestTrue(Tag + TEXT(": the patch is on the ground"), FMath::Abs(Patch->GetParams().Center.Z - Feet) < 5.f);
			if (LeadFactor > 0.f)
			{
				Test->TestTrue(Tag + TEXT(": the moving runner is in the patch at its first burn"), Molotov->DidLastPatchHitTarget() && Away <= Patch->GetParams().Radius);
				Test->TestEqual(Tag + TEXT(": counted as a target hit"), Molotov->GetFirstBurnTargetHits(), S->TargetHitsBefore + 1);
				Test->TestTrue(Tag + TEXT(": the runner burns"), MolotovLost(*S, 0) >= 4.f - 0.01f);
			}
			else
			{
				Test->TestTrue(Tag + TEXT(": the moving runner is outside the patch at its first burn"), !Molotov->DidLastPatchHitTarget() && Away > Patch->GetParams().Radius);
				Test->TestEqual(Tag + TEXT(": counted as a miss"), Molotov->GetFirstBurnTargetHits(), S->TargetHitsBefore);
				Test->TestEqual(Tag + TEXT(": the runner takes nothing"), MolotovLost(*S, 0), 0.f, 0.01f);
			}
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSMolotovTest, "BanditBaller.Upgrades.Molotov", GSTests::TestFlags)
bool FGSMolotovTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	TSharedRef<FMolotovState> S = MakeShared<FMolotovState>();

	// Data, a clear lane, the player near its start. Game time x4.
	Step([this, S]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const UGSMolotovSettings* Settings = GameMode->GetMolotovSettings();
		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("Molotov"));
		if (!TestNotNull(TEXT("DA_MolotovSettings on BP_GSGameMode"), Settings) || !TestNotNull(TEXT("DA_Upgrade_Molotov"), Upgrade)
			|| !TestNotNull(TEXT("Molotov component on the character"), GetMolotov()))
		{
			return;
		}
		TestEqual(TEXT("A throw every 5 s"), Settings->ThrowInterval, 5.f);
		TestEqual(TEXT("15 m range"), Settings->Range, 1500.f);
		TestEqual(TEXT("0.6 s flight"), Settings->FlightTime, 0.6f);
		TestEqual(TEXT("4 damage per burn"), Settings->BurnDamage, 4.f);
		TestEqual(TEXT("A burn every 0.5 s"), Settings->BurnInterval, 0.5f);
		TestEqual(TEXT("A patch burns 4 s"), Settings->PatchDuration, 4.f);
		TestEqual(TEXT("2.5 m patch"), Settings->PatchRadius, 250.f);
		TestEqual(TEXT("+10% burn damage per power"), Settings->DamageBonusPerPower, 0.1f);
		TestEqual(TEXT("+5% patch radius per power"), Settings->RadiusBonusPerPower, 0.05f);
		TestNotNull(TEXT("Damage effect (GE_Damage)"), Settings->DamageEffect.Get());
		TestTrue(TEXT("Bottle and patch meshes and materials"), Settings->BottleMesh && Settings->BottleMaterial && Settings->PatchMesh && Settings->PatchMaterial);
		TestTrue(TEXT("Category Weapon"), Upgrade->Category == EGSUpgradeCategory::Weapon);
		TestEqual(TEXT("The first pick gives the weapon"), Upgrade->WeaponGivingPicks, 1);
		TestTrue(TEXT("Effect GE_Upgrade_Molotov"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_Molotov::StaticClass()));
		TestTrue(TEXT("In DA_UpgradePool"), GameMode->GetUpgradePool() && GameMode->GetUpgradePool()->Upgrades.Contains(Upgrade));
		TestEqual(TEXT("Card text, none owned"), Upgrade->GetFormattedDescription(0).ToString(), FString(TEXT("New weapon: throws a bottle that leaves a patch of fire on the ground.")));
		TestEqual(TEXT("Card text, owned"), Upgrade->GetFormattedDescription(1).ToString(), FString(TEXT("Makes the Molotov's fire stronger and its patch bigger.")));
		TestEqual(TEXT("Card line of a later Common pick"), Upgrade->FormatValueText(GameMode->GetUpgradePool()->GetPickValue(Upgrade, EGSUpgradeRarity::Common, 1)).ToString(),
			FString(TEXT("+10% Molotov burn damage, +5% patch size")));
		TestEqual(TEXT("Card line of a Rare later pick (halves)"), Upgrade->FormatValueText(1.5f).ToString(), FString(TEXT("+15% Molotov burn damage, +7.5% patch size")));
		TestEqual(TEXT("The card's +10% matches the settings"), Upgrade->GetDisplayScale() / 100.f, Settings->DamageBonusPerPower, 0.0001f);
		TestEqual(TEXT("The card's +5% matches the settings"), Upgrade->SecondaryDisplayScale / 100.f, Settings->RadiusBonusPerPower, 0.0001f);
		TestEqual(TEXT("LeadFactor 1: it aims where the target will be"), Settings->LeadFactor, 1.f);

		// From here on the test runs on a copy with shorter times (the asset's are checked above).
		UGSMolotovSettings* Copy = MakeWeaponSettingsCopy(Settings);
		if (!TestNotNull(TEXT("A test copy of DA_MolotovSettings"), Copy))
		{
			return;
		}
		Copy->ThrowInterval = MolotovTestThrowInterval;
		Copy->BurnInterval = MolotovTestBurnInterval;
		Copy->PatchDuration = MolotovTestPatchDuration;
		GameMode->SetMolotovSettingsOverride(Copy);
		S->TestSettings = Copy;
		TestEqual(TEXT("The test copy is in use"), GetMolotov()->GetThrowInterval(), static_cast<double>(MolotovTestThrowInterval), 1e-6);

		FVector Lane;
		if (!TestTrue(TEXT("A clear lane in the town"), FindClearLane(2600.f, Lane, S->Dir)))
		{
			return;
		}
		S->Side = FVector(-S->Dir.Y, S->Dir.X, 0.f);
		S->PlayerSpot = Lane + S->Dir * 300.f;
		S->Ground = FVector(S->PlayerSpot.X, S->PlayerSpot.Y, 0.f);
		GetPlayer()->TeleportTo(S->PlayerSpot, S->Dir.Rotation());
		RemoveAllEnemies(GetPIEWorld());

		// Granted; the only bandit is 16 m away (out of range).
		TestEqual(TEXT("gs.Upgrade.Grant Molotov"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Molotov"), 1), 1);
		TestEqual(TEXT("Owned: level 1"), GetMolotov()->GetLevel(), 1);
		TestEqual(TEXT("Power 0: 4 per burn"), GetMolotov()->GetBurnDamage(), 4.f, 0.0001f);
		TestEqual(TEXT("Power 0: radius 250"), GetMolotov()->GetPatchRadius(), 250.f, 0.01f);
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 1600.f));
		RecordMolotov(*S);
	});
	WaitWorldSeconds(this, 1.5 * MolotovThrowTimeScale);
	Step([this, S]()
	{
		TestNull(TEXT("16 m: no target"), GetMolotov()->FindTarget());
		TestEqual(TEXT("16 m: no throw"), GetMolotov()->GetThrows(), S->ThrowsBefore);
		// It walks into range: a bottle at once (nothing banked).
		AGSEnemyCharacter* Bandit = S->Bandits[0].Get();
		if (Bandit)
		{
			const FVector At = S->Ground + S->Dir * 800.f;
			Bandit->SetActorLocation(FVector(At.X, At.Y, Bandit->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
		}
		S->InRangeAt = GetPIEWorld()->GetTimeSeconds();
	});
	WaitUntil(this, TEXT("a patch"), [S]() { return GetMolotov()->GetPatchesLit() > S->PatchesBefore; }, 10.0);
	Step([this, S]()
	{
		UGSMolotovComponent* Molotov = GetMolotov();
		const double Delay = Molotov->GetBurstStartTime() - S->InRangeAt;
		TestEqual(TEXT("One throw (nothing banked)"), Molotov->GetThrows(), S->ThrowsBefore + 1);
		TestTrue(FString::Printf(TEXT("The throw comes at once (%.3f s)"), Delay), Delay >= -0.001 && Delay < 0.25);
		const AGSFirePatch* Patch = FindActivePatch();
		const AGSEnemyCharacter* Bandit = S->Bandits[0].Get();
		if (TestNotNull(TEXT("A patch is burning"), Patch) && Bandit)
		{
			const float Feet = Bandit->GetActorLocation().Z - Bandit->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			TestTrue(TEXT("The patch is at the target's feet"), FVector::Dist2D(Patch->GetParams().Center, Bandit->GetActorLocation()) < 1.f
				&& FMath::IsNearlyEqual(Patch->GetParams().Center.Z, Feet, 1.f));
			TestTrue(TEXT("The patch has no collision"), Patch->GetMesh()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			TestEqual(TEXT("The patch is drawn at its radius"), static_cast<float>(Patch->GetActorScale3D().X * 50.0), 250.f, 0.1f);
			// LeadFactor 1 (the default) with a standing target: exactly the old landing point.
			TestFalse(TEXT("A standing target: the landing isn't led"), Molotov->WasLastThrowLed());
			TestTrue(TEXT("A standing target: its first burn caught it"), Molotov->DidLastPatchHitTarget());
		}
	});

	// A bandit running straight down the lane: in the patch at its first burn with LeadFactor 1, outside it with 0.
	EnqueueMolotovLeadRun(this, S, 1.f);
	EnqueueMolotovLeadRun(this, S, 0.f);
	Step([this, S]()
	{
		if (S->TestSettings.IsValid())
		{
			S->TestSettings->LeadFactor = 1.f;
		}
		// The burn test: T 1.2 m in front of the player (who stands in the fire, god mode off); A 1.5 m and B 2.4 m from T
		// (inside), C 2.7 m (outside at 2.5 m), D floating 3 m above the patch.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		const FVector T = S->Ground + S->Dir * 120.f;
		S->Bandits.Add(SpawnToughBandit(T));                                                   // [0] T
		S->Bandits.Add(SpawnToughBandit(T + S->Side * 150.f));                                 // [1] A
		S->Bandits.Add(SpawnToughBandit(T - S->Side * 240.f));                                 // [2] B
		S->Bandits.Add(SpawnToughBandit(T + S->Dir * 270.f));                                  // [3] C
		S->Bandits.Add(SpawnToughBandit(T + S->Dir * 100.f + FVector(0.f, 0.f, 300.f)));       // [4] D, 3 m up
	});
	WaitMolotovIdleAndRecord(this, S);
	Step([this, S]()
	{
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		S->PlayerHealthBefore = GetHealth(GetPlayerASC());
		S->bScheduleOk = true;
		S->Frames = 0;
	});
	WaitPatchesDone(this, S, 2, true);
	Step([this, S]()
	{
		UGSMolotovComponent* Molotov = GetMolotov();
		const int32 Patches = Molotov->GetPatchesLit() - S->PatchesBefore;
		TestTrue(FString::Printf(TEXT("Every frame on the exact schedule (the test's %.2f s interval; %d frames)"), Molotov->GetThrowInterval(), S->Frames), S->bScheduleOk);
		TestEqual(TEXT("Target T"), Molotov->GetLastTarget(), S->Bandits[0].Get());
		TestEqual(TEXT("8 burns per patch (the first on landing, then one every burn interval for the duration: 4 s at 0.5 s, here 2 s at 0.25 s)"),
			Molotov->GetBurns() - S->BurnsBefore, 8 * Patches);
		const FString Names[] = { TEXT("T"), TEXT("A (1.5 m)"), TEXT("B (2.4 m)"), TEXT("C (2.7 m, outside)"), TEXT("D (3 m up, outside)") };
		const float Expect[] = { 32.f, 32.f, 32.f, 0.f, 0.f };
		for (int32 Index = 0; Index < 5; ++Index)
		{
			TestEqual(FString::Printf(TEXT("%s: %.0f per patch"), *Names[Index], Expect[Index]), MolotovLost(*S, Index), Expect[Index] * Patches, 0.01f);
		}
		TestEqual(TEXT("The player standing in the fire is unhurt"), GetHealth(GetPlayerASC()), S->PlayerHealthBefore, 0.01f);
		TestEqual(TEXT("Run stats count it as Molotov damage"), GetGameState()->GetRunStats()->GetDamageDealtBy(EGSDamageSource::Molotov) - S->SourceDamageBefore,
			96.f * Patches, 0.01f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		// Power 1: 4.4 per burn, 262.5 cm (C still outside).
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Molotov"), 1);
		TestEqual(TEXT("Power 1: 4.4 per burn"), Molotov->GetBurnDamage(), 4.4f, 0.0001f);
		TestEqual(TEXT("Power 1: radius 262.5"), Molotov->GetPatchRadius(), 262.5f, 0.01f);
	});
	WaitMolotovIdleAndRecord(this, S);
	WaitPatchesDone(this, S, 1);
	Step([this, S]()
	{
		// Tough bandits have 100,000 HP, where a float step is about 0.008: eight burns of a fractional amount drift by a
		// few hundredths, so these allow 0.1.
		TestEqual(TEXT("Power 1: T takes 8 x 4.4"), MolotovLost(*S, 0), 35.2f, 0.1f);
		TestEqual(TEXT("Power 1: C still outside"), MolotovLost(*S, 3), 0.f, 0.01f);
		// An Epic pick (+2): power 3, 5.2 per burn, 287.5 cm: C (2.7 m) is inside now.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Molotov"), 1, EGSUpgradeRarity::Epic);
		TestEqual(TEXT("Power 3: 5.2 per burn"), GetMolotov()->GetBurnDamage(), 5.2f, 0.0001f);
		TestEqual(TEXT("Power 3: radius 287.5"), GetMolotov()->GetPatchRadius(), 287.5f, 0.01f);
	});
	WaitMolotovIdleAndRecord(this, S);
	WaitPatchesDone(this, S, 1);
	Step([this, S]()
	{
		TestEqual(TEXT("Power 3: T takes 8 x 5.2"), MolotovLost(*S, 0), 41.6f, 0.1f);
		TestEqual(TEXT("Power 3: C (2.7 m) burns now"), MolotovLost(*S, 3), 41.6f, 0.1f);
		TestEqual(TEXT("Power 3: D (3 m up) still doesn't"), MolotovLost(*S, 4), 0.f, 0.01f);

		// Paused with a patch burning: no burns, the patch doesn't age.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 500.f));
	});
	WaitUntil(this, TEXT("a patch burning"), []() { return FindActivePatch() != nullptr; }, 20.0);
	Step([S]()
	{
		RecordMolotov(*S);
		const AGSFirePatch* Patch = FindActivePatch();
		S->AgeBefore = Patch ? Patch->GetAge() : -1.f;
		S->PatchBurnsBefore = Patch ? Patch->GetBurns() : -1;
		UGameplayStatics::SetGamePaused(GetPIEWorld(), true);
	});
	WaitRealSeconds(this, 1.0);
	Step([this, S]()
	{
		const AGSFirePatch* Patch = FindActivePatch();
		TestTrue(TEXT("Paused: the patch doesn't age"), Patch && FMath::IsNearlyEqual(Patch->GetAge(), S->AgeBefore));
		TestTrue(TEXT("Paused: no burns"), Patch && Patch->GetBurns() == S->PatchBurnsBefore && GetMolotov()->GetBurns() == S->BurnsBefore);
		TestEqual(TEXT("Paused: no damage"), MolotovLost(*S, 0), 0.f, 0.01f);
		UGameplayStatics::SetGamePaused(GetPIEWorld(), false);
		// A safe zone: the burning patch skips its burns, and nothing is thrown.
		GetPlayerASC()->AddLooseGameplayTag(GSGameplayTags::State_SafeZone);
		RecordMolotov(*S);
	});
	WaitWorldSeconds(this, 1.2 * MolotovBurnTimeScale); // 2.4 burn intervals
	Step([this, S]()
	{
		TestFalse(TEXT("Safe zone: it can't throw"), GetMolotov()->CanThrow());
		TestEqual(TEXT("Safe zone: no burns"), GetMolotov()->GetBurns(), S->BurnsBefore);
		TestEqual(TEXT("Safe zone: no damage"), MolotovLost(*S, 0), 0.f, 0.01f);
		GetPlayerASC()->RemoveLooseGameplayTag(GSGameplayTags::State_SafeZone);

		// DPS: one bandit in the fire, then five.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 500.f));
	});
	WaitMolotovIdleAndRecord(this, S);
	WaitPatchesDone(this, S, 1);
	Step([this, S]()
	{
		// DPS at the asset's times (a throw every 5 s, a patch burns 4 s); the damage per patch is the same in the copy.
		const float Lost = MolotovLost(*S, 0);
		const double AssetThrowInterval = MolotovTestThrowInterval / MolotovThrowTimeScale;
		const double AssetPatchDuration = MolotovTestPatchDuration / MolotovBurnTimeScale;
		AddInfo(FString::Printf(TEXT("DPS vs one enemy (power 3): %.1f (%.1f per patch, a throw every %.0f s; %.1f DPS while it burns)"),
			Lost / AssetThrowInterval, Lost, AssetThrowInterval, Lost / AssetPatchDuration));
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		const FVector Center = S->Ground + S->Dir * 600.f;
		for (const FVector& Offset : { FVector::ZeroVector, S->Dir * 120.f, S->Dir * -120.f, S->Side * 120.f, S->Side * -120.f })
		{
			S->Bandits.Add(SpawnToughBandit(Center + Offset));
		}
	});
	WaitMolotovIdleAndRecord(this, S);
	WaitPatchesDone(this, S, 1);
	Step([this, S]()
	{
		float Lost = 0.f;
		for (int32 Index = 0; Index < S->Bandits.Num(); ++Index)
		{
			Lost += MolotovLost(*S, Index);
		}
		TestEqual(TEXT("Five in the fire: all burn every tick"), Lost, 5.f * 8.f * GetMolotov()->GetBurnDamage(), 0.5f);
		AddInfo(FString::Printf(TEXT("DPS vs five enemies (power 3): %.1f (%.1f per patch)"), Lost / (MolotovTestThrowInterval / MolotovThrowTimeScale), Lost));

		// Its kill is the player's: a weak bandit, Leech, a sure health orb.
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Leech"), 1);
		GetGameMode()->SetHealthOrbDropChanceOverride(1.f);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		ApplyDamage(GetPlayerASC(), 10.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		S->PlayerHealthBefore = GetHealth(GetPlayerASC());
		GetPlayerCombo()->EndCombo(EGSComboEndReason::Reset);
		S->KillsBefore = GetGameState()->GetRunStats()->GetTotalKills();
		S->XPOrbsBefore = GetCashSubsystem()->GetNumXPOrbsDropped();
		S->HealthOrbsBefore = GetCashSubsystem()->GetNumHealthOrbsDropped();
		S->Weak = SpawnFrozenBandit(S->Ground + S->Dir * 500.f);
		if (S->Weak.IsValid())
		{
			S->Weak->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 3.f);
		}
	});
	WaitUntil(this, TEXT("the fire to kill the bandit"), [S]() { return !S->Weak.IsValid() || S->Weak->GetHealthComponent()->IsDead(); }, 20.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Fire kill: counted in run stats"), GetGameState()->GetRunStats()->GetTotalKills(), S->KillsBefore + 1);
		TestEqual(TEXT("Fire kill: counts for the combo"), GetPlayerCombo()->GetComboCount(), 1);
		TestEqual(TEXT("Fire kill: an XP orb"), GetCashSubsystem()->GetNumXPOrbsDropped(), S->XPOrbsBefore + 1);
		TestEqual(TEXT("Fire kill: the health orb chance (1: one orb)"), GetCashSubsystem()->GetNumHealthOrbsDropped(), S->HealthOrbsBefore + 1);
		TestTrue(TEXT("Fire kill: a cash drop"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) > 0);
		TestEqual(TEXT("Fire kill: Leech heals"), GetHealth(GetPlayerASC()), S->PlayerHealthBefore + 1.f, 0.01f);
		GetGameMode()->SetHealthOrbDropChanceOverride(0.f);
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 500.f));
	});

	// A new run with a patch burning: the patch and any bottle are gone, the weapon is gone.
	WaitUntil(this, TEXT("a patch burning"), []() { return FindActivePatch() != nullptr; }, 20.0);
	Step([this, S]()
	{
		GetGameMode()->RestartRunWithSeed(5101);
		GetGameMode()->SetRoundTimerPaused(true);
		GetPlayer()->TeleportTo(S->PlayerSpot, S->Dir.Rotation());
		TestEqual(TEXT("New run: no patch or bottle left"), NumThrownActive(), 0);
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 500.f));
		RecordMolotov(*S);
	});
	WaitWorldSeconds(this, 2.0 * MolotovThrowTimeScale);
	Step([this, S]()
	{
		UGSMolotovComponent* Molotov = GetMolotov();
		TestEqual(TEXT("New run: level 0"), Molotov->GetLevel(), 0);
		TestEqual(TEXT("New run: power 0"), Molotov->GetPower(), 0.f);
		TestEqual(TEXT("New run: no throws"), Molotov->GetThrows(), S->ThrowsBefore);
		TestEqual(TEXT("New run: no burns"), Molotov->GetBurns(), S->BurnsBefore);
		TestEqual(TEXT("New run: no patches"), Molotov->GetActivePatches().Num(), 0);
		TestEqual(TEXT("New run: a bandit in range takes nothing"), MolotovLost(*S, 0), 0.f, 0.01f);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		GetGameMode()->SetMolotovSettingsOverride(nullptr);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Tomahawk (Prompt 52): flies out through every enemy on its line, then back to the player and through them again
// ---------------------------------------------------------------------------

namespace GSTests
{
	struct FTomahawkState
	{
		FVector PlayerSpot = FVector::ZeroVector;
		FVector Ground = FVector::ZeroVector;
		FVector Dir = FVector::ForwardVector;
		FVector Side = FVector::RightVector;
		TArray<TWeakObjectPtr<AGSEnemyCharacter>> Bandits;
		TArray<float> HealthBefore;
		int32 ThrowsBefore = 0;
		int32 HitsBefore = 0;
		float SourceDamageBefore = 0.f;
		double SpawnedAt = 0.0;
		FVector FlyingAt = FVector::ZeroVector;
		bool bScheduleOk = true;
		int32 Frames = 0;
		TWeakObjectPtr<AStaticMeshActor> Wall;
		TWeakObjectPtr<AGSEnemyCharacter> Weak;
		float PlayerHealthBefore = 0.f;
		int32 KillsBefore = 0;
		int32 XPOrbsBefore = 0;
		int32 HealthOrbsBefore = 0;
	};

	/**
	 * The Tomahawk test's copy throws every 2.25 s instead of 3 s: out 18 m and back at 20 m/s takes about 1.8 s, so this
	 * still leaves a moment with no tomahawk in the air between throws. Fixed waits scale with it.
	 */
	constexpr float TomahawkTestInterval = 2.25f;
	constexpr double TomahawkTimeScale = TomahawkTestInterval / 3.0;

	bool TomahawkOnSchedule()
	{
		const UGSTomahawkComponent* Tomahawk = GetTomahawk();
		if (!Tomahawk || Tomahawk->GetBurstShots() == 0)
		{
			return true;
		}
		const double Elapsed = GetPIEWorld()->GetTimeSeconds() - Tomahawk->GetBurstStartTime();
		return Tomahawk->GetBurstShots() == 1 + FMath::FloorToInt(Elapsed / Tomahawk->GetThrowInterval() + 1e-6);
	}

	void RecordTomahawk(FTomahawkState& S)
	{
		S.HealthBefore.Reset();
		for (const TWeakObjectPtr<AGSEnemyCharacter>& Bandit : S.Bandits)
		{
			S.HealthBefore.Add(Bandit.IsValid() ? GetHealth(Bandit->GetAbilitySystemComponent()) : 0.f);
		}
		S.ThrowsBefore = GetTomahawk()->GetThrows();
		S.HitsBefore = GetTomahawk()->GetHitsDealt();
		S.SourceDamageBefore = GetGameState()->GetRunStats()->GetDamageDealtBy(EGSDamageSource::Tomahawk);
	}

	float TomahawkLost(const FTomahawkState& S, int32 Index)
	{
		const TWeakObjectPtr<AGSEnemyCharacter>& Bandit = S.Bandits[Index];
		return Bandit.IsValid() ? S.HealthBefore[Index] - GetHealth(Bandit->GetAbilitySystemComponent()) : -1.f;
	}

	/** Enqueue: wait until no tomahawk is in the air, then record. */
	void WaitTomahawksDownAndRecord(FAutomationTestBase* Test, TSharedRef<FTomahawkState> S)
	{
		WaitUntil(Test, TEXT("no tomahawk in the air"), []() { return GetTomahawk()->GetActiveTomahawks().Num() == 0; }, 20.0);
		Step([S]() { RecordTomahawk(*S); });
	}

	/** Enqueue: wait for Count more throws after the last record, all back. */
	void WaitThrowsBack(FAutomationTestBase* Test, TSharedRef<FTomahawkState> S, int32 Count, bool bCheckSchedule = false)
	{
		WaitUntil(Test, FString::Printf(TEXT("%d throw(s) back"), Count), [S, Count, bCheckSchedule]()
		{
			if (bCheckSchedule)
			{
				S->bScheduleOk &= TomahawkOnSchedule();
				++S->Frames;
			}
			return GetTomahawk()->GetThrows() >= S->ThrowsBefore + Count && GetTomahawk()->GetActiveTomahawks().Num() == 0;
		}, 30.0);
	}

	/** A line of Count tough frozen bandits from Start along Dir, Spacing apart. */
	void SpawnBanditLine(FTomahawkState& S, float Start, float Spacing, int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			S.Bandits.Add(SpawnToughBandit(S.Ground + S.Dir * (Start + Spacing * Index)));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSTomahawkTest, "BanditBaller.Upgrades.Tomahawk", GSTests::TestFlags)
bool FGSTomahawkTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	TSharedRef<FTomahawkState> S = MakeShared<FTomahawkState>();

	// Data, a clear lane, the player near its start. Game time x4.
	Step([this, S]()
	{
		SpeedUpWorld(4.f);
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		GameMode->SetHealthOrbDropChanceOverride(0.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);

		const UGSTomahawkSettings* Settings = GameMode->GetTomahawkSettings();
		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("Tomahawk"));
		if (!TestNotNull(TEXT("DA_TomahawkSettings on BP_GSGameMode"), Settings) || !TestNotNull(TEXT("DA_Upgrade_Tomahawk"), Upgrade)
			|| !TestNotNull(TEXT("Tomahawk component on the character"), GetTomahawk()))
		{
			return;
		}
		TestEqual(TEXT("12 damage per hit"), Settings->Damage, 12.f);
		TestEqual(TEXT("A throw every 3 s"), Settings->ThrowInterval, 3.f);
		TestEqual(TEXT("18 m range"), Settings->Range, 1800.f);
		TestEqual(TEXT("Flies 18 m out"), Settings->Distance, 1800.f);
		TestEqual(TEXT("1 tomahawk"), Settings->BaseTomahawks, 1);
		TestEqual(TEXT("+1 tomahawk per 3 full power"), Settings->PowerPerTomahawk, 3.f);
		TestEqual(TEXT("+10% damage per power"), Settings->DamageBonusPerPower, 0.1f);
		TestEqual(TEXT("Tomahawks at power 0 / 2.9 / 3 / 5.9 / 6"), FString::Printf(TEXT("%d %d %d %d %d"), Settings->GetTomahawks(0.f), Settings->GetTomahawks(2.9f),
			Settings->GetTomahawks(3.f), Settings->GetTomahawks(5.9f), Settings->GetTomahawks(6.f)), FString(TEXT("1 1 2 2 3")));
		TestNotNull(TEXT("Damage effect (GE_Damage)"), Settings->DamageEffect.Get());
		TestTrue(TEXT("Tomahawk mesh and material"), Settings->TomahawkMesh && Settings->TomahawkMaterial);
		TestTrue(TEXT("Category Weapon"), Upgrade->Category == EGSUpgradeCategory::Weapon);
		TestEqual(TEXT("The first pick gives the weapon"), Upgrade->WeaponGivingPicks, 1);
		TestTrue(TEXT("Effect GE_Upgrade_Tomahawk"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_Tomahawk::StaticClass()));
		TestTrue(TEXT("In DA_UpgradePool"), GameMode->GetUpgradePool() && GameMode->GetUpgradePool()->Upgrades.Contains(Upgrade));
		TestEqual(TEXT("Card text, none owned"), Upgrade->GetFormattedDescription(0).ToString(), FString(TEXT("New weapon: throws a tomahawk that cuts through enemies and comes back.")));
		TestEqual(TEXT("Card text, owned"), Upgrade->GetFormattedDescription(1).ToString(), FString(TEXT("Makes the Tomahawk stronger and, in time, adds tomahawks.")));
		TestEqual(TEXT("Card line of a later Common pick"), Upgrade->FormatValueText(GameMode->GetUpgradePool()->GetPickValue(Upgrade, EGSUpgradeRarity::Common, 1)).ToString(),
			FString(TEXT("+10% Tomahawk damage")));
		TestEqual(TEXT("The card's +10% matches the settings"), Upgrade->GetDisplayScale() / 100.f, Settings->DamageBonusPerPower, 0.0001f);

		// From here on the test runs on a copy that throws every 2.25 s (the asset's 3 s is checked above).
		UGSTomahawkSettings* Copy = MakeWeaponSettingsCopy(Settings);
		if (!TestNotNull(TEXT("A test copy of DA_TomahawkSettings"), Copy))
		{
			return;
		}
		Copy->ThrowInterval = TomahawkTestInterval;
		GameMode->SetTomahawkSettingsOverride(Copy);
		TestEqual(TEXT("The test copy is in use"), GetTomahawk()->GetThrowInterval(), static_cast<double>(TomahawkTestInterval), 1e-6);

		FVector Lane;
		if (!TestTrue(TEXT("A clear lane in the town"), FindClearLane(2600.f, Lane, S->Dir)))
		{
			return;
		}
		S->Side = FVector(-S->Dir.Y, S->Dir.X, 0.f);
		S->PlayerSpot = Lane + S->Dir * 100.f;
		S->Ground = FVector(S->PlayerSpot.X, S->PlayerSpot.Y, 0.f);
		GetPlayer()->TeleportTo(S->PlayerSpot, S->Dir.Rotation());
		RemoveAllEnemies(GetPIEWorld());
		TestEqual(TEXT("gs.Upgrade.Grant Tomahawk"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Tomahawk"), 1), 1);
		TestEqual(TEXT("Owned: level 1"), GetTomahawk()->GetLevel(), 1);
		TestEqual(TEXT("Power 0: one tomahawk"), GetTomahawk()->GetTomahawksPerThrow(), 1);
		TestEqual(TEXT("Power 0: 12 per hit"), GetTomahawk()->GetHitDamage(), 12.f, 0.0001f);
		S->Bandits.Reset();
		RecordTomahawk(*S);
	});
	// No enemy: nothing is thrown and nothing banked.
	WaitWorldSeconds(this, 1.5 * TomahawkTimeScale); // half a throw interval
	Step([this, S]()
	{
		TestEqual(TEXT("No target: no throws"), GetTomahawk()->GetThrows(), S->ThrowsBefore);
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 400.f));
		S->SpawnedAt = GetPIEWorld()->GetTimeSeconds();
		RecordTomahawk(*S);
	});
	WaitUntil(this, TEXT("the first throw"), [S]() { return GetTomahawk()->GetThrows() > S->ThrowsBefore; }, 10.0);
	Step([this, S]()
	{
		const double Delay = GetPIEWorld()->GetTimeSeconds() - S->SpawnedAt;
		TestEqual(TEXT("One throw (nothing banked)"), GetTomahawk()->GetThrows(), S->ThrowsBefore + 1);
		TestTrue(FString::Printf(TEXT("The first throw comes at once (%.3f s)"), Delay), Delay < 0.3);
	});

	// One bandit: hit once out and once back each throw. DPS against one enemy.
	WaitTomahawksDownAndRecord(this, S);
	WaitThrowsBack(this, S, 2);
	Step([this, S]()
	{
		const int32 Throws = GetTomahawk()->GetThrows() - S->ThrowsBefore;
		TestEqual(TEXT("One bandit: 24 a throw (12 out, 12 back)"), TomahawkLost(*S, 0), 24.f * Throws, 0.01f);
		TestEqual(TEXT("One bandit: two hits a throw"), GetTomahawk()->GetHitsDealt() - S->HitsBefore, 2 * Throws);
		// DPS at the asset's interval (3 s); the damage per throw is the same in the test copy.
		const double AssetInterval = TomahawkTestInterval / TomahawkTimeScale;
		AddInfo(FString::Printf(TEXT("DPS vs one enemy: %.1f (%.0f in %d throws, one every %.0f s)"), TomahawkLost(*S, 0) / (Throws * AssetInterval),
			TomahawkLost(*S, 0), Throws, AssetInterval));

		// A line of five, 3 m apart, and one bandit 2.5 m beside the line.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		SpawnBanditLine(*S, 400.f, 300.f, 5);                                         // [0..4]
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 700.f + S->Side * 250.f)); // [5] beside
	});
	WaitTomahawksDownAndRecord(this, S);
	Step([S]() { S->bScheduleOk = true; S->Frames = 0; });
	WaitThrowsBack(this, S, 2, true);
	Step([this, S]()
	{
		UGSTomahawkComponent* Tomahawk = GetTomahawk();
		const int32 Throws = Tomahawk->GetThrows() - S->ThrowsBefore;
		TestTrue(FString::Printf(TEXT("Every frame on the exact schedule (the test's %.2f s interval; %d frames)"), Tomahawk->GetThrowInterval(), S->Frames), S->bScheduleOk);
		TestEqual(TEXT("Thrown at the nearest"), Tomahawk->GetLastTargets().Num() == 1 ? Tomahawk->GetLastTargets()[0].Get() : nullptr, S->Bandits[0].Get());
		float Lost = 0.f;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			TestEqual(FString::Printf(TEXT("Line of five: bandit %d hit once out and once back"), Index), TomahawkLost(*S, Index), 24.f * Throws, 0.01f);
			Lost += TomahawkLost(*S, Index);
		}
		TestEqual(TEXT("Beside the line: untouched"), TomahawkLost(*S, 5), 0.f, 0.01f);
		TestEqual(TEXT("Ten hits a throw"), Tomahawk->GetHitsDealt() - S->HitsBefore, 10 * Throws);
		TestEqual(TEXT("Run stats count it as Tomahawk damage"), GetGameState()->GetRunStats()->GetDamageDealtBy(EGSDamageSource::Tomahawk) - S->SourceDamageBefore, Lost, 0.01f);
		AddInfo(FString::Printf(TEXT("DPS vs a line of five (3 m apart): %.1f (%.0f in %d throws)"), Lost / (Throws * (TomahawkTestInterval / TomahawkTimeScale)), Lost, Throws));

		// A wall across the lane between bandits 2 (1 m before it) and 3: it turns back there.
		const bool bAlongX = FMath::Abs(S->Dir.X) > FMath::Abs(S->Dir.Y);
		const FVector WallAt = S->Ground + S->Dir * 850.f + FVector(0.f, 0.f, 150.f);
		S->Wall = SpawnTestWall(WallAt, bAlongX ? FVector(0.2f, 6.f, 3.f) : FVector(6.f, 0.2f, 3.f));
		TestTrue(TEXT("A wall across the lane"), S->Wall.IsValid());
	});
	WaitTomahawksDownAndRecord(this, S);
	WaitThrowsBack(this, S, 1);
	Step([this, S]()
	{
		TestEqual(TEXT("Wall: bandit 0 (in front) hit out and back"), TomahawkLost(*S, 0), 24.f, 0.01f);
		TestEqual(TEXT("Wall: bandit 1 (in front) hit out and back"), TomahawkLost(*S, 1), 24.f, 0.01f);
		for (int32 Index = 2; Index < 5; ++Index)
		{
			TestEqual(FString::Printf(TEXT("Wall: bandit %d (behind) untouched"), Index), TomahawkLost(*S, Index), 0.f, 0.01f);
		}
		if (S->Wall.IsValid())
		{
			S->Wall->Destroy();
		}

		// Power 1: 13.2 per hit, still one tomahawk.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Tomahawk"), 1);
		TestEqual(TEXT("Power 1: 13.2 per hit"), GetTomahawk()->GetHitDamage(), 13.2f, 0.0001f);
		TestEqual(TEXT("Power 1: one tomahawk"), GetTomahawk()->GetTomahawksPerThrow(), 1);
	});
	WaitTomahawksDownAndRecord(this, S);
	WaitThrowsBack(this, S, 1);
	Step([this, S]()
	{
		TestEqual(TEXT("Power 1: bandit 0 takes 2 x 13.2"), TomahawkLost(*S, 0), 26.4f, 0.05f);
		TestEqual(TEXT("Power 1: the far bandit too (no wall now)"), TomahawkLost(*S, 4), 26.4f, 0.05f);

		// An Epic pick (+2): power 3, x1.3, two tomahawks: at the nearest and the next nearest.
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Tomahawk"), 1, EGSUpgradeRarity::Epic);
		TestEqual(TEXT("Power 3: 15.6 per hit"), GetTomahawk()->GetHitDamage(), 15.6f, 0.0001f);
		TestEqual(TEXT("Power 3: two tomahawks"), GetTomahawk()->GetTomahawksPerThrow(), 2);
	});
	WaitTomahawksDownAndRecord(this, S);
	WaitThrowsBack(this, S, 1);
	Step([this, S]()
	{
		UGSTomahawkComponent* Tomahawk = GetTomahawk();
		const TArray<TWeakObjectPtr<AGSEnemyCharacter>>& Targets = Tomahawk->GetLastTargets();
		TestTrue(TEXT("Power 3: two tomahawks, at the nearest and the next nearest"),
			Targets.Num() == 2 && Targets[0].Get() == S->Bandits[0].Get() && Targets[1].Get() == S->Bandits[1].Get());
		TestEqual(TEXT("Power 3: both pass bandit 0, out and back (4 hits)"), TomahawkLost(*S, 0), 4.f * 15.6f, 0.05f);
		TestEqual(TEXT("Power 3: 20 hits a throw"), Tomahawk->GetHitsDealt() - S->HitsBefore, 20);

		// Its kill is the player's: a weak bandit, Leech, a sure health orb.
		RemoveAllEnemies(GetPIEWorld());
		UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("Leech"), 1);
		GetGameMode()->SetHealthOrbDropChanceOverride(1.f);
		UGSDebugLibrary::SetGodMode(GetPC(), false);
		ApplyDamage(GetPlayerASC(), 10.f);
		UGSDebugLibrary::SetGodMode(GetPC(), true);
		S->PlayerHealthBefore = GetHealth(GetPlayerASC());
		GetPlayerCombo()->EndCombo(EGSComboEndReason::Reset);
		S->KillsBefore = GetGameState()->GetRunStats()->GetTotalKills();
		S->XPOrbsBefore = GetCashSubsystem()->GetNumXPOrbsDropped();
		S->HealthOrbsBefore = GetCashSubsystem()->GetNumHealthOrbsDropped();
		S->Weak = SpawnFrozenBandit(S->Ground + S->Dir * 400.f);
		if (S->Weak.IsValid())
		{
			S->Weak->GetAbilitySystemComponent()->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), 3.f);
		}
	});
	WaitUntil(this, TEXT("a tomahawk to kill the bandit"), [S]() { return !S->Weak.IsValid() || S->Weak->GetHealthComponent()->IsDead(); }, 10.0);
	Step([this, S]()
	{
		TestEqual(TEXT("Tomahawk kill: counted in run stats"), GetGameState()->GetRunStats()->GetTotalKills(), S->KillsBefore + 1);
		TestEqual(TEXT("Tomahawk kill: counts for the combo"), GetPlayerCombo()->GetComboCount(), 1);
		TestEqual(TEXT("Tomahawk kill: an XP orb"), GetCashSubsystem()->GetNumXPOrbsDropped(), S->XPOrbsBefore + 1);
		TestEqual(TEXT("Tomahawk kill: the health orb chance (1: one orb)"), GetCashSubsystem()->GetNumHealthOrbsDropped(), S->HealthOrbsBefore + 1);
		TestTrue(TEXT("Tomahawk kill: a cash drop"), GetCashSubsystem()->GetNumActiveOfKind(EGSPickupKind::Cash) > 0);
		TestEqual(TEXT("Tomahawk kill: Leech heals"), GetHealth(GetPlayerASC()), S->PlayerHealthBefore + 1.f, 0.01f);
		GetGameMode()->SetHealthOrbDropChanceOverride(0.f);

		// Paused with a tomahawk in the air: it stays put, nothing is thrown.
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 400.f));
	});
	WaitUntil(this, TEXT("a tomahawk in the air"), []() { return GetTomahawk()->GetActiveTomahawks().Num() > 0; }, 10.0);
	Step([S]()
	{
		RecordTomahawk(*S);
		S->FlyingAt = GetTomahawk()->GetActiveTomahawks()[0]->GetActorLocation();
		UGameplayStatics::SetGamePaused(GetPIEWorld(), true);
	});
	WaitRealSeconds(this, 1.0);
	Step([this, S]()
	{
		const TArray<AGSTomahawk*> Active = GetTomahawk()->GetActiveTomahawks();
		TestTrue(TEXT("Paused: the tomahawk doesn't move"), Active.Num() > 0 && Active[0]->GetActorLocation().Equals(S->FlyingAt, 0.01f));
		TestEqual(TEXT("Paused: no throws"), GetTomahawk()->GetThrows(), S->ThrowsBefore);
		UGameplayStatics::SetGamePaused(GetPIEWorld(), false);

		// A new run with tomahawks in the air: they are gone, the weapon is gone.
		GetGameMode()->RestartRunWithSeed(5201);
		GetGameMode()->SetRoundTimerPaused(true);
		GetPlayer()->TeleportTo(S->PlayerSpot, S->Dir.Rotation());
		TestEqual(TEXT("New run: no tomahawk in the air"), GetTomahawk()->GetActiveTomahawks().Num(), 0);
		RemoveAllEnemies(GetPIEWorld());
		S->Bandits.Reset();
		S->Bandits.Add(SpawnToughBandit(S->Ground + S->Dir * 400.f));
		RecordTomahawk(*S);
	});
	WaitWorldSeconds(this, 2.0 * TomahawkTimeScale);
	Step([this, S]()
	{
		UGSTomahawkComponent* Tomahawk = GetTomahawk();
		TestEqual(TEXT("New run: level 0"), Tomahawk->GetLevel(), 0);
		TestEqual(TEXT("New run: power 0"), Tomahawk->GetPower(), 0.f);
		TestEqual(TEXT("New run: no throws"), Tomahawk->GetThrows(), S->ThrowsBefore);
		TestEqual(TEXT("New run: a bandit in range takes nothing"), TomahawkLost(*S, 0), 0.f, 0.01f);
		GetGameMode()->SetHealthOrbDropChanceOverride(-1.f);
		GetGameMode()->SetTomahawkSettingsOverride(nullptr);
		RestoreWorldSpeed();
	});
	EndPIE();
	return true;
}

// ---------------------------------------------------------------------------
// Weapons checkpoint (Prompt 53): the full pool of seven weapons and the weapon-slot rules
// ---------------------------------------------------------------------------

namespace GSTests
{
	/** The index of a card in Offer, or INDEX_NONE: an unowned weapon while a slot is free, else an owned weapon. */
	int32 FindWeaponCardForCheckpoint(const TArray<FGSUpgradeChoice>& Offer, const UGSUpgradeComponent* Upgrades, int32 MaxWeapons)
	{
		const bool bSlotFree = Upgrades->GetOwnedWeapons().Num() < MaxWeapons;
		int32 Owned = INDEX_NONE;
		for (int32 Index = 0; Index < Offer.Num(); ++Index)
		{
			const UGSUpgradeData* Upgrade = Offer[Index].Upgrade;
			if (Upgrade && Upgrade->Category == EGSUpgradeCategory::Weapon)
			{
				if (Upgrades->GetStacks(Upgrade) == 0 && bSlotFree)
				{
					return Index;
				}
				Owned = Upgrades->GetStacks(Upgrade) > 0 && Owned == INDEX_NONE ? Index : Owned;
			}
		}
		return Owned;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGSCheckpointSevenWeaponsTest, "BanditBaller.Checkpoint.SevenWeapons", GSTests::TestFlags)
bool FGSCheckpointSevenWeaponsTest::RunTest(const FString& Parameters)
{
	BeginPIE(this);
	Step([this]()
	{
		AGSGameMode* GameMode = GetGameMode();
		GameMode->SetRoundTimerPaused(true);
		const UGSUpgradePool* Pool = GameMode->GetUpgradePool();
		AGSPlayerState* PS = GetPC() ? GetPC()->GetPlayerState<AGSPlayerState>() : nullptr;
		UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		if (!TestNotNull(TEXT("DA_UpgradePool"), Pool) || !TestNotNull(TEXT("Upgrade component"), Upgrades) || !PS)
		{
			return;
		}

		// 1) DA_UpgradePool holds every upgrade once, each in its category.
		struct FExpected
		{
			const TCHAR* Name;
			EGSUpgradeCategory Category;
			bool bLegendaryAbility;
		};
		const FExpected Expected[] = {
			{ TEXT("Damage"), EGSUpgradeCategory::StatBoost, false }, { TEXT("FireRate"), EGSUpgradeCategory::StatBoost, false },
			{ TEXT("ReloadSpeed"), EGSUpgradeCategory::StatBoost, false }, { TEXT("MoveSpeed"), EGSUpgradeCategory::StatBoost, false },
			{ TEXT("Magnet"), EGSUpgradeCategory::StatBoost, false }, { TEXT("BiggerMags"), EGSUpgradeCategory::StatBoost, false },
			{ TEXT("LoadedDice"), EGSUpgradeCategory::StatBoost, false },
			{ TEXT("ExtraJump"), EGSUpgradeCategory::Ability, false }, { TEXT("DashCharge"), EGSUpgradeCategory::Ability, false },
			{ TEXT("FloatingShield"), EGSUpgradeCategory::Ability, false }, { TEXT("PiercingRounds"), EGSUpgradeCategory::Ability, false },
			{ TEXT("Leech"), EGSUpgradeCategory::Ability, false }, { TEXT("PowderKeg"), EGSUpgradeCategory::Ability, false },
			{ TEXT("SecondWind"), EGSUpgradeCategory::Ability, true },
			{ TEXT("FloatingGun"), EGSUpgradeCategory::Weapon, false }, { TEXT("RazorCards"), EGSUpgradeCategory::Weapon, false },
			{ TEXT("BarbedWire"), EGSUpgradeCategory::Weapon, false }, { TEXT("Dynamite"), EGSUpgradeCategory::Weapon, false },
			{ TEXT("LuckyCoin"), EGSUpgradeCategory::Weapon, false }, { TEXT("Molotov"), EGSUpgradeCategory::Weapon, false },
			{ TEXT("Tomahawk"), EGSUpgradeCategory::Weapon, false },
		};
		TestEqual(TEXT("21 upgrades in DA_UpgradePool"), Pool->Upgrades.Num(), static_cast<int32>(UE_ARRAY_COUNT(Expected)));
		TArray<const UGSUpgradeData*> Weapons;
		for (const FExpected& E : Expected)
		{
			const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(E.Name);
			if (!TestNotNull(FString::Printf(TEXT("DA_Upgrade_%s"), E.Name), Upgrade))
			{
				continue;
			}
			int32 Count = 0;
			for (const UGSUpgradeData* InPool : Pool->Upgrades)
			{
				Count += InPool == Upgrade ? 1 : 0;
			}
			TestEqual(FString::Printf(TEXT("%s is in the pool once"), E.Name), Count, 1);
			const TCHAR* CategoryName = E.Category == EGSUpgradeCategory::StatBoost ? TEXT("Stat Boost") : E.Category == EGSUpgradeCategory::Ability ? TEXT("Ability") : TEXT("Weapon");
			TestTrue(FString::Printf(TEXT("%s is a %s"), E.Name, CategoryName), Upgrade->Category == E.Category);
			TestTrue(FString::Printf(TEXT("%s: Legendary ability %s"), E.Name, E.bLegendaryAbility ? TEXT("yes") : TEXT("no")), Upgrade->bLegendaryAbility == E.bLegendaryAbility);
			if (E.Category == EGSUpgradeCategory::Weapon)
			{
				Weapons.Add(Upgrade);
			}
		}
		for (const FString& Name : UGSDebugLibrary::GetUpgradeDataNames())
		{
			const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(Name);
			TestTrue(FString::Printf(TEXT("Every upgrade asset is in the pool: %s"), *Name), Upgrade && Pool->Upgrades.Contains(Upgrade));
		}
		TestEqual(TEXT("Seven weapons"), Weapons.Num(), 7);
		TestEqual(TEXT("MaxWeapons 3"), Pool->MaxWeapons, 3);
		TestEqual(TEXT("Three HUD weapon slots"), GetHUD() ? GetHUD()->GetWeaponSlotCount() : -1, 3);

		// 2) The roll itself, for each of the 35 sets of three owned weapons: the other four are never offered (offers and
		// first offers with their weapon card), and each owned one still is.
		FRandomStream Stream(5300);
		int32 Sets = 0, Rolls = 0, UnownedCards = 0, SetsMissingAnOwned = 0;
		for (int32 A = 0; A < Weapons.Num(); ++A)
		{
			for (int32 B = A + 1; B < Weapons.Num(); ++B)
			{
				for (int32 C = B + 1; C < Weapons.Num(); ++C)
				{
					const TSet<const UGSUpgradeData*> Owned = { Weapons[A], Weapons[B], Weapons[C] };
					auto GetStacks = [&Owned](const UGSUpgradeData* U) { return Owned.Contains(U) ? 1 : 0; };
					TMap<const UGSUpgradeData*, int32> OwnedCards;
					for (int32 Roll = 0; Roll < 200; ++Roll)
					{
						for (const FGSUpgradeChoice& Choice : Pool->RollOffer(Stream, GetStacks, Roll % 2 == 0))
						{
							if (Choice.Upgrade && Choice.Upgrade->Category == EGSUpgradeCategory::Weapon)
							{
								UnownedCards += Owned.Contains(Choice.Upgrade) ? 0 : 1;
								++OwnedCards.FindOrAdd(Choice.Upgrade);
							}
						}
						++Rolls;
					}
					SetsMissingAnOwned += OwnedCards.Num() == 3 ? 0 : 1;
					++Sets;
				}
			}
		}
		AddInfo(FString::Printf(TEXT("Roll: %d sets of three owned weapons, %d offers"), Sets, Rolls));
		TestEqual(TEXT("35 sets of three weapons"), Sets, 35);
		TestEqual(TEXT("Three owned: none of the other four is ever rolled"), UnownedCards, 0);
		TestEqual(TEXT("Three owned: each owned weapon is still rolled"), SetsMissingAnOwned, 0);

		// 3) Real runs, picking weapons whenever offered and rerolling every offer while rerolls last: never more than
		// MaxWeapons, and once three are owned no other weapon shows up, rerolls included.
		int32 RunsFull = 0, OffersAfterFull = 0, RerollsAfterFull = 0, UnownedAfterFull = 0, MostWeapons = 0;
		TArray<FString> RunLines;
		for (int32 Run = 0; Run < 8; ++Run)
		{
			GameMode->RestartRunWithSeed(5310 + Run);
			GameMode->SetRoundTimerPaused(true);
			TArray<FString> Picked;
			for (int32 OfferIndex = 0; OfferIndex < 40; ++OfferIndex)
			{
				GameMode->QueueUpgradeOfferForDebug(PS);
				while (true)
				{
					const bool bFull = Upgrades->GetOwnedWeapons().Num() >= Pool->MaxWeapons;
					for (const FGSUpgradeChoice& Choice : Upgrades->GetCurrentOffer())
					{
						const bool bUnownedWeapon = Choice.Upgrade && Choice.Upgrade->Category == EGSUpgradeCategory::Weapon && Upgrades->GetStacks(Choice.Upgrade) == 0;
						UnownedAfterFull += bFull && bUnownedWeapon ? 1 : 0;
					}
					// The run's rerolls are kept for once the slots are full (that is what this part checks).
					if (!bFull || Upgrades->GetRerollsLeft() == 0 || !GameMode->RerollUpgradeOffer(PS))
					{
						break;
					}
					++RerollsAfterFull;
				}
				OffersAfterFull += Upgrades->GetOwnedWeapons().Num() >= Pool->MaxWeapons ? 1 : 0;
				const int32 WeaponIndex = FindWeaponCardForCheckpoint(Upgrades->GetCurrentOffer(), Upgrades, Pool->MaxWeapons);
				const TArray<FGSUpgradeChoice> Offer = Upgrades->GetCurrentOffer();
				const int32 Pick = WeaponIndex != INDEX_NONE ? WeaponIndex : 0;
				if (Offer.IsValidIndex(Pick) && Offer[Pick].Upgrade->Category == EGSUpgradeCategory::Weapon && Upgrades->GetStacks(Offer[Pick].Upgrade) == 0)
				{
					Picked.Add(Offer[Pick].Upgrade->DisplayName.ToString());
				}
				GameMode->ChooseUpgrade(PS, Pick);
				MostWeapons = FMath::Max(MostWeapons, Upgrades->GetOwnedWeapons().Num());
			}
			RunsFull += Upgrades->GetOwnedWeapons().Num() == Pool->MaxWeapons ? 1 : 0;
			RunLines.Add(FString::Printf(TEXT("seed %d: %s"), 5310 + Run, *FString::Join(Picked, TEXT(", "))));
		}
		AddInfo(FString::Printf(TEXT("Real runs (weapons in pick order): %s"), *FString::Join(RunLines, TEXT("; "))));
		AddInfo(FString::Printf(TEXT("Real runs: %d of 8 filled all slots; %d offers and %d rerolls after the slots were full"), RunsFull, OffersAfterFull, RerollsAfterFull));
		TestEqual(TEXT("A run never holds more than MaxWeapons"), MostWeapons, Pool->MaxWeapons);
		TestEqual(TEXT("Every run filled its three slots"), RunsFull, 8);
		TestTrue(TEXT("Many offers after the slots were full"), OffersAfterFull >= 100);
		TestEqual(TEXT("Each run's three rerolls were used after the slots were full"), RerollsAfterFull, 24);
		TestEqual(TEXT("Slots full: no unowned weapon in any offer or reroll"), UnownedAfterFull, 0);

		// 4) The first offer of a run always has a weapon card, rerolls included, and over many runs every weapon is the
		// first offer's weapon at some point.
		int32 FirstOffers = 0, FirstWithout = 0;
		TMap<FString, int32> FirstWeapons;
		for (int32 Run = 0; Run < 48; ++Run)
		{
			GameMode->RestartRunWithSeed(5400 + Run);
			GameMode->SetRoundTimerPaused(true);
			GameMode->AwardXP(PS, 100.f, false);
			for (int32 Reroll = 0; Reroll <= 3; ++Reroll)
			{
				if (Reroll > 0 && !GameMode->RerollUpgradeOffer(PS))
				{
					break;
				}
				++FirstOffers;
				FirstWithout += OfferHasWeapon(Upgrades->GetCurrentOffer()) ? 0 : 1;
				for (const FGSUpgradeChoice& Choice : Upgrades->GetCurrentOffer())
				{
					if (Choice.Upgrade && Choice.Upgrade->Category == EGSUpgradeCategory::Weapon)
					{
						++FirstWeapons.FindOrAdd(Choice.Upgrade->DisplayName.ToString());
					}
				}
			}
			GameMode->ChooseUpgrade(PS, FirstNonWeaponCard(Upgrades->GetCurrentOffer()));
		}
		TArray<FString> Counts;
		for (const TPair<FString, int32>& Pair : FirstWeapons)
		{
			Counts.Add(FString::Printf(TEXT("%s %d"), *Pair.Key, Pair.Value));
		}
		AddInfo(FString::Printf(TEXT("First offers: %d (48 runs, 3 rerolls each); weapon cards: %s"), FirstOffers, *FString::Join(Counts, TEXT(", "))));
		TestEqual(TEXT("192 first offers"), FirstOffers, 192);
		TestEqual(TEXT("Every first offer (and its rerolls) has a weapon card"), FirstWithout, 0);
		TestEqual(TEXT("Every weapon came up as a first-offer weapon"), FirstWeapons.Num(), 7);
		GameMode->RestartRunWithSeed(5499);
	});
	EndPIE();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
