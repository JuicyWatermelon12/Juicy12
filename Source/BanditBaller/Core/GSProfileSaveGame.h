// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GSProfileSaveGame.generated.h"

/**
 * The player profile on disk (slot "GSProfile"), read and written only by UGSProfileSubsystem.
 * Holds permanent progression that survives runs and deaths: today the cash balance; OwnedItems is reserved for
 * hub purchases later.
 */
UCLASS()
class BANDITBALLER_API UGSProfileSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Bump when the layout changes; UGSProfileSubsystem upgrades older saves on load. */
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY(SaveGame)
	int32 SaveVersion = CurrentVersion;

	/** Permanent cash: earned in runs, spent / gambled in the hub. */
	UPROPERTY(SaveGame)
	int32 Cash = 0;

	/** Reserved for items bought in the hub (not used yet). */
	UPROPERTY(SaveGame)
	TArray<FName> OwnedItems;
};
