// BanditBaller

#include "GSGameMode.h"
#include "BanditBaller.h"
#include "GSCharacter.h"
#include "GSGameState.h"
#include "GSHealthComponent.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSRoundSettings.h"
#include "GSEnemyCharacter.h"
#include "GSEnemyData.h"
#include "GSRunRandom.h"
#include "GSRunStats.h"
#include "GSProjectileSubsystem.h"
#include "GSSecondWindComponent.h"
#include "GSShieldComponent.h"
#include "GSRunSummaryWidget.h"
#include "GSWaveDirector.h"
#include "GameFramework/PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSCashPickupSubsystem.h"
#include "GSCashSettings.h"
#include "GSComboComponent.h"
#include "GSComboSettings.h"
#include "GSCombatSettings.h"
#include "GSDifficultyCurve.h"
#include "GSGameplayEffects.h"
#include "GSGameplayTags.h"
#include "GSWeaponDebug.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "GSProgressionComponent.h"
#include "GSUpgradeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GSProfileSubsystem.h"
#include "GSRoundClearSettings.h"
#include "GSXPCurve.h"
#include "GSLevelRewards.h"

const TCHAR* AGSGameMode::RunSeedOption = TEXT("GSSeed");

AGSGameMode::AGSGameMode()
{
	// Native defaults; BP_GSGameMode overrides these with the Blueprint subclasses.
	GameStateClass = AGSGameState::StaticClass();
	PlayerStateClass = AGSPlayerState::StaticClass();
	PlayerControllerClass = AGSPlayerController::StaticClass();
	DefaultPawnClass = AGSCharacter::StaticClass();

	WaveDirector = CreateDefaultSubobject<UGSWaveDirector>(TEXT("WaveDirector"));
	HealthOnKillEffect = UGSHealthOnKillEffect::StaticClass();
}

namespace
{
	/** A player pawn or player state (damage instigators can be either). */
	bool IsPlayerActor(const AActor* Actor)
	{
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			return Pawn->IsPlayerControlled();
		}
		return Actor && Actor->IsA<APlayerState>();
	}

	/** The AGSPlayerState of a player pawn, or Actor itself if it is one (damage instigators can be either). */
	AGSPlayerState* GetGSPlayerStateOf(AActor* Actor)
	{
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			return Pawn->GetPlayerState<AGSPlayerState>();
		}
		return Cast<AGSPlayerState>(Actor);
	}

	/** "MeleeBandit" from DA_Enemy_MeleeBandit; class name for anything without enemy data. */
	FName GetEnemyTypeName(const AActor* Enemy)
	{
		if (const AGSEnemyCharacter* EnemyCharacter = Cast<AGSEnemyCharacter>(Enemy))
		{
			if (const UGSEnemyData* Data = EnemyCharacter->GetEnemyData())
			{
				FString Name = Data->GetName();
				Name.RemoveFromStart(TEXT("DA_Enemy_"));
				return FName(*Name);
			}
		}
		return Enemy ? Enemy->GetClass()->GetFName() : NAME_None;
	}
}

const UGSCombatSettings* AGSGameMode::GetCombatSettings() const
{
	return CombatSettingsOverride ? CombatSettingsOverride.Get() : CombatSettings.Get();
}

void AGSGameMode::HandleEnemyKilled(AActor* Enemy, AActor* Killer)
{
	UE_LOG(LogBanditBaller, Log, TEXT("OnEnemyKilled: %s killed by %s"), *GetNameSafe(Enemy), *GetNameSafe(Killer));

	// Run stats count enemies the party killed.
	UGSRunStats* Stats = GetRunStats();
	if (Stats && IsPlayerActor(Killer))
	{
		Stats->RecordKill(GetEnemyTypeName(Enemy));
	}

	const AGSEnemyCharacter* EnemyCharacter = Cast<AGSEnemyCharacter>(Enemy);
	const UGSEnemyData* EnemyData = EnemyCharacter ? EnemyCharacter->GetEnemyData() : nullptr;

	// Kill combo: enemy kills (not target dummies) count for the player who made them. Counted before the drops, so
	// this kill already pays at the step it reaches, and the drop keeps that multiplier however late it is collected.
	const AGSPlayerState* KillerState = EnemyCharacter ? GetGSPlayerStateOf(Killer) : nullptr;
	UGSComboComponent* Combo = KillerState ? KillerState->GetComboComponent() : nullptr;
	const int32 ComboCount = Combo ? Combo->RegisterKill() : 0;
	const float ComboMultiplier = ComboCount > 0 ? Combo->GetCashMultiplier() : 1.f;
	if (Stats && ComboCount > 0)
	{
		Stats->RecordCombo(ComboCount);
	}

	// Leech: the killer heals its HealthOnKill (enemy kills only, like the combo; never above MaxHealth, never if dead).
	if (UAbilitySystemComponent* KillerASC = KillerState ? KillerState->GetAbilitySystemComponent() : nullptr)
	{
		const float HealthOnKill = KillerASC->GetNumericAttribute(UGSAttributeSet::GetHealthOnKillAttribute());
		if (HealthOnKill > 0.f && HealthOnKillEffect && !KillerASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead))
		{
			const float Before = KillerASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute());
			KillerASC->ApplyGameplayEffectToSelf(HealthOnKillEffect->GetDefaultObject<UGameplayEffect>(), 1.f, KillerASC->MakeEffectContext());
			UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] %s: Leech +%.0f on kill -> Health %.0f -> %.0f"), *GetNameSafe(KillerState), HealthOnKill,
				Before, KillerASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute()));
		}
	}

	// Drops before the broadcast: if this kill clears the wave, RoundClear's vacuum includes them. Cash and XP per
	// enemy are flat (the same every round); only the combo multiplies them. No XP on the kill itself: it comes from
	// the orb, at pickup.
	UGSCashPickupSubsystem* Pickups = GetWorld()->GetSubsystem<UGSCashPickupSubsystem>();
	if (EnemyData && Pickups && CashSettings)
	{
		const UGSComboSettings* Combos = GetComboSettings();
		const int32 Cash = FMath::RoundToInt(EnemyData->CashValue * ComboMultiplier);
		const int32 XP = (Combos && Combos->bApplyComboToXP) ? FMath::RoundToInt(EnemyData->XPValue * ComboMultiplier) : EnemyData->XPValue;
		if (ComboCount > 0)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("[combo] %s: combo %d (x%.2f): cash $%d -> $%d, XP %d"), *GetNameSafe(KillerState), ComboCount,
				ComboMultiplier, EnemyData->CashValue, Cash, XP);
		}
		if (Cash > 0)
		{
			Pickups->SpawnCashDrop(CashSettings, Enemy->GetActorLocation(), Cash);
		}
		if (XP > 0)
		{
			Pickups->SpawnXPDrop(CashSettings, Enemy->GetActorLocation(), XP);
		}
		// Health orb: only enemies the player killed (shots, Powder Keg blasts; the Floating Gun later).
		if (KillerState)
		{
			RollHealthOrbDrop(Enemy->GetActorLocation());
		}
	}

	OnEnemyKilled.Broadcast(Enemy, Killer);

	// Powder Keg: an enemy the player killed may explode where it died (after the kill is fully counted).
	if (KillerState && EnemyCharacter)
	{
		TriggerPowderKeg(Enemy->GetActorLocation(), KillerState);
	}
}

