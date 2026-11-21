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
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "GSAbilitySystemComponent.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSDebugLibrary.h"
#include "GSEnemyCharacter.h"
#include "GSEnemyData.h"
#include "GSGameplayTags.h"
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
		TArray<AGSEnemyCharacter*> Spawned = UGSDebugLibrary::SpawnEnemiesAround(GetPC(), Data, 1, 1500.f);
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

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
