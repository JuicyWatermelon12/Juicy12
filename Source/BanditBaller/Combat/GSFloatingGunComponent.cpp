// BanditBaller

#include "GSFloatingGunComponent.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "Engine/World.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSFloatingGun.h"
#include "GSGameMode.h"

UGSFloatingGunComponent::UGSFloatingGunComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f;
}

const UGSFloatingGunSettings* UGSFloatingGunComponent::GetSettings() const
{
	const AGSGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr;
	return GameMode ? GameMode->GetFloatingGunSettings() : nullptr;
}

int32 UGSFloatingGunComponent::GetLevel() const
{
	const AGSCharacter* Character = Cast<AGSCharacter>(GetOwner());
	const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	return ASC ? FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(UGSAttributeSet::GetFloatingGunLevelAttribute()))) : 0;
}

void UGSFloatingGunComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	// Level 1+: the right-shoulder gun. (Level 2's left gun comes in Prompt 39.)
	SyncGuns((GetLevel() >= 1 && GetSettings()) ? 1 : 0);
}

void UGSFloatingGunComponent::SyncGuns(int32 Wanted)
{
	Guns.RemoveAll([](const TObjectPtr<AGSFloatingGun>& Gun) { return !IsValid(Gun); });
	while (Guns.Num() > Wanted)
	{
		Guns.Last()->Destroy();
		Guns.Pop();
	}
	AGSCharacter* Character = Cast<AGSCharacter>(GetOwner());
	while (Guns.Num() < Wanted && Character)
	{
		FActorSpawnParameters Params;
		Params.Owner = Character;
		Params.Instigator = Character;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		AGSFloatingGun* Gun = GetWorld()->SpawnActor<AGSFloatingGun>(AGSFloatingGun::StaticClass(), Character->GetActorTransform(), Params);
		if (!Gun)
		{
			break;
		}
		const float Side = Guns.Num() == 0 ? 1.f : -1.f; // right shoulder first
		Gun->InitGun(Character, GetSettings(), Side);
		Guns.Add(Gun);
		UE_LOG(LogBanditBaller, Log, TEXT("[gun] %s: Floating Gun level %d -> gun %d over the %s shoulder"), *GetNameSafe(Character), GetLevel(),
			Guns.Num(), Side > 0.f ? TEXT("right") : TEXT("left"));
	}
}

void UGSFloatingGunComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AGSFloatingGun* Gun : Guns)
	{
		if (IsValid(Gun))
		{
			Gun->Destroy();
		}
	}
	Guns.Reset();
	Super::EndPlay(EndPlayReason);
}
