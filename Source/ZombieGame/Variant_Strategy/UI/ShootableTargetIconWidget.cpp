#include "ShootableTargetIconWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"

void UShootableTargetIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!TargetImage && WidgetTree)
	{
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_Root"));
		RootSizeBox->SetWidthOverride(IconSize.X);
		RootSizeBox->SetHeightOverride(IconSize.Y);

		TargetImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TargetImage"));
		RootSizeBox->AddChild(TargetImage);
		WidgetTree->RootWidget = RootSizeBox;
	}
}

void UShootableTargetIconWidget::SetTargetIcon(UTexture2D* IconTexture)
{
	if (!TargetImage)
	{
		if (WidgetTree)
		{
			USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_Root"));
			RootSizeBox->SetWidthOverride(IconSize.X);
			RootSizeBox->SetHeightOverride(IconSize.Y);

			TargetImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TargetImage"));
			RootSizeBox->AddChild(TargetImage);
			WidgetTree->RootWidget = RootSizeBox;
		}
	}

	if (!TargetImage)
	{
		return;
	}

	if (IconTexture)
	{
		TargetImage->SetBrushFromTexture(IconTexture, true);
		TargetImage->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		TargetImage->SetBrushFromTexture(nullptr);
		TargetImage->SetColorAndOpacity(FLinearColor::Transparent);
	}
}
