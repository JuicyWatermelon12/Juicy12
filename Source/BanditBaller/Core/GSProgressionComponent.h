// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSProgressionComponent.generated.h"

class UGSXPCurve;

/** Replicated as one struct so clients always see a consistent level / XP pair. */
USTRUCT(BlueprintType)
struct FGSProgressionState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	int32 Level = 1;

	/** XP into the current level (carries over between level-ups). 0 at max level. */
	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	int32 CurrentXP = 0;

	/** XP needed to reach Level + 1. 0 at max level (or without an XP curve). */
	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	int32 XPForNextLevel = 0;

	/** All XP this player was granted this run (including XP past max level). */
	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	int32 TotalXP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Progression")
	int32 MaxLevel = 1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGSOnXPChangedSignature, int32, CurrentXP, int32, XPForNextLevel, int32, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSOnLevelUpSignature, int32, NewLevel);

/**
 * Player level and XP, on AGSPlayerState. Server-authoritative: the only way to gain XP is AddXP (authority);
 * UI only reads and listens. XP needed per level comes from the GameMode's UGSXPCurve (DA_XPCurve).
 * A big gain can level up several times; leftover XP carries over. Events fire on server and clients
 * (OnLevelUp once per level gained, then OnXPChanged).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSProgressionComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/**
	 * Server: grant Amount XP (already multiplied by XPGain; see AGSGameMode::AwardXP). Levels up as many times as
	 * the XP covers. Returns the number of levels gained.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Progression")
	int32 AddXP(int32 Amount);

	/** Server: back to level 1 with 0 XP (new run). */
	void ResetProgression();

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetLevel() const { return State.Level; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCurrentXP() const { return State.CurrentXP; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetXPForNextLevel() const { return State.XPForNextLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetTotalXP() const { return State.TotalXP; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetMaxLevel() const { return State.MaxLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	bool IsMaxLevel() const { return State.Level >= State.MaxLevel; }

	/** 0..1 progress towards the next level (1 at max level). */
	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetLevelProgress() const;

	const FGSProgressionState& GetState() const { return State; }

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FGSOnXPChangedSignature OnXPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FGSOnLevelUpSignature OnLevelUp;

protected:
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_State, Category = "Progression")
	FGSProgressionState State;

private:
	UFUNCTION()
	void OnRep_State(const FGSProgressionState& OldState);

	/** The server's XP curve (AGSGameMode). nullptr on clients or if unset. */
	const UGSXPCurve* GetXPCurve() const;

	/** Recompute MaxLevel / XPForNextLevel from the curve (server). */
	void RefreshFromCurve(const UGSXPCurve* Curve);

	void BroadcastChanges(const FGSProgressionState& OldState, int32 Delta);
};
