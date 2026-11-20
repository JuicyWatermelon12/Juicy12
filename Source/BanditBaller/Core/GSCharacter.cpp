// BanditBaller

#include "GSCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GSAttributeSet.h"
#include "GSPlayerState.h"

AGSCharacter::AGSCharacter()
{
}

UAbilitySystemComponent* AGSCharacter::GetAbilitySystemComponent() const
{
	return CachedAbilitySystem.Get();
}

void AGSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Server: bind avatar, then grant the startup set once per PlayerState.
	InitAbilitySystem();

	if (AGSPlayerState* PS = GetPlayerState<AGSPlayerState>())
	{
		PS->GrantStartupAbilitySet(StartupAbilitySet);
	}
}

void AGSCharacter::UnPossessed()
{
	ClearAbilitySystem();
	Super::UnPossessed();
}

void AGSCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Client: bind avatar once the PlayerState has replicated (granting is server-only).
	InitAbilitySystem();
}

void AGSCharacter::InitAbilitySystem()
{
	AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	ClearAbilitySystem();

	ASC->InitAbilityActorInfo(PS, this);
	CachedAbilitySystem = ASC;

	MoveSpeedChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetMoveSpeedAttribute())
		.AddUObject(this, &AGSCharacter::OnMoveSpeedChanged);

	// Already initialized (e.g. respawn): sync immediately. A 0 value means stats aren't applied yet.
	ApplyMoveSpeed(ASC->GetNumericAttribute(UGSAttributeSet::GetMoveSpeedAttribute()));
}

void AGSCharacter::ClearAbilitySystem()
{
	if (UAbilitySystemComponent* ASC = CachedAbilitySystem.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetMoveSpeedAttribute()).Remove(MoveSpeedChangedHandle);
	}

	MoveSpeedChangedHandle.Reset();
	CachedAbilitySystem.Reset();
}

void AGSCharacter::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	ApplyMoveSpeed(Data.NewValue);
}

void AGSCharacter::ApplyMoveSpeed(float NewSpeed)
{
	if (NewSpeed > 0.f)
	{
		GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
	}
}
