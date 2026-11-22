// BanditBaller

#include "GSComboComponent.h"
#include "BanditBaller.h"
#include "Engine/World.h"
#include "GSComboSettings.h"
#include "GSGameMode.h"
#include "TimerManager.h"

UGSComboComponent::UGSComboComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGSComboComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WindowTimer);
	}
	Super::EndPlay(EndPlayReason);
}

const UGSComboSettings* UGSComboComponent::GetSettings() const
{
	const UWorld* World = GetWorld();
	const AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	return GameMode ? GameMode->GetComboSettings() : nullptr;
}

int32 UGSComboComponent::RegisterKill()
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !Owner->HasAuthority() || !World)
	{
		return 0;
	}
	const UGSComboSettings* Settings = GetSettings();
	if (!Settings)
	{
		if (!bWarnedNoSettings)
		{
			bWarnedNoSettings = true;
			UE_LOG(LogBanditBaller, Warning, TEXT("[combo] No ComboSettings on the GameMode; kills don't build a combo."));
		}
		return 0;
	}

	++ComboCount;
	CashMultiplier = Settings->GetMultiplierForCombo(ComboCount);
	ActiveWindow = FMath::Max(Settings->ComboWindow, 0.1f);
	// A world timer: it doesn't advance while the game is paused, so the window freezes on the upgrade screen.
	World->GetTimerManager().SetTimer(WindowTimer, this, &UGSComboComponent::HandleWindowExpired, ActiveWindow, false);
	OnComboChanged.Broadcast(ComboCount, CashMultiplier);
	return ComboCount;
}

void UGSComboComponent::HandleDamageTaken(float Amount)
{
	const UGSComboSettings* Settings = GetSettings();
	if (Amount > 0.f && IsComboActive() && Settings && Settings->bEndComboOnDamage)
	{
		EndCombo(EGSComboEndReason::DamageTaken);
	}
}

void UGSComboComponent::HandleWindowExpired()
{
	EndCombo(EGSComboEndReason::TimedOut);
}

void UGSComboComponent::EndCombo(EGSComboEndReason Reason)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || ComboCount <= 0)
	{
		return;
	}
	const int32 FinalCount = ComboCount;
	ComboCount = 0;
	CashMultiplier = 1.f;
	ActiveWindow = 0.f;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WindowTimer);
	}

	UE_LOG(LogBanditBaller, Log, TEXT("[combo] %s: combo of %d ended (%s)."), *GetNameSafe(Owner), FinalCount, *GetEndReasonName(Reason));
	OnComboEnded.Broadcast(FinalCount, Reason);
	OnComboEndedNative.Broadcast(FinalCount, Reason);
	OnComboChanged.Broadcast(0, 1.f);
}

float UGSComboComponent::GetTimeRemaining() const
{
	const UWorld* World = GetWorld();
	return (World && ComboCount > 0) ? FMath::Max(World->GetTimerManager().GetTimerRemaining(WindowTimer), 0.f) : 0.f;
}

float UGSComboComponent::GetTimeRemainingFraction() const
{
	return ActiveWindow > 0.f ? FMath::Clamp(GetTimeRemaining() / ActiveWindow, 0.f, 1.f) : 0.f;
}

FString UGSComboComponent::GetEndReasonName(EGSComboEndReason Reason)
{
	return StaticEnum<EGSComboEndReason>()->GetNameStringByValue(static_cast<int64>(Reason));
}
