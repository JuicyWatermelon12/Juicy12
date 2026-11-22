// BanditBaller

#include "GSDebugLibrary.h"
#include "AbilitySystemComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "BanditBaller.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"
#include "GSAttributeSet.h"
#include "GSCharacter.h"
#include "GSComboComponent.h"
#include "GSEnemyCharacter.h"
#include "GSEnemyData.h"
#include "GSGameMode.h"
#include "GSGameplayAbility_Dash.h"
#include "GSGameplayTags.h"
#include "GSMovementAbilitySettings.h"
#include "GSGameState.h"
#include "GSHealthComponent.h"
#include "GSHubGameMode.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSProfileSubsystem.h"
#include "GSProgressionComponent.h"
#include "GSRunRandom.h"
#include "GSSpawnLocationFinder.h"
#include "GSUpgradeComponent.h"
#include "GSUpgradeData.h"
#include "GSUpgradePool.h"
#include "GSWeaponComponent.h"
#include "GSWeaponData.h"
#include "NavigationSystem.h"

namespace GSDebug
{
	/** "DA_Enemy_MeleeBandit" -> "meleebandit"; also strips DA_Weapon_ / DA_. */
	FString NormalizeAssetName(FString Name)
	{
		Name = Name.TrimStartAndEnd().ToLower();
		for (const TCHAR* Prefix : { TEXT("da_enemy_"), TEXT("da_weapon_"), TEXT("da_upgrade_"), TEXT("da_") })
		{
			if (Name.StartsWith(Prefix))
			{
				Name.RightChopInline(FCString::Strlen(Prefix));
				break;
			}
		}
		return Name;
	}

	TArray<FAssetData> GetAssetsOfClass(const UClass* Class)
	{
		TArray<FAssetData> Assets;
		IAssetRegistry::GetChecked().GetAssetsByClass(Class->GetClassPathName(), Assets, /*bSearchSubClasses*/ true);
		return Assets;
	}

	UObject* FindAssetByName(const UClass* Class, const FString& Name)
	{
		const FString Wanted = NormalizeAssetName(Name);
		for (const FAssetData& Asset : GetAssetsOfClass(Class))
		{
			if (NormalizeAssetName(Asset.AssetName.ToString()) == Wanted)
			{
				return Asset.GetAsset();
			}
		}
		return nullptr;
	}

	TArray<FString> GetAssetNames(const UClass* Class)
	{
		TArray<FString> Names;
		for (const FAssetData& Asset : GetAssetsOfClass(Class))
		{
			Names.Add(Asset.AssetName.ToString());
		}
		Names.Sort();
		return Names;
	}

	UAbilitySystemComponent* GetPlayerASC(const AGSPlayerController* PC)
	{
		const AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
		return PS ? PS->GetAbilitySystemComponent() : nullptr;
	}

	FGameplayAttribute FindAttribute(const FString& Name)
	{
		for (TFieldIterator<FProperty> It(UGSAttributeSet::StaticClass()); It; ++It)
		{
			if (FGameplayAttribute::IsGameplayAttributeDataProperty(*It) && It->GetName().Equals(Name, ESearchCase::IgnoreCase))
			{
				return FGameplayAttribute(*It);
			}
		}
		return FGameplayAttribute();
	}

	bool IsAlive(const AGSEnemyCharacter& Enemy)
	{
		const UGSHealthComponent* Health = Enemy.GetHealthComponent();
		return IsValid(&Enemy) && (!Health || !Health->IsDead());
	}

	bool CheckAuthority(const AGSPlayerController* PC, const TCHAR* What)
	{
		if (!PC)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("%s: no player controller (is the game running?)"), What);
			return false;
		}
		if (!PC->HasAuthority())
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("%s must run on the server."), What);
			return false;
		}
		return true;
	}
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

UWorld* UGSDebugLibrary::FindGameWorld(UWorld* World)
{
	if (World && World->IsGameWorld())
	{
		return World;
	}
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
	}
	return nullptr;
}

UGSEnemyData* UGSDebugLibrary::FindEnemyData(const FString& Name)
{
	return Cast<UGSEnemyData>(GSDebug::FindAssetByName(UGSEnemyData::StaticClass(), Name));
}

