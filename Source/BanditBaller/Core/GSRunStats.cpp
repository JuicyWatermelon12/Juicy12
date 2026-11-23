// BanditBaller

#include "GSRunStats.h"
#include "GameFramework/GameStateBase.h"
#include "GSGameplayTags.h"
#include "Net/UnrealNetwork.h"

EGSDamageSource GSDamageSource::FromTags(const FGameplayTagContainer& Tags)
{
	if (Tags.HasTagExact(GSGameplayTags::Damage_Source_Gun)) { return EGSDamageSource::Gun; }
	if (Tags.HasTagExact(GSGameplayTags::Damage_Source_FloatingGun)) { return EGSDamageSource::FloatingGun; }
	if (Tags.HasTagExact(GSGameplayTags::Damage_Source_RazorCards)) { return EGSDamageSource::RazorCards; }
	if (Tags.HasTagExact(GSGameplayTags::Damage_Source_BarbedWire)) { return EGSDamageSource::BarbedWire; }
	if (Tags.HasTagExact(GSGameplayTags::Damage_Source_PowderKeg)) { return EGSDamageSource::PowderKeg; }
	if (Tags.HasTagExact(GSGameplayTags::Damage_Source_Dynamite)) { return EGSDamageSource::Dynamite; }
	if (Tags.HasTagExact(GSGameplayTags::Damage_Source_LuckyCoin)) { return EGSDamageSource::LuckyCoin; }
	if (Tags.HasTagExact(GSGameplayTags::Damage_Source_Molotov)) { return EGSDamageSource::Molotov; }
	if (Tags.HasTagExact(GSGameplayTags::Damage_Source_Tomahawk)) { return EGSDamageSource::Tomahawk; }
	return EGSDamageSource::Other;
}

const TCHAR* GSDamageSource::GetName(EGSDamageSource Source)
{
	switch (Source)
	{
	case EGSDamageSource::Gun: return TEXT("rifle/pistol");
	case EGSDamageSource::FloatingGun: return TEXT("Floating Gun");
	case EGSDamageSource::RazorCards: return TEXT("Razor Cards");
	case EGSDamageSource::BarbedWire: return TEXT("Barbed Wire");
	case EGSDamageSource::PowderKeg: return TEXT("Powder Keg");
	case EGSDamageSource::Dynamite: return TEXT("Dynamite");
	case EGSDamageSource::LuckyCoin: return TEXT("Lucky Coin");
	case EGSDamageSource::Molotov: return TEXT("Molotov");
	case EGSDamageSource::Tomahawk: return TEXT("Tomahawk");
	default: return TEXT("other");
	}
}

FString UGSRunStats::DescribeDamageBySource(const float* BySource) const
{
	FString Text;
	for (int32 Index = 0; Index < GSDamageSource::Num; ++Index)
	{
		if (BySource[Index] > 0.f)
		{
			Text += FString::Printf(TEXT("%s%s %.0f"), Text.IsEmpty() ? TEXT("") : TEXT(", "), GSDamageSource::GetName(static_cast<EGSDamageSource>(Index)), BySource[Index]);
		}
	}
	return Text.IsEmpty() ? FString(TEXT("none")) : Text;
}

UGSRunStats::UGSRunStats()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGSRunStats::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGSRunStats, RoundsCleared);
	DOREPLIFETIME(UGSRunStats, TotalKills);
	DOREPLIFETIME(UGSRunStats, KillsByType);
	DOREPLIFETIME(UGSRunStats, CashEarned);
	DOREPLIFETIME(UGSRunStats, CashSpent);
	DOREPLIFETIME(UGSRunStats, DamageDealt);
	DOREPLIFETIME(UGSRunStats, DamageTaken);
	DOREPLIFETIME(UGSRunStats, XPEarned);
	DOREPLIFETIME(UGSRunStats, LevelReached);
	DOREPLIFETIME(UGSRunStats, RunStartTime);
	DOREPLIFETIME(UGSRunStats, RunEndTime);
}