float AGSGameMode::GetPowderKegChance(const AGSPlayerState* Player) const
{
	const UAbilitySystemComponent* ASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	const float Stacks = ASC ? ASC->GetNumericAttribute(UGSAttributeSet::GetPowderKegAttribute()) : 0.f;
	const UGSCombatSettings* Settings = GetCombatSettings() ? GetCombatSettings() : GetDefault<UGSCombatSettings>();
	return Settings->GetExplodeChance(Stacks);
}

bool AGSGameMode::RollPowderKegChance(const AGSPlayerState* Player)
{
	const float Chance = GetPowderKegChance(Player);
	if (Chance <= 0.f)
	{
		return false; // no stacks: no roll (the run stream is untouched)
	}
	return Chance >= 1.f || UGSRunRandom::RollChance(this, Chance); // 100%: no dice either
}

bool AGSGameMode::RollPowderKegDoubleSize(const AGSPlayerState* Player)
{
	const UAbilitySystemComponent* ASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	const float Stacks = ASC ? ASC->GetNumericAttribute(UGSAttributeSet::GetPowderKegAttribute()) : 0.f;
	const UGSCombatSettings* Settings = GetCombatSettings() ? GetCombatSettings() : GetDefault<UGSCombatSettings>();
	const float Chance = Settings->GetDoubleBlastChance(Stacks);
	return Chance > 0.f && (Chance >= 1.f || UGSRunRandom::RollChance(this, Chance)); // no dice below 100% or from 200%
}

void AGSGameMode::TriggerPowderKeg(const FVector& Location, const AGSPlayerState* Player)
{
	// Every kill rolls its own chance at the moment it dies (chain kills too, so the order of rolls follows the kills).
	if (!RollPowderKegChance(Player))
	{
		return;
	}
	if (!bApplyingPowderKeg)
	{
		ExplodePowderKeg(Location, Player);
		return;
	}

	// Killed by a blast: explode a moment later, so a chain ripples outwards instead of happening in one frame.
	const UGSCombatSettings* Settings = GetCombatSettings() ? GetCombatSettings() : GetDefault<UGSCombatSettings>();
	PendingPowderKegs.RemoveAll([this](const FTimerHandle& Handle) { return !GetWorldTimerManager().IsTimerActive(Handle); });
	FTimerHandle& Handle = PendingPowderKegs.AddDefaulted_GetRef();
	const TWeakObjectPtr<const AGSPlayerState> WeakPlayer(Player);
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Location, WeakPlayer]()
	{
		if (const AGSPlayerState* PlayerState = WeakPlayer.Get())
		{
			ExplodePowderKeg(Location, PlayerState);
		}
	}), FMath::Max(Settings->ChainDelay, 0.001f), false);
}

int32 AGSGameMode::GetPendingPowderKegs() const
{
	int32 Count = 0;
	for (const FTimerHandle& Handle : PendingPowderKegs)
	{
		Count += GetWorldTimerManager().IsTimerActive(Handle) ? 1 : 0;
	}
	return Count;
}