UGSWeaponData* UGSDebugLibrary::FindWeaponData(const FString& Name)
{
	return Cast<UGSWeaponData>(GSDebug::FindAssetByName(UGSWeaponData::StaticClass(), Name));
}

UGSUpgradeData* UGSDebugLibrary::FindUpgradeData(const FString& Name)
{
	return Cast<UGSUpgradeData>(GSDebug::FindAssetByName(UGSUpgradeData::StaticClass(), Name));
}

TArray<FString> UGSDebugLibrary::GetUpgradeDataNames()
{
	return GSDebug::GetAssetNames(UGSUpgradeData::StaticClass());
}

int32 UGSDebugLibrary::GrantUpgradeTo(AGSPlayerController* PC, const FString& UpgradeName, int32 Count)
{
#if !UE_BUILD_SHIPPING
	const AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
	UGSUpgradeComponent* Upgrades = PS ? PS->GetUpgradeComponent() : nullptr;
	if (!GSDebug::CheckAuthority(PC, TEXT("gs.Upgrade.Grant")) || !Upgrades)
	{
		return 0;
	}
	const UGSUpgradeData* Upgrade = FindUpgradeData(UpgradeName);
	if (!Upgrade)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.Upgrade.Grant: unknown upgrade '%s'. Upgrades: %s"), *UpgradeName, *FString::Join(GetUpgradeDataNames(), TEXT(", ")));
		return 0;
	}
	int32 Granted = 0;
	for (int32 Index = 0; Index < FMath::Max(Count, 1); ++Index)
	{
		if (!Upgrades->GrantUpgrade(Upgrade, TEXT("Debug")))
		{
			break; // at MaxStacks
		}
		++Granted;
	}
	if (Count > 1)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("gs.Upgrade.Grant: %s x%d granted (now %d stacks)."), *Upgrade->GetName(), Granted, Upgrades->GetStacks(Upgrade));
	}
	return Granted;
#else
	return 0;
#endif
}

void UGSDebugLibrary::ListUpgrades(AGSPlayerController* PC)
{
#if !UE_BUILD_SHIPPING
	const AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
	const UGSUpgradeComponent* Upgrades = PS ? PS->GetUpgradeComponent() : nullptr;
	const UAbilitySystemComponent* ASC = GSDebug::GetPlayerASC(PC);

	FString Text = FString::Printf(TEXT("Upgrades this run: %d pick(s)"), Upgrades ? Upgrades->GetTotalPicks() : 0);
	for (const FString& Name : GetUpgradeDataNames())
	{
		const UGSUpgradeData* Upgrade = FindUpgradeData(Name);
		if (!Upgrade)
		{
			continue;
		}
		const int32 Stacks = Upgrades ? Upgrades->GetStacks(Upgrade) : 0;
		Text += FString::Printf(TEXT("\n  %s [%s] x%d/%s: %s"), *Name, *UGSUpgradeData::GetRarityName(Upgrade->Rarity), Stacks,
			Upgrade->MaxStacks > 0 ? *FString::FromInt(Upgrade->MaxStacks) : TEXT("inf"), *Upgrade->GetFormattedDescription().ToString());
	}
	if (ASC)
	{
		Text += FString::Printf(TEXT("\n  Stats: ReloadSpeed %.2f, Damage %.2f, FireRate %.2f, MoveSpeed %.0f, PickupRadius %.0f, ExtraJumps %.0f, MagazineSizeMultiplier %.2f, HealthOnKill %.0f"),
			ASC->GetNumericAttribute(UGSAttributeSet::GetReloadSpeedAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetFireRateAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetMoveSpeedAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetPickupRadiusAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetExtraJumpsAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetMagazineSizeMultiplierAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetHealthOnKillAttribute()));
		if (const AGSCharacter* Character = Cast<AGSCharacter>(PC->GetPawn()))
		{
			if (const UGSWeaponComponent* Weapons = Character->GetWeaponComponent())
			{
				Text += FString::Printf(TEXT("\n  Magazine: %d / %d"), Weapons->GetCurrentAmmo(), Weapons->GetCurrentMagazineSize());
			}
		}
		if (const UGSGameplayAbility_Dash* Dash = UGSGameplayAbility_Dash::FindGrantedDash(ASC))
		{
			const UGSDashSettings* DashSettings = Dash->GetSettings();
			Text += FString::Printf(TEXT("\n  Dash charges: %d (base %d + ExtraDashCharges %.0f), %d ready"),
				Dash->GetMaxCharges(ASC->AbilityActorInfo.Get()), DashSettings ? DashSettings->MaxCharges : 1,
				ASC->GetNumericAttribute(UGSAttributeSet::GetExtraDashChargesAttribute()),
				Dash->GetAvailableCharges(ASC->AbilityActorInfo.Get()));
		}
	}

	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	for (const FString& Line : Lines)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("%s"), *Line);
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, Text);
	}
