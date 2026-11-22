// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSHubGameMode.generated.h"

class UGameplayEffect;

/**
 * GameMode of the hub map (L_Hub_Graybox, BP_GSHubGameMode via its World Settings): the safe Market District between
 * runs. Single-player for now.
 * - No round loop: no phases, waves, enemies, drops or run seed (AGSGameState stays in WaitingToStart, round 0).
 * - CurrentArea = MarketDistrict from the start (the HUD hides its combat elements there; spawning is refused).
 * - Every player gets SafeZoneEffect (GE_SafeZone: State.SafeZone), so GA_Fire can't activate and damage effects are
 *   ignored.
 * - The permanent cash balance (UGSProfileSubsystem) is the same as in the town; the HUD shows it.
 * - Start Run (AGSStartRunPortal, gs.Market.Return): fade out, then open RunMap; the run starts when it loads.
 * Shop and casino come later (AGSGameMode runs the town).
 */
UCLASS()
class BANDITBALLER_API AGSHubGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSHubGameMode();

	virtual void StartPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	TSubclassOf<UGameplayEffect> GetSafeZoneEffect() const { return SafeZoneEffect; }

	// --- Start a run ---

	/**
	 * Requester asked to start a run (walked into the portal): fade every player out over StartRunFadeTime, then open
	 * RunMap (single-player: OpenLevel, with ?GSSeed=N when gs.Run.Seed chose a seed). False if a start is already
	 * under way or RunMap is unset. TODO(co-op): ServerTravel so the whole party goes together (M6).
	 */
	bool StartRun(APlayerController* Requester);

	bool IsStartingRun() const { return bStartingRun; }

	/** Debug (gs.Run.Seed N in the hub): the next run uses Seed; 0 = a random seed. */
	void SetNextRunSeed(int32 Seed);
	int32 GetNextRunSeed() const { return NextRunSeed; }

	const TSoftObjectPtr<UWorld>& GetRunMap() const { return RunMap; }

protected:
	/** GE_SafeZone (UGSSafeZoneEffect child), applied once to every player's ASC. */
	UPROPERTY(EditDefaultsOnly, Category = "Hub")
	TSubclassOf<UGameplayEffect> SafeZoneEffect;

	/** The run map the portal opens (L_Town_Graybox). */
	UPROPERTY(EditDefaultsOnly, Category = "Start Run")
	TSoftObjectPtr<UWorld> RunMap;

	/** Fade to black before the run map opens. */
	UPROPERTY(EditDefaultsOnly, Category = "Start Run", meta = (ClampMin = "0", Units = "s"))
	float StartRunFadeTime = 0.5f;

private:
	/** Apply SafeZoneEffect to Player's ability system unless it already has State.SafeZone. */
	void ApplySafeZone(AController* Player) const;

	void OpenRunMap();

	FTimerHandle StartRunTimer;
	bool bStartingRun = false;
	int32 NextRunSeed = 0;
};
