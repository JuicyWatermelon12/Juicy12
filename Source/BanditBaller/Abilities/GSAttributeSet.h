// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GSAttributeSet.generated.h"

/**
 * Core character attributes. Values are initialized and modified only through GameplayEffects
 * (starting values come from GE_InitPlayerStats, not C++).
 * Lives on the owner of the ASC (AGSPlayerState for players).
 */
UCLASS()
class BANDITBALLER_API UGSAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGSAttributeSet();

	//~ UAttributeSet
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Attributes|Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Damage, Category = "Attributes|Combat")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, Damage)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FireRate, Category = "Attributes|Combat")
	FGameplayAttributeData FireRate;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, FireRate)

	/**
	 * Cash pieces and XP orbs inside this radius (cm) fly to the player. Starting value from GE_InitPlayerStats; raised
	 * by the Magnet upgrade.
	 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PickupRadius, Category = "Attributes|Economy")
	FGameplayAttributeData PickupRadius;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, PickupRadius)

	/** XP multiplier: XP granted = enemy XPValue x XPGain (items can raise it). Default 1. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_XPGain, Category = "Attributes|Progression")
	FGameplayAttributeData XPGain;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, XPGain)

	/**
	 * Reload speed multiplier: GA_Reload takes ReloadTime / ReloadSpeed (1.2 = 20% faster). Default 1, clamped to
	 * >= 0.1. Raised by the ReloadSpeed upgrade.
	 * TODO(co-op): not replicated (single-player first); replicate it in M6 so a client's predicted reload matches.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData ReloadSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, ReloadSpeed)

	/**
	 * Air jumps on top of the normal jump: AGSCharacter keeps JumpMaxCount = 1 + ExtraJumps. Default 0, clamped to
	 * >= 0. Raised by the Extra Jump upgrade.
	 * TODO(co-op): not replicated (single-player first); replicate it in M6 so a client's predicted jump count matches.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Movement")
	FGameplayAttributeData ExtraJumps;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, ExtraJumps)

	/**
	 * Dash charges on top of DA_DashSettings' MaxCharges: GA_Dash reads it live, so max charges = MaxCharges +
	 * ExtraDashCharges from the moment it changes. Default 0, clamped to >= 0. Raised by the Dash Charge upgrade.
	 * TODO(co-op): not replicated (single-player first); replicate it in M6 so a client's predicted dash charges match.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Movement")
	FGameplayAttributeData ExtraDashCharges;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, ExtraDashCharges)

	/**
	 * Magazine size multiplier for every player weapon: UGSWeaponComponent uses ceil(MagazineSize x it). Default 1,
	 * clamped to >= 0.1. Raised by the Bigger Mags upgrade.
	 * TODO(co-op): not replicated (single-player first); replicate it in M6 so a client's HUD and reload checks match.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData MagazineSizeMultiplier;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, MagazineSizeMultiplier)

	/**
	 * Health healed for each enemy kill credited to this player (AGSGameMode::HandleEnemyKilled), never above MaxHealth.
	 * Default 0, clamped to >= 0. Raised by the Leech upgrade.
	 * TODO(co-op): not replicated (single-player first).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health")
	FGameplayAttributeData HealthOnKill;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, HealthOnKill)

	/**
	 * Chance (0..1) that a hit from the player's weapons is critical (GSCombat::RollCriticalHit; damage x CritMultiplier
	 * from DA_CombatSettings). Default 0, clamped to 0..1. Raised by the Loaded Dice upgrade.
	 * TODO(co-op): not replicated (single-player first); the server rolls anyway.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, CritChance)

	/**
	 * Targets a shot from the player's weapons passes through (UGSGameplayAbility_Fire::TraceShot): a shot hits up to
	 * 1 + Pierce targets at full damage and still stops at walls. Default 0, clamped to >= 0, rounded when read.
	 * Raised by the Piercing Rounds upgrade.
	 * TODO(co-op): not replicated (single-player first).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData Pierce;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, Pierce)

	/**
	 * Powder Keg stacks: while above 0, every enemy this player kills explodes (AGSGameMode; damage and radius grow per
	 * stack, DA_CombatSettings). Default 0, clamped to >= 0, rounded when read. Raised by the Powder Keg upgrade.
	 * TODO(co-op): not replicated (single-player first).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData PowderKeg;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, PowderKeg)

	/** Meta attribute: damage to apply this execution. Server-only, never replicated; converted to Health loss. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UGSAttributeSet, IncomingDamage)

	/** Lowest ReloadSpeed allowed (keeps the reload time finite). */
	static constexpr float MinReloadSpeed = 0.1f;

	/** Lowest MagazineSizeMultiplier allowed (a magazine keeps at least one round). */
	static constexpr float MinMagazineSizeMultiplier = 0.1f;

	/** Server: who dealt the most recent damage (effect causer, else instigator). Used to report killers. */
	AActor* GetLastDamageInstigator() const { return LastDamageInstigator.Get(); }

protected:
	TWeakObjectPtr<AActor> LastDamageInstigator;

	/** Server: tell the attacking player's client about damage they dealt (damage numbers, hit marker). */
	void NotifyDamageDealt(const struct FGameplayEffectModCallbackData& Data, float DamageAmount, bool bCritical, bool bKilled) const;

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Damage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_FireRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PickupRadius(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_XPGain(const FGameplayAttributeData& OldValue);
};
