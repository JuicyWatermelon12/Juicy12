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
#include "GSGameMode.h"
#include "GSHealthComponent.h"
#include "GSMeshAttachment.h"
#include "GSRunRandom.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GSWeaponData.h"
#include "GSWeaponDebug.h"
#include "GSWaveDirector.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName TelegraphParamName(TEXT("TelegraphAmount"));
	const FName MuzzleSocketName(TEXT("Muzzle"));

	/** Aim laser beam thickness (cm). */
	constexpr float LaserThickness = 4.f;
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

	// Ranged enemies only: gun + aim laser (configured from EnemyData->Weapon in BeginPlay).
	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMeshComponent->SetupAttachment(GetMesh());
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> LaserMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	AimLaserComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AimLaser"));
	AimLaserComponent->SetupAttachment(RootComponent);
	AimLaserComponent->SetUsingAbsoluteLocation(true);
	AimLaserComponent->SetUsingAbsoluteRotation(true);
	AimLaserComponent->SetUsingAbsoluteScale(true);
	AimLaserComponent->SetStaticMesh(LaserMesh.Object);
	AimLaserComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AimLaserComponent->SetGenerateOverlapEvents(false);
	AimLaserComponent->SetCastShadow(false);
	AimLaserComponent->SetHiddenInGame(true);

	// Tick only runs while the laser is visible.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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

	// One dynamic material per slot (the slot's own entry, else the fallback), so the wind-up glow (TelegraphAmount)
	// reaches every part of the body.
	BodyMaterials.Reset();
	const TArray<FName> SlotNames = MeshComp->GetMaterialSlotNames();
	for (int32 Slot = 0; Slot < MeshComp->GetNumMaterials(); ++Slot)
	{
		const TObjectPtr<UMaterialInterface>* Entry = SlotNames.IsValidIndex(Slot) ? EnemyData->SlotMaterials.Find(SlotNames[Slot]) : nullptr;
		UMaterialInterface* Source = (Entry && *Entry) ? Entry->Get() : EnemyData->BodyMaterial.Get();
		if (Source)
		{
			BodyMaterials.Add(MeshComp->CreateDynamicMaterialInstance(Slot, Source));
		}
	}

	// Worn / held props (cap, baton): no collision, built like the player's (FGSMeshAttachment).
	GSMeshAttachments::Apply(this, MeshComp, EnemyData->MeshAttachments, MeshAttachmentComponents);

	// Performance (Prompt 60). The ragdoll restores the mesh's collision at death (OnDeathStarted).
	if (EnemyData->bMeshCollisionOnlyWhenDead)
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComp->KinematicBonesUpdateType = EKinematicBonesUpdateToPhysics::SkipAllBones;
	}
	if (EnemyData->bSkipPoseWhenOffScreen)
	{
		MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}
	MeshComp->bEnableUpdateRateOptimizations = EnemyData->bAnimUpdateRateOptimizations;
	if (EnemyData->bNoCapsuleOverlapEvents)
	{
		GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	}

	if (EnemyData->HealthBarWidgetClass)
	{
		HealthBarComponent->SetWidgetClass(EnemyData->HealthBarWidgetClass);
		HealthBarComponent->InitWidget();
	}

	if (const UGSWeaponData* Weapon = EnemyData->Weapon)
	{
		WeaponMeshComponent->SetStaticMesh(Weapon->Mesh);
		WeaponMeshComponent->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Weapon->AttachSocket);
		WeaponMeshComponent->SetRelativeTransform(Weapon->MeshTransform);
	}
	if (EnemyData->AimLaserMaterial)
	{
		AimLaserComponent->SetMaterial(0, EnemyData->AimLaserMaterial);
	}
}

void AGSEnemyCharacter::SetEnemyData(UGSEnemyData* InEnemyData)
{
	if (HasActorBegunPlay())
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s: SetEnemyData after BeginPlay is ignored."), *GetActorNameOrLabel());
		return;
	}
	EnemyData = InEnemyData;
}

AGSEnemyCharacter* AGSEnemyCharacter::SpawnEnemy(UWorld* World, UGSEnemyData* InEnemyData, const FVector& NavLocation, const FVector& FaceTowards, int32 WaveRound)
{
	if (!World || !InEnemyData)
	{
		return nullptr;
	}
	const float HalfHeight = GetDefault<AGSEnemyCharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Location = NavLocation + FVector(0.f, 0.f, HalfHeight + 2.f);
	const FTransform Transform((FaceTowards - Location).GetSafeNormal2D().Rotation(), Location);

	AGSEnemyCharacter* Enemy = World->SpawnActorDeferred<AGSEnemyCharacter>(AGSEnemyCharacter::StaticClass(), Transform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Enemy)
	{
		Enemy->SetEnemyData(InEnemyData);
		Enemy->SetWaveRound(WaveRound); // before BeginPlay: its round scaling uses it
		Enemy->FinishSpawning(Transform);
	}
	return Enemy;
}

FCollisionObjectQueryParams AGSEnemyCharacter::GetShotObjectQueryParams()
{
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	return ObjectParams;
}

