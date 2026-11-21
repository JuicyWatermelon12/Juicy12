// BanditBaller

#include "GSDeathScreenWidget.h"
#include "GSPlayerController.h"

void UGSDeathScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
}

FReply UGSDeathScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bRestartRequested && RestartKeys.Contains(InKeyEvent.GetKey()))
	{
		if (AGSPlayerController* PC = Cast<AGSPlayerController>(GetOwningPlayer()))
		{
			bRestartRequested = true;
			PC->ServerRequestRestart();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
