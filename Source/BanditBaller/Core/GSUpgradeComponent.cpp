// BanditBaller

#include "GSUpgradeComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BanditBaller.h"
#include "GameplayEffect.h"
#include "GSUpgradeData.h"

UGSUpgradeComponent::UGSUpgradeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UGSUpgradeComponent::GrantUpgrade(const UGSUpgradeData* Upgrade, const TCHAR* Source)
{
	AActor* Owner = GetOwner();
	if (!Upgrade || !Owner || !Owner->HasAuthority())
	{
		return false;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
	if (!ASC || !Upgrade->Effect)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[upgrade] %s: can't grant %s (no ASC or no Effect on the upgrade)."), *GetNameSafe(Owner), *GetNameSafe(Upgrade));
		return false;
	}

	FGSUpgradeStack* Entry = Stacks.FindByPredicate([Upgrade](const FGSUpgradeStack& S) { return S.Upgrade == Upgrade; });
	const int32 Current = Entry ? Entry->Stacks : 0;
	if (!Upgrade->CanAddStack(Current))
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: %s is at its cap (%d/%d); not granted."), *GetNameSafe(Owner), *Upgrade->GetName(), Current, Upgrade->MaxStacks);
		return false;
	}

	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Upgrade->Effect, 1.f, ASC->MakeEffectContext());
	const FActiveGameplayEffectHandle Handle = Spec.IsValid() ? ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data) : FActiveGameplayEffectHandle();
	if (!Handle.IsValid())
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[upgrade] %s: applying %s failed."), *GetNameSafe(Owner), *GetNameSafe(Upgrade->Effect));
		return false;
	}

	if (!Entry)
	{
		Entry = &Stacks.AddDefaulted_GetRef();
		Entry->Upgrade = Upgrade;
	}
	Entry->Stacks = Current + 1;
	Entry->EffectHandle = Handle; // stacking by target: every pick lands on the same active effect

	UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: %s x%d (%s) -> %s"), *GetNameSafe(Owner), *Upgrade->GetName(), Entry->Stacks,
		Source, *Upgrade->GetFormattedDescription().ToString());
	OnUpgradeStacksChanged.Broadcast(Upgrade, Entry->Stacks);
	return true;
}

int32 UGSUpgradeComponent::GetStacks(const UGSUpgradeData* Upgrade) const
{
	const FGSUpgradeStack* Entry = Stacks.FindByPredicate([Upgrade](const FGSUpgradeStack& S) { return S.Upgrade == Upgrade; });
	return Entry ? Entry->Stacks : 0;
}

int32 UGSUpgradeComponent::GetTotalPicks() const
{
	int32 Total = 0;
	for (const FGSUpgradeStack& Entry : Stacks)
	{
		Total += Entry.Stacks;
	}
	return Total;
}

void UGSUpgradeComponent::ResetUpgrades()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner))
	{
		for (const FGSUpgradeStack& Entry : Stacks)
		{
			ASC->RemoveActiveGameplayEffect(Entry.EffectHandle, /*StacksToRemove*/ -1);
		}
	}
	const TArray<FGSUpgradeStack> Removed = MoveTemp(Stacks);
	Stacks.Reset();
	if (Removed.Num() > 0)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: reset, removed %d upgrade(s)."), *GetNameSafe(Owner), Removed.Num());
	}
	for (const FGSUpgradeStack& Entry : Removed)
	{
		OnUpgradeStacksChanged.Broadcast(Entry.Upgrade, 0);
	}
}
