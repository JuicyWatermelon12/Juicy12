// BanditBaller

#include "GSRunRandom.h"
#include "BanditBaller.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GSGameMode.h"

FRandomStream& UGSRunRandom::GetStream(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr)
	{
		return GameMode->GetRunStream();
	}

	// Rolling without the server's run stream (client, or a world without AGSGameMode) breaks seeding.
	static FRandomStream Fallback(FPlatformTime::Cycles());
	static bool bWarned = false;
	if (!bWarned)
	{
		bWarned = true;
		UE_LOG(LogBanditBaller, Warning, TEXT("UGSRunRandom used without an AGSGameMode (%s); using an unseeded fallback stream."), *GetNameSafe(WorldContextObject));
	}
	return Fallback;
}

int32 UGSRunRandom::RandRange(const UObject* WorldContextObject, int32 Min, int32 Max)
{
	return GetStream(WorldContextObject).RandRange(Min, Max);
}

float UGSRunRandom::FRandRange(const UObject* WorldContextObject, float Min, float Max)
{
	return GetStream(WorldContextObject).FRandRange(Min, Max);
}

float UGSRunRandom::FRand(const UObject* WorldContextObject)
{
	return GetStream(WorldContextObject).FRand();
}

bool UGSRunRandom::RandBool(const UObject* WorldContextObject)
{
	return GetStream(WorldContextObject).FRand() < 0.5f;
}

bool UGSRunRandom::RollChance(const UObject* WorldContextObject, float Chance)
{
	return GetStream(WorldContextObject).FRand() < FMath::Clamp(Chance, 0.f, 1.f);
}

FVector UGSRunRandom::VRandCone(const UObject* WorldContextObject, const FVector& Direction, float HalfAngleRadians)
{
	return GetStream(WorldContextObject).VRandCone(Direction, HalfAngleRadians);
}

FRandomStream UGSRunRandom::MakeRoundStream(const UObject* WorldContextObject, int32 Round)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	const int32 RunSeed = GameMode ? GameMode->GetRunSeed() : 0;
	return FRandomStream(static_cast<int32>(HashCombine(GetTypeHash(RunSeed), GetTypeHash(Round * 7919 + 13))));
}