#endif
}

void UGSDebugLibrary::PrintUpgradeOffer(AGSPlayerController* PC)
{
#if !UE_BUILD_SHIPPING
	const AGSPlayerState* PS = PC ? PC->GetPlayerState<AGSPlayerState>() : nullptr;
	const UGSUpgradeComponent* Upgrades = PS ? PS->GetUpgradeComponent() : nullptr;
	if (!Upgrades)
	{
		return;
	}
	FString Text = Upgrades->DescribeOffer();
	const TArray<UGSUpgradeData*> Choices = Upgrades->GetCurrentChoices();
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		Text += FString::Printf(TEXT("\n  [%d] %s: %s"), Index, *Choices[Index]->DisplayName.ToString(), *Choices[Index]->GetFormattedDescription().ToString());
	}
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	for (const FString& Line : Lines)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("%s"), *Line);
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, Text);
	}
#endif
}

FGSUpgradeOfferSimulation UGSDebugLibrary::SimulateUpgradeOffers(int32 Count, int32 Seed)
{
	FGSUpgradeOfferSimulation Result;
	Result.Seed = Seed;

	const TArray<FAssetData> Pools = GSDebug::GetAssetsOfClass(UGSUpgradePool::StaticClass());
	const UGSUpgradePool* Pool = Pools.Num() > 0 ? Cast<UGSUpgradePool>(Pools[0].GetAsset()) : nullptr;
	if (!Pool)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("SimulateUpgradeOffers: no UGSUpgradePool asset found."));
		return Result;
	}
	Result.Pool = Pool->GetName();
	Result.ChoicesPerOffer = Pool->ChoicesPerOffer;

	// Every rarity and pooled upgrade starts at 0, so ones that never come up are reported too.
	for (const EGSUpgradeRarity Rarity : UGSUpgradePool::GetAllRarities())
	{
		Result.RarityChoices.Add(UGSUpgradeData::GetRarityName(Rarity), 0);
		Result.RarityOffers.Add(UGSUpgradeData::GetRarityName(Rarity), 0);
	}
	for (const UGSUpgradeData* Upgrade : Pool->Upgrades)
	{
		if (Upgrade)
		{
			Result.UpgradeOffers.Add(Upgrade->GetName(), 0);
		}
	}

	// A private stream: the run stream is never touched.
	FRandomStream Stream(Seed);
	for (int32 OfferIndex = 0; OfferIndex < FMath::Max(Count, 0); ++OfferIndex)
	{
		const TArray<UGSUpgradeData*> Offer = Pool->RollOffer(Stream, [](const UGSUpgradeData*) { return 0; });
		++Result.Offers;
		Result.TotalChoices += Offer.Num();
		Result.ShortOffers += Offer.Num() < Pool->ChoicesPerOffer ? 1 : 0;

		TSet<EGSUpgradeRarity> RaritiesInOffer;
		for (const UGSUpgradeData* Choice : Offer)
		{
			++Result.UpgradeOffers.FindOrAdd(Choice->GetName());
			++Result.RarityChoices.FindOrAdd(UGSUpgradeData::GetRarityName(Choice->Rarity));
			RaritiesInOffer.Add(Choice->Rarity);
		}
		for (const EGSUpgradeRarity Rarity : RaritiesInOffer)
		{
			++Result.RarityOffers.FindOrAdd(UGSUpgradeData::GetRarityName(Rarity));
		}
	}
	return Result;
}

TArray<FString> UGSDebugLibrary::GetEnemyDataNames()
{
	return GSDebug::GetAssetNames(UGSEnemyData::StaticClass());
}

