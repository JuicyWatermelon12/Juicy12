// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "BanditBallerPlayerController.h"
#include "GSPlayerController.generated.h"

class UUserWidget;

/**
 * Game player controller. Inherits input-mapping setup from the template controller
 * and adds the HUD widget (crosshair) for the local player.
 */
UCLASS()
class BANDITBALLER_API AGSPlayerController : public ABanditBallerPlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	/** HUD widget added to the local player's screen (WBP_Crosshair for now). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;
};
