#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"

class AStrategyUnit;
class SHorizontalBox;
class SVerticalBox;
struct FStrategyWeaponInstance;

class SWeaponInfoSlateWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWeaponInfoSlateWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetUnit(AStrategyUnit* SelectedUnit);

private:
	void AddWeaponRow(const FStrategyWeaponInstance& Weapon);

	TSharedPtr<SVerticalBox> WeaponListBox;
	TArray<TSharedPtr<FSlateBrush>> WeaponIconBrushes;
};
