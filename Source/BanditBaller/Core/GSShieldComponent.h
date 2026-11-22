// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GSShieldComponent.generated.h"

class UAbilitySystemComponent;

/** One Floating Shield and its recharge. */
USTRUCT()
struct FGSShieldSlot
{
	GENERATED_BODY()

	/** Ready to block a hit (false while it recharges). */
	bool bReady = true;

	/** World time it comes back (while recharging). */
	double ReadyAtTime = 0.0;

	FTimerHandle ReturnTimer;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGSOnShieldEventNative, int32 /*ShieldIndex*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FGSOnShieldCountChangedNative, int32 /*NumShields*/);

/**
 * The player's Floating Shields, on AGSPlayerState (run state).
 *
 * - The number of shields is the Shields attribute (Floating Shield upgrade), kept in sync through the attribute's
 *   change delegate: new shields are ready at once, removed ones drop their recharge.
 * - UGSAttributeSet calls TryAbsorbHit when damage is about to become Health loss. The first ready shield cancels the
 *   whole hit and recharges for DA_CombatSettings ShieldCooldown on its own world timer (it pauses with the game).
 * - Dash invulnerability comes first: GE_Damage isn't applied at all under State.Invulnerable, so no shield is used.
 * - A blocked hit is not damage taken: no Health lost, so the combo and the no-damage round bonus are unaffected.
 *
 * Authority only. TODO(co-op): not replicated (single-player first); clients' shield visuals need the ready states.
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSShieldComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSShieldComponent();

	/**
	 * Server: damage Amount is about to hit this player. If a shield is ready, it blocks the whole hit, starts its
	 * recharge and true is returned (no Health lost). False without a ready shield.
	 */
	bool TryAbsorbHit(float Amount, const AActor* Instigator);

	/** A new run: every recharge is cancelled and every shield (from the attribute, 0 after the upgrades reset) is ready. */
	void ResetShields();

	int32 GetNumShields() const { return Slots.Num(); }
	int32 GetNumReady() const;
	bool IsShieldReady(int32 Index) const { return Slots.IsValidIndex(Index) && Slots[Index].bReady; }

	/** Seconds until shield Index comes back (0 if it is ready). */
	float GetCooldownRemaining(int32 Index) const;

	/** Hits blocked since this run started (tests / logs). */
	int32 GetHitsBlocked() const { return HitsBlocked; }

	/** Re-read the Shields attribute now (normally done by the attribute's change delegate). */
	void SyncShieldCount();

	/** Shield Index blocked a hit and is gone until it recharges. */
	FGSOnShieldEventNative OnShieldBlocked;

	/** Shield Index recharged and is ready again. */
	FGSOnShieldEventNative OnShieldReturned;

	/** The number of shields changed (a pick, or a new run). */
	FGSOnShieldCountChangedNative OnShieldCountChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UAbilitySystemComponent* GetASC() const;
	float GetCooldown() const;
	void HandleShieldsAttributeChanged(const FOnAttributeChangeData& Data);
	void HandleShieldReturned(int32 Index);

	UPROPERTY(Transient)
	TArray<FGSShieldSlot> Slots;

	int32 HitsBlocked = 0;
	FDelegateHandle AttributeChangedHandle;
};
