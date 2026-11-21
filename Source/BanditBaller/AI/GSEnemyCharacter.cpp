// BanditBaller

#include "GSEnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "BanditBaller.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GSAbilitySet.h"
#include "GSAttributeSet.h"
#include "GSEnemyAIController.h"
#include "GSEnemyData.h"
#include "GSEnemyHealthBarWidget.h"
#include "GSHealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

namespace
{
	const FName TelegraphParamName(TEXT("TelegraphAmount"));
}

AGSEnemyCharacter::AGSEnemyCharacter()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UGSAttributeSet>(TEXT("AttributeSet"));
	HealthComponent = CreateDefaultSubobject<UGSHealthComponent>(TEXT("HealthComponent"));

	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBarComponent->SetupAttachment(RootComponent);
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawSize(FVector2D(90.f, 10.f));
	HealthBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 115.f));
	HealthBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HealthBarComponent->SetHiddenInGame(true);

	// Mannequin-style mesh setup (mesh/anim come from the data asset).
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -89.f), FRotator(0.f, -90.f, 0.f));

	// Weapon hitscan traces ECC_Visibility; the default Pawn profile ignores it, so block it here.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	AIControllerClass = AGSEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

UAbilitySystemComponent* AGSEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSEnemyCharacter, bTelegraphing);
}

void AGSEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyEnemyDataVisuals();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAttributeSet::GetMoveSpeedAttribute())
		.AddUObject(this, &AGSEnemyCharacter::OnMoveSpeedChanged);

	HealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	HealthComponent->OnHealthChanged.AddDynamic(this, &AGSEnemyCharacter::OnHealthChanged);
	HealthComponent->OnDeathStarted.AddDynamic(this, &AGSEnemyCharacter::OnDeathStarted);

	if (HasAuthority())
	{
		InitializeStats();
	}
}

void AGSEnemyCharacter::ApplyEnemyDataVisuals()
{
	if (!EnemyData)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s has no EnemyData."), *GetActorNameOrLabel());
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (EnemyData->Mesh)
	{
		MeshComp->SetSkeletalMesh(EnemyData->Mesh);
	}
	if (EnemyData->AnimClass)
	{
		MeshComp->SetAnimInstanceClass(EnemyData->AnimClass);
	}

	BodyMaterials.Reset();
	if (EnemyData->BodyMaterial)
	{
		for (int32 Slot = 0; Slot < MeshComp->GetNumMaterials(); ++Slot)
		{
			BodyMaterials.Add(MeshComp->CreateDynamicMaterialInstance(Slot, EnemyData->BodyMaterial));
		}
	}

	if (EnemyData->HealthBarWidgetClass)
	{
		HealthBarComponent->SetWidgetClass(EnemyData->HealthBarWidgetClass);
		HealthBarComponent->InitWidget();
	}
}

void AGSEnemyCharacter::InitializeStats()
{
	if (!EnemyData)
	{
		return;
	}

	// Per-spawn speed variance so groups spread out.
	const float Variance = EnemyData->MoveSpeedVariance;
	const float Speed = EnemyData->MoveSpeed * FMath::FRandRange(1.f - Variance, 1.f + Variance);

	UAbilitySystemComponent* ASC = AbilitySystemComponent;
	ASC->SetNumericAttributeBase(UGSAttributeSet::GetMaxHealthAttribute(), EnemyData->MaxHealth);
	ASC->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), EnemyData->MaxHealth);
	ASC->SetNumericAttributeBase(UGSAttributeSet::GetMoveSpeedAttribute(), Speed);
	ASC->SetNumericAttributeBase(UGSAttributeSet::GetDamageAttribute(), EnemyData->DamageMultiplier);
	ASC->SetNumericAttributeBase(UGSAttributeSet::GetFireRateAttribute(), 1.f);
	GetCharacterMovement()->MaxWalkSpeed = Speed;

	if (EnemyData->AbilitySet)
	{
		EnemyData->AbilitySet->GiveToAbilitySystem(ASC, nullptr, this);
	}
}

void AGSEnemyCharacter::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue > 0.f)
	{
		GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
	}
}

void AGSEnemyCharacter::OnHealthChanged(UGSHealthComponent* InHealthComponent, float OldHealth, float NewHealth, AActor* DamageInstigator)
{
	// Only show the bar once the enemy has been hurt.
	if (NewHealth < OldHealth && NewHealth > 0.f)
	{
		HealthBarComponent->SetHiddenInGame(false);
	}
	UpdateHealthBar();
}

void AGSEnemyCharacter::UpdateHealthBar()
{
	if (UGSEnemyHealthBarWidget* Bar = Cast<UGSEnemyHealthBarWidget>(HealthBarComponent->GetUserWidgetObject()))
	{
		const float Max = HealthComponent->GetMaxHealth();
		Bar->SetHealthFraction(Max > 0.f ? HealthComponent->GetHealth() / Max : 0.f);
	}
}

void AGSEnemyCharacter::OnDeathStarted(AActor* DeadActor, AActor* Killer)
{
	HealthBarComponent->SetHiddenInGame(true);
	bTelegraphing = false;
	ApplyTelegraphVisual();

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->StopMovement();
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Dead"));
		}
	}

	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->StopMovementImmediately();
	CMC->DisableMovement();

	// Ragdoll; the capsule stops blocking so players can walk through the body.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();

	if (HasAuthority())
	{
		SetLifeSpan(EnemyData ? FMath::Max(EnemyData->CorpseLifetime, 0.1f) : 5.f);
	}
}

void AGSEnemyCharacter::SetTelegraphing(bool bInTelegraphing)
{
	if (bTelegraphing != bInTelegraphing)
	{
		bTelegraphing = bInTelegraphing;
		ApplyTelegraphVisual();
	}
}

void AGSEnemyCharacter::OnRep_Telegraphing()
{
	ApplyTelegraphVisual();
}

void AGSEnemyCharacter::ApplyTelegraphVisual()
{
	for (UMaterialInstanceDynamic* MID : BodyMaterials)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TelegraphParamName, bTelegraphing ? 1.f : 0.f);
		}
	}
}