int32 AGSGameMode::ExplodePowderKeg(const FVector& Location, const AGSPlayerState* Player)
{
	UAbilitySystemComponent* SourceASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	const float Stacks = SourceASC ? SourceASC->GetNumericAttribute(UGSAttributeSet::GetPowderKegAttribute()) : 0.f;
	if (Stacks <= 0)
	{
		return 0;
	}
	const UGSCombatSettings* Settings = GetCombatSettings() ? GetCombatSettings() : GetDefault<UGSCombatSettings>();
	// The same blast at every stack count (stacks only raise the chance); the player's Damage scales it. Past 100% the
	// rest of the chance is rolled per blast for a double-size one (twice the radius, the same damage). Below 100% there
	// is no roll, so the run stream is used as before; from 200% every blast is double size without dice.
	const bool bDouble = RollPowderKegDoubleSize(Player);
	const float SizeMultiplier = bDouble ? Settings->DoubleBlastRadiusMultiplier : 1.f;
	const float Radius = Settings->BlastRadius * SizeMultiplier;
	const float Damage = Settings->BlastDamage * SourceASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute());
	++PowderKegBlasts;
	DoublePowderKegBlasts += bDouble ? 1 : 0;
	LastPowderKegRadius = Radius;

	// Every living enemy in the radius (no line of sight); players are never targets.
	TArray<AGSEnemyCharacter*> Targets;
	for (TActorIterator<AGSEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const UGSHealthComponent* Health = It->GetHealthComponent();
		if (Health && !Health->IsDead() && FVector::DistSquared(It->GetActorLocation(), Location) <= FMath::Square(Radius))
		{
			Targets.Add(*It);
		}
	}

	int32 Killed = 0;
	if (Settings->BlastDamageEffect && Damage > 0.f)
	{
		// Enemies this blast kills come back through HandleEnemyKilled as the player's kills and chain after ChainDelay.
		TGuardValue<bool> ApplyingBlast(bApplyingPowderKeg, true);
		for (AGSEnemyCharacter* Target : Targets)
		{
			UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
			FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Settings->BlastDamageEffect, 1.f, SourceASC->MakeEffectContext());
			if (!TargetASC || !Spec.IsValid())
			{
				continue;
			}
			Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, Damage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
			Killed += Target->GetHealthComponent()->IsDead() ? 1 : 0;
		}
	}
	else if (!Settings->BlastDamageEffect)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[blast] DA_CombatSettings has no BlastDamageEffect; the Powder Keg blast did no damage."));
	}

	// Placeholder explosion (local: single-player). TODO(co-op): multicast the effect and sound.
	if (Settings->BlastEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Settings->BlastEffect, Location, FRotator::ZeroRotator, FVector(SizeMultiplier)); // its size at scale 1 matches BlastRadius
	}
	if (Settings->BlastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Settings->BlastSound, Location);
	}
	if (GSWeaponDebug::IsEnabled())
	{
		DrawDebugSphere(GetWorld(), Location, Radius, 16, FColor::Orange, false, GSWeaponDebug::DrawDuration);
	}

	UE_LOG(LogBanditBaller, Log, TEXT("[blast] Powder Keg x%g (%.1f%% chance per kill) at %s: %sradius %.0f, damage %.0f, %d enemies hit, %d killed (blast %d this run)"),
		Stacks, Settings->GetRawExplodeChance(Stacks) * 100.f, *Location.ToCompactString(), bDouble ? TEXT("DOUBLE SIZE, ") : TEXT(""), Radius, Damage,
		Targets.Num(), Killed, PowderKegBlasts);
	return Targets.Num();
}

int32 AGSGameMode::AwardXP(AGSPlayerState* Player, float BaseXP, bool bApplyXPGain)
{
	UGSProgressionComponent* Progression = Player ? Player->GetProgressionComponent() : nullptr;
	if (!Progression || BaseXP <= 0.f)
	{
		return 0;
	}
	float Gain = 1.f;
	if (bApplyXPGain)
	{
		const UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
		Gain = ASC ? FMath::Max(0.f, ASC->GetNumericAttribute(UGSAttributeSet::GetXPGainAttribute())) : 1.f;
	}
	const int32 Amount = FMath::RoundToInt(BaseXP * Gain);
	if (Amount <= 0)
	{
		return 0;
	}
	Progression->AddXP(Amount);
	if (UGSRunStats* Stats = GetRunStats())
	{
		Stats->AddXPEarned(Amount);
		Stats->RecordLevelReached(Progression->GetLevel());
	}
	return Amount;
}

bool AGSGameMode::ChooseUpgrade(AGSPlayerState* Player, int32 Index)
{
	UGSUpgradeComponent* Upgrades = Player ? Player->GetUpgradeComponent() : nullptr;
	return Upgrades && Upgrades->ChooseUpgrade(Index);
}

bool AGSGameMode::RerollUpgradeOffer(AGSPlayerState* Player)
{
	UGSUpgradeComponent* Upgrades = Player ? Player->GetUpgradeComponent() : nullptr;
	return Upgrades && Upgrades->RerollOffer();
}

void AGSGameMode::QueueUpgradeOfferForDebug(AGSPlayerState* Player)
{
	if (UGSUpgradeComponent* Upgrades = Player ? Player->GetUpgradeComponent() : nullptr)
	{
		Upgrades->QueueOffer(TEXT("debug"));
	}
}

void AGSGameMode::UpdateUpgradeChoicePause()
{
	bool bAnyOfferOpen = false;
	if (const AGSGameState* GS = GetGameState<AGSGameState>())
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			const AGSPlayerState* GSPS = Cast<AGSPlayerState>(PS);
			const UGSUpgradeComponent* Upgrades = GSPS ? GSPS->GetUpgradeComponent() : nullptr;
			bAnyOfferOpen |= Upgrades && Upgrades->HasOpenOffer();
		}
	}
	// TODO(co-op): M6 shows the other players who is choosing while the party is paused.
	const bool bWantPause = bPauseDuringUpgradeChoice && bAnyOfferOpen;
	if (bWantPause == bPausedForUpgradeChoice)
	{
		return;
	}
	if (bWantPause)
	{
		bPausedForUpgradeChoice = UGameplayStatics::SetGamePaused(this, true);
	}
	else
	{
		UGameplayStatics::SetGamePaused(this, false);
		bPausedForUpgradeChoice = false;
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] Game %s for the upgrade choice."), bPausedForUpgradeChoice ? TEXT("paused") : TEXT("unpaused"));
}

