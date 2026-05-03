#include "TargetingActionBarWidget.h"

#include "Components/Button.h"

void UTargetingActionBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Fire)
	{
		Button_Fire->OnClicked.AddDynamic(this, &UTargetingActionBarWidget::HandleFireClicked);
	}

	if (Button_Cancel)
	{
		Button_Cancel->OnClicked.AddDynamic(this, &UTargetingActionBarWidget::HandleCancelClicked);
	}

	if (Button_CycleTarget)
	{
		Button_CycleTarget->OnClicked.AddDynamic(this, &UTargetingActionBarWidget::HandleCycleTargetClicked);
	}
}

void UTargetingActionBarWidget::HandleFireClicked()
{
	OnFireClicked.Broadcast();
}

void UTargetingActionBarWidget::HandleCancelClicked()
{
	OnCancelClicked.Broadcast();
}

void UTargetingActionBarWidget::HandleCycleTargetClicked()
{
	OnCycleTargetClicked.Broadcast();
}
