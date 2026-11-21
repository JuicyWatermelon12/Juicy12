// BanditBaller

#pragma once

#include "CoreMinimal.h"

/** Weapon debug drawing, toggled with the console variable gs.Debug.Weapons (0/1). */
namespace GSWeaponDebug
{
	BANDITBALLER_API bool IsEnabled();

	/** How long debug shapes stay on screen (seconds). */
	constexpr float DrawDuration = 1.5f;
}
