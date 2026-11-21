// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GSProfileSaveGame.generated.h"

/** Permanent data of one player, kept in the host's save. */
USTRUCT(BlueprintType)
struct FGSProfilePlayerEntry
{
	GENERATED_BODY()

	/** Last known player name (for logs / a future roster). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Profile")
	FString DisplayName;

	/** Permanent cash: earned in runs, spent / gambled in the hub. Kept across runs and deaths. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Profile")
	int32 Cash = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Profile")
	FDateTime LastSeen;
};

/**
 * The profile file of one machine (slot "GSProfile"), read and written only by UGSProfileSubsystem.
 * - LocalPlayerId: this machine's stable player ID (generated once). Clients send it to the host when they join.
 * - Players: permanent data per player ID. Only the host's file matters for co-op: it holds every player who played
 *   with that host. A client's own file only provides its LocalPlayerId.
 */
UCLASS()
class BANDITBALLER_API UGSProfileSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY(SaveGame)
	int32 Version = CurrentVersion;

	UPROPERTY(SaveGame)
	FGuid LocalPlayerId;

	/** Key: player ID (FGuid string, EGuidFormats::Digits). */
	UPROPERTY(SaveGame)
	TMap<FString, FGSProfilePlayerEntry> Players;
};
