#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AStrategyUnit;
class UStrategyWeaponData;

DECLARE_DELEGATE_OneParam(FOnDebugWeaponPicked, UStrategyWeaponData*);

class SWeaponDebugSlateWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWeaponDebugSlateWidget) {}
		SLATE_EVENT(FOnDebugWeaponPicked, OnWeaponPicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetContext(const TArray<AStrategyUnit*>& InUnits, const TArray<UStrategyWeaponData*>& InWeapons);

private:
	FText GetTitleText() const;
	FReply HandleWeaponClicked(UStrategyWeaponData* Weapon);
	void RebuildWeaponList();

	FOnDebugWeaponPicked OnWeaponPicked;
	TArray<TWeakObjectPtr<AStrategyUnit>> Units;
	TArray<TWeakObjectPtr<UStrategyWeaponData>> Weapons;
	TSharedPtr<class SVerticalBox> WeaponListBox;
};
