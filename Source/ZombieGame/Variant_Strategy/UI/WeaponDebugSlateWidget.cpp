#include "UI/WeaponDebugSlateWidget.h"

#include "Data/Weapon/StrategyWeaponData.h"
#include "StrategyUnit.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SWeaponDebugSlateWidget::Construct(const FArguments& InArgs)
{
	OnWeaponPicked = InArgs._OnWeaponPicked;
	OnLevelPicked = InArgs._OnLevelPicked;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(320.0f)
		[
			SNew(SBorder)
			.Padding(12.0f)
			.BorderBackgroundColor(FLinearColor(0.02f, 0.025f, 0.03f, 0.94f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
					.Text(this, &SWeaponDebugSlateWidget::GetTitleText)
					.ColorAndOpacity(FLinearColor(0.7f, 0.9f, 1.0f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.MaxHeight(420.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(WeaponListBox, SVerticalBox)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 10.0f, 0.0f, 8.0f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("WeaponDebug", "LevelsTitle", "Levels"))
					.ColorAndOpacity(FLinearColor(1.0f, 0.82f, 0.38f, 1.0f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(LevelListBox, SVerticalBox)
				]
			]
		]
	];

	RebuildWeaponList();
	RebuildLevelList();
}

void SWeaponDebugSlateWidget::SetContext(
	const TArray<AStrategyUnit*>& InUnits,
	const TArray<UStrategyWeaponData*>& InWeapons,
	const TArray<FDebugLevelEntry>& InLevels)
{
	Units.Reset();
	for (AStrategyUnit* Unit : InUnits)
	{
		if (IsValid(Unit))
		{
			Units.Add(Unit);
		}
	}

	Weapons.Reset();
	for (UStrategyWeaponData* Weapon : InWeapons)
	{
		if (Weapon)
		{
			Weapons.Add(Weapon);
		}
	}

	Levels = InLevels;

	RebuildWeaponList();
	RebuildLevelList();
}

FText SWeaponDebugSlateWidget::GetTitleText() const
{
	int32 ValidUnitCount = 0;
	for (const TWeakObjectPtr<AStrategyUnit>& Unit : Units)
	{
		if (Unit.IsValid())
		{
			++ValidUnitCount;
		}
	}

	return FText::Format(NSLOCTEXT("WeaponDebug", "Title", "Weapon Debug ({0} unit(s))"), ValidUnitCount);
}

FReply SWeaponDebugSlateWidget::HandleWeaponClicked(UStrategyWeaponData* Weapon)
{
	if (OnWeaponPicked.IsBound())
	{
		OnWeaponPicked.Execute(Weapon);
	}

	return FReply::Handled();
}

FReply SWeaponDebugSlateWidget::HandleLevelClicked(FName LevelPackageName)
{
	if (OnLevelPicked.IsBound())
	{
		OnLevelPicked.Execute(LevelPackageName);
	}

	return FReply::Handled();
}

void SWeaponDebugSlateWidget::RebuildWeaponList()
{
	if (!WeaponListBox.IsValid())
	{
		return;
	}

	WeaponListBox->ClearChildren();

	WeaponListBox->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 3.0f, 0.0f, 8.0f)
	[
		SNew(SButton)
		.OnClicked(this, &SWeaponDebugSlateWidget::HandleWeaponClicked, static_cast<UStrategyWeaponData*>(nullptr))
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("WeaponDebug", "ClearWeapon", "Clear Weapon"))
		]
	];

	if (Weapons.Num() == 0)
	{
		WeaponListBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("WeaponDebug", "NoWeapons", "No weapons found"))
		];
		return;
	}

	for (const TWeakObjectPtr<UStrategyWeaponData>& WeaponPtr : Weapons)
	{
		UStrategyWeaponData* Weapon = WeaponPtr.Get();
		if (!Weapon)
		{
			continue;
		}

		const FText WeaponName = Weapon->DisplayName.IsEmpty()
			? FText::FromName(Weapon->ItemId)
			: Weapon->DisplayName;

		const FText WeaponDetails = FText::Format(
			NSLOCTEXT("WeaponDebug", "WeaponDetails", "{0} | Damage {1} | Range {2}"),
			WeaponName,
			FText::AsNumber(Weapon->AttackStats.Damage),
			FText::AsNumber(Weapon->Range));

		WeaponListBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 3.0f)
		[
			SNew(SButton)
			.OnClicked(this, &SWeaponDebugSlateWidget::HandleWeaponClicked, Weapon)
			[
				SNew(STextBlock)
				.Text(WeaponDetails)
			]
		];
	}
}

void SWeaponDebugSlateWidget::RebuildLevelList()
{
	if (!LevelListBox.IsValid())
	{
		return;
	}

	LevelListBox->ClearChildren();

	if (Levels.Num() == 0)
	{
		LevelListBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("WeaponDebug", "NoLevels", "No debug levels configured"))
		];
		return;
	}

	for (const FDebugLevelEntry& Level : Levels)
	{
		if (Level.PackageName.IsNone())
		{
			continue;
		}

		LevelListBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 3.0f)
		[
			SNew(SButton)
			.OnClicked(this, &SWeaponDebugSlateWidget::HandleLevelClicked, Level.PackageName)
			[
				SNew(STextBlock)
				.Text(Level.DisplayName.IsEmpty() ? FText::FromName(Level.PackageName) : Level.DisplayName)
			]
		];
	}
}