TArray<FString> UGSDebugLibrary::GetWeaponDataNames()
{
	return GSDebug::GetAssetNames(UGSWeaponData::StaticClass());
}

AGSPlayerController* UGSDebugLibrary::GetFirstPlayer(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	World = FindGameWorld(World);
	return World ? Cast<AGSPlayerController>(World->GetFirstPlayerController()) : nullptr;
}

// ---------------------------------------------------------------------------
// Server-side operations
// ---------------------------------------------------------------------------

TArray<AGSEnemyCharacter*> UGSDebugLibrary::SpawnEnemiesAround(AGSPlayerController* PC, UGSEnemyData* EnemyData, int32 Count)
{
	TArray<AGSEnemyCharacter*> Spawned;
#if !UE_BUILD_SHIPPING
	if (!GSDebug::CheckAuthority(PC, TEXT("gs.Spawn")) || !EnemyData || Count <= 0)
	{
		return Spawned;
	}

	// Enemies only exist in the town; the Market District is a safe area.
	const AGSGameState* GameState = PC->GetWorld()->GetGameState<AGSGameState>();
	if (GameState && GameState->GetCurrentArea() != EGSArea::Town)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.Spawn: refused, the party is in the Market District (enemies only spawn in town)."));
		return Spawned;
	}

	UWorld* World = PC->GetWorld();
	const APawn* Center = PC->GetPawn();
	const FVector Origin = Center ? Center->GetActorLocation() : FVector::ZeroVector;

	// Same location rules as round spawns (ring, NavMesh, reachable, spacing, off-screen, fallback).
	FGSSpawnSearchStats Stats;
	const TArray<FGSSpawnPoint> Points = UGSSpawnLocationFinder::FindSpawnLocations(PC, FMath::Min(Count, 200), &Stats);
	for (const FGSSpawnPoint& Point : Points)
	{
		if (AGSEnemyCharacter* Enemy = AGSEnemyCharacter::SpawnEnemy(World, EnemyData, Point.Location, Origin))
		{
			Spawned.Add(Enemy);
		}
	}

	UE_LOG(LogBanditBaller, Log, TEXT("gs.Spawn: spawned %d x %s around %s at %s (%d ring, %d fallback)"),
		Spawned.Num(), *EnemyData->GetName(), *GetNameSafe(Center), *Origin.ToCompactString(), Stats.Ring, Stats.Fallback);
#endif
	return Spawned;
}

int32 UGSDebugLibrary::KillAllEnemies(AGSPlayerController* PC)
{
	int32 Killed = 0;
#if !UE_BUILD_SHIPPING
	if (!GSDebug::CheckAuthority(PC, TEXT("gs.KillAll")))
	{
		return 0;
	}

	UAbilitySystemComponent* SourceASC = GSDebug::GetPlayerASC(PC);
	TSubclassOf<UGameplayEffect> DamageEffect = PC->GetDebugDamageEffect();
	if (!SourceASC || !DamageEffect)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.KillAll: missing player ASC or DebugDamageEffect on %s"), *GetNameSafe(PC));
		return 0;
	}

	TArray<AGSEnemyCharacter*> Targets;
	for (TActorIterator<AGSEnemyCharacter> It(PC->GetWorld()); It; ++It)
	{
		if (GSDebug::IsAlive(**It))
		{
			Targets.Add(*It);
		}
	}

	for (AGSEnemyCharacter* Enemy : Targets)
	{
		UAbilitySystemComponent* TargetASC = Enemy->GetAbilitySystemComponent();
		if (!TargetASC)
		{
			continue;
		}
		// Through GE_Damage so the normal death flow (and OnEnemyKilled credit) runs.
		const float Lethal = TargetASC->GetNumericAttribute(UGSAttributeSet::GetMaxHealthAttribute()) * 10.f + 1000.f;
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffect, 1.f, SourceASC->MakeEffectContext());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(GSGameplayTags::Data_Damage, Lethal);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
			++Killed;
		}
	}

	UE_LOG(LogBanditBaller, Log, TEXT("gs.KillAll: killed %d enemies"), Killed);
#endif
	return Killed;
}

