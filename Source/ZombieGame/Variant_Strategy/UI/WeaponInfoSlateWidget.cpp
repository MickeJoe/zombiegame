#include "UI/WeaponInfoSlateWidget.h"

#include "Data/Weapon/StrategyWeaponData.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"
#include "Variant_Strategy/StrategyUnit.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FLinearColor PanelColor(0.025f, 0.09f, 0.12f, 0.92f);
	const FLinearColor WeaponInfoAccentBlue(0.0f, 0.62f, 1.0f, 1.0f);
	const FLinearColor WeaponInfoAccentYellow(1.0f, 0.72f, 0.02f, 1.0f);
	const FLinearColor PipFilled(0.0f, 0.72f, 1.0f, 1.0f);
	const FLinearColor PipEmpty(0.0f, 0.72f, 1.0f, 0.24f);
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
		.Offset(FMargin(28.0f, -36.0f, 268.0f, 228.0f))
		[
			SNew(SBox)
			.WidthOverride(268.0f)
			.HeightOverride(228.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(PanelColor)
				.Padding(FMargin(12.0f, 10.0f))
				[
					SAssignNew(WeaponListBox, SVerticalBox)
				]
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

	const FStrategyWeaponInstance& FireWeapon = SelectedUnit->GetEquippedFireWeapon();
	const FStrategyWeaponInstance& MeleeWeapon = SelectedUnit->GetEquippedMeleeWeapon();

	const bool bHasFireWeapon = FireWeapon.WeaponData != nullptr;
	const bool bHasMeleeWeapon = MeleeWeapon.WeaponData != nullptr;
	if (!bHasFireWeapon && !bHasMeleeWeapon)
	{
		SetVisibility(EVisibility::Collapsed);
		return;
	}

	SetVisibility(EVisibility::HitTestInvisible);

	if (bHasFireWeapon)
	{
		AddWeaponRow(FireWeapon);
	}

	if (bHasMeleeWeapon && MeleeWeapon.WeaponData != FireWeapon.WeaponData)
	{
		AddWeaponRow(MeleeWeapon);
	}
}

void SWeaponInfoSlateWidget::AddWeaponRow(const FStrategyWeaponInstance& Weapon)
{
	if (!WeaponListBox.IsValid() || !Weapon.WeaponData)
	{
		return;
	}

	UStrategyWeaponData* WeaponData = Weapon.WeaponData;

	TSharedPtr<FSlateBrush> IconBrush;
	if (WeaponData->Icon)
	{
		IconBrush = MakeShared<FSlateBrush>();
		IconBrush->SetResourceObject(WeaponData->Icon);
		IconBrush->ImageSize = FVector2D(112.0f, 28.0f);
		IconBrush->DrawAs = ESlateBrushDrawType::Image;
		WeaponIconBrushes.Add(IconBrush);
	}

	WeaponListBox->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 8.0f)
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(WeaponData->DisplayName.IsEmpty()
				? FText::FromName(WeaponData->WeaponId)
				: WeaponData->DisplayName)
			.ColorAndOpacity(WeaponInfoAccentBlue)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(SBox)
			.WidthOverride(240.0f)
			.HeightOverride(28.0f)
			[
				SNew(SImage)
				.Image(IconBrush.IsValid() ? IconBrush.Get() : FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.ColorAndOpacity(IconBrush.IsValid()
					? FLinearColor::White
					: FLinearColor(0.72f, 0.72f, 0.68f, 0.65f))
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("DAMAGE")))
				.ColorAndOpacity(FLinearColor::White)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 16.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::AsNumber(WeaponData->AttackStats.Damage))
				.ColorAndOpacity(WeaponInfoAccentYellow)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("RANGE")))
				.ColorAndOpacity(FLinearColor::White)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::AsNumber(WeaponData->AttackStats.Range))
				.ColorAndOpacity(WeaponInfoAccentYellow)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
			]
		]
	];

	if (Weapon.UsesAmmo() && Weapon.GetMaxAmmo() > 0)
	{
		TSharedRef<SHorizontalBox> AmmoPips = SNew(SHorizontalBox);

		const int32 PipCount = FMath::Clamp(Weapon.GetMaxAmmo(), 0, 12);
		for (int32 Index = 0; Index < PipCount; ++Index)
		{
			const FLinearColor PipColor = Index < Weapon.CurrentAmmo ? PipFilled : PipEmpty;
			AmmoPips->AddSlot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(26.0f)
				.HeightOverride(9.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
					.BorderBackgroundColor(PipColor)
				]
			];
		}

		WeaponListBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, -8.0f, 0.0f, 8.0f)
		[
			AmmoPips
		];
	}
}
