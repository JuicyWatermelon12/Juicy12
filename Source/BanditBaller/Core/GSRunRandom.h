// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GSRunRandom.generated.h"

/**
 * The only way gameplay code rolls dice. Every roll draws from the run's seeded FRandomStream owned by
 * AGSGameMode (seed logged at run start; fixed with -GSSeed=N or gs.Run.Seed <N>), so a run can be replayed.
 *
 * Server only: the GameMode (and its stream) exists only on the server. Never use FMath::Rand / FRand /
 * RandRange / VRandCone for gameplay. Exceptions, by design: per-machine cosmetics (HUD jitter) and the
 * player's own weapon spread/recoil, which are rolled where the shot is predicted.
 */
UCLASS()
class BANDITBALLER_API UGSRunRandom : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The run stream for this world (a fallback stream, with a warning, if there is no AGSGameMode). */
	static FRandomStream& GetStream(const UObject* WorldContextObject);

	/** Integer in [Min, Max] inclusive. */
	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Random", meta = (WorldContext = "WorldContextObject"))
	static int32 RandRange(const UObject* WorldContextObject, int32 Min, int32 Max);

	/** Float in [Min, Max]. */
	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Random", meta = (WorldContext = "WorldContextObject"))
	static float FRandRange(const UObject* WorldContextObject, float Min, float Max);

	/** Float in [0, 1). */
	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Random", meta = (WorldContext = "WorldContextObject"))
	static float FRand(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Random", meta = (WorldContext = "WorldContextObject"))
	static bool RandBool(const UObject* WorldContextObject);

	/** True with probability Chance (0..1), e.g. drops, AI choices. */
	UFUNCTION(BlueprintCallable, Category = "BanditBaller|Random", meta = (WorldContext = "WorldContextObject"))
	static bool RollChance(const UObject* WorldContextObject, float Chance);

	/** Random unit vector within a cone of HalfAngleRadians around Direction (spread). */
	static FVector VRandCone(const UObject* WorldContextObject, const FVector& Direction, float HalfAngleRadians);

	/**
	 * A stream for one round's wave contents (Prompt 59), seeded from the run seed and the round number. With rounds
	 * rolling over, a round starts when the player has killed enough, so the run stream is at a different point each
	 * play; this keeps the same seed giving the same waves however fast the player is. Only for what a round is made of
	 * (the purchase and its order); spawn points, drops and the AI stay on the run stream.
	 */
	static FRandomStream MakeRoundStream(const UObject* WorldContextObject, int32 Round);
};
