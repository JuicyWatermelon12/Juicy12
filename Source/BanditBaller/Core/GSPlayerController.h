// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "BanditBallerPlayerController.h"
#include "Engine/NetSerialization.h"
#include "GSPlayerController.generated.h"

class UGameplayEffect;
class UGSHUDWidget;
class UGSUpgradeChoiceWidget;
class UGSRoundSummaryWidget;
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

	/** An XP orb was collected at WorldLocation: the next "+N XP" popup appears there (local HUD). */
	void NotifyXPCollected(const FVector& WorldLocation);

	/** Debug (gs.Damage.Self): apply DebugDamageEffect to our own ASC on the server. */
	UFUNCTION(Server, Reliable)
	void ServerDebugDamageSelf(float Amount);

	UGSHUDWidget* GetGSHUD() const;

	/** The level-up choice screen (local player; created at BeginPlay, shows itself when an offer opens). */
	UGSUpgradeChoiceWidget* GetUpgradeChoiceWidget() const { return UpgradeChoiceWidget; }

	/** The RoundClear summary (local player; created at BeginPlay, shows itself during RoundClear). */
	UGSRoundSummaryWidget* GetRoundSummaryWidget() const { return RoundSummaryWidget; }

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

	/** Level-up upgrade choice screen (WBP_UpgradeChoice), above the HUD. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSUpgradeChoiceWidget> UpgradeChoiceWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGSUpgradeChoiceWidget> UpgradeChoiceWidget;

	/** RoundClear summary: round stats, bonus lines and the bonus paid (WBP_RoundSummary), above the HUD. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSRoundSummaryWidget> RoundSummaryWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGSRoundSummaryWidget> RoundSummaryWidget;

	/** Damage GE used by gs.Damage.Self and gs.KillAll (GE_Damage, SetByCaller Data.Damage). */
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	TSubclassOf<UGameplayEffect> DebugDamageEffect;

	bool bDebugGodMode = false;
};