bool UGSRunStats::CanWrite() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

void UGSRunStats::Changed()
{
	OnStatsChanged.Broadcast();
}

void UGSRunStats::OnRep_Stats()
{
	OnStatsChanged.Broadcast();
}

void UGSRunStats::ResetStats(double InRunStartTime)
{
	if (!CanWrite())
	{
		return;
	}
	RoundsCleared = 0;
	TotalKills = 0;
	KillsByType.Reset();
	CashEarned = 0;
	CashSpent = 0;
	DamageDealt = 0.f;
	DamageTaken = 0.f;
	XPEarned = 0;
	LevelReached = 1;
	BestCombo = 0;
	HealthOrbsCollected = 0;
	HealthHealedByOrbs = 0.f;
	HealthOrbsDropped = 0;
	FMemory::Memzero(DamageDealtBySource);
	Rounds.Reset();
	LastRoundClearPayout = FGSRoundClearPayout();
	RunStartTime = InRunStartTime;
	RunEndTime = 0.0;
	Changed();
}

const FGSRoundStats* UGSRunStats::FindRound(int32 Round) const
{
	// The newest record of that number (gs.Round.Set can replay a round).
	for (int32 Index = Rounds.Num() - 1; Index >= 0; --Index)
	{
		if (Rounds[Index].Round == Round)
		{
			return &Rounds[Index];
		}
	}
	return nullptr;
}

const FGSRoundStats& UGSRunStats::GetCurrentRound() const
{
	static const FGSRoundStats Empty;
	return Rounds.IsEmpty() ? Empty : Rounds.Last();
}

int32 UGSRunStats::ResolveRound(int32 Round) const
{
	if (Round > 0 && FindRound(Round))
	{
		return Round;
	}
	return Rounds.IsEmpty() ? 0 : Rounds.Last().Round;
}

FGSRoundStats* UGSRunStats::FindRoundForStats(int32 Round)
{
	const int32 Resolved = ResolveRound(Round);
	return Resolved > 0 ? const_cast<FGSRoundStats*>(FindRound(Resolved)) : nullptr;
}

TArray<int32> UGSRunStats::GetOpenRounds() const
{
	TArray<int32> Open;
	for (const FGSRoundStats& Record : Rounds)
	{
		if (Record.bOpen)
		{
			Open.Add(Record.Round);
		}
	}
	return Open;
}

void UGSRunStats::RecordRoundCleared(int32 Round)
{
	if (!CanWrite())
	{
		return;
	}
	++RoundsCleared;
	if (FGSRoundStats* Record = const_cast<FGSRoundStats*>(FindRound(Round)))
	{
		Record->bCleared = true;
		Record->bOpen = false;
		const AGameStateBase* GameState = Cast<AGameStateBase>(GetOwner());
		Record->ClearedAt = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0;
	}
	Changed();
}

void UGSRunStats::RecordKill(FName EnemyType, int32 Round)
{
	if (!CanWrite())
	{
		return;
	}
	++TotalKills;
	if (FGSRoundStats* Record = FindRoundForStats(Round))
	{
		++Record->Kills;
	}
	FGSEnemyKillCount* Entry = KillsByType.FindByPredicate([EnemyType](const FGSEnemyKillCount& Kill) { return Kill.EnemyType == EnemyType; });
	if (!Entry)
	{
		Entry = &KillsByType.AddDefaulted_GetRef();
		Entry->EnemyType = EnemyType;
	}
	++Entry->Count;
	Changed();
}

void UGSRunStats::AddCashEarned(int32 Amount, int32 Round)
{
	if (CanWrite() && Amount > 0)
	{
		CashEarned += Amount;
		if (FGSRoundStats* Record = FindRoundForStats(Round))
		{
			Record->CashCollected += Amount;
		}
		Changed();
	}
}

void UGSRunStats::AddCashSpent(int32 Amount)
{
	if (CanWrite() && Amount > 0)
	{
		CashSpent += Amount;
		Changed();
	}
}

