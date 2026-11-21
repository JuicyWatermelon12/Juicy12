// BanditBaller

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSEnemyHealthBarWidget.generated.h"

class UProgressBar;

/** Small bar above an enemy's head (WBP_EnemyHealthBar). AGSEnemyCharacter pushes updates. */
UCLASS(Abstract)
class BANDITBALLER_API UGSEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHealthFraction(float Fraction);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;
};