FVector AGSEnemyCharacter::GetMuzzleLocation() const
{
	if (WeaponMeshComponent && WeaponMeshComponent->GetStaticMesh())
	{
		if (WeaponMeshComponent->DoesSocketExist(MuzzleSocketName))
		{
			return WeaponMeshComponent->GetSocketLocation(MuzzleSocketName);
		}
		const FVector LocalOffset = (EnemyData && EnemyData->Weapon) ? EnemyData->Weapon->MuzzleOffset : FVector::ZeroVector;
		return WeaponMeshComponent->GetComponentTransform().TransformPosition(LocalOffset);
	}
	return GetActorLocation() + GetActorForwardVector() * 60.f + FVector(0.f, 0.f, 40.f);
}

void AGSEnemyCharacter::MulticastShotFired_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End, bool bHit)
{
	UWorld* World = GetWorld();
	const UGSWeaponData* Weapon = EnemyData ? EnemyData->Weapon.Get() : nullptr;
	if (!World || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const FVector Direction = (FVector(End) - FVector(Start)).GetSafeNormal();
	if (Weapon && Weapon->MuzzleEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Weapon->MuzzleEffect, Start, Direction.Rotation());
	}
	if (Weapon && Weapon->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, Weapon->FireSound, Start);
	}
	if (bHit && Weapon && Weapon->ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Weapon->ImpactEffect, End, (-Direction).Rotation());
	}
	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugLine(World, Start, End, bHit ? FColor::Red : FColor::Orange, false, GSWeaponDebug::DrawDuration, 0, 1.f);
	}
	// No instant tracer: the shot is a projectile (Prompt 58), which is what the player sees and dodges.
}

void AGSEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAimLaser();
}

void AGSEnemyCharacter::UpdateAimLaser()
{
	UWorld* World = GetWorld();
	const bool bHasLaser = EnemyData && EnemyData->AimLaserMaterial && World;
	const bool bAiming = bHasLaser && bTelegraphing;

	if (!bAiming)
	{
		AimLaserComponent->SetHiddenInGame(true);
		SetActorTickEnabled(false);
		return;
	}

	// Aim laser: straight out of the muzzle along the facing (the server keeps the enemy facing its target).
	const FVector Start = GetMuzzleLocation();
	const float Range = (EnemyData->Weapon ? EnemyData->Weapon->Range : 5000.f);
	FVector End = Start + GetActorForwardVector() * Range;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSEnemyAimLaser), false, this);
	if (World->LineTraceSingleByObjectType(Hit, Start, End, GetShotObjectQueryParams(), Params))
	{
		End = Hit.ImpactPoint;
	}

	const FVector Segment = End - Start;
	const float Length = Segment.Size();
	if (Length < 1.f)
	{
		AimLaserComponent->SetHiddenInGame(true);
		return;
	}

	// /Engine/BasicShapes/Cylinder: 100 cm tall along Z, 100 cm wide, pivot at its center.
	const FVector Direction = Segment / Length;
	AimLaserComponent->SetWorldLocationAndRotation(Start + Segment * 0.5f, FRotationMatrix::MakeFromZ(Direction).Rotator());
	AimLaserComponent->SetWorldScale3D(FVector(LaserThickness / 100.f, LaserThickness / 100.f, Length / 100.f));
	AimLaserComponent->SetHiddenInGame(false);
}

void AGSEnemyCharacter::InitializeStats()
{
	if (!EnemyData)
	{
		return;
	}

	// Per-spawn speed variance so groups spread out.
	const float Variance = EnemyData->MoveSpeedVariance;
	const float Speed = EnemyData->MoveSpeed * UGSRunRandom::FRandRange(this, 1.f - Variance, 1.f + Variance);

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

	// Round difficulty: GE_EnemyRoundScaling with the current round's multipliers (DA_DifficultyCurve).
	if (const AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		GameMode->ApplyEnemyRoundScaling(*this);
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
	// A body that was off screen may have skipped its pose; the ragdoll starts from the current one.
	MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MeshComp->KinematicBonesUpdateType = EKinematicBonesUpdateToPhysics::SkipSimulatingBones;
	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	if (EnemyData && EnemyData->bMeshCollisionOnlyWhenDead)
	{
		// The bodies didn't follow the animation while alive: rebuild them at the current pose before simulating.
		MeshComp->RefreshBoneTransforms();
		MeshComp->RecreatePhysicsState();
	}
	MeshComp->SetAllBodiesSimulatePhysics(true);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();

	if (HasAuthority())
	{
		SetLifeSpan(EnemyData ? FMath::Max(EnemyData->CorpseLifetime, 0.1f) : 5.f);

		// At most DA_WaveSettings MaxCorpses ragdolls at a time; the oldest go first.
		if (const AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>())
		{
			if (UGSWaveDirector* Director = GameMode->GetWaveDirector())
			{
				Director->RegisterCorpse(this);
			}
		}
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

	if (bTelegraphing)
	{
		SetActorTickEnabled(true);
	}
	UpdateAimLaser();
}
