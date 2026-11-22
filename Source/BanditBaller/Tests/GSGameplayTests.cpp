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

		// A huge gain: every level up to the cap at once, one offer per level gained (one open, the rest queued).
		PickAllUpgradeOffers(); // SetLevel 10 queued 9 offers
		const UGSUpgradeComponent* Upgrades = PS->GetUpgradeComponent();
		TestFalse(TEXT("No offer before the huge gain"), Upgrades->HasOpenOffer());
		GameMode->AwardXP(PS, 100000000.f, false);
		const int32 LevelsGained = Curve->GetMaxLevel() - 10;
		TestTrue(TEXT("Huge gain: an offer open"), Upgrades->HasOpenOffer());
		TestEqual(TEXT("Huge gain: one offer per level gained"), Upgrades->GetQueuedOffers() + 1, LevelsGained);
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
			TestTrue(FString::Printf(TEXT("%s is Common"), E.Name), Upgrade->Rarity == EGSUpgradeRarity::Common);
			TestEqual(FString::Printf(TEXT("%s has no cap"), E.Name), Upgrade->MaxStacks, 0);
			TestFalse(FString::Printf(TEXT("%s has a display name"), E.Name), Upgrade->DisplayName.IsEmpty());
			TestTrue(FString::Printf(TEXT("%s effect is GE_Upgrade_%s"), E.Name, E.Name), Upgrade->Effect && Upgrade->Effect->IsChildOf(E.EffectClass));
			const FString Description = Upgrade->GetFormattedDescription().ToString();
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
		struct FExpect { const TCHAR* Name; EGSUpgradeRarity Rarity; const TCHAR* Text; UClass* EffectClass; };
		for (const FExpect& E : { FExpect{ TEXT("BiggerMags"), EGSUpgradeRarity::Common, TEXT("+15%"), UGSUpgradeEffect_BiggerMags::StaticClass() },
			FExpect{ TEXT("Leech"), EGSUpgradeRarity::Rare, TEXT("+1"), UGSUpgradeEffect_Leech::StaticClass() } })
		{
			const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(E.Name);
			if (!TestNotNull(FString::Printf(TEXT("DA_Upgrade_%s exists"), E.Name), Upgrade))
			{
				continue;
			}
			TestTrue(FString::Printf(TEXT("%s rarity"), E.Name), Upgrade->Rarity == E.Rarity);
			TestEqual(FString::Printf(TEXT("%s has no cap"), E.Name), Upgrade->MaxStacks, 0);
			TestFalse(FString::Printf(TEXT("%s has a display name"), E.Name), Upgrade->DisplayName.IsEmpty());
			TestTrue(FString::Printf(TEXT("%s effect is GE_Upgrade_%s"), E.Name, E.Name), Upgrade->Effect && Upgrade->Effect->IsChildOf(E.EffectClass));
			const FString Description = Upgrade->GetFormattedDescription().ToString();
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
			TestTrue(TEXT("Loaded Dice is Rare"), Upgrade->Rarity == EGSUpgradeRarity::Rare);
			TestEqual(TEXT("Loaded Dice: MaxStacks 20"), Upgrade->MaxStacks, 20);
			TestTrue(TEXT("Loaded Dice effect is GE_Upgrade_LoadedDice"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_LoadedDice::StaticClass()));
			const FString Description = Upgrade->GetFormattedDescription().ToString();
			TestTrue(FString::Printf(TEXT("Description '%s' shows +5%%"), *Description), Description.Contains(TEXT("+5%")));
			const UGSUpgradePool* Pool = GetGameMode()->GetUpgradePool();
			TestTrue(TEXT("Loaded Dice is in DA_UpgradePool"), Pool && Pool->Upgrades.Contains(Upgrade));
		}
		const UGSCombatSettings* Combat = GetGameMode()->GetCombatSettings();
		if (TestNotNull(TEXT("CombatSettings (DA_CombatSettings) set on the GameMode"), Combat))
		{
			TestEqual(TEXT("CritMultiplier 2"), Combat->CritMultiplier, 2.f);
		}

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

		// 20 stacks: 100%, always a crit without dice; MaxStacks stops a 21st.
		UGSDebugLibrary::ExecuteDebugCommand(GetPC(), TEXT("UpgradeGrant"), { TEXT("LoadedDice"), TEXT("10") });
		TestEqual(TEXT("20 stacks: CritChance 1"), GetAttr(UGSAttributeSet::GetCritChanceAttribute()), 1.f, 0.0001f);
		TestEqual(TEXT("A 21st stack is refused (MaxStacks 20)"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("LoadedDice")), 0);
		const int32 SeedAtFull = Stream.GetCurrentSeed();
		bool bAllCrit = true;
		for (int32 i = 0; i < 100; ++i)
		{
			bAllCrit &= GSCombat::RollCriticalHit(ASC).bCritical;
		}
		TestTrue(TEXT("CritChance 1: every hit is a crit"), bAllCrit);
		TestEqual(TEXT("CritChance 1: the run stream is untouched"), Stream.GetCurrentSeed(), SeedAtFull);

		// The attribute is clamped to 0..1.
		UGSDebugLibrary::SetAttributeOn(GetPC(), TEXT("CritChance"), 1.5f);
		TestTrue(TEXT("CritChance clamped to at most 1"), GetAttr(UGSAttributeSet::GetCritChanceAttribute()) <= 1.f + 0.0001f);
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
			TestEqual(TEXT("One more critical number"), HUD->GetCriticalDamageNumbersAdded(), S->CritNumbersBefore + 1);
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
			TestTrue(TEXT("Piercing Rounds is Epic"), Upgrade->Rarity == EGSUpgradeRarity::Epic);
			TestEqual(TEXT("Piercing Rounds has no cap"), Upgrade->MaxStacks, 0);
			TestTrue(TEXT("Effect is GE_Upgrade_PiercingRounds"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_PiercingRounds::StaticClass()));
			const FString Description = Upgrade->GetFormattedDescription().ToString();
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
		TestEqual(TEXT("Chance clamped to 100%"), Combat->GetExplodeChance(50), 1.f);
		TestNotNull(TEXT("Blast damage effect set"), Combat->BlastDamageEffect.Get());
		TestNotNull(TEXT("Placeholder blast effect set"), Combat->BlastEffect.Get());
		TestNotNull(TEXT("Placeholder blast sound set"), Combat->BlastSound.Get());

		const UGSUpgradeData* Upgrade = UGSDebugLibrary::FindUpgradeData(TEXT("PowderKeg"));
		if (TestNotNull(TEXT("DA_Upgrade_PowderKeg exists"), Upgrade))
		{
			TestTrue(TEXT("Powder Keg is Epic"), Upgrade->Rarity == EGSUpgradeRarity::Epic);
			TestEqual(TEXT("MaxStacks 34 (the chance reaches 100%)"), Upgrade->MaxStacks, 34);
			TestTrue(TEXT("MaxStacks is the first stack count at 100%"),
				Combat->GetExplodeChance(Upgrade->MaxStacks) >= 1.f && Combat->GetExplodeChance(Upgrade->MaxStacks - 1) < 1.f);
			const FString Card = Upgrade->GetFormattedDescription().ToString();
			const FString PerStack = FString::Printf(TEXT("+%d%% chance per stack"), FMath::RoundToInt(Combat->ChancePerStack * 100.f));
			TestTrue(FString::Printf(TEXT("Card text matches ChancePerStack (%s): %s"), *PerStack, *Card), Card.Contains(PerStack));
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
		TestEqual(TEXT("A 35th stack is refused (MaxStacks 34)"), UGSDebugLibrary::GrantUpgradeTo(GetPC(), TEXT("PowderKeg")), 0);
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
			TestTrue(TEXT("Dash Charge is Rare"), Upgrade->Rarity == EGSUpgradeRarity::Rare);
			TestEqual(TEXT("Dash Charge has no cap"), Upgrade->MaxStacks, 0);
			TestTrue(TEXT("Dash Charge effect is GE_Upgrade_DashCharge"), Upgrade->Effect && Upgrade->Effect->IsChildOf(UGSUpgradeEffect_DashCharge::StaticClass()));
			const FString Description = Upgrade->GetFormattedDescription().ToString();
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
		TestEqual(TEXT("Pool has the 12 upgrades"), Pool->Upgrades.Num(), 12);
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

		// Real pool: always 3 different upgrades (the empty Legendary tier never wastes a pick). Weights Common 100, Rare 20,
		// Epic 6 (two Epics: Piercing Rounds, Powder Keg; the tier leaves the weighting once both are in the offer). An
		// offer has no Rare with probability 0.5954 (every C/E ordering), so the Rare tier is in about 40.5% of offers.
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
		AddInfo(FString::Printf(TEXT("Rare in %.1f%% of 3000 offers (expected 40.5%%)"), 100.0 * WithRare / 3000.0));
		TestTrue(TEXT("Rare frequency matches the tier weights"), FMath::Abs(WithRare / 3000.0 - 0.4046) < 0.03);

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
		TestTrue(TEXT("Epic choices now appear (Piercing Rounds)"), Sim1.RarityChoices.FindRef(TEXT("Epic")) > 0);
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
		S->Picked = GetPlayerUpgrades()->GetCurrentChoices()[0];
		TestEqual(TEXT("Upgrade picked"), PickAllUpgradeOffers(), 1);
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
			if (Upgrade && Upgrade != S->Picked)
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

		// Upgrades: one icon per upgrade in pick order with its stacks, and the same as text.
		const UGSUpgradeComponent* Upgrades = GetPlayerUpgrades();
		const TArray<FGSUpgradeStack>& Stacks = Upgrades->GetUpgradeStacks();
		TArray<FString> Expected;
		for (const FGSUpgradeStack& Stack : Stacks)
		{
			Expected.Add(FString::Printf(TEXT("%s x%d"), *Stack.Upgrade->DisplayName.ToString(), Stack.Stacks));
		}
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
		GameMode->AwardXP(PS, 225.f, false);
		TestEqual(TEXT("Level 3"), PS->GetProgressionComponent()->GetLevel(), 3);
		TestTrue(TEXT("A reroll used"), GameMode->RerollUpgradeOffer(PS));
		TestEqual(TEXT("Both offers picked"), PickAllUpgradeOffers(), 2);
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
#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
