// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GSFloatingShieldsComponent.generated.h"

class UGSShieldComponent;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The Floating Shields you see: one placeholder disc per shield, circling the player at chest height, evenly spaced and
 * facing out. Visual only: no collision (blocking happens in the damage path, UGSShieldComponent).
 *
 * Reads the owning player's UGSShieldComponent (AGSPlayerState): a ready shield is shown; a blocked one flashes (grows,
 * BlockEffect + BlockSound) and disappears; a returning one pops back in (ReturnSound). On AGSCharacter; tuning on
 * BP_GSCharacter. Local cosmetics. TODO(co-op): clients need the shield states replicated.
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSFloatingShieldsComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGSFloatingShieldsComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Test hooks ---
	int32 GetNumShieldMeshes() const { return Meshes.Num(); }
	UStaticMeshComponent* GetShieldMesh(int32 Index) const { return Meshes.IsValidIndex(Index) ? Meshes[Index].Get() : nullptr; }
	bool IsShieldShown(int32 Index) const;
	int32 GetBlocksShown() const { return BlocksShown; }
	int32 GetReturnsShown() const { return ReturnsShown; }
	float GetOrbitRadius() const { return OrbitRadius; }
	float GetHeight() const { return Height; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Placeholder shield: the engine cylinder flattened into a disc (set on BP_GSCharacter). */
	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield")
	TObjectPtr<UStaticMesh> ShieldMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield")
	TObjectPtr<UMaterialInterface> ShieldMaterial;

	/** Mesh scale; the mesh's local Z axis points outward from the player. */
	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield")
	FVector ShieldScale = FVector(0.45f, 0.45f, 0.06f);

	/** Distance from the player's center (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield", meta = (ClampMin = "0", Units = "cm"))
	float OrbitRadius = 85.f;

	/** Height above the capsule center (cm); the capsule center is at the hips, so this puts them at chest height. */
	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield", meta = (Units = "cm"))
	float Height = 40.f;

	/** Degrees per second the ring turns. */
	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield")
	float SpinSpeed = 120.f;

	/** A blocking shield grows to BlockFlashScale over BlockFlashTime, then disappears. */
	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield", meta = (ClampMin = "0", Units = "s"))
	float BlockFlashTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield", meta = (ClampMin = "1"))
	float BlockFlashScale = 1.8f;

	/** A returning shield grows from 0 to full size over ReturnPopTime. */
	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield", meta = (ClampMin = "0", Units = "s"))
	float ReturnPopTime = 0.35f;

	/** Placeholder flash where a shield blocked a hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield")
	TObjectPtr<UNiagaraSystem> BlockEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield")
	FVector BlockEffectScale = FVector(0.5f);

	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield")
	TObjectPtr<USoundBase> BlockSound;

	UPROPERTY(EditDefaultsOnly, Category = "Floating Shield")
	TObjectPtr<USoundBase> ReturnSound;

private:
	UGSShieldComponent* FindShieldComponent() const;
	void Bind(UGSShieldComponent* Shields);
	void Unbind();
	void SyncMeshes(int32 Count);
	void HandleBlocked(int32 Index);
	void HandleReturned(int32 Index);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Meshes;

	/** Per shield: seconds left of the block flash / the return pop (0 = none). */
	TArray<float> FlashLeft;
	TArray<float> PopLeft;

	TWeakObjectPtr<UGSShieldComponent> BoundShields;
	FDelegateHandle BlockedHandle;
	FDelegateHandle ReturnedHandle;

	float SpinAngle = 0.f;
	int32 BlocksShown = 0;
	int32 ReturnsShown = 0;
};
