#include "EndTurnWidget.h"
#include "Components/Button.h"

void UEndTurnWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetPlayerEndTurnButtonEnabled(false);
	if (Button_EndTurn)
	{
		Button_EndTurn->OnClicked.AddDynamic(this, &UEndTurnWidget::HandleEndTurnClicked);
	}
}

void UEndTurnWidget::HandleEndTurnClicked()
{
	SetPlayerEndTurnButtonEnabled(false);
	OnEndTurnClicked.Broadcast();
}

void UEndTurnWidget::SetPlayerEndTurnButtonEnabled(bool bIsEnabledIn)
{
	Button_EndTurn->SetIsEnabled(bIsEnabledIn);
}
