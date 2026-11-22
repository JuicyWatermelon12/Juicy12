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
	CurrentRound = FGSRoundStats();
	LastRoundClearPayout = FGSRoundClearPayout();
	RunStartTime = InRunStartTime;
	RunEndTime = 0.0;
	Changed();
}

void UGSRunStats::RecordRoundCleared()
{
	if (CanWrite())
	{
		++RoundsCleared;
		Changed();
	}
}

void UGSRunStats::RecordKill(FName EnemyType)
{
	if (!CanWrite())
	{
		return;
	}
	++TotalKills;
	++CurrentRound.Kills;
	FGSEnemyKillCount* Entry = KillsByType.FindByPredicate([EnemyType](const FGSEnemyKillCount& Kill) { return Kill.EnemyType == EnemyType; });
	if (!Entry)
	{
		Entry = &KillsByType.AddDefaulted_GetRef();
		Entry->EnemyType = EnemyType;
	}
	++Entry->Count;
	Changed();
}

void UGSRunStats::AddCashEarned(int32 Amount)
{
	if (CanWrite() && Amount > 0)
	{
		CashEarned += Amount;
		CurrentRound.CashCollected += Amount;
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

void UGSRunStats::AddDamageDealt(float Amount, EGSDamageSource Source)
{
	if (CanWrite() && Amount > 0.f)
	{
		const int32 Index = FMath::Clamp(static_cast<int32>(Source), 0, GSDamageSource::Num - 1);
		DamageDealt += Amount;
		DamageDealtBySource[Index] += Amount;
		CurrentRound.DamageBySource[Index] += Amount;
		Changed();
	}
}

void UGSRunStats::RecordHealthOrbDropped()
{
	if (CanWrite())
	{
		++HealthOrbsDropped;
		++CurrentRound.HealthOrbsDropped;
		Changed();
	}
}

void UGSRunStats::AddDamageTaken(float Amount)
{
	if (CanWrite() && Amount > 0.f)
	{
		DamageTaken += Amount;
		CurrentRound.DamageTaken += Amount;
		Changed();
	}
}

void UGSRunStats::AddXPEarned(int32 Amount)
{
	if (CanWrite() && Amount > 0)
	{
		XPEarned = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(XPEarned) + Amount, MAX_int32));
		CurrentRound.XPEarned = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(CurrentRound.XPEarned) + Amount, MAX_int32));
		Changed();
	}
}

void UGSRunStats::BeginRound(int32 Round)
{
	if (CanWrite())
	{
		CurrentRound = FGSRoundStats();
		CurrentRound.Round = Round;
		Changed();
	}
}

void UGSRunStats::RecordCombatStarted(double Time, int32 WaveEnemies)
{
	if (CanWrite())
	{
		CurrentRound.CombatStartTime = Time;
		CurrentRound.WaveEnemies = WaveEnemies;
		Changed();
	}
}

void UGSRunStats::RecordRoundClearPayout(const FGSRoundClearPayout& Payout)
{
	if (CanWrite())
	{
		LastRoundClearPayout = Payout;
		CashEarned += Payout.GetTotal();
		Changed();
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

void UGSRunStats::RecordHealthOrbCollected(float Healed)
{
	if (CanWrite())
	{
		++HealthOrbsCollected;
		HealthHealedByOrbs += FMath::Max(Healed, 0.f);
		++CurrentRound.HealthOrbsCollected;
		CurrentRound.HealthHealedByOrbs += FMath::Max(Healed, 0.f);
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
		CurrentRound.Round, CurrentRound.Kills, CurrentRound.CashCollected, CurrentRound.XPEarned, CurrentRound.DamageTaken,
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
