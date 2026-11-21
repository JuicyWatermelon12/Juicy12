// BanditBaller

#include "GSHUDWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GSWeaponComponent.h"
#include "GSWeaponData.h"

UGSWeaponComponent* UGSHUDWidget::FindWeaponComponent() const
{
	const APawn* Pawn = GetOwningPlayerPawn();
	return Pawn ? Pawn->FindComponentByClass<UGSWeaponComponent>() : nullptr;
}

void UGSHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UGSWeaponComponent* Weapons = FindWeaponComponent();
	const UGSWeaponData* Data = Weapons ? Weapons->GetCurrentWeaponData() : nullptr;
	const int32 Ammo = Weapons ? Weapons->GetCurrentAmmo() : 0;
	const int32 Magazine = Data ? Data->MagazineSize : 0;

	if (Data == LastWeaponData.Get() && Ammo == LastAmmo && Magazine == LastMagazine)
	{
		return;
	}

	LastWeaponData = Data;
	LastAmmo = Ammo;
	LastMagazine = Magazine;

	if (WeaponNameText)
	{
		WeaponNameText->SetText(Data ? Data->DisplayName : FText::GetEmpty());
	}

	if (AmmoText)
	{
		AmmoText->SetText(Data
			? FText::Format(NSLOCTEXT("GSHUD", "AmmoFormat", "{0} / {1}"), FText::AsNumber(Ammo), FText::AsNumber(Magazine))
			: FText::GetEmpty());
	}
}