void AGSGameMode::SetPlayerLevelForDebug(AGSPlayerState* Player, int32 Level)
{
	UGSProgressionComponent* Progression = Player ? Player->GetProgressionComponent() : nullptr;
	if (!Progression || !XPCurve)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.XP.SetLevel: no progression component or no XPCurve on the GameMode."));
		return;
	}
	// No level cap. XP amounts are int32, so the one limit is the highest level whose total XP still fits in one.
	int32 Target = FMath::Max(Level, 1);
	if (XPCurve->GetTotalXPToReachLevel(Target) > MAX_int32)
	{
		int32 Low = 1, High = Target;
		while (Low < High)
		{
			const int32 Mid = Low + (High - Low + 1) / 2;
			if (XPCurve->GetTotalXPToReachLevel(Mid) <= MAX_int32)
			{
				Low = Mid;
			}
			else
			{
				High = Mid - 1;
			}
		}
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.XP.SetLevel: level %d needs more XP than fits in an int32; using level %d."), Level, Low);
		Target = Low;
	}
	Progression->ResetProgression();
	const int64 Total = XPCurve->GetTotalXPToReachLevel(Target);
	if (Total > 0)
	{
		Progression->AddXP(static_cast<int32>(Total));
	}
	if (UGSRunStats* Stats = GetRunStats())
	{
		Stats->RecordLevelReached(Progression->GetLevel());
	}
}

void AGSGameMode::AwardCash(APlayerController* Player, int32 Amount, const FVector& Location)
{
	if (!Player || Amount <= 0)
	{
		return;
	}
	// Permanent cash lives in this machine's profile (UGSProfileSubsystem), so only the local player (single player
	// or the listen-server host) can be credited here.
	if (!Player->IsLocalController())
	{
		// TODO(co-op): credit remote players (decided design: the host's save holds every player's cash).
		if (!bWarnedRemoteCash)
		{
			bWarnedRemoteCash = true;
			UE_LOG(LogBanditBaller, Warning, TEXT("[cash] %s collected $%d but remote players' cash isn't credited yet (co-op TODO)."), *GetNameSafe(Player), Amount);
		}
		return;
	}
	UGSProfileSubsystem* Profile = UGSProfileSubsystem::Get(this);
	if (!Profile || !Profile->AddCash(Amount, TEXT("Pickup")))
	{
		return;
	}
	if (UGSRunStats* Stats = GetRunStats())
	{
		Stats->AddCashEarned(Amount);
	}
	if (AGSPlayerController* PC = Cast<AGSPlayerController>(Player))
	{
		PC->ClientNotifyCashCollected(Location, Amount);
	}
}

int32 AGSGameMode::AwardXPPickup(APlayerController* Player, int32 BaseXP, const FVector& Location)
{
	AGSPlayerState* PS = Player ? Player->GetPlayerState<AGSPlayerState>() : nullptr;
	if (!PS || BaseXP <= 0)
	{
		return 0;
	}
	// TODO(co-op): XP goes to whoever collects the orb (not shared with the party). The popup position reaches only
	// the local HUD; remote clients' "+N XP" falls back to above their pawn until M6 adds an RPC.
	if (AGSPlayerController* PC = Cast<AGSPlayerController>(Player))
	{
		PC->NotifyXPCollected(Location); // before AwardXP, so the popup pairs with this XP gain at once
	}
	return AwardXP(PS, static_cast<float>(BaseXP)); // XPGain applies here; level-ups, rewards and offers follow
}

float AGSGameMode::GetHealthOrbDropChance() const
{
	if (HealthOrbDropChanceOverride >= 0.f)
	{
		return FMath::Clamp(HealthOrbDropChanceOverride, 0.f, 1.f);
	}
	return CashSettings ? FMath::Clamp(CashSettings->HealthOrbDropChance, 0.f, 1.f) : 0.f;
}

void AGSGameMode::RollHealthOrbDrop(const FVector& Location)
{
	UGSCashPickupSubsystem* Pickups = GetWorld()->GetSubsystem<UGSCashPickupSubsystem>();
	const float Chance = GetHealthOrbDropChance();
	if (!Pickups || !CashSettings || Chance <= 0.f)
	{
		return; // no dice: a 0 chance leaves the run stream untouched
	}
	if (Chance >= 1.f || UGSRunRandom::RollChance(this, Chance))
	{
		Pickups->SpawnHealthDrop(CashSettings, Location);
	}
}

int32 AGSGameMode::SpawnHealthOrbsForDebug(APlayerController* Player, int32 Count)
{
	UGSCashPickupSubsystem* Pickups = GetWorld()->GetSubsystem<UGSCashPickupSubsystem>();
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (!Pickups || !CashSettings || !Pawn)
	{
		return 0;
	}
	int32 Dropped = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Dropped += Pickups->SpawnHealthDrop(CashSettings, Pawn->GetActorLocation()) ? 1 : 0;
	}
	return Dropped;
}

float AGSGameMode::AwardHealthPickup(APlayerController* Player, int32 HealAmount, const FVector& Location)
{
	const AGSPlayerState* PS = Player ? Player->GetPlayerState<AGSPlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC || HealAmount <= 0 || ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead))
	{
		return 0.f;
	}
	// TODO(co-op): the orb heals whoever collects it (not shared with the party). The popup reaches only the local HUD.
	const TSubclassOf<UGameplayEffect> HealEffect = (CashSettings && CashSettings->HealthOrbHealEffect)
		? CashSettings->HealthOrbHealEffect : TSubclassOf<UGameplayEffect>(UGSHealEffect::StaticClass());
	const float Before = ASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute());
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(HealEffect, 1.f, ASC->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return 0.f;
	}
	Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Heal, static_cast<float>(HealAmount));
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	const float Healed = FMath::Max(0.f, ASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute()) - Before);

	if (UGSRunStats* Stats = GetRunStats())
	{
		Stats->RecordHealthOrbCollected(Healed);
	}
	// Placeholder sound (local: single-player). TODO(co-op): play it on the collecting player's machine.
	if (CashSettings && CashSettings->HealthOrbSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CashSettings->HealthOrbSound, Location);
	}
	if (AGSPlayerController* PC = Cast<AGSPlayerController>(Player))
	{
		PC->NotifyHealthCollected(Location, Healed);
	}
	UE_LOG(LogBanditBaller, Log, TEXT("[heal] %s: health orb +%d -> Health %.0f -> %.0f (healed %.0f)"), *GetNameSafe(PS), HealAmount, Before,
		ASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute()), Healed);
	return Healed;
}

