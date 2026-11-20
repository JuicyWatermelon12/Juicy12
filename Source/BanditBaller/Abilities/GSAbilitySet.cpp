// BanditBaller

#include "GSAbilitySet.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "GameplayEffect.h"
#include "GSGameplayAbility.h"

void FGSAbilitySetGrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* ASC)
{
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : EffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	Reset();
}

void UGSAbilitySet::GiveToAbilitySystem(UAbilitySystemComponent* ASC, FGSAbilitySetGrantedHandles* OutHandles, UObject* SourceObject) const
{
	check(ASC);

	if (!ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	for (int32 Index = 0; Index < GrantedAbilities.Num(); ++Index)
	{
		const FGSAbilitySet_GameplayAbility& Entry = GrantedAbilities[Index];
		if (!IsValid(Entry.Ability))
		{
			UE_LOG(LogBanditBaller, Error, TEXT("GrantedAbilities[%d] on ability set [%s] is not valid."), Index, *GetNameSafe(this));
			continue;
		}

		const UGSGameplayAbility* AbilityCDO = Entry.Ability->GetDefaultObject<UGSGameplayAbility>();

		FGameplayAbilitySpec Spec(AbilityCDO->GetClass(), Entry.AbilityLevel, INDEX_NONE, SourceObject);
		if (AbilityCDO->GetActivationInputTag().IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(AbilityCDO->GetActivationInputTag());
		}

		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		if (OutHandles)
		{
			OutHandles->AbilitySpecHandles.Add(Handle);
		}
	}

	for (int32 Index = 0; Index < GrantedEffects.Num(); ++Index)
	{
		const FGSAbilitySet_GameplayEffect& Entry = GrantedEffects[Index];
		if (!IsValid(Entry.GameplayEffect))
		{
			UE_LOG(LogBanditBaller, Error, TEXT("GrantedEffects[%d] on ability set [%s] is not valid."), Index, *GetNameSafe(this));
			continue;
		}

		const UGameplayEffect* EffectCDO = Entry.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(EffectCDO, Entry.EffectLevel, ASC->MakeEffectContext());
		if (OutHandles)
		{
			OutHandles->EffectHandles.Add(Handle);
		}
	}
}
