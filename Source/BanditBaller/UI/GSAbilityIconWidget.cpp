// BanditBaller

#include "GSAbilityIconWidget.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GSGameplayAbility.h"
#include "GSPlayerState.h"

void UGSAbilityIconWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (LabelText)
	{
		LabelText->SetText(Label);
	}
}

FString UGSAbilityIconWidget::GetStatusString() const
{
	return StatusText ? StatusText->GetText().ToString() : FString();
}

void UGSAbilityIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AGSPlayerState* PS = GetOwningPlayerState<AGSPlayerState>();
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !AbilityTag.IsValid())
	{
		return;
	}

	const UGSGameplayAbility* Ability = nullptr;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTagExact(AbilityTag))
		{
			Ability = Cast<UGSGameplayAbility>(Spec.Ability);
			break;
		}
	}

	if (!Ability)
	{
		return;
	}

	// Cooldowns are time-based, so this is read per frame (cheap: one effect query).
	float Remaining = 0.f;
	float Duration = 0.f;
	int32 Charges = 1;
	int32 MaxCharges = 1;
	Ability->GetCooldownDisplayInfo(ASC->AbilityActorInfo.Get(), Remaining, Duration, Charges, MaxCharges);

	if (CooldownBar)
	{
		CooldownBar->SetPercent(Remaining > 0.f && Duration > 0.f ? 1.f - Remaining / Duration : 1.f);
	}

	if (StatusText)
	{
		FString Status;
		if (MaxCharges > 1)
		{
			Status = FString::Printf(TEXT("%d/%d"), Charges, MaxCharges);
			if (Remaining > 0.f)
			{
				Status += FString::Printf(TEXT("  %.1fs"), Remaining);
			}
		}
		else
		{
			Status = Remaining > 0.f ? FString::Printf(TEXT("%.1fs"), Remaining) : TEXT("READY");
		}
		StatusText->SetText(FText::FromString(Status));
	}

	SetRenderOpacity(Charges > 0 ? 1.f : UnavailableOpacity);
}
