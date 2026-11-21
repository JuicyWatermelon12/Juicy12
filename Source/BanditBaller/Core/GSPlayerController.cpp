// BanditBaller

#include "GSPlayerController.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "GSAbilitySystemComponent.h"
#include "GSDebugLibrary.h"
#include "GSGameMode.h"
#include "GSGameplayTags.h"
#include "GSHUDWidget.h"
#include "GSPlayerState.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
namespace
{
	/** The world we should act on: the given one if it is a game world, else the first PIE/game world. */
	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void DamageSelfCommand(const TArray<FString>& Args, UWorld* World)
	{
		const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.f;
		UWorld* GameWorld = FindGameWorld(World);
		AGSPlayerController* PC = GameWorld ? Cast<AGSPlayerController>(GameWorld->GetFirstPlayerController()) : nullptr;
		if (!PC)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.Damage.Self: no local AGSPlayerController (is the game running?)"));
			return;
		}
		PC->ServerDebugDamageSelf(Amount);
	}

	FAutoConsoleCommandWithWorldAndArgs GSDamageSelfCommand(
		TEXT("gs.Damage.Self"),
		TEXT("Apply <amount> damage to the local player through GAS (GE_Damage). Usage: gs.Damage.Self 25"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DamageSelfCommand),
		ECVF_Cheat);
}
#endif

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// The death screen switches to UI-only input, which makes the game viewport ignore input. The viewport
	// survives the restart travel, so without this the respawned player can't move.
	if (IsLocalPlayerController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}

	if (IsLocalPlayerController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToPlayerScreen(0);
		}
	}
}

UGSHUDWidget* AGSPlayerController::GetGSHUD() const
{
	return Cast<UGSHUDWidget>(HUDWidget);
}

void AGSPlayerController::ClientShowDeathScreen_Implementation(TSubclassOf<UUserWidget> WidgetClass)
{
	if (!WidgetClass || DeathScreenWidget)
	{
		return;
	}

	DeathScreenWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (!DeathScreenWidget)
	{
		return;
	}

	DeathScreenWidget->AddToPlayerScreen(100);

	// UI-only input so the death screen receives the restart key.
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(DeathScreenWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	SetInputMode(InputMode);
}

void AGSPlayerController::ServerRequestRestart_Implementation()
{
	if (AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		GameMode->RequestRestart(this);
	}
}

void AGSPlayerController::ClientNotifyDamageDealt_Implementation(FVector_NetQuantize WorldLocation, float Damage, bool bCritical, bool bKilled)
{
	if (UGSHUDWidget* HUD = GetGSHUD())
	{
		HUD->AddDamageNumber(WorldLocation, Damage, bCritical);
		HUD->ShowHitMarker(bKilled);
		if (bKilled)
		{
			HUD->NotifyKill(WorldLocation); // anchors the "+N XP" popup of this kill
		}
	}
}

void AGSPlayerController::ClientNotifyCashCollected_Implementation(FVector_NetQuantize WorldLocation, int32 Amount)
{
	if (UGSHUDWidget* HUD = GetGSHUD())
	{
		HUD->AddCashPopup(WorldLocation, Amount);
	}
}

void AGSPlayerController::ServerDebugDamageSelf_Implementation(float Amount)
{
#if !UE_BUILD_SHIPPING
	const AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !DebugDamageEffect || Amount <= 0.f)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.Damage.Self: missing ASC or DebugDamageEffect on %s"), *GetNameSafe(this));
		return;
	}

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DebugDamageEffect, 1.f, ASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, Amount);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		UE_LOG(LogBanditBaller, Log, TEXT("gs.Damage.Self: applied %.1f damage to %s"), Amount, *GetNameSafe(GetPawn()));
	}
#endif
}

void AGSPlayerController::ServerExecDebugCommand_Implementation(const FString& Command, const TArray<FString>& Args)
{
#if !UE_BUILD_SHIPPING
	UGSDebugLibrary::ExecuteDebugCommand(this, Command, Args);
#endif
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

void AGSPlayerController::GSPress(const FString& InputTagName)
{
#if !UE_BUILD_SHIPPING
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*InputTagName), false);
	const AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	UGSAbilitySystemComponent* ASC = PS ? PS->GetGSAbilitySystemComponent() : nullptr;
	if (!Tag.IsValid() || !ASC)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("GSPress: unknown tag '%s' or no ASC"), *InputTagName);
		return;
	}

	ASC->AbilityInputTagPressed(Tag);
	ASC->AbilityInputTagReleased(Tag);
#endif
}

void AGSPlayerController::GSJump()
{
#if !UE_BUILD_SHIPPING
	ACharacter* MyCharacter = GetCharacter();
	if (!MyCharacter)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("GSJump: no character"));
		return;
	}
	MyCharacter->Jump();
	TWeakObjectPtr<ACharacter> WeakCharacter = MyCharacter;
	GetWorldTimerManager().SetTimerForNextTick([WeakCharacter]()
	{
		if (ACharacter* C = WeakCharacter.Get())
		{
			C->StopJumping();
		}
	});
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
