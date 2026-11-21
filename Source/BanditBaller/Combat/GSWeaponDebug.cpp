// BanditBaller

#include "GSWeaponDebug.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<bool> CVarGSDebugWeapons(
	TEXT("gs.Debug.Weapons"),
	false,
	TEXT("Draw weapon traces, hits and damage numbers (0 = off, 1 = on)."),
	ECVF_Cheat);

bool GSWeaponDebug::IsEnabled()
{
	return CVarGSDebugWeapons.GetValueOnGameThread();
}
