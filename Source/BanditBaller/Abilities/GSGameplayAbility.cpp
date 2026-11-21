// BanditBaller

#include "GSGameplayAbility.h"
#include "GSCharacter.h"
#include "GSWeaponComponent.h"

UGSGameplayAbility::UGSGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

AGSCharacter* UGSGameplayAbility::GetGSCharacter(const FGameplayAbilityActorInfo* ActorInfo)
{
	return ActorInfo ? Cast<AGSCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
}

UGSWeaponComponent* UGSGameplayAbility::GetWeaponComponent(const FGameplayAbilityActorInfo* ActorInfo)
{
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	return Avatar ? Avatar->FindComponentByClass<UGSWeaponComponent>() : nullptr;
}
