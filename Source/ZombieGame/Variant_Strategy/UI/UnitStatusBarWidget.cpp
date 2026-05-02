#include "UnitStatusBarWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"

void UUnitStatusBarWidget::SetHealthAndArmor(
	int32 CurrentHealth,
	int32 MaxHealth,
	int32 CurrentArmor,
	int32 MaxArmor)
{
	RebuildChunks(
		HorizontalBox_ArmorChunks,
		CurrentArmor,
		MaxArmor,
		FLinearColor(0.35f, 0.65f, 1.0f, 1.0f),
		FLinearColor(0.35f, 0.65f, 1.0f, 0.18f));

	RebuildChunks(
		HorizontalBox_HealthChunks,
		CurrentHealth,
		MaxHealth,
		FLinearColor(0.1f, 1.0f, 0.1f, 1.0f),
		FLinearColor(0.1f, 1.0f, 0.1f, 0.18f));
}

void UUnitStatusBarWidget::RebuildChunks(
	UHorizontalBox* Box,
	int32 CurrentValue,
	int32 MaxValue,
	const FLinearColor& FilledColor,
	const FLinearColor& EmptyColor)
{
	if (!Box)
	{
		return;
	}

	Box->ClearChildren();

	if (MaxValue <= 0)
	{
		Box->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Box->SetVisibility(ESlateVisibility::Visible);

	const int32 ClampedCurrent = FMath::Clamp(CurrentValue, 0, MaxValue);

	for (int32 Index = 0; Index < MaxValue; ++Index)
	{
		UImage* ChunkImage = NewObject<UImage>(this);
		if (!ChunkImage)
		{
			continue;
		}

		const bool bFilled = Index < ClampedCurrent;
		ChunkImage->SetColorAndOpacity(bFilled ? FilledColor : EmptyColor);

		UHorizontalBoxSlot* ChunkSlot = Box->AddChildToHorizontalBox(ChunkImage);
		if (ChunkSlot)
		{
			ChunkSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ChunkSlot->SetPadding(FMargin(1.0f, 0.0f));
		}
	}
}