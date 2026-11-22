// BanditBaller

#include "GSGameplayAbility_Dash.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSGameplayEffects.h"
#include "GSGameplayTags.h"
#include "GSMovementAbilitySettings.h"

UGSGameplayAbility_Dash::UGSGameplayAbility_Dash()
{
	ActivationInputTag = GSGameplayTags::Ability_Dash;
	SetAssetTags(FGameplayTagContainer(GSGameplayTags::Ability_Dash));
	ActivationOwnedTags.AddTag(GSGameplayTags::State_Dashing);
	ActivationBlockedTags.AddTag(GSGameplayTags::State_Dead);
	CancelAbilitiesWithTag.AddTag(GSGameplayTags::Ability_Slide);
	CooldownGameplayEffectClass = UGSCooldownEffect_Dash::StaticClass();
}

float UGSGameplayAbility_Dash::GetCooldownDuration(const FGameplayAbilityActorInfo* ActorInfo) const
{
	return Settings ? Settings->RechargeTime : 0.f;
}

int32 UGSGameplayAbility_Dash::GetUsedCharges(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (!ASC || !CooldownTags || CooldownTags->IsEmpty())
	{
		return 0;
	}
	return ASC->GetAggregatedStackCount(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags));
}

int32 UGSGameplayAbility_Dash::GetMaxCharges(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const int32 BaseCharges = Settings ? FMath::Max(Settings->MaxCharges, 1) : 1;

	// Read live (no cached value), so a Dash Charge pick or a new run's reset counts from the next check on.
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAttribute ExtraAttribute = UGSAttributeSet::GetExtraDashChargesAttribute();
	const float Extra = ASC && ASC->HasAttributeSetForAttribute(ExtraAttribute) ? ASC->GetNumericAttribute(ExtraAttribute) : 0.f;
	return BaseCharges + FMath::Max(0, FMath::RoundToInt(Extra));
}

int32 UGSGameplayAbility_Dash::GetAvailableCharges(const FGameplayAbilityActorInfo* ActorInfo) const
{
	return FMath::Max(GetMaxCharges(ActorInfo) - GetUsedCharges(ActorInfo), 0);
}

const UGSGameplayAbility_Dash* UGSGameplayAbility_Dash::FindGrantedDash(const UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return nullptr;
	}
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (const UGSGameplayAbility_Dash* Dash = Cast<UGSGameplayAbility_Dash>(Spec.Ability))
		{
			return Dash;
		}
	}
	return nullptr;
}

bool UGSGameplayAbility_Dash::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (GetUsedCharges(ActorInfo) < GetMaxCharges(ActorInfo))
	{
		return true;
	}

	if (OptionalRelevantTags && GetCooldownTags())
	{
		OptionalRelevantTags->AppendTags(*GetCooldownTags());
	}
	return false;
}

bool UGSGameplayAbility_Dash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Settings || !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AGSCharacter* Character = GetGSCharacter(ActorInfo);
	if (!Character)
	{
		return false;
	}

	return Settings->bAllowInAir || !Character->GetCharacterMovement()->IsFalling();
}

void UGSGameplayAbility_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AGSCharacter* Character = GetGSCharacter(ActorInfo);
	if (!Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bDashMoveDone = false;
	bInvulnerable = false;

	// I-frames: GE_Damage (UGSDamageEffect) can't apply while the target has State.Invulnerable.
	if (Settings->InvulnerabilityDuration > 0.f)
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			ASC->AddLooseGameplayTag(GSGameplayTags::State_Invulnerable);
			bInvulnerable = true;
		}

		UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, Settings->InvulnerabilityDuration);
		Wait->OnFinish.AddDynamic(this, &UGSGameplayAbility_Dash::OnInvulnerabilityExpired);
		Wait->ReadyForActivation();
	}

	DashFinishedHandle = Character->OnDashFinished.AddUObject(this, &UGSGameplayAbility_Dash::OnDashFinished);
	Character->StartDash(Character->GetDashDirection(), *Settings);
}

void UGSGameplayAbility_Dash::OnDashFinished()
{
	bDashMoveDone = true;
	TryEnd();
}

void UGSGameplayAbility_Dash::OnInvulnerabilityExpired()
{
	RemoveInvulnerability();
	TryEnd();
}

void UGSGameplayAbility_Dash::RemoveInvulnerability()
{
	if (!bInvulnerable)
	{
		return;
	}

	bInvulnerable = false;
	if (UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		ASC->RemoveLooseGameplayTag(GSGameplayTags::State_Invulnerable);
	}
}

void UGSGameplayAbility_Dash::TryEnd()
{
	if (bDashMoveDone && !bInvulnerable)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGSGameplayAbility_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RemoveInvulnerability();

	if (AGSCharacter* Character = GetGSCharacter(ActorInfo))
	{
		Character->OnDashFinished.Remove(DashFinishedHandle);
		DashFinishedHandle.Reset();

		if (Character->IsDashing())
		{
			Character->StopDash();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGSGameplayAbility_Dash::GetCooldownDisplayInfo(const FGameplayAbilityActorInfo* ActorInfo, float& OutRemaining, float& OutDuration,
	int32& OutCharges, int32& OutMaxCharges) const
{
	// Base gives the remaining time of the charge currently recharging.
	Super::GetCooldownDisplayInfo(ActorInfo, OutRemaining, OutDuration, OutCharges, OutMaxCharges);

	OutMaxCharges = GetMaxCharges(ActorInfo);
	OutCharges = FMath::Min(GetAvailableCharges(ActorInfo), OutMaxCharges);
}