void AGSGameMode::HandleDamageApplied(AActor* Victim, AActor* DamageInstigator, float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}
	UGSRunStats* Stats = GetRunStats();
	if (IsPlayerActor(Victim))
	{
		if (Stats)
		{
			Stats->AddDamageTaken(Amount);
		}
		// Health lost (blocked damage never gets here) can end the player's combo (bEndComboOnDamage).
		const AGSPlayerState* VictimState = GetGSPlayerStateOf(Victim);
		if (UGSComboComponent* Combo = VictimState ? VictimState->GetComboComponent() : nullptr)
		{
			Combo->HandleDamageTaken(Amount);
		}
	}
	else if (Stats && IsPlayerActor(DamageInstigator))
	{
		Stats->AddDamageDealt(Amount);
	}
}

const UGSComboSettings* AGSGameMode::GetComboSettings() const
{
	return ComboSettingsOverride ? ComboSettingsOverride.Get() : ComboSettings.Get();
}

const UGSRoundClearSettings* AGSGameMode::GetRoundClearSettings() const
{
	return RoundClearSettingsOverride ? RoundClearSettingsOverride.Get() : RoundClearSettings.Get();
}

void AGSGameMode::PayRoundClearBonus()
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	UGSRunStats* Stats = GetRunStats();
	const UGSRoundClearSettings* Settings = GetRoundClearSettings();
	if (!GS || !Stats)
	{
		return;
	}
	if (!Settings)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("[cash] %s has no RoundClearSettings; no round-clear bonus."), *GetName());
		return;
	}

	// The same amounts every round (flat economy): nothing here depends on the round number. Only the fast-clear time
	// limit follows the wave size.
	const FGSRoundStats& Round = Stats->GetCurrentRound();
	FGSRoundClearPayout Payout;
	Payout.Round = GS->GetRoundNumber();
	Payout.ClearTime = Round.CombatStartTime >= 0.0 ? static_cast<float>(GS->GetServerWorldTimeSeconds() - Round.CombatStartTime) : 0.f;
	Payout.FastClearTime = Settings->GetFastClearTime(Round.WaveEnemies);
	Payout.bNoDamage = Round.DamageTaken <= 0.f;
	Payout.bFastClear = Payout.ClearTime <= Payout.FastClearTime;
	Payout.BaseBonus = Settings->BaseBonus;
	Payout.NoDamageBonus = Payout.bNoDamage ? Settings->NoDamageBonus : 0;
	Payout.FastClearBonus = Payout.bFastClear ? Settings->FastClearBonus : 0;

	// TODO(co-op): one party-wide bonus into this machine's profile (single player / host); M6 pays every player.
	const int32 Total = Payout.GetTotal();
	if (Total > 0)
	{
		if (UGSProfileSubsystem* Profile = UGSProfileSubsystem::Get(this))
		{
			Profile->AddCash(Total, TEXT("RoundClear"));
		}
	}
	Stats->RecordRoundClearPayout(Payout);
	UE_LOG(LogBanditBaller, Log, TEXT("[cash] Round %d clear bonus: $%d (base %d, no damage %d, fast clear %d; cleared in %.1f s of %.1f s, %.0f damage taken)"),
		Payout.Round, Total, Payout.BaseBonus, Payout.NoDamageBonus, Payout.FastClearBonus, Payout.ClearTime, Payout.FastClearTime, Round.DamageTaken);
}

void AGSGameMode::LogRoundSummary(const TCHAR* Ending) const
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	const UGSRunStats* Stats = GetRunStats();
	if (!GS || !Stats)
	{
		return;
	}
	const FGSRoundStats& Round = Stats->GetCurrentRound();
	const FGSRoundClearPayout& Payout = Stats->GetLastRoundClearPayout();
	const bool bPaid = Payout.Round == Round.Round && Round.Round > 0;
	const float CombatTime = bPaid ? Payout.ClearTime
		: (Round.CombatStartTime >= 0.0 ? static_cast<float>(GS->GetServerWorldTimeSeconds() - Round.CombatStartTime) : 0.f);
	UE_LOG(LogBanditBaller, Log, TEXT("[round] Round %d %s: combat %.1f s, wave %d, %d kills, $%d collected + $%d bonus, %d XP, %.0f damage taken, level %d, fallback spawns %d"),
		Round.Round, Ending, CombatTime, Round.WaveEnemies, Round.Kills, Round.CashCollected, bPaid ? Payout.GetTotal() : 0, Round.XPEarned, Round.DamageTaken,
		Stats->GetLevelReached(), WaveDirector ? WaveDirector->GetFallbackSpawns() : 0);
}

UGSRunStats* AGSGameMode::GetRunStats() const
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	return GS ? GS->GetRunStats() : nullptr;
}

// ---------------------------------------------------------------------------
// Run seed
// ---------------------------------------------------------------------------

int32 AGSGameMode::MakeRandomSeed()
{
	const uint64 Mixed = FPlatformTime::Cycles64() ^ static_cast<uint64>(FDateTime::Now().GetTicks());
	const int32 Seed = static_cast<int32>(GetTypeHash(Mixed) & 0x7FFFFFFF);
	return Seed != 0 ? Seed : 1;
}

