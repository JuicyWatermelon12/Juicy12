// BanditBaller

#include "GSPlayerController.h"
#include "Blueprint/UserWidget.h"

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToPlayerScreen(0);
		}
	}
}
