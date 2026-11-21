// BanditBaller

#include "GSHealthComponent.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectExtension.h"
#include "GSAttributeSet.h"
#include "GSGameMode.h"
#include "GSGameplayTags.h"
#include "Net/UnrealNetwork.h"

namespace
{
	/** Who caused this attribute change (server only): the effect causer (pawn) if set, else the instigator. */
	AActor* GetDamageInstigator(const FOnAttributeChangeData& Data)
	{
		if (!Data.GEModData)
		{
			return nullptr;
		}

		const FGameplayEffectContextHandle& Context = Data.GEModData->EffectSpec.GetEffectContext();
		if (AActor* Causer = Context.GetEffectCauser())
		{
			return Causer;
		}
		return Context.GetOriginalInstigator();
	}
}

UGSHealthComponent::UGSHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGSHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGSHealthComponent, bIsDead);
	DOREPLIFETIME(UGSHealthComponent, Killer);
}

UGSHealthComponent* UGSHealthComponent::FindHealthComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UGSHealthComponent>() : nullptr;
}

void UGSHealthComponent::InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystem)
{
	if (AbilitySystem.Get() == InAbilitySystem)
	{
		return;
	}

	UninitializeFromAbilitySystem();

	if (!InAbilitySystem)
	{
		return;
	}

	AbilitySystem = InAbilitySystem;
	HealthChangedHandle = InAbilitySystem->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UGSHealthComponent::HandleHealthChanged);
}

void UGSHealthComponent::UninitializeFromAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = AbilitySystem.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
	}

	HealthChangedHandle.Reset();
	AbilitySystem.Reset();
}

float UGSHealthComponent::GetHealth() const
{
	const UAbilitySystemComponent* ASC = AbilitySystem.Get();
	return ASC ? ASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute()) : 0.f;
}

float UGSHealthComponent::GetMaxHealth() const
{
	const UAbilitySystemComponent* ASC = AbilitySystem.Get();
	return ASC ? ASC->GetNumericAttribute(UGSAttributeSet::GetMaxHealthAttribute()) : 0.f;
}

void UGSHealthComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	AActor* DamageInstigator = GetDamageInstigator(Data);

	// Health is usually changed indirectly (IncomingDamage meta attribute), so there's no GE data;
	// the attribute set records the attacker on the server.
	if (!DamageInstigator && Data.NewValue < Data.OldValue)
	{
		const UAbilitySystemComponent* ASC = AbilitySystem.Get();
		if (const UGSAttributeSet* Set = ASC ? ASC->GetSet<UGSAttributeSet>() : nullptr)
		{
			DamageInstigator = Set->GetLastDamageInstigator();
		}
	}
	OnHealthChanged.Broadcast(this, Data.OldValue, Data.NewValue, DamageInstigator);

	if (GetOwner()->HasAuthority() && !bIsDead && Data.NewValue <= 0.f)
	{
		StartDeath(DamageInstigator);
	}
}

void UGSHealthComponent::StartDeath(AActor* InKiller)
{
	bIsDead = true;
	Killer = InKiller;

	if (UAbilitySystemComponent* ASC = AbilitySystem.Get())
	{
		ASC->AddLooseGameplayTag(GSGameplayTags::State_Dead, 1, EGameplayTagReplicationState::TagOnly);
		ASC->CancelAllAbilities();
	}

	UE_LOG(LogBanditBaller, Log, TEXT("%s died (killer: %s)"), *GetOwner()->GetActorNameOrLabel(), *GetNameSafe(InKiller));

	if (AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		if (OwnerPawn && OwnerPawn->IsPlayerControlled())
		{
			GameMode->HandlePlayerDeath(OwnerPawn, InKiller);
		}
		else if (bReportEnemyKill)
		{
			GameMode->HandleEnemyKilled(GetOwner(), InKiller);
		}
	}

	BroadcastDeath();
}

void UGSHealthComponent::ResetDeath()
{
	if (!GetOwner()->HasAuthority() || !bIsDead)
	{
		return;
	}

	bIsDead = false;
	Killer = nullptr;
	bDeathBroadcast = false;

	if (UAbilitySystemComponent* ASC = AbilitySystem.Get())
	{
		ASC->RemoveLooseGameplayTag(GSGameplayTags::State_Dead, 1, EGameplayTagReplicationState::TagOnly);
	}
}

void UGSHealthComponent::OnRep_IsDead()
{
	if (bIsDead)
	{
		BroadcastDeath();
	}
	else
	{
		bDeathBroadcast = false;
	}
}

void UGSHealthComponent::BroadcastDeath()
{
	if (bDeathBroadcast)
	{
		return;
	}

	bDeathBroadcast = true;
	OnDeathStarted.Broadcast(GetOwner(), Killer);
}
