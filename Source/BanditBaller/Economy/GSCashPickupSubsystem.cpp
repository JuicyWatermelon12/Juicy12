// BanditBaller

#include "GSCashPickupSubsystem.h"
#include "BanditBaller.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GSCashPickup.h"
#include "GSCashSettings.h"
#include "GSGameMode.h"
#include "GSHealthComponent.h"
#include "GSRunRandom.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<float> CVarCashLifetime(
		TEXT("gs.Cash.Lifetime"), 0.f,
		TEXT("Seconds an uncollected cash pickup stays before it vanishes (0 = DA_CashSettings Lifetime)."),
		ECVF_Cheat);

	/** Height above the drop origin the ground trace starts from, and how far down it looks. */
	constexpr float GroundTraceUp = 100.f;
	constexpr float GroundTraceDown = 1000.f;
	/** Pieces stop this far short of a wall they were tossed into. */
	constexpr float WallPadding = 20.f;

	bool IsLivingPlayerPawn(const APawn* Pawn)
	{
		const UGSHealthComponent* Health = UGSHealthComponent::FindHealthComponent(Pawn);
		return Pawn && (!Health || !Health->IsDead());
	}
}

bool UGSCashPickupSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGSCashPickupSubsystem::Deinitialize()
{
	Pool.Reset();
	Active.Reset();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Pool
// ---------------------------------------------------------------------------

AGSCashPickup* UGSCashPickupSubsystem::CreatePickup()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AGSCashPickup* Pickup = GetWorld()->SpawnActor<AGSCashPickup>(AGSCashPickup::StaticClass(), FTransform::Identity, Params);
	if (Pickup)
	{
		++NumCreated;
	}
	return Pickup;
}

void UGSCashPickupSubsystem::Prewarm(int32 Count)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (AGSCashPickup* Pickup = CreatePickup())
		{
			Pool.Add(Pickup);
		}
	}
}

AGSCashPickup* UGSCashPickupSubsystem::AcquirePickup()
{
	while (Pool.Num() > 0)
	{
		AGSCashPickup* Pickup = Pool.Pop(EAllowShrinking::No);
		if (IsValid(Pickup))
		{
			return Pickup;
		}
	}
	return CreatePickup(); // pool exhausted: grow
}

void UGSCashPickupSubsystem::ReleasePickup(AGSCashPickup* Pickup, bool bExpired)
{
	if (!Pickup || Active.RemoveSingleSwap(Pickup, EAllowShrinking::No) == 0)
	{
		return;
	}
	if (bExpired && Pickup->IsXPOrb())
	{
		++NumXPOrbsExpired;
	}
	else if (bExpired && Pickup->IsHealthOrb())
	{
		++NumHealthOrbsExpired;
	}
	else if (bExpired)
	{
		++NumExpired;
		ValueExpired += Pickup->GetValue();
	}
	Pickup->DeactivatePickup();
	Pool.Add(Pickup);
}

void UGSCashPickupSubsystem::ReleaseAll()
{
	const TArray<TObjectPtr<AGSCashPickup>> ToRelease = Active;
	for (AGSCashPickup* Pickup : ToRelease)
	{
		ReleasePickup(Pickup, /*bExpired*/ false);
	}
}

float UGSCashPickupSubsystem::GetLifetime(EGSPickupKind Kind) const
{
	if (Kind == EGSPickupKind::XP)
	{
		return LastSettings ? FMath::Max(LastSettings->XPOrbLifetime, 0.f) : 0.f;
	}
	if (Kind == EGSPickupKind::Health)
	{
		return LastSettings ? FMath::Max(LastSettings->HealthOrbLifetime, 0.f) : 0.f;
	}
	const float Override = CVarCashLifetime.GetValueOnGameThread();
	if (Override > 0.f)
	{
		return Override;
	}
	return LastSettings ? LastSettings->Lifetime : 20.f;
}

double UGSCashPickupSubsystem::GetExpireAt(EGSPickupKind Kind) const
{
	const float Lifetime = GetLifetime(Kind);
	if (Lifetime <= 0.f)
	{
		return 0.0; // never expires
	}
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	const double Now = GameState ? GameState->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
	return Now + Lifetime;
}

int32 UGSCashPickupSubsystem::GetNumActiveOfKind(EGSPickupKind Kind) const
{
	int32 Count = 0;
	for (const AGSCashPickup* Pickup : Active)
	{
		Count += (Pickup && Pickup->GetKind() == Kind) ? 1 : 0;
	}
	return Count;
}

void UGSCashPickupSubsystem::PrewarmOnce(const UGSCashSettings& Settings)
{
	if (!bPrewarmed)
	{
		bPrewarmed = true;
		Prewarm(Settings.PoolPrewarm);
	}
}

