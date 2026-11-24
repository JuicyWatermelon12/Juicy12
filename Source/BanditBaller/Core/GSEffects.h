// BanditBaller

#pragma once

#include "CoreMinimal.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UObject;

/**
 * One-shot effects that can come many at a time (hits on a crowd, impacts, blasts). Prompt 60: they come from Niagara's
 * component pool instead of a new component each time (the same effect, without creating and destroying components).
 * gs.Debug.NoFXPool 1 turns the pool off (for measuring).
 */
namespace GSEffects
{
	UNiagaraComponent* SpawnOneShot(const UObject* WorldContextObject, UNiagaraSystem* System, const FVector& Location,
		const FRotator& Rotation = FRotator::ZeroRotator, const FVector& Scale = FVector(1.f));
}
