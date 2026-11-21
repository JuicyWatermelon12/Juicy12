// BanditBaller

#include "GSProgressionComponent.h"
#include "BanditBaller.h"
#include "GSGameMode.h"
#include "GSXPCurve.h"
#include "GSLevelRewards.h"
#include "GSAttributeSet.h"
#include "GSGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UGSProgressionComponent::UGSProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGSProgressionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGSProgressionComponent, State);
}

void UGSProgressionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		const UGSXPCurve* Curve = GetXPCurve();
		if (!Curve)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("[xp] %s: no XP curve on the GameMode (set XPCurve on BP_GSGameMode); levels won't advance."), *GetNameSafe(GetOwner()));
		}
		const FGSProgressionState OldState = State;
		RefreshFromCurve(Curve);
		BroadcastChanges(OldState, 0);
	}
}

const UGSXPCurve* UGSProgressionComponent::GetXPCurve() const
{
	const UWorld* World = GetWorld();
	const AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	return GameMode ? GameMode->GetXPCurve() : nullptr;
}

void UGSProgressionComponent::RefreshFromCurve(const UGSXPCurve* Curve)
{
	State.MaxLevel = Curve ? Curve->GetMaxLevel() : FMath::Max(1, State.Level);
	State.Level = FMath::Clamp(State.Level, 1, State.MaxLevel);
	if (State.Level >= State.MaxLevel || !Curve)
	{
		State.XPForNextLevel = 0;
		if (Curve)
		{
			State.CurrentXP = 0;
		}
	}
	else
	{
		State.XPForNextLevel = Curve->GetXPToReachLevel(State.Level + 1);
	}
}

int32 UGSProgressionComponent::AddXP(int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0)
	{
		return 0;
	}

	const UGSXPCurve* Curve = GetXPCurve();
	const FGSProgressionState OldState = State;
	State.TotalXP = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(State.TotalXP) + Amount, MAX_int32));

	RefreshFromCurve(Curve);
	int64 XP = static_cast<int64>(State.CurrentXP) + Amount;
	if (Curve)
	{
		// Several level-ups in a row; the remainder carries over.
		while (State.Level < State.MaxLevel)
		{
			const int32 Needed = Curve->GetXPToReachLevel(State.Level + 1);
			if (Needed <= 0 || XP < Needed)
			{
				break;
			}
			XP -= Needed;
			++State.Level;
		}
	}
	State.CurrentXP = static_cast<int32>(FMath::Min<int64>(XP, MAX_int32));
	RefreshFromCurve(Curve);

	const int32 LevelsGained = State.Level - OldState.Level;
	UE_LOG(LogBanditBaller, Log, TEXT("[xp] %s +%d XP -> level %d, %d / %d XP (total %d)%s"),
		*GetNameSafe(GetOwner()), Amount, State.Level, State.CurrentXP, State.XPForNextLevel, State.TotalXP,
		LevelsGained > 0 ? *FString::Printf(TEXT(", LEVEL UP x%d"), LevelsGained) : TEXT(""));

	// Rewards before the events, so level-up listeners see the new stats. Works the same in every phase / area.
	if (LevelsGained > 0)
	{
		ApplyLevelUpRewards(OldState.Level + 1, State.Level);
	}

	BroadcastChanges(OldState, Amount);
	return LevelsGained;
}

void UGSProgressionComponent::ResetProgression()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	// Lasting level-up rewards go with the levels (Health is clamped to the lower MaxHealth by the attribute set).
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		for (const FActiveGameplayEffectHandle& Handle : RewardEffectHandles)
		{
			ASC->RemoveActiveGameplayEffect(Handle, /*StacksToRemove*/ -1);
		}
	}
	RewardEffectHandles.Reset();

	const FGSProgressionState OldState = State;
	State = FGSProgressionState();
	RefreshFromCurve(GetXPCurve());
	BroadcastChanges(OldState, 0);
}

void UGSProgressionComponent::ApplyLevelUpRewards(int32 FirstLevel, int32 LastLevel)
{
	const UWorld* World = GetWorld();
	const AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	const UGSLevelRewards* Rewards = GameMode ? GameMode->GetLevelRewards() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!Rewards || !ASC)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[xp] %s: no LevelRewards on the GameMode (or no ASC); level-up grants nothing."), *GetNameSafe(GetOwner()));
		return;
	}

	TArray<TSubclassOf<UGameplayEffect>> Effects;
	for (int32 Level = FirstLevel; Level <= LastLevel; ++Level)
	{
		Effects.Reset();
		Rewards->GetEffectsForLevel(Level, Effects);
		for (const TSubclassOf<UGameplayEffect>& Effect : Effects)
		{
			const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, static_cast<float>(Level), ASC->MakeEffectContext());
			if (!Spec.IsValid())
			{
				continue;
			}
			const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
			if (Handle.IsValid()) // lasting effects only (instant ones have no active handle)
			{
				RewardEffectHandles.AddUnique(Handle); // stacking effects return the same handle
			}
		}
	}

	// Full heal to the new MaxHealth through a GE (not for a dead player).
	if (Rewards->FullHealEffect && !ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead))
	{
		const FGameplayEffectSpecHandle Heal = ASC->MakeOutgoingSpec(Rewards->FullHealEffect, 1.f, ASC->MakeEffectContext());
		if (Heal.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Heal.Data);
		}
	}

	UE_LOG(LogBanditBaller, Log, TEXT("[xp] %s level-up rewards for levels %d-%d: MaxHealth %.0f, Health %.0f"),
		*GetNameSafe(GetOwner()), FirstLevel, LastLevel,
		ASC->GetNumericAttribute(UGSAttributeSet::GetMaxHealthAttribute()), ASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute()));
}

float UGSProgressionComponent::GetLevelProgress() const
{
	if (IsMaxLevel() || State.XPForNextLevel <= 0)
	{
		return 1.f;
	}
	return FMath::Clamp(static_cast<float>(State.CurrentXP) / static_cast<float>(State.XPForNextLevel), 0.f, 1.f);
}

void UGSProgressionComponent::OnRep_State(const FGSProgressionState& OldState)
{
	BroadcastChanges(OldState, FMath::Max(0, State.TotalXP - OldState.TotalXP));
}

void UGSProgressionComponent::BroadcastChanges(const FGSProgressionState& OldState, int32 Delta)
{
	for (int32 NewLevel = OldState.Level + 1; NewLevel <= State.Level; ++NewLevel)
	{
		OnLevelUp.Broadcast(NewLevel);
	}
	if (OldState.CurrentXP != State.CurrentXP || OldState.XPForNextLevel != State.XPForNextLevel
		|| OldState.Level != State.Level || OldState.TotalXP != State.TotalXP)
	{
		OnXPChanged.Broadcast(State.CurrentXP, State.XPForNextLevel, Delta);
	}
}