void AGSGameMode::StartRun(int32 Seed, const TCHAR* Source)
{
	RunSeed = Seed;
	RunStream.Initialize(Seed);

	// Chain blasts from the last run don't carry into this one.
	for (FTimerHandle& Handle : PendingPowderKegs)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	PendingPowderKegs.Reset();
	PowderKegBlasts = 0;
	DoublePowderKegBlasts = 0;
	LastPowderKegRadius = 0.f;

	// A fresh run starts with no cash on the ground and no Floating Gun shots in flight.
	if (UGSCashPickupSubsystem* Cash = GetWorld()->GetSubsystem<UGSCashPickupSubsystem>())
	{
		Cash->ReleaseAll();
	}
	if (UGSProjectileSubsystem* Projectiles = GetWorld()->GetSubsystem<UGSProjectileSubsystem>())
	{
		Projectiles->ReleaseAll();
	}

	if (AGSGameState* GS = GetGameState<AGSGameState>())
	{
		GS->SetRunSeed(Seed);
		if (UGSRunStats* Stats = GS->GetRunStats())
		{
			Stats->ResetStats(GS->GetServerWorldTimeSeconds());
		}
		// Levels, upgrades and combos are per run.
		for (APlayerState* PS : GS->PlayerArray)
		{
			const AGSPlayerState* GSPS = Cast<AGSPlayerState>(PS);
			if (UGSProgressionComponent* Progression = GSPS ? GSPS->GetProgressionComponent() : nullptr)
			{
				Progression->ResetProgression();
			}
			if (UGSUpgradeComponent* Upgrades = GSPS ? GSPS->GetUpgradeComponent() : nullptr)
			{
				Upgrades->ResetUpgrades();
			}
			if (UGSComboComponent* Combo = GSPS ? GSPS->GetComboComponent() : nullptr)
			{
				Combo->EndCombo(EGSComboEndReason::Reset);
			}
			if (UGSShieldComponent* Shields = GSPS ? GSPS->GetShieldComponent() : nullptr)
			{
				Shields->ResetShields(); // after the upgrades reset: no shields left, no recharges running
			}
			if (UGSSecondWindComponent* SecondWind = GSPS ? GSPS->GetSecondWindComponent() : nullptr)
			{
				SecondWind->ResetSecondWind();
			}
		}
	}
	UE_LOG(LogBanditBaller, Log, TEXT("Run started: seed %d (%s). Replay with -GSSeed=%d or gs.Run.Seed %d"), Seed, Source, Seed, Seed);
}

void AGSGameMode::RestartRunWithSeed(int32 Seed)
{
	StartRun(Seed, TEXT("gs.Run.Seed"));
	SetRound(1);
}

void AGSGameMode::HandlePlayerDeath(APawn* Player, AActor* Killer)
{
	UE_LOG(LogBanditBaller, Log, TEXT("Player %s died (killer: %s)"), *GetNameSafe(Player), *GetNameSafe(Killer));
	OnPlayerDied.Broadcast(Player, Killer);

	// The run is over. TODO(co-op): only once the whole party is dead (M6, with revives).
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (GS && GS->GetRoundPhase() != EGSRoundPhase::GameOver)
	{
		// Stops the wave and the phase timer: nothing more spawns and no RoundClear (or its bonus) can follow.
		SetPhase(EGSRoundPhase::GameOver);
	}
	// No upgrade picks after the run: close open offers (which lifts their pause; the return to the hub runs on a world
	// timer) before the summary takes input. GameOver also refuses new offers (UGSUpgradeComponent::QueueOffer).
	if (GS)
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			const AGSPlayerState* GSPS = Cast<AGSPlayerState>(PS);
			if (UGSUpgradeComponent* Upgrades = GSPS ? GSPS->GetUpgradeComponent() : nullptr)
			{
				Upgrades->DropOffers(TEXT("run over"));
			}
		}
	}
	UGSRunStats* Stats = GetRunStats();
	if (Stats && GS)
	{
		Stats->EndRun(GS->GetServerWorldTimeSeconds()); // freezes the run time for the summary
	}

	// Run end: keep the cash earned this run.
	if (UGSProfileSubsystem* Profile = UGSProfileSubsystem::Get(this))
	{
		Profile->Save(TEXT("RunEnd"));
	}
	if (Stats && GS)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("Run over in round %d (seed %d): %d rounds cleared, level %d, %d kills, best combo %d, $%d earned, %.1f s."),
			GS->GetRoundNumber(), GS->GetRunSeed(), Stats->GetRoundsCleared(), Stats->GetLevelReached(), Stats->GetTotalKills(), Stats->GetBestCombo(),
			Stats->GetCashEarned(), Stats->GetRunTime());
	}

	if (AGSPlayerController* PC = Player ? Cast<AGSPlayerController>(Player->GetController()) : nullptr)
	{
		PC->ClientShowRunSummary(RunSummaryWidgetClass);
	}
}

bool AGSGameMode::RequestReturnToHub(AController* Requester)
{
	// Only a dead player may leave the run. TODO(co-op): "all players dead" instead (M6).
	const APawn* Pawn = Requester ? Requester->GetPawn() : nullptr;
	const UGSHealthComponent* Health = UGSHealthComponent::FindHealthComponent(Pawn);
	if (bReturningToHub || !Health || !Health->IsDead())
	{
		return false;
	}
	bReturningToHub = true;

	// The run is over: back to the hub (the market), where the next run starts from the portal. Nothing from the run
	// carries over (the next run rebuilds the town); the cash is in the profile, saved at death.
	UE_LOG(LogBanditBaller, Log, TEXT("Run over: %s returns to the hub (fade %.2f s)."), *GetNameSafe(Requester), ReturnToHubFadeTime);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			PC->ClientSetCameraFade(true, FColor::Black, FVector2D(0.f, 1.f), ReturnToHubFadeTime, /*bFadeAudio*/ true, /*bHoldWhenFinished*/ true);
		}
	}
	if (ReturnToHubFadeTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(ReturnToHubTimer, this, &AGSGameMode::FinishReturnToHub, ReturnToHubFadeTime, false);
	}
	else
	{
		FinishReturnToHub();
	}
	return true;
}

