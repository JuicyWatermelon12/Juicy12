// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GSThrownSubsystem.generated.h"

class AGSThrownActor;

/**
 * The pool of thrown weapon actors (AGSThrownActor subclasses: lobbed throws, later fire patches and tomahawks), one per
 * game world. Acquire takes a pooled actor of the class (or spawns one); actors call Release when they are done;
 * ReleaseAll (a new run, AGSGameMode::StartRun) clears everything in flight or burning. Server only (single-player).
 */
UCLASS()
class BANDITBALLER_API UGSThrownSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Server: a pooled (or new) actor of Class, activated. nullptr on a client or if it can't be spawned. */
	AGSThrownActor* Acquire(TSubclassOf<AGSThrownActor> Class);

	template <class T>
	T* Acquire() { return Cast<T>(Acquire(T::StaticClass())); }

	/** Back to the pool (deactivated). */
	void Release(AGSThrownActor* Actor);

	/** Every active actor back to the pool (a new run). */
	void ReleaseAll();

	const TArray<TObjectPtr<AGSThrownActor>>& GetActive() const { return Active; }

	/** Active actors of a class (tests / debug). */
	int32 GetNumActive(TSubclassOf<AGSThrownActor> Class) const;

	int32 GetNumCreated() const { return NumCreated; }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGSThrownActor>> Pool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGSThrownActor>> Active;

	int32 NumCreated = 0;
};
