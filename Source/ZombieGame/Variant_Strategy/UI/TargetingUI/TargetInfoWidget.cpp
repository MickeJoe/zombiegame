#include "UI/TargetingUI/TargetInfoWidget.h"

#include "Components/TextBlock.h"
#include "Data/Unit/UnitData.h"
#include "UI/UnitStatusBarWidget.h"
#include "StrategyUnit.h"

void UTargetInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetHitChance(0);
	SetCritChance(0);
}

void UTargetInfoWidget::SetTarget(AStrategyUnit* InTarget)
{
	Target = InTarget;

	if (!Target)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (Text_TargetName)
	{
		const FText TargetName = Target->UnitData && !Target->UnitData->DisplayName.IsEmpty()
			? Target->UnitData->DisplayName
			: FText::FromString(Target->GetName());

		Text_TargetName->SetText(TargetName);
	}
/*
	if (UnitStatusBar)
	{
		// Anpassa namnet efter din befintliga funktion
		UnitStatusBar->SetTarget(Target);
	}
	*/
}

void UTargetInfoWidget::SetHitChance(int32 InHitChance)
{
	if (Text_HitChance)
	{
		Text_HitChance->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), InHitChance)));
	}
}

void UTargetInfoWidget::SetCritChance(int32 InCritChance)
{
	if (Text_CritChance)
	{
		Text_CritChance->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), InCritChance)));
	}
}

