// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSCashPickup.generated.h"

class UGSCashSettings;
class UStaticMeshComponent;
struct FGSCashPieceLook;

/** What an enemy-drop pickup pays out when collected. */
UENUM(BlueprintType)
enum class EGSPickupKind : uint8
{
	/** A coin or bill: permanent cash (AGSGameMode::AwardCash). */
	Cash,
	/** An XP orb: run XP for the collecting player (AGSGameMode::AwardXPPickup). */
	XP
};

/**
 * One enemy drop (Megabonk-style): a coin or bill, or an XP orb. Pooled by UGSCashPickupSubsystem (one pool for both
 * kinds; the look is set on activation); never placed.
 *
 * Server: tosses to a landing spot, waits; when a living player's PickupRadius covers it, flies to that player
 * (accelerating) and is collected. Uncollected pickups return to the pool after their lifetime (XP orbs: none by
 * default). Clients: cosmetic spin (cash) or pulse (XP), bob, and blink before expiry. Replicated (movement + state).
 */
UCLASS(NotPlaceable)
class BANDITBALLER_API AGSCashPickup : public AActor
{
	GENERATED_BODY()

public:
	AGSCashPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Server: take this pooled pickup out of the pool. ExpireAtServerTime is a GameState server time (0 = never
	 * expires). bInBill only matters for cash.
	 */
	void ActivatePickup(EGSPickupKind InKind, int32 InValue, bool bInBill, const UGSCashSettings* InSettings, const FVector& From, const FVector& Landing, double InExpireAtServerTime);

	/** Server: fly to Target regardless of its pickup radius (vacuum) or because it came in range. */
	void StartMagnet(APawn* Target);

	/** Server: back to the pool (hidden, no tick). */
	void DeactivatePickup();

	bool IsActivePickup() const { return bActive; }
	bool IsMagnetized() const { return State == EState::Magnet; }
	int32 GetValue() const { return Value; }
	EGSPickupKind GetKind() const { return Kind; }
	bool IsCash() const { return Kind == EGSPickupKind::Cash; }
	bool IsXPOrb() const { return Kind == EGSPickupKind::XP; }

	/** Server time it expires; 0 = never (or flying to a player). */
	double GetExpireAtServerTime() const { return ExpireAtServerTime; }

	UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** In its last BlinkTime seconds before expiring (any machine; uses the replicated server time). */
	bool IsBlinking() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Cash")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Cash")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	enum class EState : uint8 { Tossing, Idle, Magnet };

	void TickServer(float DeltaSeconds);
	void TickCosmetic(float DeltaSeconds);
	APawn* FindPlayerInRange() const;
	static bool IsLivingPlayer(const APawn* Pawn);

	UFUNCTION()
	void OnRep_Active();

	UFUNCTION()
	void OnRep_Look();

	void ApplyLook();
	void ApplyActive();
	const FGSCashPieceLook* GetLook() const;

	// --- Replicated ---

	UPROPERTY(ReplicatedUsing = OnRep_Active)
	bool bActive = false;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	int32 Value = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	bool bBill = false;

	/** TODO(co-op): not replicated (single-player first); clients need it in M6 to show the XP orb look. */
	UPROPERTY()
	EGSPickupKind Kind = EGSPickupKind::Cash;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	TObjectPtr<const UGSCashSettings> Settings;

	/** Server world time when it expires; 0 while flying to a player (no blink, no expiry). */
	UPROPERTY(Replicated)
	double ExpireAtServerTime = 0.0;

	// --- Server state ---

	EState State = EState::Idle;
	FVector TossFrom = FVector::ZeroVector;
	FVector TossTo = FVector::ZeroVector;
	float TossElapsed = 0.f;
	TWeakObjectPtr<APawn> MagnetTarget;
	float MagnetSpeed = 0.f;

	// --- Cosmetic ---

	float CosmeticTime = 0.f;
};
