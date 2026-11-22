// BanditBaller

#include "GSSecondWindComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BanditBaller.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSCombatSettings.h"
#include "GSGameMode.h"
#include "GSGameplayEffects.h"
#include "GSGameplayTags.h"
#include "GSPlayerState.h"
#include "GSUpgradeComponent.h"

UGSSecondWindComponent::UGSSecondWindComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAbilitySystemComponent* UGSSecondWindComponent::GetASC() const
{
	const IAbilitySystemInterface* Owner = Cast<IAbilitySystemInterface>(GetOwner());
	return Owner ? Owner->GetAbilitySystemComponent() : nullptr;
}

const UGSCombatSettings* UGSSecondWindComponent::GetSettings() const
{
	const AGSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
	return (GameMode && GameMode->GetCombatSettings()) ? GameMode->GetCombatSettings() : GetDefault<UGSCombatSettings>();
}

int32 UGSSecondWindComponent::GetSecondWindsLeft() const
{
	const UAbilitySystemComponent* ASC = GetASC();
	return ASC ? FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(UGSAttributeSet::GetSecondWindsAttribute()))) : 0;
}

bool UGSSecondWindComponent::CanSave() const
{
	return GetOwner() && GetOwner()->HasAuthority() && GetSecondWindsLeft() > 0;
}

float UGSSecondWindComponent::GetRestoredHealth(float MaxHealth) const
{
	// Whole health points: 100 x 0.3f is a hair above 30 in float, which the HUD would round up to 31.
	return FMath::Clamp(FMath::RoundToFloat(MaxHealth * GetSettings()->SecondWindHealthFraction), 1.f, MaxHealth);
}

void UGSSecondWindComponent::UseSecondWind(float HealthBefore, const AActor* Killer)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !GetOwner()->HasAuthority())
	{
		return;
	}
	const UGSCombatSettings* Settings = GetSettings();
	++UsesThisRun;

	// Brief invulnerability: GE_Damage isn't applied at all meanwhile (no shield used either).
	if (Settings->SecondWindInvulnerableTime > 0.f)
	{
		const TSubclassOf<UGameplayEffect> Effect = Settings->SecondWindInvulnerableEffect
			? Settings->SecondWindInvulnerableEffect : TSubclassOf<UGameplayEffect>(UGSTimedInvulnerabilityEffect::StaticClass());
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, 1.f, ASC->MakeEffectContext());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Duration, Settings->SecondWindInvulnerableTime);
			InvulnerabilityHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}
	}

	// Used up: one stack of the upgrade comes off (the attribute drops with it).
	const AGSPlayerState* PS = Cast<AGSPlayerState>(GetOwner());
	UGSUpgradeComponent* Upgrades = PS ? PS->GetUpgradeComponent() : nullptr;
	if (!Upgrades || !Upgrades->RemoveStackByEffect(UGSUpgradeEffect_SecondWind::StaticClass(), TEXT("Second Wind")))
	{
		// Not from the upgrade (e.g. gs.SetAttr SecondWinds): take it off the base value instead.
		const float Base = ASC->GetNumericAttributeBase(UGSAttributeSet::GetSecondWindsAttribute());
		ASC->SetNumericAttributeBase(UGSAttributeSet::GetSecondWindsAttribute(), FMath::Max(0.f, Base - 1.f));
	}

	const int32 Left = GetSecondWindsLeft();
	UE_LOG(LogBanditBaller, Log, TEXT("[secondwind] %s: Second Wind! A killing blow from %s at %.0f Health -> Health %.0f, invulnerable %.1f s, %d left"),
		*GetNameSafe(GetOwner()), *GetNameSafe(Killer), HealthBefore, ASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute()),
		Settings->SecondWindInvulnerableTime, Left);
	OnSecondWindUsed.Broadcast(Left);
}

void UGSSecondWindComponent::ResetSecondWind()
{
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		if (InvulnerabilityHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(InvulnerabilityHandle);
		}
	}
	InvulnerabilityHandle.Invalidate();
	UsesThisRun = 0;
}
