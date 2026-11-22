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
 * Shop, casino and the "Start Run" portal come later (AGSGameMode keeps running the town).
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

protected:
	/** GE_SafeZone (UGSSafeZoneEffect child), applied once to every player's ASC. */
	UPROPERTY(EditDefaultsOnly, Category = "Hub")
	TSubclassOf<UGameplayEffect> SafeZoneEffect;

private:
	/** Apply SafeZoneEffect to Player's ability system unless it already has State.SafeZone. */
	void ApplySafeZone(AController* Player) const;
};
