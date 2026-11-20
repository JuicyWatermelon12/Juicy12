// BanditBaller

#pragma once

#include "NativeGameplayTags.h"

/** Native gameplay tags shared by C++ and data. Registered automatically at module load. */
namespace GSGameplayTags
{
	// Character states
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Sliding);
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dashing);

	// Abilities (also used as activation input tags)
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Fire);
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reload);
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slide);
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dash);
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_SwapWeapon);

	// Cooldowns
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Slide);
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dash);

	// SetByCaller data
	BANDITBALLER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
}
