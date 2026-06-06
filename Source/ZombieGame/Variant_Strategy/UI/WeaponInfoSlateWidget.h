#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"

class AStrategyUnit;
class UEquippableItemData;
class SHorizontalBox;
class SVerticalBox;
struct FStrategyWeaponInstance;
enum class EEquippableItemSlot : uint8;

class SWeaponInfoSlateWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWeaponInfoSlateWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetUnit(AStrategyUnit* SelectedUnit);

private:
	void AddWeaponRow(AStrategyUnit* SelectedUnit, EEquippableItemSlot WeaponSlot, UEquippableItemData* ItemData, const FStrategyWeaponInstance& Weapon);
	FReply OnWeaponSlotClicked(AStrategyUnit* SelectedUnit, EEquippableItemSlot WeaponSlot);

	TSharedPtr<SVerticalBox> WeaponListBox;
	TArray<TSharedPtr<FSlateBrush>> WeaponIconBrushes;
};