void AGSGameMode::FinishReturnToHub()
{
	if (!OpenHubMap())
	{
		GetWorld()->ServerTravel(TEXT("?Restart")); // no HubMap set: at least start over in this map
	}
}

// ---------------------------------------------------------------------------
// Map travel
// ---------------------------------------------------------------------------

bool AGSGameMode::OpenHubMap()
{
	if (HubMap.IsNull())
	{
		UE_LOG(LogBanditBaller, Error, TEXT("%s has no HubMap; can't leave for the hub."), *GetName());
		return false;
	}
	// Single-player: the new map rebuilds everything; only the profile (GameInstance) carries over.
	// TODO(co-op): ServerTravel so the whole party goes together (M6).
	UE_LOG(LogBanditBaller, Log, TEXT("Opening the hub map %s."), *HubMap.ToSoftObjectPath().GetLongPackageName());
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, HubMap);
	return true;
}

// ---------------------------------------------------------------------------
// Round loop
// ---------------------------------------------------------------------------

namespace
{
	FString PhaseName(EGSRoundPhase Phase)
	{
		return StaticEnum<EGSRoundPhase>()->GetNameStringByValue(static_cast<int64>(Phase));
	}
}

void AGSGameMode::StartPlay()
{
	Super::StartPlay();

	if (!RoundSettings)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s has no RoundSettings; using UGSRoundSettings defaults."), *GetName());
	}

	// A run starts when the town map loads. Seed: ?GSSeed=N from the hub's portal (gs.Run.Seed there), else -GSSeed=N
	// on the command line, else random. Logged so any run can be replayed.
	const int32 UrlSeed = FCString::Atoi(*UGameplayStatics::ParseOption(OptionsString, RunSeedOption));
	int32 Seed = 0;
	if (UrlSeed > 0)
	{
		StartRun(UrlSeed, TEXT("hub gs.Run.Seed"));
	}
	else if (FParse::Value(FCommandLine::Get(), TEXT("GSSeed="), Seed))
	{
		StartRun(Seed, TEXT("-GSSeed"));
	}
	else
	{
		StartRun(MakeRandomSeed(), TEXT("random"));
	}

	WaveDirector->OnWaveCleared.AddUObject(this, &AGSGameMode::HandleWaveCleared);

	// Clients describe level-up rewards from this (HUD banner).
	if (AGSGameState* GS = GetGameState<AGSGameState>())
	{
		GS->SetLevelRewards(LevelRewards);
	}

	if (bAutoStartRounds)
	{
		StartNextRound();
	}
}

void AGSGameMode::HandleWaveCleared()
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (GS && GS->GetRoundPhase() == EGSRoundPhase::Combat)
	{
		SetPhase(EGSRoundPhase::RoundClear);
	}
}

void AGSGameMode::HandlePhaseTimerExpired()
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (GS && GS->GetRoundPhase() == EGSRoundPhase::Combat && WaveDirector->IsWaveActive())
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("Round %d: Combat safety timer (%.0f s) expired with %d enemies remaining (%d alive); ending combat anyway."),
			GS->GetRoundNumber(), GetPhaseDuration(EGSRoundPhase::Combat), WaveDirector->GetRemaining(), WaveDirector->GetAlive());
	}
	AdvancePhase();
}

const UGSRoundSettings& AGSGameMode::GetRoundSettings() const
{
	return RoundSettings ? *RoundSettings : *GetDefault<UGSRoundSettings>();
}

float AGSGameMode::GetPhaseDuration(EGSRoundPhase Phase) const
{
	const UGSRoundSettings& Settings = GetRoundSettings();
	switch (Phase)
	{
	case EGSRoundPhase::PreRound:   return Settings.PreRoundDuration;
	case EGSRoundPhase::Combat:     return Settings.CombatFallbackDuration;
	case EGSRoundPhase::RoundClear: return Settings.RoundClearDuration;
	default:                        return 0.f; // WaitingToStart, GameOver
	}
}