bool UGSDebugLibrary::SetGodMode(AGSPlayerController* PC, bool bEnabled)
{
#if !UE_BUILD_SHIPPING
	UAbilitySystemComponent* ASC = GSDebug::GetPlayerASC(PC);
	if (!GSDebug::CheckAuthority(PC, TEXT("gs.God")) || !ASC)
	{
		return false;
	}

	// One loose-tag count of our own, so dash i-frames adding/removing theirs don't interfere.
	if (bEnabled != PC->IsDebugGodMode())
	{
		if (bEnabled)
		{
			ASC->AddLooseGameplayTag(GSGameplayTags::State_Invulnerable);
		}
		else
		{
			ASC->RemoveLooseGameplayTag(GSGameplayTags::State_Invulnerable);
		}
		PC->SetDebugGodMode(bEnabled);
	}
	UE_LOG(LogBanditBaller, Log, TEXT("gs.God: %s for %s"), bEnabled ? TEXT("ON") : TEXT("OFF"), *GetNameSafe(PC->GetPawn()));
	return true;
#else
	return false;
#endif
}

bool UGSDebugLibrary::GiveWeaponTo(AGSPlayerController* PC, UGSWeaponData* WeaponData)
{
#if !UE_BUILD_SHIPPING
	if (!GSDebug::CheckAuthority(PC, TEXT("gs.GiveWeapon")) || !WeaponData)
	{
		return false;
	}
	const AGSCharacter* Character = Cast<AGSCharacter>(PC->GetPawn());
	UGSWeaponComponent* Weapons = Character ? Character->GetWeaponComponent() : nullptr;
	const bool bGiven = Weapons && Weapons->GiveWeapon(WeaponData);
	UE_LOG(LogBanditBaller, Log, TEXT("gs.GiveWeapon: %s %s"), *WeaponData->GetName(), bGiven ? TEXT("equipped") : TEXT("FAILED (no pawn/weapon component)"));
	return bGiven;
#else
	return false;
#endif
}

bool UGSDebugLibrary::SetAttributeOn(AGSPlayerController* PC, const FString& AttributeName, float Value)
{
#if !UE_BUILD_SHIPPING
	UAbilitySystemComponent* ASC = GSDebug::GetPlayerASC(PC);
	if (!GSDebug::CheckAuthority(PC, TEXT("gs.SetAttr")) || !ASC)
	{
		return false;
	}

	const FGameplayAttribute Attribute = GSDebug::FindAttribute(AttributeName);
	if (!Attribute.IsValid())
	{
		TArray<FString> Names;
		for (TFieldIterator<FProperty> It(UGSAttributeSet::StaticClass()); It; ++It)
		{
			if (FGameplayAttribute::IsGameplayAttributeDataProperty(*It))
			{
				Names.Add(It->GetName());
			}
		}
		UE_LOG(LogBanditBaller, Warning, TEXT("gs.SetAttr: unknown attribute '%s'. Known: %s"), *AttributeName, *FString::Join(Names, TEXT(", ")));
		return false;
	}

	ASC->SetNumericAttributeBase(Attribute, Value);
	UE_LOG(LogBanditBaller, Log, TEXT("gs.SetAttr: %s = %.2f (current %.2f)"), *Attribute.GetName(), Value, ASC->GetNumericAttribute(Attribute));
	return true;
#else
	return false;
#endif
}

