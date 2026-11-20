// BanditBaller

#include "GSCharacter.h"
#include "AbilitySystemComponent.h"
#include "GSAttributeSet.h"

AGSCharacter::AGSCharacter()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// Created as a subobject of the owner actor so the ASC registers it automatically.
	AttributeSet = CreateDefaultSubobject<UGSAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AGSCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}
