// BanditBaller

#include "GSGameplayTags.h"

namespace GSGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Character is dead.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Sliding, "State.Sliding", "Character is sliding.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dashing, "State.Dashing", "Character is dashing.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire, "Ability.Fire", "Fire the equipped weapon.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reload, "Ability.Reload", "Reload the equipped weapon.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Slide, "Ability.Slide", "Slide.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dash, "Ability.Dash", "Dash.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_SwapWeapon, "Ability.SwapWeapon", "Swap to the next weapon.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Slide, "Cooldown.Slide", "Slide is on cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dash, "Cooldown.Dash", "Dash is on cooldown.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller magnitude for damage.");
}
