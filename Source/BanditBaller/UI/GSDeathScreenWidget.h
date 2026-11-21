// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSDeathScreenWidget.generated.h"

/**
 * "You Died" screen (WBP_DeathScreen derives from this). Takes keyboard focus and asks the
 * server to restart when the restart key is pressed (R, or Start/Y on gamepad).
 */
UCLASS(Abstract)
class BANDITBALLER_API UGSDeathScreenWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditDefaultsOnly, Category = "Death Screen")
	TArray<FKey> RestartKeys = { EKeys::R, EKeys::Gamepad_Special_Right, EKeys::Gamepad_FaceButton_Top };

private:
	bool bRestartRequested = false;
};
