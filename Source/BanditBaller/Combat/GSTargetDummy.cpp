// BanditBaller

#include "GSTargetDummy.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSGameplayTags.h"
#include "GSWeaponDebug.h"
#include "TimerManager.h"

AGSTargetDummy::AGSTargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Mesh;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UGSAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AGSTargetDummy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSTargetDummy::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetHealthAttribute())
		.AddUObject(this, &AGSTargetDummy::OnHealthChanged);

	if (HasAuthority())
	{
		ApplyInitEffect();
	}
}

void AGSTargetDummy::ApplyInitEffect()
{
	if (!InitEffect)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s has no InitEffect; it starts with 0 health."), *GetNameSafe(this));
		return;
	}

	AbilitySystemComponent->ApplyGameplayEffectToSelf(InitEffect->GetDefaultObject<UGameplayEffect>(), 1.f, AbilitySystemComponent->MakeEffectContext());
}

void AGSTargetDummy::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	const float Delta = Data.NewValue - Data.OldValue;
	if (Delta >= 0.f)
	{
		return;
	}

	UE_LOG(LogBanditBaller, Log, TEXT("%s took %.1f damage (health %.1f)"), *GetActorNameOrLabel(), -Delta, Data.NewValue);

	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 120.f),
			FString::Printf(TEXT("-%.0f (%.0f)"), -Delta, Data.NewValue), nullptr, FColor::Red, GSWeaponDebug::DrawDuration);
	}

	if (HasAuthority() && Data.NewValue <= 0.f && !AbilitySystemComponent->HasMatchingGameplayTag(GSGameplayTags::State_Dead))
	{
		AbilitySystemComponent->AddLooseGameplayTag(GSGameplayTags::State_Dead);
		UE_LOG(LogBanditBaller, Log, TEXT("%s is down; resetting in %.1fs"), *GetActorNameOrLabel(), ResetDelay);
		GetWorldTimerManager().SetTimer(ResetTimer, this, &AGSTargetDummy::ResetDummy, FMath::Max(ResetDelay, 0.01f));
	}
}

void AGSTargetDummy::ResetDummy()
{
	AbilitySystemComponent->RemoveLooseGameplayTag(GSGameplayTags::State_Dead);
	ApplyInitEffect();
}
