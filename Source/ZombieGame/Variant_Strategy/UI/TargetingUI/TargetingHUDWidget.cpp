#include "TargetingHUDWidget.h"

void UTargetingHUDWidget::Show()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UTargetingHUDWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTargetingHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (ensure(WBP_TargetingActionBar))
	{
		WBP_TargetingActionBar->OnCycleTargetClicked.AddDynamic(
			this,
			&UTargetingHUDWidget::HandleCycleTargetClicked
		);
		
		WBP_TargetingActionBar->OnCancelClicked.AddDynamic(
			this,
			&UTargetingHUDWidget::HandleCancelClicked
		);
		
		WBP_TargetingActionBar->OnFireClicked.AddDynamic(
			this,
			&UTargetingHUDWidget::HandleFireClicked
		);
	}	
}

void UTargetingHUDWidget::HandleCycleTargetClicked()
{
	OnCycleTargetClicked.Broadcast();
}

void UTargetingHUDWidget::HandleCancelClicked()
{
	OnCancelClicked.Broadcast();
}

void UTargetingHUDWidget::HandleFireClicked()
{
	OnFireClicked.Broadcast();
}