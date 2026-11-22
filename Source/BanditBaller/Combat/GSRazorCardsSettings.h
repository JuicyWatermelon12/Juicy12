// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSRazorCardsSettings.generated.h"

class UGameplayEffect;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

/**
 * Razor Cards tuning (DA_RazorCardsSettings, set as RazorCardsSettings on BP_GSGameMode): the weapon throws playing
 * cards from the player, one at a time, at the nearest living enemy in range with a clear line; each card homes in on
 * the enemy it was thrown at (UGSRazorCardsComponent, AGSProjectile homing).
 */
UCLASS(BlueprintType, Const)
class BANDITBALLER_API UGSRazorCardsSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Throwing ---

	/** Damage per card, before the power bonus, the player's Damage attribute and a critical hit (Loaded Dice). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0"))
	float Damage = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0.01"))
	float CardsPerSecond = 2.f;

	/** It only throws at living enemies this close to the player with a clear line (world geometry blocks it). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0", Units = "cm"))
	float Range = 2500.f;

	/** How often it looks for the nearest target (it also checks right before each throw). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing", meta = (ClampMin = "0", Units = "s"))
	float TargetRefreshInterval = 0.1f;

	/**
	 * Each point of weapon power (the RazorCardsPower attribute: Common picks after the first add 1, a Rare one 1.5, ...)
	 * adds this much damage and fire rate (0.1 = +10% of the base per point, added up, not compounded; fractions count).
	 * The card's "+10%" comes from DA_Upgrade_RazorCards' DisplayScale; the Razor Cards test checks that they match.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing|Power", meta = (ClampMin = "0"))
	float DamageBonusPerPower = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing|Power", meta = (ClampMin = "0"))
	float FireRateBonusPerPower = 0.1f;

	/** The damage effect (GE_Damage: SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** Where cards leave the player: from the capsule center, in the player's facing frame (X forward, Z up). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwing")
	FVector ThrowOffset = FVector(30.f, 0.f, 40.f);

	// --- The card (a homing projectile) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ClampMin = "1", Units = "cm/s"))
	float CardSpeed = 3000.f;

	/** How fast a card turns toward its target (degrees a second; "tight" homing). Its speed stays the same. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ClampMin = "0"))
	float HomingTurnRate = 720.f;

	/** Seconds before an unspent card disappears (it can chase a little past the range). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ClampMin = "0.01", Units = "s"))
	float CardLifetime = 1.25f;

	/** Radius of the card's sweep. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ClampMin = "0", Units = "cm"))
	float CardRadius = 10.f;

	/** Projectiles created up front the first time a card is thrown. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card", meta = (ClampMin = "0"))
	int32 PoolPrewarm = 8;

	// --- Look (placeholder) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> CardMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> CardMaterial;

	/** Mesh scale: a thin flat card (the engine cube is 100 cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector CardScale = FVector(0.25f, 0.18f, 0.01f);

	/** Spin around the card's own up axis while it flies (degrees a second). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	float CardSpinRate = 1080.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<USoundBase> ThrowSound;

	/** Played where a card hits something. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Look")
	FVector ImpactEffectScale = FVector(0.2f);

	float GetDamageMultiplier(float Power) const { return 1.f + DamageBonusPerPower * FMath::Max(Power, 0.f); }
	float GetFireRateMultiplier(float Power) const { return 1.f + FireRateBonusPerPower * FMath::Max(Power, 0.f); }

	/** Seconds between cards at weapon power Power. */
	double GetThrowInterval(float Power = 0.f) const
	{
		return 1.0 / FMath::Max(static_cast<double>(CardsPerSecond) * GetFireRateMultiplier(Power), 0.01);
	}
};