// ---------------------------------------------------------------------------
// Drops
// ---------------------------------------------------------------------------

void UGSCashPickupSubsystem::SplitValue(const UGSCashSettings& Settings, int32 TotalValue, TArray<int32>& OutPieces)
{
	OutPieces.Reset();
	if (TotalValue <= 0)
	{
		return;
	}

	const int32 MinPieces = FMath::Max(Settings.MinPieces, 1);
	const int32 MaxPieces = FMath::Max(Settings.MaxPieces, MinPieces);
	int32 Count = FMath::RoundToInt(static_cast<float>(TotalValue) / FMath::Max(Settings.TargetPieceValue, 1));
	Count = FMath::Clamp(Count, MinPieces, MaxPieces);
	Count = FMath::Min(Count, TotalValue); // every piece is worth at least 1

	// Even split; the remainder goes to the first pieces (largest first).
	const int32 Base = TotalValue / Count;
	const int32 Remainder = TotalValue % Count;
	OutPieces.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		OutPieces.Add(Base + (Index < Remainder ? 1 : 0));
	}
}

FVector UGSCashPickupSubsystem::FindLanding(const UGSCashSettings& Settings, const FVector& Origin, const FVector& Target) const
{
	const UWorld* World = GetWorld();
	FCollisionObjectQueryParams StaticOnly(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSCashLanding), false);

	// Don't toss through walls / cover: stop short of the first static hit.
	FVector Flat = FVector(Target.X, Target.Y, Origin.Z);
	FHitResult WallHit;
	if (World->LineTraceSingleByObjectType(WallHit, Origin, Flat, StaticOnly, Params))
	{
		const FVector Dir = (Flat - Origin).GetSafeNormal();
		Flat = WallHit.Location - Dir * WallPadding;
	}

	// Rest just above the floor below.
	FHitResult GroundHit;
	const FVector Start = Flat + FVector(0.f, 0.f, GroundTraceUp);
	const FVector End = Flat - FVector(0.f, 0.f, GroundTraceDown);
	if (World->LineTraceSingleByObjectType(GroundHit, Start, End, StaticOnly, Params))
	{
		return GroundHit.Location + FVector(0.f, 0.f, Settings.RestHeight);
	}
	return Flat;
}

int32 UGSCashPickupSubsystem::SpawnCashDrop(const UGSCashSettings* Settings, const FVector& Origin, int32 TotalValue)
{
	UWorld* World = GetWorld();
	if (!Settings || TotalValue <= 0 || !World || World->GetNetMode() == NM_Client)
	{
		return 0;
	}
	LastSettings = Settings;
	PrewarmOnce(*Settings);

	TArray<int32> Pieces;
	SplitValue(*Settings, TotalValue, Pieces);

	const double ExpireAt = GetExpireAt(EGSPickupKind::Cash);

	// Spread pieces around the body: even angles with jitter, random radius (run stream).
	const float StartAngle = UGSRunRandom::FRandRange(this, 0.f, UE_TWO_PI);
	for (int32 Index = 0; Index < Pieces.Num(); ++Index)
	{
		const float Angle = StartAngle + UE_TWO_PI * Index / Pieces.Num() + UGSRunRandom::FRandRange(this, -0.35f, 0.35f);
		const float Radius = UGSRunRandom::FRandRange(this, Settings->ScatterRadiusMin, FMath::Max(Settings->ScatterRadiusMin, Settings->ScatterRadiusMax));
		const FVector Target = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
		const FVector Landing = FindLanding(*Settings, Origin, Target);

		AGSCashPickup* Pickup = AcquirePickup();
		if (!Pickup)
		{
			UE_LOG(LogBanditBaller, Error, TEXT("[cash] could not create a cash pickup."));
			break;
		}
		const bool bBill = Pieces[Index] >= Settings->BillThreshold;
		Pickup->ActivatePickup(EGSPickupKind::Cash, Pieces[Index], bBill, Settings, Origin, Landing, ExpireAt);
		Active.Add(Pickup);
	}

	UE_LOG(LogBanditBaller, Verbose, TEXT("[cash] drop $%d in %d pieces at %s (active %d, pooled %d)"),
		TotalValue, Pieces.Num(), *Origin.ToCompactString(), Active.Num(), Pool.Num());
	return Pieces.Num();
}

