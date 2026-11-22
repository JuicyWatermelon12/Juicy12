// BanditBaller

#include "GSCriticalHit.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GSAttributeSet.h"
#include "GSCombatSettings.h"
#include "GSGameMode.h"
#include "GSRunRandom.h"

FGSCritRoll GSCombat::RollCriticalHit(const UAbilitySystemComponent* Source)
{
	FGSCritRoll Roll;
	const FGameplayAttribute Attribute = UGSAttributeSet::GetCritChanceAttribute();
	if (!Source || !Source->HasAttributeSetForAttribute(Attribute))
	{
		return Roll;
	}

	const float RawChance = FMath::Max(Source->GetNumericAttribute(Attribute), 0.f);
	const float Chance = SnapChance(RawChance);
	if (Chance <= 0.f)
	{
		return Roll; // no dice: the run stream stays untouched
	}
	Roll.bCritical = Chance >= 1.f || UGSRunRandom::RollChance(Source, Chance);
	if (!Roll.bCritical)
	{
		return Roll;
	}
	// Past 100%: a second roll for a triple hit (none below 100%, so runs there replay as before; no dice from 200%).
	const float TripleChance = GetTripleCritChance(RawChance);
	Roll.bTriple = TripleChance > 0.f && (TripleChance >= 1.f || UGSRunRandom::RollChance(Source, TripleChance));

	const UWorld* World = Source->GetWorld();
	const AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	const UGSCombatSettings* Settings = GameMode && GameMode->GetCombatSettings() ? GameMode->GetCombatSettings() : GetDefault<UGSCombatSettings>();
	Roll.DamageMultiplier = Roll.bTriple ? Settings->TripleCritMultiplier : Settings->CritMultiplier;
	return Roll;
}

float GSCombat::GetTripleCritChance(float CritChance)
{
	return SnapChance(CritChance - 1.f);
}

float GSCombat::SnapChance(float Chance)
{
	constexpr float Epsilon = 1e-4f;
	if (Chance <= Epsilon)
	{
		return 0.f;
	}
	return Chance >= 1.f - Epsilon ? 1.f : Chance;
}
