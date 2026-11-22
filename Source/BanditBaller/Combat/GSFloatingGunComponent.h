// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSFloatingGunComponent.generated.h"

class AGSFloatingGun;
class UGSFloatingGunSettings;

/**
 * Keeps the player's Floating Guns in step with the FloatingGunLevel attribute (on AGSCharacter): level 1 or more = one
 * gun over the right shoulder (AGSFloatingGun). The second gun and the level 3+ bonuses are Prompt 39. Settings come
 * from AGSGameMode (DA_FloatingGunSettings), so there are no guns in the hub.
 * TODO(co-op): server-only actors for now (single-player first).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSFloatingGunComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSFloatingGunComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The rounded FloatingGunLevel attribute of the owner's player. */
	int32 GetLevel() const;

	const TArray<TObjectPtr<AGSFloatingGun>>& GetGuns() const { return Guns; }
	AGSFloatingGun* GetGun(int32 Index) const { return Guns.IsValidIndex(Index) ? Guns[Index].Get() : nullptr; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	const UGSFloatingGunSettings* GetSettings() const;
	void SyncGuns(int32 Wanted);

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGSFloatingGun>> Guns;
};