AGSCashPickup* UGSCashPickupSubsystem::SpawnXPDrop(const UGSCashSettings* Settings, const FVector& Origin, int32 XPValue)
{
	UWorld* World = GetWorld();
	if (!Settings || XPValue <= 0 || !World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	LastSettings = Settings;
	PrewarmOnce(*Settings);

	const FVector Landing = RollOrbLanding(*Settings, Origin);
	AGSCashPickup* Orb = AcquirePickup();
	if (!Orb)
	{
		UE_LOG(LogBanditBaller, Error, TEXT("[xp] could not create an XP orb."));
		return nullptr;
	}
	Orb->ActivatePickup(EGSPickupKind::XP, XPValue, false, Settings, Origin, Landing, GetExpireAt(EGSPickupKind::XP));
	Active.Add(Orb);
	++NumXPOrbsDropped;
	UE_LOG(LogBanditBaller, Verbose, TEXT("[xp] orb %d XP dropped at %s"), XPValue, *Origin.ToCompactString());
	return Orb;
}

FVector UGSCashPickupSubsystem::RollOrbLanding(const UGSCashSettings& Settings, const FVector& Origin)
{
	// Tossed like a cash piece, to a random spot around the body (run stream).
	const float Angle = UGSRunRandom::FRandRange(this, 0.f, UE_TWO_PI);
	const float Radius = UGSRunRandom::FRandRange(this, Settings.ScatterRadiusMin, FMath::Max(Settings.ScatterRadiusMin, Settings.ScatterRadiusMax));
	const FVector Target = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
	return FindLanding(Settings, Origin, Target);
}

AGSCashPickup* UGSCashPickupSubsystem::SpawnHealthDrop(const UGSCashSettings* Settings, const FVector& Origin)
{
	UWorld* World = GetWorld();
	if (!Settings || Settings->HealthOrbHealAmount <= 0 || !World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	LastSettings = Settings;
	PrewarmOnce(*Settings);

	const FVector Landing = RollOrbLanding(*Settings, Origin);
	AGSCashPickup* Orb = AcquirePickup();
	if (!Orb)
	{
		UE_LOG(LogBanditBaller, Error, TEXT("[heal] could not create a health orb."));
		return nullptr;
	}
	Orb->ActivatePickup(EGSPickupKind::Health, Settings->HealthOrbHealAmount, false, Settings, Origin, Landing, GetExpireAt(EGSPickupKind::Health));
	Active.Add(Orb);
	++NumHealthOrbsDropped;
	UE_LOG(LogBanditBaller, Verbose, TEXT("[heal] health orb +%d dropped at %s"), Settings->HealthOrbHealAmount, *Origin.ToCompactString());
	return Orb;
}

// ---------------------------------------------------------------------------
// Collection
// ---------------------------------------------------------------------------

void UGSCashPickupSubsystem::CollectPickup(AGSCashPickup* Pickup, APlayerController* Player)
{
	if (!Pickup || !Active.Contains(Pickup))
	{
		return;
	}
	const int32 Value = Pickup->GetValue();
	const FVector Location = Pickup->GetActorLocation();
	const bool bXPOrb = Pickup->IsXPOrb();
	const bool bHealthOrb = Pickup->IsHealthOrb();
	ReleasePickup(Pickup, /*bExpired*/ false);

	AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>();
	if (bHealthOrb)
	{
		++NumHealthOrbsCollected;
		LastHealthCollectLocation = Location;
		if (GameMode)
		{
			GameMode->AwardHealthPickup(Player, Value, Location);
		}
		return;
	}
	if (bXPOrb)
	{
		++NumXPOrbsCollected;
		LastXPCollectLocation = Location;
		if (GameMode)
		{
			GameMode->AwardXPPickup(Player, Value, Location);
		}
		return;
	}

	++NumCollected;
	ValueCollected += Value;
	if (GameMode)
	{
		GameMode->AwardCash(Player, Value, Location);
	}
}

int32 UGSCashPickupSubsystem::VacuumAll()
{
	TArray<APawn*> Players;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr;
		if (IsLivingPlayerPawn(Pawn))
		{
			Players.Add(Pawn);
		}
	}
	if (Players.Num() == 0)
	{
		return 0;
	}

	int32 Sent = 0;
	for (AGSCashPickup* Pickup : Active)
	{
		if (!Pickup)
		{
			continue;
		}
		if (Pickup->IsMagnetized())
		{
			Pickup->ForceMagnet(); // a health orb already flying in is collected even if its player is now full
			continue;
		}
		APawn* Nearest = nullptr;
		float NearestDistanceSq = TNumericLimits<float>::Max();
		for (APawn* Pawn : Players)
		{
			const float DistanceSq = FVector::DistSquared(Pawn->GetActorLocation(), Pickup->GetActorLocation());
			if (DistanceSq < NearestDistanceSq)
			{
				Nearest = Pawn;
				NearestDistanceSq = DistanceSq;
			}
		}
		Pickup->StartMagnet(Nearest, /*bForced*/ true); // health orbs come in even at full health (the excess is lost)
		++Sent;
	}
	if (Sent > 0)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("[cash] round clear: %d pickups fly to the party"), Sent);
	}
	return Sent;
}
