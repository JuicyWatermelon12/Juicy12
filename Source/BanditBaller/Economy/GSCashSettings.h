// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSCashSettings.generated.h"

class UMaterialInterface;
class UStaticMesh;

/** How one cash piece looks (coin or bill). */
USTRUCT(BlueprintType)
struct FGSCashPieceLook
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector Scale = FVector(0.3f, 0.3f, 0.05f);

	/** Mesh rotation relative to the pickup (it spins around Z on top of this). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FRotator Rotation = FRotator::ZeroRotator;
};

/**
 * Pickup tuning for both kinds of enemy drops (DA_CashSettings), read by UGSCashPickupSubsystem via AGSGameMode and
 * replicated to clients on each AGSCashPickup (for the look and blink timing).
 * - Cash pieces: Split, Lifetime, CoinLook / BillLook.
 * - XP orbs: the "XP Orb" section.
 * - Shared by both: Scatter (toss), Magnet, Pool.
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSCashSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Splitting a drop into pieces ---

	/** A drop is split into about TotalValue / TargetPieceValue pieces (clamped to Min/MaxPieces). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Split", meta = (ClampMin = "1"))
	int32 TargetPieceValue = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Split", meta = (ClampMin = "1"))
	int32 MinPieces = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Split", meta = (ClampMin = "1"))
	int32 MaxPieces = 6;

	/** Pieces worth at least this much look like bills; smaller ones are coins. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Split", meta = (ClampMin = "1"))
	int32 BillThreshold = 4;

	// --- Scatter ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scatter", meta = (ClampMin = "0", Units = "cm"))
	float ScatterRadiusMin = 40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scatter", meta = (ClampMin = "0", Units = "cm"))
	float ScatterRadiusMax = 140.f;

	/** Time for a piece to arc from the enemy to its landing spot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scatter", meta = (ClampMin = "0.01", Units = "s"))
	float TossTime = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scatter", meta = (ClampMin = "0", Units = "cm"))
	float TossHeight = 80.f;

	/** Pieces rest this far above the ground. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scatter", meta = (ClampMin = "0", Units = "cm"))
	float RestHeight = 25.f;

	// --- Lifetime ---

	/** Uncollected pieces vanish after this long (gs.Cash.Lifetime overrides). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lifetime", meta = (ClampMin = "0.1", Units = "s"))
	float Lifetime = 20.f;

	/** Pieces blink for the last BlinkTime seconds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lifetime", meta = (ClampMin = "0", Units = "s"))
	float BlinkTime = 4.f;

	// --- Magnet ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magnet", meta = (ClampMin = "0", Units = "cm/s"))
	float MagnetStartSpeed = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magnet", meta = (ClampMin = "0"))
	float MagnetAcceleration = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magnet", meta = (ClampMin = "0", Units = "cm/s"))
	float MagnetMaxSpeed = 3000.f;

	/** Collected when this close to the player's center. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magnet", meta = (ClampMin = "1", Units = "cm"))
	float CollectDistance = 80.f;

	// --- Pool / look ---

	/** Pickups created up front the first time cash drops. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool", meta = (ClampMin = "0"))
	int32 PoolPrewarm = 48;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FGSCashPieceLook CoinLook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FGSCashPieceLook BillLook;

	// --- XP orbs (one per killed enemy, worth its XPValue) ---

	/** Seconds an uncollected XP orb stays; 0 = it never expires. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XP Orb", meta = (ClampMin = "0", Units = "s"))
	float XPOrbLifetime = 0.f;

	/** Placeholder look: a glowing blue sphere (it pulses instead of spinning). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XP Orb")
	FGSCashPieceLook XPOrbLook;

	/** Scale pulse of the orb: +-XPOrbPulseAmount around its look scale, XPOrbPulseSpeed times per second. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XP Orb", meta = (ClampMin = "0", ClampMax = "1"))
	float XPOrbPulseAmount = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XP Orb", meta = (ClampMin = "0"))
	float XPOrbPulseSpeed = 1.5f;
};
