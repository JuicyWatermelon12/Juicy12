// BanditBaller

#include "GSGameplayTags.h"

namespace GSGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Character is dead.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Sliding, "State.Sliding", "Character is sliding.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dashing, "State.Dashing", "Character is dashing.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Firing, "State.Firing", "Weapon is firing (between shots).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Reloading, "State.Reloading", "Weapon is reloading.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "Damage effects are blocked (e.g. dash i-frames).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking", "Enemy is performing an attack.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_SafeZone, "State.SafeZone", "In the safe hub (GE_SafeZone): can't fire, damage effects are blocked.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire, "Ability.Fire", "Fire the equipped weapon.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reload, "Ability.Reload", "Reload the equipped weapon.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Slide, "Ability.Slide", "Slide.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dash, "Ability.Dash", "Dash.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_SwapWeapon, "Ability.SwapWeapon", "Swap to the next weapon.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Melee, "Ability.Attack.Melee", "Enemy melee attack.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Ranged, "Ability.Attack.Ranged", "Enemy ranged attack (aimed burst).");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Slide, "Cooldown.Slide", "Slide is on cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dash, "Cooldown.Dash", "Dash is on cooldown.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller magnitude for damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Critical, "Data.Critical", "SetByCaller on a damage spec: 1 = a critical hit (damage numbers show it).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Heal, "Data.Heal", "SetByCaller magnitude for heals (health orbs).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cooldown,"Data.Cooldown", "SetByCaller duration for cooldown effects.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_HealthMultiplier, "Data.HealthMultiplier", "SetByCaller MaxHealth multiplier (enemy round scaling).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_DamageMultiplier, "Data.DamageMultiplier", "SetByCaller Damage multiplier (enemy round scaling).");
}
