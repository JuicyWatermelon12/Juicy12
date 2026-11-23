// BanditBaller

#include "GSDebugLibrary.h"
#include "AbilitySystemComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "BanditBaller.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
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
#include "GSCombatSettings.h"
#include "GSCriticalHit.h"
#include "GSGameMode.h"
#include "GSFloatingGun.h"
#include "GSFloatingGunComponent.h"
#include "GSRazorCardsComponent.h"
#include "GSRazorCardsSettings.h"
#include "GSBarbedWireComponent.h"
#include "GSBarbedWireSettings.h"
#include "GSDynamiteComponent.h"
#include "GSDynamiteSettings.h"
#include "GSLuckyCoinComponent.h"
#include "GSLuckyCoinSettings.h"
#include "GSMolotovComponent.h"
#include "GSMolotovSettings.h"
#include "GSTomahawkComponent.h"
#include "GSTomahawkSettings.h"
#include "GSSecondWindComponent.h"
#include "GSShieldComponent.h"
#include "GSGameplayAbility_Dash.h"
#include "GSGameplayAbility_Slide.h"
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

int32 UGSDebugLibrary::GrantUpgradeTo(AGSPlayerController* PC, const FString& UpgradeName, int32 Count, EGSUpgradeRarity Rarity)
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
		if (!Upgrades->GrantUpgrade(Upgrade, Rarity, TEXT("Debug")))
		{
			break; // no such rarity (whole units have no Rare)
		}
		++Granted;
	}
	if (Count > 1 || Rarity != EGSUpgradeRarity::Common)
	{
		UE_LOG(LogBanditBaller, Log, TEXT("gs.Upgrade.Grant: %s x%d %s granted (now %d pick(s), total %s)."), *Upgrade->GetName(), Granted,
			*UGSUpgradeData::GetRarityName(Rarity), Upgrades->GetStacks(Upgrade), *Upgrade->FormatValue(Upgrades->GetTotalValue(Upgrade)));
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

	// Grouped by category: Stat Boosts, Abilities, Legendary abilities, Weapons.
	TArray<const UGSUpgradeData*> All;
	for (const FString& Name : GetUpgradeDataNames())
	{
		if (const UGSUpgradeData* Upgrade = FindUpgradeData(Name))
		{
			All.Add(Upgrade);
		}
	}
	auto GroupOf = [](const UGSUpgradeData* Upgrade)
	{
		switch (Upgrade->Category)
		{
		case EGSUpgradeCategory::StatBoost: return 0;
		case EGSUpgradeCategory::Ability: return Upgrade->bLegendaryAbility ? 2 : 1;
		default: return 3;
		}
	};
	const TCHAR* GroupNames[] = { TEXT("Stat Boosts"), TEXT("Abilities"), TEXT("Legendary abilities"), TEXT("Weapons") };
	for (int32 Group = 0; Group < UE_ARRAY_COUNT(GroupNames); ++Group)
	{
		Text += FString::Printf(TEXT("\n  %s:"), GroupNames[Group]);
		for (const UGSUpgradeData* Upgrade : All)
		{
			if (GroupOf(Upgrade) != Group)
			{
				continue;
			}
			const FGSUpgradeStack* Entry = Upgrades ? Upgrades->FindEntry(Upgrade) : nullptr;
			const int32 Picks = Entry ? Entry->Stacks : 0;
			FString PickRarities;
			for (const EGSUpgradeRarity Rarity : UGSUpgradePool::GetAllRarities())
			{
				const int32 Count = Entry ? Entry->Picks.FilterByPredicate([Rarity](const FGSUpgradePick& P) { return P.Rarity == Rarity; }).Num() : 0;
				if (Count > 0)
				{
					PickRarities += FString::Printf(TEXT("%s%d %s"), PickRarities.IsEmpty() ? TEXT("") : TEXT(", "), Count, *UGSUpgradeData::GetRarityName(Rarity));
				}
			}
			Text += FString::Printf(TEXT("\n    %s: %d pick(s)%s, total %s (Common pick %s%s): %s"), *Upgrade->GetName(), Picks,
				PickRarities.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" (%s)"), *PickRarities),
				*Upgrade->FormatNowText(Entry ? Entry->TotalValue : 0.f).ToString(), *Upgrade->FormatValue(Upgrade->BaseValue),
				Upgrade->bWholeUnits ? TEXT(", whole units") : TEXT(""),
				*Upgrade->GetFormattedDescription(Picks).ToString());
		}
	}
	if (Upgrades)
	{
		const AGSGameMode* PoolOwner = PC->GetWorld()->GetAuthGameMode<AGSGameMode>();
		const UGSUpgradePool* Pool = PoolOwner && PoolOwner->GetUpgradePool() ? PoolOwner->GetUpgradePool() : GetDefault<UGSUpgradePool>();
		const TArray<const UGSUpgradeData*> Weapons = Upgrades->GetOwnedWeapons();
		FString Slots;
		for (int32 Index = 0; Index < FMath::Max(Pool->MaxWeapons, Weapons.Num()); ++Index)
		{
			Slots += FString::Printf(TEXT("%s%s"), Index > 0 ? TEXT(", ") : TEXT(""), Weapons.IsValidIndex(Index) ? *Weapons[Index]->DisplayName.ToString() : TEXT("-"));
		}
		Text += FString::Printf(TEXT("\n  Weapon slots (MaxWeapons %d): %s; first offer's weapon card %s"), Pool->MaxWeapons, *Slots,
			Upgrades->IsFirstOfferPending() ? TEXT("still pending") : TEXT("done"));
	}
	if (ASC)
	{
		Text += FString::Printf(TEXT("\n  Stats: ReloadSpeed %.2f, Damage %.2f, FireRate %.2f, MoveSpeed %.0f, PickupRadius %.0f, ExtraJumps %.0f, MagazineSizeMultiplier %.3f, HealthOnKill %.0f, CritChance %.3f, Pierce %.0f, PowderKeg %g, Shields %.0f, SecondWinds %.0f, FloatingGunLevel %.0f, FloatingGunPower %g, RazorCardsLevel %.0f, RazorCardsPower %g, BarbedWireLevel %.0f, BarbedWirePower %g"),
			ASC->GetNumericAttribute(UGSAttributeSet::GetReloadSpeedAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetFireRateAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetMoveSpeedAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetPickupRadiusAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetExtraJumpsAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetMagazineSizeMultiplierAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetHealthOnKillAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetCritChanceAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetPierceAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetPowderKegAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetShieldsAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetSecondWindsAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetFloatingGunLevelAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetFloatingGunPowerAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetRazorCardsLevelAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetRazorCardsPowerAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetBarbedWireLevelAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetBarbedWirePowerAttribute()));
		Text += FString::Printf(TEXT(", MolotovLevel %.0f, MolotovPower %g, TomahawkLevel %.0f, TomahawkPower %g"),
			ASC->GetNumericAttribute(UGSAttributeSet::GetMolotovLevelAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetMolotovPowerAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetTomahawkLevelAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetTomahawkPowerAttribute()));
		Text += FString::Printf(TEXT(", DynamiteLevel %.0f, DynamitePower %g, LuckyCoinLevel %.0f, LuckyCoinPower %g"),
			ASC->GetNumericAttribute(UGSAttributeSet::GetDynamiteLevelAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetDynamitePowerAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetLuckyCoinLevelAttribute()),
			ASC->GetNumericAttribute(UGSAttributeSet::GetLuckyCoinPowerAttribute()));
		if (const AGSCharacter* Character = Cast<AGSCharacter>(PC->GetPawn()))
		{
			// Slide and dash scale with Move Speed (at least x1); a slide or dash keeps the ratio it started with.
			const float SpeedRatio = Character->GetMoveSpeedRatio();
			const UGSGameplayAbility_Slide* Slide = UGSGameplayAbility_Slide::FindGrantedSlide(ASC);
			const UGSGameplayAbility_Dash* Dash = UGSGameplayAbility_Dash::FindGrantedDash(ASC);
			const UGSSlideSettings* SlideSettings = Slide ? Slide->GetSettings() : nullptr;
			const UGSDashSettings* DashSettings = Dash ? Dash->GetSettings() : nullptr;
			Text += FString::Printf(TEXT(", speed ratio x%.2f, slide cap %.0f cm/s, dash distance %.0f cm"), SpeedRatio,
				SlideSettings ? SlideSettings->GetMaxSpeed(SpeedRatio) : 0.f, DashSettings ? DashSettings->GetDistance(SpeedRatio) : 0.f);

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
		if (const UGSShieldComponent* Shields = PS ? PS->GetShieldComponent() : nullptr)
		{
			FString Cooldowns;
			for (int32 Index = 0; Index < Shields->GetNumShields(); ++Index)
			{
				Cooldowns += FString::Printf(TEXT("%s%s"), Cooldowns.IsEmpty() ? TEXT("") : TEXT(", "),
					Shields->IsShieldReady(Index) ? TEXT("ready") : *FString::Printf(TEXT("%.1f s"), Shields->GetCooldownRemaining(Index)));
			}
			Text += FString::Printf(TEXT("\n  Floating shields: %d (%d ready%s%s), %d hit(s) blocked this run"), Shields->GetNumShields(),
				Shields->GetNumReady(), Cooldowns.IsEmpty() ? TEXT("") : TEXT(": "), *Cooldowns, Shields->GetHitsBlocked());
		}
		if (const AGSCharacter* GunOwner = Cast<AGSCharacter>(PC->GetPawn()))
		{
			if (const UGSFloatingGunComponent* Guns = GunOwner->GetFloatingGuns())
			{
				FString GunText;
				for (const AGSFloatingGun* Gun : Guns->GetGuns())
				{
					if (Gun)
					{
						GunText += FString::Printf(TEXT("%s%s shoulder: %d shot(s), target %s"), GunText.IsEmpty() ? TEXT("") : TEXT("; "),
							Gun->GetSide() > 0.f ? TEXT("right") : TEXT("left"), Gun->GetShotsFired(), *GetNameSafe(Gun->GetTarget()));
					}
				}
				const AGSFloatingGun* FirstGun = Guns->GetGun(0);
				const FString Rate = FirstGun ? FString::Printf(TEXT(", x%.2f damage, %.2f shots/s each"), FirstGun->GetDamageMultiplier(),
					FirstGun->GetShotInterval() > 0.0 ? 1.0 / FirstGun->GetShotInterval() : 0.0) : FString();
				Text += FString::Printf(TEXT("\n  Floating Gun: %d pick(s), power %g, %d gun(s)%s%s%s"), Guns->GetLevel(), Guns->GetPower(), Guns->GetGuns().Num(), *Rate,
					GunText.IsEmpty() ? TEXT("") : TEXT(": "), *GunText);
			}
			if (const UGSRazorCardsComponent* Cards = GunOwner->GetRazorCards())
			{
				const UGSRazorCardsSettings* CardSettings = Cards->GetSettings();
				const double Interval = Cards->GetThrowInterval();
				Text += FString::Printf(TEXT("\n  Razor Cards: %d pick(s), power %g, x%.2f damage (%.1f per card), %.2f cards/s, %d card(s) thrown, target %s"),
					Cards->GetLevel(), Cards->GetPower(), Cards->GetDamageMultiplier(),
					CardSettings ? CardSettings->Damage * Cards->GetDamageMultiplier() : 0.f, Interval > 0.0 ? 1.0 / Interval : 0.0,
					Cards->GetCardsThrown(), *GetNameSafe(Cards->GetTarget()));
			}
			if (const UGSBarbedWireComponent* Wire = GunOwner->GetBarbedWire())
			{
				const UGSBarbedWireSettings* WireSettings = Wire->GetSettings();
				Text += FString::Printf(TEXT("\n  Barbed Wire: %d pick(s), power %g, radius %.0f cm, x%.2f damage (%.2f per tick x Damage), every %.2f s, %d tick(s), %d hit(s), %d inside last tick"),
					Wire->GetLevel(), Wire->GetPower(), Wire->GetRadius(), Wire->GetDamageMultiplier(),
					WireSettings ? WireSettings->Damage * Wire->GetDamageMultiplier() : 0.f, WireSettings ? WireSettings->TickInterval : 0.f,
					Wire->GetTicks(), Wire->GetHitsDealt(), Wire->GetLastTickHits());
			}
			if (const UGSDynamiteComponent* Dynamite = GunOwner->GetDynamite())
			{
				Text += FString::Printf(TEXT("\n  Dynamite: %d pick(s), power %g, x%.2f damage (%.1f per blast x Damage), blast radius %.0f cm, every %.2f s, lead x%.2f, %d thrown, %d blast(s) (%d caught their target), %d hit(s), %d hit by the last blast"),
					Dynamite->GetLevel(), Dynamite->GetPower(), Dynamite->GetDamageMultiplier(),
					Dynamite->GetSettings() ? Dynamite->GetSettings()->Damage * Dynamite->GetDamageMultiplier() : 0.f, Dynamite->GetBlastRadius(),
					Dynamite->GetThrowInterval(), Dynamite->GetSettings() ? Dynamite->GetSettings()->LeadFactor : 0.f, Dynamite->GetThrows(), Dynamite->GetBlasts(),
					Dynamite->GetTargetHits(), Dynamite->GetHitsDealt(), Dynamite->GetLastBlastHits());
			}
			if (const UGSLuckyCoinComponent* Coin = GunOwner->GetLuckyCoin())
			{
				Text += FString::Printf(TEXT("\n  Lucky Coin: %d pick(s), power %g, x%.2f damage (%.1f per hit x Damage), %d bounce(s) (up to %d hits a flip), every %.2f s, %d flip(s), %d hit(s), %d hit by the last flip, target %s"),
					Coin->GetLevel(), Coin->GetPower(), Coin->GetDamageMultiplier(),
					Coin->GetSettings() ? Coin->GetSettings()->Damage * Coin->GetDamageMultiplier() : 0.f, Coin->GetBounces(), 1 + Coin->GetBounces(),
					Coin->GetFlipInterval(), Coin->GetFlips(), Coin->GetHitsDealt(), Coin->GetLastFlipHits().Num(), *GetNameSafe(Coin->GetTarget()));
			}
			if (const UGSMolotovComponent* Molotov = GunOwner->GetMolotov())
			{
				const UGSMolotovSettings* MolotovSettings = Molotov->GetSettings();
				Text += FString::Printf(TEXT("\n  Molotov: %d pick(s), power %g, x%.2f damage (%.2f per burn x Damage), patch radius %.0f cm, every %.2f s, lead x%.2f, %d thrown, %d patch(es) lit (%d caught their target at the first burn), %d burning now, %d burn(s), %d hit(s)"),
					Molotov->GetLevel(), Molotov->GetPower(), Molotov->GetDamageMultiplier(),
					MolotovSettings ? MolotovSettings->BurnDamage * Molotov->GetDamageMultiplier() : 0.f, Molotov->GetPatchRadius(), Molotov->GetThrowInterval(),
					MolotovSettings ? MolotovSettings->LeadFactor : 0.f, Molotov->GetThrows(), Molotov->GetPatchesLit(), Molotov->GetFirstBurnTargetHits(),
					Molotov->GetActivePatches().Num(), Molotov->GetBurns(), Molotov->GetHitsDealt());
			}
			if (const UGSTomahawkComponent* Tomahawk = GunOwner->GetTomahawk())
			{
				const UGSTomahawkSettings* TomahawkSettings = Tomahawk->GetSettings();
				Text += FString::Printf(TEXT("\n  Tomahawk: %d pick(s), power %g, x%.2f damage (%.1f per hit x Damage), %d tomahawk(s) a throw, every %.2f s, %d throw(s), %d tomahawk(s) thrown, %d in the air, %d hit(s)"),
					Tomahawk->GetLevel(), Tomahawk->GetPower(), Tomahawk->GetDamageMultiplier(),
					TomahawkSettings ? TomahawkSettings->Damage * Tomahawk->GetDamageMultiplier() : 0.f, Tomahawk->GetTomahawksPerThrow(), Tomahawk->GetThrowInterval(),
					Tomahawk->GetThrows(), Tomahawk->GetTomahawksThrown(), Tomahawk->GetActiveTomahawks().Num(), Tomahawk->GetHitsDealt());
			}
		}
		if (const UGSSecondWindComponent* SecondWind = PS ? PS->GetSecondWindComponent() : nullptr)
		{
			Text += FString::Printf(TEXT("\n  Second Wind: %d left, %d used this run"), SecondWind->GetSecondWindsLeft(), SecondWind->GetUsesThisRun());
		}
		if (const AGSGameMode* GameMode = PC->GetWorld()->GetAuthGameMode<AGSGameMode>())
		{
			const UGSCombatSettings* Combat = GameMode->GetCombatSettings() ? GameMode->GetCombatSettings() : GetDefault<UGSCombatSettings>();
			const float Kegs = ASC->GetNumericAttribute(UGSAttributeSet::GetPowderKegAttribute());
			Text += FString::Printf(TEXT("\n  Powder Keg: %g stack(s), %.1f%% chance per kill (%.0f%% per stack), blast %.0f damage x Damage %.2f in %.1f m"),
				Kegs, Combat->GetRawExplodeChance(Kegs) * 100.f, Combat->ChancePerStack * 100.f, Combat->BlastDamage,
				ASC->GetNumericAttribute(UGSAttributeSet::GetDamageAttribute()), Combat->BlastRadius / 100.f);
			if (Combat->GetDoubleBlastChance(Kegs) > 0.f)
			{
				Text += FString::Printf(TEXT("; past 100%%: every kill explodes, %.1f%% of blasts double size (%.1f m), %d so far"),
					Combat->GetDoubleBlastChance(Kegs) * 100.f, Combat->BlastRadius * Combat->DoubleBlastRadiusMultiplier / 100.f,
					GameMode->GetDoublePowderKegBlasts());
			}
			const float Crit = ASC->GetNumericAttribute(UGSAttributeSet::GetCritChanceAttribute());
			Text += FString::Printf(TEXT("\n  Loaded Dice: %.1f%% crit chance (x%.1f)"), Crit * 100.f, Combat->CritMultiplier);
			if (GSCombat::GetTripleCritChance(Crit) > 0.f)
			{
				Text += FString::Printf(TEXT("; past 100%%: every hit crits, %.1f%% of them triple (x%.1f)"),
					GSCombat::GetTripleCritChance(Crit) * 100.f, Combat->TripleCritMultiplier);
			}
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
	const TArray<FGSUpgradeChoice>& Choices = Upgrades->GetCurrentOffer();
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		const FGSUpgradeChoice& Choice = Choices[Index];
		Text += FString::Printf(TEXT("\n  [%d] %s %s (%s): %s. %s"), Index, *UGSUpgradeData::GetRarityName(Choice.Rarity),
			*Choice.Upgrade->DisplayName.ToString(), *Choice.Upgrade->GetCategoryName(), *Choice.Upgrade->FormatValue(Upgrades->GetChoiceValue(Choice)),
			*Choice.Upgrade->GetFormattedDescription(Upgrades->GetStacks(Choice.Upgrade)).ToString());
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

namespace
{
	TAutoConsoleVariable<int32> CVarAutoPickUpgrades(
		TEXT("gs.Debug.AutoPickUpgrades"),
		0,
		TEXT("1 = take every upgrade offer automatically a frame after it opens (debug playtests): an unowned weapon while a ")
		TEXT("slot is free, else an owned weapon, else the rarest card; an all-Common offer without a weapon is rerolled first ")
		TEXT("while rerolls last."),
		ECVF_Cheat);

	bool GAutoPickScheduled = false;
}

int32 UGSDebugLibrary::ChooseUpgradeForAutoPick(const TArray<FGSUpgradeChoice>& Offer, const UGSUpgradeComponent* Upgrades, int32 MaxWeapons, bool bCanReroll)
{
	if (Offer.IsEmpty() || !Upgrades)
	{
		return INDEX_NONE;
	}
	const int32 OwnedWeapons = Upgrades->GetOwnedWeapons().Num();
	int32 NewWeapon = INDEX_NONE, OwnedWeapon = INDEX_NONE, Rarest = 0;
	for (int32 Index = 0; Index < Offer.Num(); ++Index)
	{
		const FGSUpgradeChoice& Choice = Offer[Index];
		if (!Choice.Upgrade)
		{
			continue;
		}
		if (Choice.Upgrade->Category == EGSUpgradeCategory::Weapon)
		{
			const bool bOwned = Upgrades->GetStacks(Choice.Upgrade) > 0;
			if (!bOwned && OwnedWeapons < MaxWeapons && NewWeapon == INDEX_NONE)
			{
				NewWeapon = Index;
			}
			else if (bOwned && OwnedWeapon == INDEX_NONE)
			{
				OwnedWeapon = Index;
			}
		}
		if (static_cast<int32>(Choice.Rarity) > static_cast<int32>(Offer[Rarest].Rarity))
		{
			Rarest = Index;
		}
	}
	if (NewWeapon != INDEX_NONE)
	{
		return NewWeapon;
	}
	if (OwnedWeapon != INDEX_NONE)
	{
		return OwnedWeapon;
	}
	if (bCanReroll && Offer[Rarest].Rarity == EGSUpgradeRarity::Common)
	{
		return INDEX_NONE; // nothing but plain Common cards: try a reroll
	}
	return Rarest;
}

void UGSDebugLibrary::ScheduleAutoPickUpgrades(AGSGameMode* GameMode)
{
#if !UE_BUILD_SHIPPING
	if (CVarAutoPickUpgrades.GetValueOnGameThread() == 0 || !GameMode || GAutoPickScheduled)
	{
		return;
	}
	// A frame later (the core ticker runs while the game is paused for the choice), outside the offer events.
	GAutoPickScheduled = true;
	TWeakObjectPtr<AGSGameMode> WeakGameMode(GameMode);
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakGameMode](float) -> bool
	{
		GAutoPickScheduled = false;
		AGSGameMode* Mode = WeakGameMode.Get();
		const AGSGameState* GS = Mode ? Mode->GetGameState<AGSGameState>() : nullptr;
		if (!GS || CVarAutoPickUpgrades.GetValueOnGameThread() == 0)
		{
			return false;
		}
		const int32 MaxWeapons = Mode->GetUpgradePool() ? Mode->GetUpgradePool()->MaxWeapons : 3;
		for (APlayerState* BasePS : GS->PlayerArray)
		{
			AGSPlayerState* PS = Cast<AGSPlayerState>(BasePS);
			UGSUpgradeComponent* Upgrades = PS ? PS->GetUpgradeComponent() : nullptr;
			// Every queued offer in turn (each pick opens the next); at most a few rerolls per offer.
			for (int32 Guard = 0; Upgrades && Upgrades->HasOpenOffer() && Guard < 1000; ++Guard)
			{
				const int32 Index = ChooseUpgradeForAutoPick(Upgrades->GetCurrentOffer(), Upgrades, MaxWeapons, Upgrades->GetRerollsLeft() > 0);
				if (Index == INDEX_NONE)
				{
					UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] auto-pick (debug): reroll (%d left)"), Upgrades->GetRerollsLeft());
					if (!Mode->RerollUpgradeOffer(PS))
					{
						Mode->ChooseUpgrade(PS, 0);
					}
					continue;
				}
				const FGSUpgradeChoice Choice = Upgrades->GetCurrentOffer()[Index];
				UE_LOG(LogBanditBaller, Log, TEXT("[upgrade] auto-pick (debug): card %d, %s %s"), Index, *UGSUpgradeData::GetRarityName(Choice.Rarity),
					*GetNameSafe(Choice.Upgrade));
				Mode->ChooseUpgrade(PS, Index);
			}
		}
		return false; // one shot
	}));