void UGSRunStats::AddDamageDealt(float Amount, EGSDamageSource Source, int32 Round)
{
	if (CanWrite() && Amount > 0.f)
	{
		const int32 Index = FMath::Clamp(static_cast<int32>(Source), 0, GSDamageSource::Num - 1);
		DamageDealt += Amount;
		DamageDealtBySource[Index] += Amount;
		if (FGSRoundStats* Record = FindRoundForStats(Round))
		{
			Record->DamageBySource[Index] += Amount;
		}
		Changed();
	}
}

void UGSRunStats::RecordHealthOrbDropped(int32 Round)
{
	if (CanWrite())
	{
		++HealthOrbsDropped;
		if (FGSRoundStats* Record = FindRoundForStats(Round))
		{
			++Record->HealthOrbsDropped;
		}
		Changed();
	}
}

void UGSRunStats::AddDamageTaken(float Amount)
{
	if (CanWrite() && Amount > 0.f)
	{
		DamageTaken += Amount;
		// Every open round: a hit while two rounds overlap costs both their no-damage bonus.
		for (FGSRoundStats& Record : Rounds)
		{
			if (Record.bOpen)
			{
				Record.DamageTaken += Amount;
			}
		}
		Changed();
	}
}

void UGSRunStats::AddXPEarned(int32 Amount, int32 Round)
{
	if (CanWrite() && Amount > 0)
	{
		XPEarned = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(XPEarned) + Amount, MAX_int32));
		if (FGSRoundStats* Record = FindRoundForStats(Round))
		{
			Record->XPEarned = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(Record->XPEarned) + Amount, MAX_int32));
		}
		Changed();
	}
}

void UGSRunStats::BeginRound(int32 Round)
{
	if (!CanWrite() || Round <= 0)
	{
		return;
	}
	if (FGSRoundStats* Existing = const_cast<FGSRoundStats*>(FindRound(Round)); Existing && Existing->bOpen && !Existing->bCleared
		&& Existing->CombatStartTime < 0.0)
	{
		Changed(); // already opened (round 1's PreRound) and not started yet: keep it
		return;
	}
	FGSRoundStats& Record = Rounds.AddDefaulted_GetRef();
	Record.Round = Round;
	Record.bOpen = true;
	Changed();
}

void UGSRunStats::RecordCombatStarted(int32 Round, double Time, int32 WaveEnemies)
{
	if (!CanWrite())
	{
		return;
	}
	FGSRoundStats* Record = const_cast<FGSRoundStats*>(FindRound(Round));
	if (!Record || !Record->bOpen)
	{
		BeginRound(Round);
		Record = const_cast<FGSRoundStats*>(FindRound(Round));
	}
	if (Record)
	{
		Record->CombatStartTime = Time;
		Record->WaveEnemies = WaveEnemies;
	}
	Changed();
}

void UGSRunStats::CloseOpenRounds()
{
	if (!CanWrite())
	{
		return;
	}
	for (FGSRoundStats& Record : Rounds)
	{
		Record.bOpen = false;
	}
	Changed();
}

void UGSRunStats::RecordRoundClearPayout(const FGSRoundClearPayout& Payout)
{
	if (CanWrite())
	{
		LastRoundClearPayout = Payout;
		if (FGSRoundStats* Record = const_cast<FGSRoundStats*>(FindRound(Payout.Round)))
		{
			Record->Payout = Payout;
		}
		CashEarned += Payout.GetTotal();
		Changed();
		OnRoundClearPaid.Broadcast(Payout);
	}
}

void UGSRunStats::RecordLevelReached(int32 Level)
{
	if (CanWrite() && Level > LevelReached)
	{
		LevelReached = Level;
		Changed();
	}
}

void UGSRunStats::RecordCombo(int32 Count)
{
	if (CanWrite() && Count > BestCombo)
	{
		BestCombo = Count;
		Changed();
	}
}

