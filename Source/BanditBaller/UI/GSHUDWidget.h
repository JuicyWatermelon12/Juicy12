// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSHUDWidget.generated.h"

class UTextBlock;
class UGSWeaponComponent;

/**
 * In-game HUD base. Shows the equipped weapon's name and ammo.
 * WBP_HUD derives from this; text blocks are bound by name (optional).
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** "12 / 30" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AmmoText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponNameText;

private:
	UGSWeaponComponent* FindWeaponComponent() const;

	// Cached so text is only rebuilt when something changes.
	TWeakObjectPtr<const UObject> LastWeaponData;
	int32 LastAmmo = -1;
	int32 LastMagazine = -1;
};