TMap<FString, float> UGSDebugLibrary::GetAttributesOf(const AGSPlayerController* PC)
{
	TMap<FString, float> Result;
	const UAbilitySystemComponent* ASC = GSDebug::GetPlayerASC(PC);
	if (!ASC)
	{
		return Result;
	}

	for (TFieldIterator<FProperty> It(UGSAttributeSet::StaticClass()); It; ++It)
	{
		if (FGameplayAttribute::IsGameplayAttributeDataProperty(*It))
		{
			Result.Add(It->GetName(), ASC->GetNumericAttribute(FGameplayAttribute(*It)));
		}
	}

	if (const AGSCharacter* Character = Cast<AGSCharacter>(PC->GetPawn()))
	{
		if (const UGSWeaponComponent* Weapons = Character->GetWeaponComponent())
		{
			Result.Add(TEXT("Ammo"), static_cast<float>(Weapons->GetCurrentAmmo()));
		}
	}
	if (const UGSProfileSubsystem* Profile = UGSProfileSubsystem::Get(PC))
	{
		Result.Add(TEXT("Cash"), static_cast<float>(Profile->GetCash())); // permanent balance (profile)
	}
	if (const AGSPlayerState* PS = PC->GetPlayerState<AGSPlayerState>())
	{
		if (const UGSProgressionComponent* Progression = PS->GetProgressionComponent())
		{
			Result.Add(TEXT("Level"), static_cast<float>(Progression->GetLevel()));
			Result.Add(TEXT("XP"), static_cast<float>(Progression->GetCurrentXP()));
			Result.Add(TEXT("XPForNextLevel"), static_cast<float>(Progression->GetXPForNextLevel()));
			Result.Add(TEXT("TotalXP"), static_cast<float>(Progression->GetTotalXP()));
		}
		if (const UGSComboComponent* Combo = PS->GetComboComponent())
		{
			Result.Add(TEXT("Combo"), static_cast<float>(Combo->GetComboCount()));
			Result.Add(TEXT("ComboMultiplier"), Combo->GetCashMultiplier());
			Result.Add(TEXT("ComboTimeLeft"), Combo->GetTimeRemaining());
		}
	}
	Result.Add(TEXT("God"), PC->IsDebugGodMode() ? 1.f : 0.f);
	Result.Add(TEXT("Invulnerable"), ASC->HasMatchingGameplayTag(GSGameplayTags::State_Invulnerable) ? 1.f : 0.f);
	Result.Add(TEXT("Dead"), ASC->HasMatchingGameplayTag(GSGameplayTags::State_Dead) ? 1.f : 0.f);
	return Result;
}

