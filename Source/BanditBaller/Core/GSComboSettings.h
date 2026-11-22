// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSComboSettings.generated.h"

/** From MinCombo kills on, a combo multiplies the cash enemies drop by Multiplier. */
USTRUCT(BlueprintType)
struct FGSComboStep
{
	GENERATED_BODY()

	FGSComboStep() = default;
	FGSComboStep(int32 InMinCombo, float InMultiplier) : MinCombo(InMinCombo), Multiplier(InMultiplier) {}

	/** Combo count (kills in the combo, this kill included) where this step starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "1"))
	int32 MinCombo = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0"))
	float Multiplier = 1.f;
};

/**
 * Kill combo tuning (DA_ComboSettings, set as ComboSettings on BP_GSGameMode), read by UGSComboComponent and
 * AGSGameMode::HandleEnemyKilled.
 * - Each enemy kill within ComboWindow of the previous one adds 1 to the combo and restarts the window.
 * - The combo ends when the window runs out, or when the player loses health (bEndComboOnDamage).
 * - Cash dropped by a kill is CashValue x the multiplier of the step the combo is on after that kill (rounded).
 *   XP orbs use it too only with bApplyComboToXP.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSComboSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSComboSettings()
	{
		CashMultiplierSteps = {
			FGSComboStep(1, 1.f),
			FGSComboStep(3, 1.25f),
			FGSComboStep(6, 1.5f),
			FGSComboStep(10, 2.f),
		};
	}

	/** Seconds after a kill to get the next one; every kill restarts it. Game time (stops while the game is paused). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.1", Units = "s"))
	float ComboWindow = 2.5f;

	/** Cash multiplier by combo count. The step with the highest MinCombo <= the combo applies; x1 below every step. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TArray<FGSComboStep> CashMultiplierSteps;

	/** Losing health (damage that gets through: not blocked by dash i-frames or god mode) ends the combo. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	bool bEndComboOnDamage = true;

	/** Also multiply the XP orb a kill drops. Off by default: the combo pays cash only. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	bool bApplyComboToXP = false;

	/** Index into CashMultiplierSteps for a combo of Combo kills; INDEX_NONE below every step. */
	int32 GetStepIndexForCombo(int32 Combo) const
	{
		int32 Best = INDEX_NONE;
		for (int32 Index = 0; Index < CashMultiplierSteps.Num(); ++Index)
		{
			const FGSComboStep& Step = CashMultiplierSteps[Index];
			if (Step.MinCombo <= Combo && (Best == INDEX_NONE || Step.MinCombo > CashMultiplierSteps[Best].MinCombo))
			{
				Best = Index;
			}
		}
		return Best;
	}

	/** Cash multiplier for a combo of Combo kills (x1 for 0 or below every step). */
	float GetMultiplierForCombo(int32 Combo) const
	{
		const int32 Index = Combo > 0 ? GetStepIndexForCombo(Combo) : INDEX_NONE;
		return Index != INDEX_NONE ? CashMultiplierSteps[Index].Multiplier : 1.f;
	}
};