void AGSGameMode::SetPhase(EGSRoundPhase NewPhase)
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS)
	{
		UE_LOG(LogBanditBaller, Error, TEXT("SetPhase(%s): no AGSGameState."), *PhaseName(NewPhase));
		return;
	}

	GetWorldTimerManager().ClearTimer(PhaseTimer);

	const EGSRoundPhase OldPhase = GS->GetRoundPhase();

	// One line per finished round (before the next PreRound starts its stats over), and for the round a death ends.
	if (OldPhase == EGSRoundPhase::RoundClear && NewPhase != EGSRoundPhase::RoundClear)
	{
		LogRoundSummary(TEXT("cleared"));
	}
	else if (NewPhase == EGSRoundPhase::GameOver && OldPhase != EGSRoundPhase::GameOver)
	{
		LogRoundSummary(TEXT("died"));
	}
	const float Duration = GetPhaseDuration(NewPhase);
	const double EndTime = Duration > 0.f ? GS->GetServerWorldTimeSeconds() + Duration : 0.0;

	UE_LOG(LogBanditBaller, Log, TEXT("Round %d: %s -> %s (%s) at t=%.2fs"), GS->GetRoundNumber(), *PhaseName(OldPhase), *PhaseName(NewPhase),
		Duration > 0.f ? *FString::Printf(TEXT("%.1f s"), Duration) : TEXT("untimed"), GetWorld()->GetTimeSeconds());

	GS->SetRoundPhase(NewPhase, EndTime);

	// Publish this round's enemy multipliers (the round number may have changed).
	GS->SetEnemyRoundMultipliers(GetEnemyHealthMultiplier(GS->GetRoundNumber()), GetEnemyDamageMultiplier(GS->GetRoundNumber()));

	// A round's stats run from its PreRound to the next one (RoundClear's vacuum still counts for the round it ends).
	if (NewPhase == EGSRoundPhase::PreRound)
	{
		if (UGSRunStats* Stats = GetRunStats())
		{
			Stats->BeginRound(GS->GetRoundNumber());
		}
	}

	// Combat finished (wave cleared, safety timer or skip): the round counts as cleared and pays its bonus.
	if (OldPhase == EGSRoundPhase::Combat && NewPhase == EGSRoundPhase::RoundClear)
	{
		if (UGSRunStats* Stats = GetRunStats())
		{
			Stats->RecordRoundCleared();
		}
		PayRoundClearBonus();
	}
	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(PhaseTimer, this, &AGSGameMode::HandlePhaseTimerExpired, Duration, false);
		if (bRoundTimerPaused)
		{
			GetWorldTimerManager().PauseTimer(PhaseTimer);
		}
	}

	// Waves: Combat runs the round's wave; leaving Combat early (skip, safety timer) stops it and removes leftovers.
	// GameOver (the player died) stops spawning and cancels telegraphs but leaves the living enemies where they are.
	if (NewPhase == EGSRoundPhase::GameOver)
	{
		WaveDirector->StopWave(/*bDespawnLeftovers*/ false);
	}
	else if (OldPhase == EGSRoundPhase::Combat && NewPhase != EGSRoundPhase::Combat)
	{
		WaveDirector->StopWave(/*bDespawnLeftovers*/ true);
	}
	if (NewPhase == EGSRoundPhase::Combat)
	{
		WaveDirector->StartWave(GS->GetRoundNumber());
		if (UGSRunStats* Stats = GetRunStats())
		{
			Stats->RecordCombatStarted(GS->GetServerWorldTimeSeconds(), WaveDirector->GetTotal()); // fast-clear clock
		}
	}

	// Round won: every cash piece still on the ground flies to the party.
	if (NewPhase == EGSRoundPhase::RoundClear)
	{
		if (UGSCashPickupSubsystem* Cash = GetWorld()->GetSubsystem<UGSCashPickupSubsystem>())
		{
			Cash->VacuumAll();
		}
	}

	// Each RoundClear saves the profile. Done when RoundClear ends, so the cash its vacuum just paid out is included.
	if (OldPhase == EGSRoundPhase::RoundClear && NewPhase != EGSRoundPhase::RoundClear)
	{
		if (UGSProfileSubsystem* Profile = UGSProfileSubsystem::Get(this))
		{
			Profile->Save(TEXT("RoundClear"));
		}
	}
}

void AGSGameMode::AdvancePhase()
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS)
	{
		return;
	}

	switch (GS->GetRoundPhase())
	{
	case EGSRoundPhase::WaitingToStart: StartNextRound(); break;
	case EGSRoundPhase::PreRound:       SetPhase(EGSRoundPhase::Combat); break;
	case EGSRoundPhase::Combat:         SetPhase(EGSRoundPhase::RoundClear); break;
	case EGSRoundPhase::RoundClear:     StartNextRound(); break;
	case EGSRoundPhase::GameOver:
		UE_LOG(LogBanditBaller, Log, TEXT("AdvancePhase: GameOver is final."));
		break;
	}
}

void AGSGameMode::StartNextRound()
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS)
	{
		return;
	}
	GS->SetRoundNumber(GS->GetRoundNumber() + 1);
	SetPhase(EGSRoundPhase::PreRound);
}

void AGSGameMode::SetRound(int32 NewRound)
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS || NewRound < 1)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.Round.Set: round must be >= 1."));
		return;
	}
	GS->SetRoundNumber(NewRound);
	SetPhase(EGSRoundPhase::PreRound);
}

// ---------------------------------------------------------------------------
// Enemy difficulty scaling
// ---------------------------------------------------------------------------

float AGSGameMode::GetEnemyHealthMultiplier(int32 Round) const
{
	return DifficultyCurve ? DifficultyCurve->GetHealthMultiplier(Round) : 1.f;
}

float AGSGameMode::GetEnemyDamageMultiplier(int32 Round) const
{
	return DifficultyCurve ? DifficultyCurve->GetDamageMultiplier(Round) : 1.f;
}

void AGSGameMode::ApplyEnemyRoundScaling(AGSEnemyCharacter& Enemy) const
{
	UAbilitySystemComponent* ASC = Enemy.GetAbilitySystemComponent();
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (!ASC || !GS)
	{
		return;
	}
	if (!EnemyRoundScalingEffect)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("%s has no EnemyRoundScalingEffect; enemies are not scaled by round."), *GetName());
		return;
	}

	const int32 Round = FMath::Max(GS->GetRoundNumber(), 1);
	const float HealthMultiplier = GetEnemyHealthMultiplier(Round);
	const float DamageMultiplier = GetEnemyDamageMultiplier(Round);

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EnemyRoundScalingEffect, static_cast<float>(Round), ASC->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_HealthMultiplier, HealthMultiplier);
	Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_DamageMultiplier, DamageMultiplier);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);

	// Spawn at full (scaled) health.
	ASC->SetNumericAttributeBase(UGSAttributeSet::GetHealthAttribute(), ASC->GetNumericAttribute(UGSAttributeSet::GetMaxHealthAttribute()));
}

void AGSGameMode::SetRoundTimerPaused(bool bPaused)
{
	bRoundTimerPaused = bPaused;
	if (bPaused)
	{
		GetWorldTimerManager().PauseTimer(PhaseTimer);
	}
	else
	{
		GetWorldTimerManager().UnPauseTimer(PhaseTimer);
	}
	UE_LOG(LogBanditBaller, Log, TEXT("gs.Round.Pause: phase timer %s"), bPaused ? TEXT("PAUSED") : TEXT("running"));
}
