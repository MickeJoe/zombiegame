#include "ShootableTargetIconBarWidget.h"

#include "ShootableTargetIconWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Variant_Strategy/StrategyUnit.h"

void UShootableTargetIconBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!IconBox && WidgetTree)
	{
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_Root"));
		RootSizeBox->SetWidthOverride(420.0f);
		RootSizeBox->SetHeightOverride(48.0f);

		IconBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("IconBox"));
		RootSizeBox->AddChild(IconBox);
		WidgetTree->RootWidget = RootSizeBox;
	}
}

void UShootableTargetIconBarWidget::SetTargets(
	const TArray<AStrategyUnit*>& Targets,
	TSubclassOf<UShootableTargetIconWidget> IconWidgetClass)
{
	if (!IconBox)
	{
		if (WidgetTree)
		{
			USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_Root"));
			RootSizeBox->SetWidthOverride(420.0f);
			RootSizeBox->SetHeightOverride(48.0f);

			IconBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("IconBox"));
			RootSizeBox->AddChild(IconBox);
			WidgetTree->RootWidget = RootSizeBox;
		}
	}

	if (!IconBox)
	{
		return;
	}

	IconBox->ClearChildren();

	TSubclassOf<UShootableTargetIconWidget> WidgetClass = IconWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UShootableTargetIconWidget::StaticClass();
	}

	for (AStrategyUnit* Target : Targets)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		UShootableTargetIconWidget* IconWidget = CreateWidget<UShootableTargetIconWidget>(GetOwningPlayer(), WidgetClass);
		if (!IconWidget)
		{
			continue;
		}

		IconWidget->SetTargetIcon(Target->GetShootableTargetIconTexture());

		UHorizontalBoxSlot* IconSlot = IconBox->AddChildToHorizontalBox(IconWidget);
		if (IconSlot)
		{
			IconSlot->SetPadding(IconPadding);
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	SetVisibility(IconBox->GetChildrenCount() > 0
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
}