void UGSRunStats::RecordHealthOrbCollected(float Healed, int32 Round)
{
	if (CanWrite())
	{
		++HealthOrbsCollected;
		HealthHealedByOrbs += FMath::Max(Healed, 0.f);
		if (FGSRoundStats* Record = FindRoundForStats(Round))
		{
			++Record->HealthOrbsCollected;
			Record->HealthHealedByOrbs += FMath::Max(Healed, 0.f);
		}
		Changed();
	}
}

void UGSRunStats::EndRun(double InRunEndTime)
{
	if (CanWrite() && RunEndTime <= 0.0)
	{
		RunEndTime = InRunEndTime;
		Changed();
	}
}

int32 UGSRunStats::GetKillsOf(FName EnemyType) const
{
	const FGSEnemyKillCount* Entry = KillsByType.FindByPredicate([EnemyType](const FGSEnemyKillCount& Kill) { return Kill.EnemyType == EnemyType; });
	return Entry ? Entry->Count : 0;
}

float UGSRunStats::GetRunTime() const
{
	if (RunEndTime > 0.0)
	{
		return static_cast<float>(RunEndTime - RunStartTime);
	}
	const AGameStateBase* GameState = Cast<AGameStateBase>(GetOwner());
	const double Now = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0;
	return FMath::Max(0.f, static_cast<float>(Now - RunStartTime));
}

FString UGSRunStats::ToDebugString() const
{
	const float Seconds = GetRunTime();
	FString Kills;
	for (const FGSEnemyKillCount& Kill : KillsByType)
	{
		Kills += FString::Printf(TEXT("%s%s %d"), Kills.IsEmpty() ? TEXT("") : TEXT(", "), *Kill.EnemyType.ToString(), Kill.Count);
	}
	const FGSRoundClearPayout& Payout = LastRoundClearPayout;
	const FString RoundLines = FString::Printf(
		TEXT("\nThis round:     round %d, %d kills, $%d collected, %d XP, %.0f damage taken\n")
		TEXT("Last clear bonus: %s"),
		GetCurrentRound().Round, GetCurrentRound().Kills, GetCurrentRound().CashCollected, GetCurrentRound().XPEarned, GetCurrentRound().DamageTaken,
		Payout.Round > 0
			? *FString::Printf(TEXT("round %d, $%d (base %d, no damage %d, fast clear %d; %.1f s of %.1f s)"), Payout.Round, Payout.GetTotal(),
				Payout.BaseBonus, Payout.NoDamageBonus, Payout.FastClearBonus, Payout.ClearTime, Payout.FastClearTime)
			: TEXT("none yet"));
	return FString::Printf(
		TEXT("Run time:       %d:%02d (%.1f s)%s\n")
		TEXT("Rounds cleared: %d\n")
		TEXT("Kills:          %d (%s)\n")
		TEXT("Cash earned:    %d\n")
		TEXT("Cash spent:     %d\n")
		TEXT("Damage dealt:   %.0f\n")
		TEXT("Damage taken:   %.0f\n")
		TEXT("XP earned:      %d\n")
		TEXT("Level reached:  %d\n")
		TEXT("Best combo:     %d\n")
		TEXT("Damage by source: %s\n")
		TEXT("Health orbs:    %d dropped, %d collected, %.0f health healed%s"),
		FMath::FloorToInt(Seconds / 60.f), FMath::FloorToInt(FMath::Fmod(Seconds, 60.f)), Seconds, RunEndTime > 0.0 ? TEXT(" [ended]") : TEXT(""),
		RoundsCleared, TotalKills, Kills.IsEmpty() ? TEXT("none") : *Kills, CashEarned, CashSpent, DamageDealt, DamageTaken, XPEarned, LevelReached,
		BestCombo, *DescribeDamageBySource(DamageDealtBySource), HealthOrbsDropped, HealthOrbsCollected, HealthHealedByOrbs, *RoundLines);
}
