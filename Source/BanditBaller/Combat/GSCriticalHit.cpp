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

	const float Chance = FMath::Clamp(Source->GetNumericAttribute(Attribute), 0.f, 1.f);
	if (Chance <= 0.f)
	{
		return Roll; // no dice: the run stream stays untouched
	}
	Roll.bCritical = Chance >= 1.f || UGSRunRandom::RollChance(Source, Chance);
	if (Roll.bCritical)
	{
		const UWorld* World = Source->GetWorld();
		const AGSGameMode* GameMode = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
		const UGSCombatSettings* Settings = GameMode ? GameMode->GetCombatSettings() : nullptr;
		Roll.DamageMultiplier = (Settings ? Settings : GetDefault<UGSCombatSettings>())->CritMultiplier;
	}
	return Roll;
}
