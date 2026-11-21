// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GSAbilityIconWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * HUD icon for one ability (WBP_AbilityIcon). Finds the granted ability by AbilityTag on the
 * owning player's ASC and shows its cooldown progress, remaining seconds and charges.
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSAbilityIconWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Asset tag of the ability to display (e.g. Ability.Dash). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText Label;

	/** Opacity of the icon while no charge is available. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0", ClampMax = "1"))
	float UnavailableOpacity = 0.45f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

	/** Fills up as the cooldown / current charge recharges. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> CooldownBar;

	/** "READY", "0.6s", or "1/2  2.1s" for multi-charge abilities. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;
};
