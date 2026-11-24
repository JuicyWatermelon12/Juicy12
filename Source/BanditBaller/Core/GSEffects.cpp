// BanditBaller

#include "GSEffects.h"
#include "HAL/IConsoleManager.h"
#include "NiagaraComponent.h"
#include "NiagaraComponentPool.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
	TAutoConsoleVariable<bool> CVarGSEffectsNoPool(
		TEXT("gs.Debug.NoFXPool"), false,
		TEXT("1 = spawn one-shot hit / impact / blast effects without Niagara's component pool (for measuring the pool's gain)."));
}

UNiagaraComponent* GSEffects::SpawnOneShot(const UObject* WorldContextObject, UNiagaraSystem* System, const FVector& Location,
	const FRotator& Rotation, const FVector& Scale)
{
	if (!System || !WorldContextObject)
	{
		return nullptr;
	}
	const ENCPoolMethod Pool = CVarGSEffectsNoPool.GetValueOnGameThread() ? ENCPoolMethod::None : ENCPoolMethod::AutoRelease;
	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject, System, Location, Rotation, Scale,
		/*bAutoDestroy*/ true, /*bAutoActivate*/ true, Pool);
}
