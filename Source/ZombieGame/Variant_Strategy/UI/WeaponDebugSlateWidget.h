#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AStrategyUnit;
class UStrategyWeaponData;

DECLARE_DELEGATE_OneParam(FOnDebugWeaponPicked, UStrategyWeaponData*);
DECLARE_DELEGATE_OneParam(FOnDebugLevelPicked, FName);

struct FDebugLevelEntry
{
	FDebugLevelEntry() = default;
	FDebugLevelEntry(FName InPackageName, FText InDisplayName)
		: PackageName(InPackageName)
		, DisplayName(MoveTemp(InDisplayName))
	{
	}

	FName PackageName;
	FText DisplayName;
};

class SWeaponDebugSlateWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWeaponDebugSlateWidget) {}
		SLATE_EVENT(FOnDebugWeaponPicked, OnWeaponPicked)
		SLATE_EVENT(FOnDebugLevelPicked, OnLevelPicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetContext(
		const TArray<AStrategyUnit*>& InUnits,
		const TArray<UStrategyWeaponData*>& InWeapons,
		const TArray<FDebugLevelEntry>& InLevels);

private:
	FText GetTitleText() const;
	FReply HandleWeaponClicked(UStrategyWeaponData* Weapon);
	FReply HandleLevelClicked(FName LevelPackageName);
	void RebuildWeaponList();
	void RebuildLevelList();

	FOnDebugWeaponPicked OnWeaponPicked;
	FOnDebugLevelPicked OnLevelPicked;
	TArray<TWeakObjectPtr<AStrategyUnit>> Units;
	TArray<TWeakObjectPtr<UStrategyWeaponData>> Weapons;
	TArray<FDebugLevelEntry> Levels;
	TSharedPtr<class SVerticalBox> WeaponListBox;
	TSharedPtr<class SVerticalBox> LevelListBox;
};
