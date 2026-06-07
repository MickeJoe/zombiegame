#include "UI/WeaponInfoSlateWidget.h"

#include "Data/Item/EquippableItemData.h"
#include "Data/Weapon/StrategyWeaponData.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"
#include "Variant_Strategy/StrategyUnit.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor WeaponCardColor(0.0f, 0.0f, 0.0f, 0.94f);
	const FLinearColor WeaponCardSelectedColor(0.0f, 0.0f, 0.0f, 0.98f);
	const FLinearColor WeaponCardBorderColor(0.58f, 0.68f, 0.68f, 0.72f);
	const FLinearColor WeaponCardSelectedBorderColor(0.0f, 0.92f, 0.88f, 1.0f);
	const FLinearColor WeaponInfoAccentYellow(1.0f, 0.72f, 0.02f, 1.0f);

	FText GetItemDisplayName(UEquippableItemData* ItemData)
	{
		FString DisplayName = TEXT("Empty");
		if (ItemData)
		{
			DisplayName = ItemData->DisplayName.IsEmpty()
				? ItemData->ItemId.ToString()
				: ItemData->DisplayName.ToString();
		}
		DisplayName.ToUpperInline();
		return FText::FromString(DisplayName);
	}
}

void SWeaponInfoSlateWidget::Construct(const FArguments& InArgs)
{
	SetVisibility(EVisibility::Collapsed);

	ChildSlot
	[
		SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.0f, 1.0f))
		.Alignment(FVector2D(0.0f, 1.0f))
		.Offset(FMargin(28.0f, -36.0f, 286.0f, 166.0f))
		[
			SNew(SBox)
			.WidthOverride(286.0f)
			.HeightOverride(166.0f)
			[
				SAssignNew(WeaponListBox, SVerticalBox)
			]
		]
	];
}

void SWeaponInfoSlateWidget::SetUnit(AStrategyUnit* SelectedUnit)
{
	if (!WeaponListBox.IsValid())
	{
		return;
	}

	WeaponListBox->ClearChildren();
	WeaponIconBrushes.Reset();

	if (!SelectedUnit)
	{
		SetVisibility(EVisibility::Collapsed);
		return;
	}

	UEquippableItemData* PrimaryItem = SelectedUnit->GetItemInSlot(EEquippableItemSlot::Primary);
	UEquippableItemData* SecondaryItem = SelectedUnit->GetItemInSlot(EEquippableItemSlot::Secondary);

	if (!PrimaryItem && !SecondaryItem)
	{
		SetVisibility(EVisibility::Collapsed);
		return;
	}

	SetVisibility(EVisibility::SelfHitTestInvisible);

	AddWeaponRow(SelectedUnit, EEquippableItemSlot::Primary, PrimaryItem, SelectedUnit->GetWeaponInSlot(EEquippableItemSlot::Primary));
	AddWeaponRow(SelectedUnit, EEquippableItemSlot::Secondary, SecondaryItem, SelectedUnit->GetWeaponInSlot(EEquippableItemSlot::Secondary));
}

void SWeaponInfoSlateWidget::AddWeaponRow(
	AStrategyUnit* SelectedUnit,
	EEquippableItemSlot WeaponSlot,
	UEquippableItemData* ItemData,
	const FStrategyWeaponInstance& Weapon)
{
	if (!WeaponListBox.IsValid())
	{
		return;
	}

	UStrategyWeaponData* WeaponData = Weapon.WeaponData;
	const bool bSelected = SelectedUnit && SelectedUnit->GetActiveWeaponSlot() == WeaponSlot;

	TSharedPtr<FSlateBrush> IconBrush;
	if (ItemData && ItemData->Icon)
	{
		IconBrush = MakeShared<FSlateBrush>();
		IconBrush->SetResourceObject(ItemData->Icon);
		IconBrush->ImageSize = FVector2D(148.0f, 42.0f);
		IconBrush->DrawAs = ESlateBrushDrawType::Image;
		WeaponIconBrushes.Add(IconBrush);
	}

	WeaponListBox->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 6.0f)
	[
		SNew(SButton)
		.ButtonStyle(FCoreStyle::Get(), TEXT("NoBorder"))
		.ButtonColorAndOpacity(FLinearColor::Transparent)
		.ContentPadding(0.0f)
		.OnClicked(this, &SWeaponInfoSlateWidget::OnWeaponSlotClicked, SelectedUnit, WeaponSlot)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(bSelected ? WeaponCardSelectedBorderColor : WeaponCardBorderColor)
			.Padding(bSelected ? 2.0f : 1.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(bSelected ? WeaponCardSelectedColor : WeaponCardColor)
				.Padding(FMargin(10.0f, 5.0f, 10.0f, 6.0f))
				[
					SNew(SBox)
					.HeightOverride(72.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(GetItemDisplayName(ItemData))
							.ColorAndOpacity(ItemData ? FLinearColor::White : FLinearColor(0.82f, 0.86f, 0.88f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
						]

						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						.Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SNew(SBox)
								.WidthOverride(148.0f)
								.HeightOverride(42.0f)
								[
									SNew(SImage)
									.Image(IconBrush.IsValid() ? IconBrush.Get() : FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
									.ColorAndOpacity(IconBrush.IsValid()
										? FLinearColor::White
										: FLinearColor(0.65f, 0.68f, 0.68f, 0.24f))
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Top)
							.Padding(8.0f, -1.0f, 0.0f, 0.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::Format(
										FText::FromString(TEXT("DMG {0}")),
										FText::AsNumber(WeaponData ? WeaponData->AttackStats.Damage : 0)))
									.ColorAndOpacity(WeaponInfoAccentYellow)
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::Format(
										FText::FromString(TEXT("RNG {0}")),
										FText::AsNumber(ItemData ? ItemData->Range : 0)))
									.ColorAndOpacity(WeaponInfoAccentYellow)
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::Format(
										FText::FromString(TEXT("TU {0}")),
										FText::AsNumber(ItemData ? ItemData->TimeUnitCost : 0)))
									.ColorAndOpacity(WeaponInfoAccentYellow)
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								]
							]
						]
					]
				]
			]
		]
	];
}

FReply SWeaponInfoSlateWidget::OnWeaponSlotClicked(AStrategyUnit* SelectedUnit, EEquippableItemSlot WeaponSlot)
{
	if (SelectedUnit && SelectedUnit->GetItemInSlot(WeaponSlot))
	{
		SelectedUnit->SetActiveWeaponSlot(WeaponSlot);
		SetUnit(SelectedUnit);
	}

	return FReply::Handled();
}
