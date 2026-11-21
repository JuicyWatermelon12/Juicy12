// BanditBaller

#include "GSPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "GSAbilitySystemComponent.h"
#include "GSGameplayTags.h"
#include "GSPlayerState.h"

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

void AGSPlayerController::GSFire()
{
#if !UE_BUILD_SHIPPING
	const AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	if (UGSAbilitySystemComponent* ASC = PS ? PS->GetGSAbilitySystemComponent() : nullptr)
	{
		ASC->AbilityInputTagPressed(GSGameplayTags::Ability_Fire);
		ASC->AbilityInputTagReleased(GSGameplayTags::Ability_Fire);
	}
#endif
}

void AGSPlayerController::GSAimAt(float X, float Y, float Z)
{
#if !UE_BUILD_SHIPPING
	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	SetControlRotation((FVector(X, Y, Z) - ViewLocation).Rotation());
#endif
}
