// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "BanditBallerPlayerController.h"
#include "Engine/NetSerialization.h"
#include "GSPlayerController.generated.h"

class UGameplayEffect;
class UGSHUDWidget;
class UUserWidget;

/**
 * Game player controller. Inherits input-mapping setup from the template controller,
 * owns the HUD and death screen for the local player, and relays hit feedback.
 */
UCLASS()
class BANDITBALLER_API AGSPlayerController : public ABanditBallerPlayerController
{
	GENERATED_BODY()

public:
	/** Server -> owning client: show the death screen and switch to UI input. */
	UFUNCTION(Client, Reliable)
	void ClientShowDeathScreen(TSubclassOf<UUserWidget> WidgetClass);

	/** Death screen -> server: restart the level (only honored while dead). */
	UFUNCTION(Server, Reliable)
	void ServerRequestRestart();

	/** Server -> attacker's client: damage number + hit marker for damage this player dealt. */
	UFUNCTION(Client, Unreliable)
	void ClientNotifyDamageDealt(FVector_NetQuantize WorldLocation, float Damage, bool bCritical, bool bKilled);

	/** Server -> collector's client: floating "+$N" where a cash pickup was collected. */
	UFUNCTION(Client, Unreliable)
	void ClientNotifyCashCollected(FVector_NetQuantize WorldLocation, int32 Amount);

	/** Client -> host: this machine's stable player ID, so the host binds our PlayerState to our cash entry. */
	UFUNCTION(Server, Reliable)
	void ServerSetProfileId(const FGuid& PlayerId);

	/** Debug (gs.Damage.Self): apply DebugDamageEffect to our own ASC on the server. */
	UFUNCTION(Server, Reliable)
	void ServerDebugDamageSelf(float Amount);

	UGSHUDWidget* GetGSHUD() const;

	/** Debug: press + release the Fire input once (goes through the ASC like the real button). */
	UFUNCTION(Exec)
	void GSFire();

	/** Debug: press + release any ability input tag once, e.g. "GSPress Ability.Dash". */
	UFUNCTION(Exec)
	void GSPress(const FString& InputTagName);

	/** Debug: point the camera at a world location. */
	UFUNCTION(Exec)
	void GSAimAt(float X, float Y, float Z);

	/** Debug console commands (gs.Spawn, gs.God, ...) from a client run on the server through this. */
	UFUNCTION(Server, Reliable)
	void ServerExecDebugCommand(const FString& Command, const TArray<FString>& Args);

	TSubclassOf<UGameplayEffect> GetDebugDamageEffect() const { return DebugDamageEffect; }

	/** Server: gs.God state for this player (adds/removes one State.Invulnerable count). */
	bool IsDebugGodMode() const { return bDebugGodMode; }
	void SetDebugGodMode(bool bEnabled) { bDebugGodMode = bEnabled; }

protected:
	virtual void BeginPlay() override;

	/** HUD widget added to the local player's screen (WBP_HUD). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> DeathScreenWidget;

	/** Damage GE used by gs.Damage.Self and gs.KillAll (GE_Damage, SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	TSubclassOf<UGameplayEffect> DebugDamageEffect;

	bool bDebugGodMode = false;
};
