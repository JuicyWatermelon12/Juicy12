// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GSFloatingGunComponent.generated.h"

class AGSFloatingGun;
class UGSFloatingGunSettings;

/**
 * Keeps the player's Floating Guns in step with the FloatingGunLevel attribute (the number of Floating Gun picks; on
 * AGSCharacter): 1 = one gun over the right shoulder (AGSFloatingGun), 2 and up = a second one over the left, never
 * more than two. The FloatingGunPower attribute (weapon power) adds DA_FloatingGunSettings' damage and fire rate bonus
 * per point (the guns read it). Level 0 (a new run) removes them. Settings come from AGSGameMode
 * (DA_FloatingGunSettings), so there are no guns in the hub.
 * TODO(co-op): server-only actors for now (single-player first).
 */
UCLASS(ClassGroup = (BanditBaller), meta = (BlueprintSpawnableComponent))
class BANDITBALLER_API UGSFloatingGunComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGSFloatingGunComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The rounded FloatingGunLevel attribute of the owner's player (Floating Gun picks). */
	int32 GetLevel() const;

	/** The FloatingGunPower attribute of the owner's player (weapon power; fractions count). */
	float GetPower() const;

	const TArray<TObjectPtr<AGSFloatingGun>>& GetGuns() const { return Guns; }
	AGSFloatingGun* GetGun(int32 Index) const { return Guns.IsValidIndex(Index) ? Guns[Index].Get() : nullptr; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	const UGSFloatingGunSettings* GetSettings() const;
	void SyncGuns(int32 Wanted);

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGSFloatingGun>> Guns;

	/** The power last logged (the bonus line). */
	float LoggedPower = 0.f;
};