void UGSDebugLibrary::ExecuteDebugCommand(AGSPlayerController* PC, const FString& Command, const TArray<FString>& Args)
{
#if !UE_BUILD_SHIPPING
	if (!GSDebug::CheckAuthority(PC, *Command))
	{
		return;
	}

	if (Command == TEXT("Spawn"))
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.Spawn <EnemyDataName> [Count=1] (locations from UGSSpawnLocationFinder). Types: %s"), *FString::Join(GetEnemyDataNames(), TEXT(", ")));
			return;
		}
		UGSEnemyData* Data = FindEnemyData(Args[0]);
		if (!Data)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.Spawn: unknown enemy '%s'. Types: %s"), *Args[0], *FString::Join(GetEnemyDataNames(), TEXT(", ")));
			return;
		}
		const int32 Count = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
		SpawnEnemiesAround(PC, Data, Count);
	}
	else if (Command == TEXT("God"))
	{
		const bool bEnable = Args.Num() > 0 ? Args[0].ToBool() : !PC->IsDebugGodMode();
		SetGodMode(PC, bEnable);
	}
	else if (Command == TEXT("KillAll"))
	{
		KillAllEnemies(PC);
	}
	else if (Command == TEXT("GiveWeapon"))
	{
		UGSWeaponData* Data = Args.Num() > 0 ? FindWeaponData(Args[0]) : nullptr;
		if (!Data)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.GiveWeapon <WeaponDataName>. Weapons: %s"), *FString::Join(GetWeaponDataNames(), TEXT(", ")));
			return;
		}
		GiveWeaponTo(PC, Data);
	}
	else if (Command == TEXT("SetAttr"))
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.SetAttr <Health|MaxHealth|MoveSpeed|Damage|FireRate|PickupRadius|XPGain|ReloadSpeed> <Value>"));
			return;
		}
		SetAttributeOn(PC, Args[0], FCString::Atof(*Args[1]));
	}
	else if (Command == TEXT("XPAdd") || Command == TEXT("XPSetLevel"))
	{
		AGSGameMode* GameMode = PC->GetWorld()->GetAuthGameMode<AGSGameMode>();
		AGSPlayerState* PS = PC->GetPlayerState<AGSPlayerState>();
		if (!GameMode || !PS)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.XP.*: no AGSGameMode / player state on the server."));
			return;
		}
		const int32 Value = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		if (Command == TEXT("XPAdd"))
		{
			if (Value <= 0)
			{
				UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.XP.Add <N> (N > 0, raw XP, XPGain not applied)"));
				return;
			}
			GameMode->AwardXP(PS, static_cast<float>(Value), /*bApplyXPGain*/ false);
		}
		else
		{
			if (Value < 1)
			{
				UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.XP.SetLevel <N> (1..max level)"));
				return;
			}
			GameMode->SetPlayerLevelForDebug(PS, Value);
		}
	}
	else if (Command == TEXT("UpgradeGrant"))
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.Upgrade.Grant <UpgradeName>. Upgrades: %s"), *FString::Join(GetUpgradeDataNames(), TEXT(", ")));
			return;
		}
		const int32 Count = (Args.Num() > 1 && Args[1].IsNumeric()) ? FCString::Atoi(*Args[1]) : 1;
		GrantUpgradeTo(PC, Args[0], Count);
	}
	else if (Command == TEXT("UpgradeList"))
	{
		ListUpgrades(PC);
	}
	else if (Command == TEXT("UpgradeOffer") || Command == TEXT("UpgradeChoose") || Command == TEXT("UpgradeReroll"))
	{
		AGSGameMode* GameMode = PC->GetWorld()->GetAuthGameMode<AGSGameMode>();
		AGSPlayerState* PS = PC->GetPlayerState<AGSPlayerState>();
		if (!GameMode || !PS)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.Upgrade.*: no AGSGameMode / player state on the server."));
			return;
		}
		if (Command == TEXT("UpgradeOffer"))
		{
			GameMode->QueueUpgradeOfferForDebug(PS);
		}
		else if (Command == TEXT("UpgradeChoose"))
		{
			if (Args.Num() < 1 || !Args[0].IsNumeric())
			{
				UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.Upgrade.Choose <0-2>"));
				return;
			}
			GameMode->ChooseUpgrade(PS, FCString::Atoi(*Args[0]));
		}
		else
		{
			GameMode->RerollUpgradeOffer(PS);
		}
		PrintUpgradeOffer(PC);
	}
	else if (Command == TEXT("MarketTeleport"))
	{
		// gs.Market.Teleport: leave the run for the hub map.
		AGSGameMode* GameMode = PC->GetWorld()->GetAuthGameMode<AGSGameMode>();
		if (!GameMode)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.Market.Teleport: not in a run (already in the hub?)."));
			return;
		}
		GameMode->OpenHubMap();
	}
	else if (Command == TEXT("MarketReturn"))
	{
		// gs.Market.Return: start a run from the hub, as the Start Run portal does.
		AGSHubGameMode* Hub = PC->GetWorld()->GetAuthGameMode<AGSHubGameMode>();
		if (!Hub)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.Market.Return: not in the hub (already in a run?)."));
			return;
		}
		Hub->StartRun(PC);
	}
	else if (Command == TEXT("RunSeed"))
	{
		// In the hub: choose the next run's seed (the portal passes it on).
		if (AGSHubGameMode* Hub = PC->GetWorld()->GetAuthGameMode<AGSHubGameMode>())
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogBanditBaller, Log, TEXT("gs.Run.Seed: the next run uses %s. Usage: gs.Run.Seed <N> (0 = random)."),
					Hub->GetNextRunSeed() > 0 ? *FString::Printf(TEXT("seed %d"), Hub->GetNextRunSeed()) : TEXT("a random seed"));
				return;
			}
			Hub->SetNextRunSeed(FCString::Atoi(*Args[0]));
			return;
		}
		AGSGameMode* GameMode = PC->GetWorld()->GetAuthGameMode<AGSGameMode>();
		if (!GameMode)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.Run.Seed: no AGSGameMode on the server."));
			return;
		}
		if (Args.Num() < 1)
		{
			UE_LOG(LogBanditBaller, Log, TEXT("gs.Run.Seed: current run seed is %d. Usage: gs.Run.Seed <N> restarts the run with seed N."), GameMode->GetRunSeed());
			return;
		}
		GameMode->RestartRunWithSeed(FCString::Atoi(*Args[0]));
	}
	else if (Command == TEXT("RoundSkip") || Command == TEXT("RoundSet") || Command == TEXT("RoundPause"))
	{
		AGSGameMode* GameMode = PC->GetWorld()->GetAuthGameMode<AGSGameMode>();
		if (!GameMode)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("gs.Round.*: no AGSGameMode on the server."));
			return;
		}
		if (Command == TEXT("RoundSkip"))
		{
			GameMode->AdvancePhase();
		}
		else if (Command == TEXT("RoundPause"))
		{
			GameMode->SetRoundTimerPaused(Args.Num() > 0 ? Args[0].ToBool() : !GameMode->IsRoundTimerPaused());
		}
		else if (Args.Num() < 1 || FCString::Atoi(*Args[0]) < 1)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.Round.Set <N> (N >= 1)"));
		}
		else
		{
			GameMode->SetRound(FCString::Atoi(*Args[0]));
		}
	}
	else
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("Unknown debug command '%s'"), *Command);
	}
