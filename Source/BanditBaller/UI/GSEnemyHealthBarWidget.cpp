// BanditBaller

#include "GSEnemyHealthBarWidget.h"
#include "Components/ProgressBar.h"

void UGSEnemyHealthBarWidget::SetHealthFraction(float Fraction)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(FMath::Clamp(Fraction, 0.f, 1.f));
	}
}
