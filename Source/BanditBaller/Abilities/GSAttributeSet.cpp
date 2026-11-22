// BanditBaller

#include "GSAttributeSet.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffectExtension.h"
#include "GSGameMode.h"
#include "GSGameplayTags.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSShieldComponent.h"
#include "Net/UnrealNetwork.h"

UGSAttributeSet::UGSAttributeSet()
{
	// Neutral default so enemies and players without GE_InitPlayerStats still gain x1 XP.
	InitXPGain(1.f);
	InitReloadSpeed(1.f);
	InitExtraJumps(0.f);
	InitExtraDashCharges(0.f);
	InitMagazineSizeMultiplier(1.f);
	InitHealthOnKill(0.f);
	InitCritChance(0.f);
	InitPierce(0.f);
	InitPowderKeg(0.f);
	InitShields(0.f);
}

void UGSAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAttributeSet, Damage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAttributeSet, FireRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAttributeSet, PickupRadius, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSAttributeSet, XPGain, COND_None, REPNOTIFY_Always);
}

void UGSAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetMoveSpeedAttribute()
		|| Attribute == GetDamageAttribute()
		|| Attribute == GetFireRateAttribute()
		|| Attribute == GetPickupRadiusAttribute()
		|| Attribute == GetXPGainAttribute()
		|| Attribute == GetExtraJumpsAttribute()
		|| Attribute == GetExtraDashChargesAttribute()
		|| Attribute == GetHealthOnKillAttribute()
		|| Attribute == GetPierceAttribute()
		|| Attribute == GetPowderKegAttribute()
		|| Attribute == GetShieldsAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetReloadSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, MinReloadSpeed);
	}
	else if (Attribute == GetMagazineSizeMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, MinMagazineSizeMultiplier);
	}
	else if (Attribute == GetCritChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
}

void UGSAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// MaxHealth went down (e.g. level-up MaxHealth stacks removed on a new run): Health may not stay above it.
	if (Attribute == GetMaxHealthAttribute() && NewValue < OldValue && GetHealth() > NewValue)
	{
		const AActor* Owner = GetOwningActor();
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
		if (ASC && Owner && Owner->HasAuthority())
		{
			ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Override, NewValue);
		}
	}
}

void UGSAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalDamage = GetIncomingDamage();
		SetIncomingDamage(0.f);

		// Damage order: dash invulnerability (State.Invulnerable keeps GE_Damage from applying at all, so it never gets
		// here) -> a ready Floating Shield cancels the whole hit -> Health. TODO(M2b 37): Second Wind on a killing blow.
		if (LocalDamage > 0.f)
		{
			const AGSPlayerState* OwnerPS = Cast<AGSPlayerState>(GetOwningActor());
			UGSShieldComponent* ShieldComponent = OwnerPS ? OwnerPS->GetShieldComponent() : nullptr;
			if (ShieldComponent && ShieldComponent->TryAbsorbHit(LocalDamage, Data.EffectSpec.GetEffectContext().GetOriginalInstigator()))
			{
				return; // blocked: no Health lost (no damage taken, combo and no-damage bonus unaffected)
			}
		}

		if (LocalDamage > 0.f)
		{
			// Record the attacker before Health changes so health listeners (death) can read it.
			const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetEffectContext();
			AActor* Causer = Context.GetEffectCauser();
			LastDamageInstigator = Causer ? Causer : Context.GetOriginalInstigator();

			const float OldHealth = GetHealth();
			SetHealth(FMath::Clamp(OldHealth - LocalDamage, 0.f, GetMaxHealth()));

			// Run stats: health actually removed (no overkill).
			const float Applied = OldHealth - GetHealth();
			const UWorld* World = GetWorld();
			if (AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr)
			{
				GameMode->HandleDamageApplied(Data.Target.GetAvatarActor(), LastDamageInstigator.Get(), Applied);
			}

			// A critical hit carries SetByCaller Data.Critical = 1 (GSCombat::RollCriticalHit; its damage is already doubled).
			const bool bCritical = Data.EffectSpec.GetSetByCallerMagnitude(GSGameplayTags::Data_Critical, /*WarnIfNotFound*/ false, 0.f) > 0.f;
			const bool bKilled = OldHealth > 0.f && GetHealth() <= 0.f;
			NotifyDamageDealt(Data, LocalDamage, bCritical, bKilled);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// PreAttributeChange only clamps the current value; clamp the base value too.
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
}

void UGSAttributeSet::NotifyDamageDealt(const FGameplayEffectModCallbackData& Data, float DamageAmount, bool bCritical, bool bKilled) const
{
	const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetEffectContext();
	AActor* TargetAvatar = Data.Target.GetAvatarActor();

	// Find the attacking player's controller: effect causer (pawn) first, then the instigator (pawn or PlayerState).
	AGSPlayerController* AttackerPC = nullptr;
	if (const APawn* CauserPawn = Cast<APawn>(Context.GetEffectCauser()))
	{
		AttackerPC = Cast<AGSPlayerController>(CauserPawn->GetController());
	}
	if (!AttackerPC)
	{
		AActor* Instigator = Context.GetOriginalInstigator();
		if (const APlayerState* InstigatorPS = Cast<APlayerState>(Instigator))
		{
			AttackerPC = Cast<AGSPlayerController>(InstigatorPS->GetPlayerController());
		}
		else if (const APawn* InstigatorPawn = Cast<APawn>(Instigator))
		{
			AttackerPC = Cast<AGSPlayerController>(InstigatorPawn->GetController());
		}
	}

	// No feedback for self-damage (e.g. gs.Damage.Self).
	if (!AttackerPC || !TargetAvatar || AttackerPC->GetPawn() == TargetAvatar)
	{
		return;
	}

	const FHitResult* Hit = Context.GetHitResult();
	const FVector Location = Hit ? FVector(Hit->ImpactPoint) : TargetAvatar->GetActorLocation() + FVector(0.f, 0.f, 80.f);
	AttackerPC->ClientNotifyDamageDealt(Location, DamageAmount, bCritical, bKilled);
}

void UGSAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAttributeSet, Health, OldValue);
}

void UGSAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAttributeSet, MaxHealth, OldValue);
}

void UGSAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAttributeSet, MoveSpeed, OldValue);
}

void UGSAttributeSet::OnRep_Damage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAttributeSet, Damage, OldValue);
}

void UGSAttributeSet::OnRep_FireRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAttributeSet, FireRate, OldValue);
}

void UGSAttributeSet::OnRep_PickupRadius(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAttributeSet, PickupRadius, OldValue);
}

void UGSAttributeSet::OnRep_XPGain(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGSAttributeSet, XPGain, OldValue);
}

