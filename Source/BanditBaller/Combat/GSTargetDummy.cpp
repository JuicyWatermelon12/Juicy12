// BanditBaller

#include "GSTargetDummy.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSHealthComponent.h"
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
	HealthComponent = CreateDefaultSubobject<UGSHealthComponent>(TEXT("HealthComponent"));
}

UAbilitySystemComponent* AGSTargetDummy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSTargetDummy::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	HealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	HealthComponent->OnHealthChanged.AddDynamic(this, &AGSTargetDummy::OnHealthChanged);
	HealthComponent->OnDeathStarted.AddDynamic(this, &AGSTargetDummy::OnDeathStarted);

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

void AGSTargetDummy::OnHealthChanged(UGSHealthComponent* InHealthComponent, float OldHealth, float NewHealth, AActor* DamageInstigator)
{
	const float Damage = OldHealth - NewHealth;
	if (Damage <= 0.f)
	{
		return;
	}

	UE_LOG(LogBanditBaller, Log, TEXT("%s took %.1f damage (health %.1f)"), *GetActorNameOrLabel(), Damage, NewHealth);

	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 120.f),
			FString::Printf(TEXT("-%.0f (%.0f)"), Damage, NewHealth), nullptr, FColor::Red, GSWeaponDebug::DrawDuration);
	}
}

void AGSTargetDummy::OnDeathStarted(AActor* DeadActor, AActor* Killer)
{
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(ResetTimer, this, &AGSTargetDummy::ResetDummy, FMath::Max(ResetDelay, 0.01f));
	}
}

void AGSTargetDummy::ResetDummy()
{
	HealthComponent->ResetDeath();
	ApplyInitEffect();
	UE_LOG(LogBanditBaller, Log, TEXT("%s reset"), *GetActorNameOrLabel());
}