#endif
}

// ---------------------------------------------------------------------------
// World-context versions
// ---------------------------------------------------------------------------

TArray<AGSEnemyCharacter*> UGSDebugLibrary::SpawnEnemies(const UObject* WorldContextObject, const FString& EnemyType, int32 Count)
{
	UGSEnemyData* Data = FindEnemyData(EnemyType);
	if (!Data)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("SpawnEnemies: unknown enemy '%s'. Types: %s"), *EnemyType, *FString::Join(GetEnemyDataNames(), TEXT(", ")));
		return {};
	}
	return SpawnEnemiesAround(GetFirstPlayer(WorldContextObject), Data, Count);
}

int32 UGSDebugLibrary::KillAll(const UObject* WorldContextObject)
{
	return KillAllEnemies(GetFirstPlayer(WorldContextObject));
}

bool UGSDebugLibrary::SetGod(const UObject* WorldContextObject, bool bEnabled)
{
	return SetGodMode(GetFirstPlayer(WorldContextObject), bEnabled);
}

bool UGSDebugLibrary::GiveWeapon(const UObject* WorldContextObject, const FString& WeaponName)
{
	return GiveWeaponTo(GetFirstPlayer(WorldContextObject), FindWeaponData(WeaponName));
}

bool UGSDebugLibrary::SetPlayerAttribute(const UObject* WorldContextObject, const FString& AttributeName, float Value)
{
	return SetAttributeOn(GetFirstPlayer(WorldContextObject), AttributeName, Value);
}

TMap<FString, float> UGSDebugLibrary::GetPlayerAttributes(const UObject* WorldContextObject)
{
	return GetAttributesOf(GetFirstPlayer(WorldContextObject));
}

TArray<FGSEnemyDebugInfo> UGSDebugLibrary::ListEnemiesAlive(const UObject* WorldContextObject)
{
	TArray<FGSEnemyDebugInfo> Result;
	const AGSPlayerController* PC = GetFirstPlayer(WorldContextObject);
	UWorld* World = PC ? PC->GetWorld() : FindGameWorld(nullptr);
	if (!World)
	{
		return Result;
	}
	const APawn* Player = PC ? PC->GetPawn() : nullptr;

	for (TActorIterator<AGSEnemyCharacter> It(World); It; ++It)
	{
		const AGSEnemyCharacter* Enemy = *It;
		if (!GSDebug::IsAlive(*Enemy))
		{
			continue;
		}

		FGSEnemyDebugInfo& Info = Result.AddDefaulted_GetRef();
		Info.Name = Enemy->GetActorNameOrLabel();
		Info.EnemyType = GetNameSafe(Enemy->GetEnemyData());
		Info.Location = Enemy->GetActorLocation();
		Info.DistanceToPlayer = Player ? FVector::Dist(Player->GetActorLocation(), Info.Location) : -1.f;
		if (const UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent())
		{
			Info.Health = ASC->GetNumericAttribute(UGSAttributeSet::GetHealthAttribute());
			Info.MaxHealth = ASC->GetNumericAttribute(UGSAttributeSet::GetMaxHealthAttribute());
			Info.bAttacking = ASC->HasMatchingGameplayTag(GSGameplayTags::State_Attacking);
		}
	}
	Result.Sort([](const FGSEnemyDebugInfo& A, const FGSEnemyDebugInfo& B) { return A.DistanceToPlayer < B.DistanceToPlayer; });
	return Result;
}
