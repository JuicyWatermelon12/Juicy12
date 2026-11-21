// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSCashPickup.generated.h"

class UGSCashSettings;
class UStaticMeshComponent;

/**
 * One coin or bill dropped by an enemy (Megabonk-style). Pooled by UGSCashPickupSubsystem; never placed.
 *
 * Server: tosses to a landing spot, waits; when a living player's PickupRadius covers it, flies to that player
 * (accelerating) and is collected (AGSGameMode::AwardCash). Uncollected pieces return to the pool after their
 * lifetime. Clients: cosmetic spin, bob and blink before expiry. Replicated (movement + state).
 */
UCLASS(NotPlaceable)
class BANDITBALLER_API AGSCashPickup : public AActor
{
	GENERATED_BODY()

public:
	AGSCashPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/** Server: take this pooled pickup out of the pool. ExpireAtServerTime is a GameState server time. */
	void ActivatePickup(int32 InValue, bool bInBill, const UGSCashSettings* InSettings, const FVector& From, const FVector& Landing, double InExpireAtServerTime);

	/** Server: fly to Target regardless of its pickup radius (vacuum) or because it came in range. */
	void StartMagnet(APawn* Target);

	/** Server: back to the pool (hidden, no tick). */
	void DeactivatePickup();

	bool IsActivePickup() const { return bActive; }
	bool IsMagnetized() const { return State == EState::Magnet; }
	int32 GetValue() const { return Value; }

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

	// --- Replicated ---

	UPROPERTY(ReplicatedUsing = OnRep_Active)
	bool bActive = false;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	int32 Value = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	bool bBill = false;

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