#endif
}

FGSUpgradeOfferSimulation UGSDebugLibrary::SimulateUpgradeOffers(int32 Count, int32 Seed)
{
	return SimulateUpgradeOffersOwning(Count, Seed, TArray<FString>());
}

FGSUpgradeOfferSimulation UGSDebugLibrary::SimulateUpgradeOffersOwning(int32 Count, int32 Seed, const TArray<FString>& OwnedUpgrades)
{
	FGSUpgradeOfferSimulation Result;
	Result.Seed = Seed;
	TSet<const UGSUpgradeData*> Owned;
	for (const FString& Name : OwnedUpgrades)
	{
		if (const UGSUpgradeData* Upgrade = FindUpgradeData(Name))
		{
			Owned.Add(Upgrade);
			Result.Owned.Add(Upgrade->GetName());
		}
		else
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("SimulateUpgradeOffers: unknown upgrade '%s' (not counted as owned)."), *Name);
		}
	}

	const TArray<FAssetData> Pools = GSDebug::GetAssetsOfClass(UGSUpgradePool::StaticClass());
	const UGSUpgradePool* Pool = Pools.Num() > 0 ? Cast<UGSUpgradePool>(Pools[0].GetAsset()) : nullptr;
	if (!Pool)
	{
		UE_LOG(LogBanditBaller, Warning, TEXT("SimulateUpgradeOffers: no UGSUpgradePool asset found."));
		return Result;
	}
	Result.Pool = Pool->GetName();
	Result.ChoicesPerOffer = Pool->ChoicesPerOffer;

	// Every rarity, category and pooled upgrade starts at 0, so ones that never come up are reported too.
	auto CategoryKey = [](const UGSUpgradeData* Upgrade) { return Upgrade->GetCategoryName(); }; // Legendary abilities apart
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
			Result.UpgradeChoices.Add(Upgrade->GetName(), 0);
			Result.CategoryChoices.Add(CategoryKey(Upgrade), 0);
			Result.CategoryOffers.Add(CategoryKey(Upgrade), 0);
			for (const EGSUpgradeRarity Rarity : UGSUpgradePool::GetAllRarities())
			{
				Result.CategoryRarityChoices.Add(CategoryKey(Upgrade) + TEXT(" / ") + UGSUpgradeData::GetRarityName(Rarity), 0);
			}
		}
	}

	// A private stream: the run stream is never touched.
	FRandomStream Stream(Seed);
	for (int32 OfferIndex = 0; OfferIndex < FMath::Max(Count, 0); ++OfferIndex)
	{
		const TArray<FGSUpgradeChoice> Offer = Pool->RollOffer(Stream, [&Owned](const UGSUpgradeData* Upgrade) { return Owned.Contains(Upgrade) ? 1 : 0; });
		++Result.Offers;
		Result.TotalChoices += Offer.Num();
		Result.ShortOffers += Offer.Num() < Pool->ChoicesPerOffer ? 1 : 0;

		TSet<EGSUpgradeRarity> RaritiesInOffer;
		TSet<FString> CategoriesInOffer;
		for (const FGSUpgradeChoice& Choice : Offer)
		{
			const FString Category = CategoryKey(Choice.Upgrade);
			++Result.UpgradeOffers.FindOrAdd(Choice.Upgrade->GetName()); // at most once per offer (cards differ)
			++Result.UpgradeChoices.FindOrAdd(Choice.Upgrade->GetName());
			++Result.RarityChoices.FindOrAdd(UGSUpgradeData::GetRarityName(Choice.Rarity));
			++Result.CategoryChoices.FindOrAdd(Category);
			++Result.CategoryRarityChoices.FindOrAdd(Category + TEXT(" / ") + UGSUpgradeData::GetRarityName(Choice.Rarity));
			RaritiesInOffer.Add(Choice.Rarity);
			CategoriesInOffer.Add(Category);
		}
		for (const EGSUpgradeRarity Rarity : RaritiesInOffer)
		{
			++Result.RarityOffers.FindOrAdd(UGSUpgradeData::GetRarityName(Rarity));
		}
		for (const FString& Category : CategoriesInOffer)
		{
			++Result.CategoryOffers.FindOrAdd(Category);
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
				UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.XP.SetLevel <N> (N >= 1; levels are endless)"));
				return;
			}
			GameMode->SetPlayerLevelForDebug(PS, Value);
		}
	}
	else if (Command == TEXT("HealthOrbSpawn"))
	{
		AGSGameMode* GameMode = PC->GetWorld()->GetAuthGameMode<AGSGameMode>();
		const int32 Count = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
		if (!GameMode || Count < 1)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.HealthOrb.Spawn [Count=1] (in a run; drops health orbs around the player)"));
			return;
		}
		const int32 Dropped = GameMode->SpawnHealthOrbsForDebug(PC, Count);
		UE_LOG(LogBanditBaller, Log, TEXT("gs.HealthOrb.Spawn: %d health orb(s) dropped around the player."), Dropped);
	}
	else if (Command == TEXT("MolotovPatches"))
	{
		const AGSCharacter* Character = Cast<AGSCharacter>(PC->GetPawn());
		UGSMolotovComponent* Molotov = Character ? Character->GetMolotov() : nullptr;
		const int32 Count = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 4;
		if (!Molotov || !Molotov->GetSettings() || Count < 1)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.Molotov.Patches [Count=4] (in a run; lights fire patches 4 m around the player at the current Molotov power)"));
			return;
		}
		const int32 Lit = Molotov->SpawnPatchesForDebug(Count);
		UE_LOG(LogBanditBaller, Log, TEXT("gs.Molotov.Patches: %d patch(es) lit around the player (radius %.0f cm, %.2f per burn)."), Lit,
			Molotov->GetPatchRadius(), Molotov->GetBurnDamage());
	}
	else if (Command == TEXT("UpgradeGrant"))
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogBanditBaller, Warning, TEXT("Usage: gs.Upgrade.Grant <UpgradeName> [Count=1] [Rarity=Common]. Upgrades: %s"), *FString::Join(GetUpgradeDataNames(), TEXT(", ")));
			return;
		}
		// [Count] and [Rarity] are both optional: "Damage 3 Epic", "Damage Epic", "Damage 3".
		int32 Count = 1;
		EGSUpgradeRarity Rarity = EGSUpgradeRarity::Common;
		for (int32 Index = 1; Index < Args.Num(); ++Index)
		{
			if (Args[Index].IsNumeric())
			{
				Count = FCString::Atoi(*Args[Index]);
			}
			else if (!UGSUpgradeData::ParseRarity(Args[Index], Rarity))
			{
				UE_LOG(LogBanditBaller, Warning, TEXT("gs.Upgrade.Grant: '%s' is no rarity (Common, Rare, Epic, Legendary)."), *Args[Index]);
				return;
			}
		}
		GrantUpgradeTo(PC, Args[0], Count, Rarity);
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
