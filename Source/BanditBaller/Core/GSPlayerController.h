// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "BanditBallerPlayerController.h"
#include "Engine/NetSerialization.h"
#include "GSPlayerController.generated.h"

class UGameplayEffect;
class UGSHUDWidget;
class UGSUpgradeChoiceWidget;
class UGSRunSummaryWidget;
class UUserWidget;

/**
 * Game player controller. Inherits input-mapping setup from the template controller,
 * owns the HUD, the summaries and the choice screen for the local player, and relays hit feedback.
 */
UCLASS()
class BANDITBALLER_API AGSPlayerController : public ABanditBallerPlayerController
{
	GENERATED_BODY()

public:
	/** Server -> owning client: the player died; show the run summary (WBP_RunSummary) with UI input and the cursor. */
	UFUNCTION(Client, Reliable)
	void ClientShowRunSummary(TSubclassOf<UGSRunSummaryWidget> WidgetClass);

	/** Run summary -> server: leave the run for the hub (only honored while dead; AGSGameMode::RequestReturnToHub). */
	UFUNCTION(Server, Reliable)
	void ServerRequestReturnToHub();

	/** Server -> attacker's client: damage number + hit marker for damage this player dealt. */
	UFUNCTION(Client, Unreliable)
	void ClientNotifyDamageDealt(FVector_NetQuantize WorldLocation, float Damage, uint8 CritLevel, bool bKilled);

	/** Server -> collector's client: floating "+$N" where a cash pickup was collected. */
	UFUNCTION(Client, Unreliable)
	void ClientNotifyCashCollected(FVector_NetQuantize WorldLocation, int32 Amount);

	/** An XP orb was collected at WorldLocation: the next "+N XP" popup appears there (local HUD). */
	void NotifyXPCollected(const FVector& WorldLocation);

	/** A health orb healed Healed at WorldLocation: green "+N" popup there (local HUD, not an RPC; TODO(co-op)). */
	void NotifyHealthCollected(const FVector& WorldLocation, float Healed);

	/** Debug (gs.Damage.Self): apply DebugDamageEffect to our own ASC on the server. */
	UFUNCTION(Server, Reliable)
	void ServerDebugDamageSelf(float Amount);

	UGSHUDWidget* GetGSHUD() const;

	/** The level-up choice screen (local player; created at BeginPlay, shows itself when an offer opens). */
	UGSUpgradeChoiceWidget* GetUpgradeChoiceWidget() const { return UpgradeChoiceWidget; }

	/** The run summary after death (local player; null until ClientShowRunSummary). */
	UGSRunSummaryWidget* GetRunSummaryWidget() const { return RunSummaryWidget; }

	/** Debug: press + release the Fire input once (goes through the ASC like the real button). */
	UFUNCTION(Exec)
	void GSFire();

	/** Debug: press + release any ability input tag once, e.g. "GSPress Ability.Dash". */
	UFUNCTION(Exec)
	void GSPress(const FString& InputTagName);

	/** Debug: point the camera at a world location. */
	UFUNCTION(Exec)
	void GSAimAt(float X, float Y, float Z);

	/** Debug: tap the jump input once (Jump, then StopJumping on the next tick), e.g. to try air jumps from the console. */
	UFUNCTION(Exec)
	void GSJump();

	/** Debug: aim down sights on (1) or off (0), as holding / releasing the aim input does. */
	UFUNCTION(Exec)
	void GSAim(int32 bOn);

	/** Debug: swap the camera to the other shoulder, as the shoulder-swap input does. */
	UFUNCTION(Exec)
	void GSSwapShoulder();

	/** Debug: hold the move input toward where the camera faces for Seconds (game time), e.g. "GSRun 2" then
	 *  "GSPress Ability.Slide" to slide from a full run. MCP can't hold a key, so runs and slides come from here. */
	UFUNCTION(Exec)
	void GSRun(float Seconds);

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
	TObjectPtr<UGSRunSummaryWidget> RunSummaryWidget;

	/** Level-up upgrade choice screen (WBP_UpgradeChoice), above the HUD. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSUpgradeChoiceWidget> UpgradeChoiceWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGSUpgradeChoiceWidget> UpgradeChoiceWidget;

	/** Damage GE used by gs.Damage.Self and gs.KillAll (GE_Damage, SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	TSubclassOf<UGameplayEffect> DebugDamageEffect;

	bool bDebugGodMode = false;
};
