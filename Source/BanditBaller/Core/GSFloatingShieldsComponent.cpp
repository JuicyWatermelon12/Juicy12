// BanditBaller

#include "GSFloatingShieldsComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GSPlayerState.h"
#include "GSShieldComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

UGSFloatingShieldsComponent::UGSFloatingShieldsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // after the character moved this frame
}

void UGSFloatingShieldsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Unbind();
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (Mesh)
		{
			Mesh->DestroyComponent();
		}
	}
	Meshes.Reset();
	Super::EndPlay(EndPlayReason);
}

UGSShieldComponent* UGSFloatingShieldsComponent::FindShieldComponent() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AGSPlayerState* PS = Pawn ? Pawn->GetPlayerState<AGSPlayerState>() : nullptr;
	return PS ? PS->GetShieldComponent() : nullptr;
}

void UGSFloatingShieldsComponent::Bind(UGSShieldComponent* Shields)
{
	Unbind();
	BoundShields = Shields;
	if (Shields)
	{
		BlockedHandle = Shields->OnShieldBlocked.AddUObject(this, &UGSFloatingShieldsComponent::HandleBlocked);
		ReturnedHandle = Shields->OnShieldReturned.AddUObject(this, &UGSFloatingShieldsComponent::HandleReturned);
	}
}

void UGSFloatingShieldsComponent::Unbind()
{
	if (UGSShieldComponent* Shields = BoundShields.Get())
	{
		Shields->OnShieldBlocked.Remove(BlockedHandle);
		Shields->OnShieldReturned.Remove(ReturnedHandle);
	}
	BoundShields.Reset();
}

void UGSFloatingShieldsComponent::SyncMeshes(int32 Count)
{
	while (Meshes.Num() > Count)
	{
		if (UStaticMeshComponent* Mesh = Meshes.Last())
		{
			Mesh->DestroyComponent();
		}
		Meshes.Pop();
	}
	while (Meshes.Num() < Count)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
		Mesh->SetStaticMesh(ShieldMesh);
		if (ShieldMaterial)
		{
			Mesh->SetMaterial(0, ShieldMaterial);
		}
		// Visual only: blocking happens in the damage path, never through collision.
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetCastShadow(false);
		Mesh->SetupAttachment(this);
		Mesh->SetUsingAbsoluteLocation(true);
		Mesh->SetUsingAbsoluteRotation(true);
		Mesh->SetUsingAbsoluteScale(true);
		Mesh->RegisterComponent();
		Meshes.Add(Mesh);
	}
	FlashLeft.SetNumZeroed(Count);
	PopLeft.SetNumZeroed(Count);
}

bool UGSFloatingShieldsComponent::IsShieldShown(int32 Index) const
{
	const UStaticMeshComponent* Mesh = GetShieldMesh(Index);
	return Mesh && Mesh->IsVisible();
}

void UGSFloatingShieldsComponent::HandleBlocked(int32 Index)
{
	if (!FlashLeft.IsValidIndex(Index))
	{
		SyncMeshes(BoundShields.IsValid() ? BoundShields->GetNumShields() : 0);
	}
	if (FlashLeft.IsValidIndex(Index))
	{
		FlashLeft[Index] = FMath::Max(BlockFlashTime, KINDA_SMALL_NUMBER);
		PopLeft[Index] = 0.f;
	}
	++BlocksShown;
	const UStaticMeshComponent* Mesh = GetShieldMesh(Index);
	const FVector Where = Mesh ? Mesh->GetComponentLocation() : GetComponentLocation();
	if (BlockEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BlockEffect, Where, FRotator::ZeroRotator, BlockEffectScale);
	}
	if (BlockSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BlockSound, Where);
	}
}

void UGSFloatingShieldsComponent::HandleReturned(int32 Index)
{
	if (PopLeft.IsValidIndex(Index))
	{
		PopLeft[Index] = FMath::Max(ReturnPopTime, KINDA_SMALL_NUMBER);
		FlashLeft[Index] = 0.f;
	}
	++ReturnsShown;
	if (ReturnSound)
	{
		const UStaticMeshComponent* Mesh = GetShieldMesh(Index);
		UGameplayStatics::PlaySoundAtLocation(this, ReturnSound, Mesh ? Mesh->GetComponentLocation() : GetComponentLocation());
	}
}

void UGSFloatingShieldsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Follow the pawn's PlayerState (possession can come after BeginPlay; a respawned pawn binds again).
	UGSShieldComponent* Shields = FindShieldComponent();
	if (Shields != BoundShields.Get())
	{
		Bind(Shields);
	}
	const int32 Count = Shields ? Shields->GetNumShields() : 0;
	if (Count != Meshes.Num())
	{
		SyncMeshes(Count);
	}
	if (Count == 0)
	{
		return;
	}

	SpinAngle = FMath::Fmod(SpinAngle + SpinSpeed * DeltaTime, 360.f);
	const FVector Center = GetOwner()->GetActorLocation() + FVector(0.f, 0.f, Height);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UStaticMeshComponent* Mesh = Meshes[Index];
		if (!Mesh)
		{
			continue;
		}
		// Evenly spaced around the player; the disc's flat face (local Z) points outward.
		const float Yaw = SpinAngle + 360.f * Index / Count;
		const FVector Out = FRotator(0.f, Yaw, 0.f).Vector();
		Mesh->SetWorldLocationAndRotation(Center + Out * OrbitRadius, FRotationMatrix::MakeFromZ(Out).Rotator());

		float Scale = 1.f;
		bool bShown = Shields->IsShieldReady(Index);
		if (FlashLeft[Index] > 0.f)
		{
			// Blocked: a quick grow, then gone until it recharges.
			FlashLeft[Index] = FMath::Max(0.f, FlashLeft[Index] - DeltaTime);
			const float Alpha = 1.f - FlashLeft[Index] / FMath::Max(BlockFlashTime, KINDA_SMALL_NUMBER);
			Scale = FMath::Lerp(1.f, BlockFlashScale, Alpha);
			bShown = FlashLeft[Index] > 0.f;
		}
		else if (PopLeft[Index] > 0.f)
		{
			// Back: grows from nothing to full size.
			PopLeft[Index] = FMath::Max(0.f, PopLeft[Index] - DeltaTime);
			Scale = 1.f - PopLeft[Index] / FMath::Max(ReturnPopTime, KINDA_SMALL_NUMBER);
		}
		Mesh->SetWorldScale3D(ShieldScale * FMath::Max(Scale, 0.01f));
		Mesh->SetVisibility(bShown);
	}
}
