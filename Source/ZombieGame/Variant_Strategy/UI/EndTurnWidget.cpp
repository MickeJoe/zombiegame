#include "EndTurnWidget.h"
#include "Components/Button.h"

void UEndTurnWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetPlayerEndTurnButtonEnabled(false);
	if (Button_EndTurn)
	{
		Button_EndTurn->SetClickMethod(EButtonClickMethod::MouseDown);
		Button_EndTurn->OnPressed.AddDynamic(this, &UEndTurnWidget::HandleEndTurnClicked);
		Button_EndTurn->OnClicked.AddDynamic(this, &UEndTurnWidget::HandleEndTurnClicked);
	}
}

void UEndTurnWidget::HandleEndTurnClicked()
{
	const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	if (LastBroadcastTime >= 0.0 && Now - LastBroadcastTime < 0.05)
	{
		return;
	}

	LastBroadcastTime = Now;
	SetPlayerEndTurnButtonEnabled(false);
	OnEndTurnClicked.Broadcast();
}

void UEndTurnWidget::SetPlayerEndTurnButtonEnabled(bool bIsEnabledIn)
{
	Button_EndTurn->SetIsEnabled(bIsEnabledIn);
}
