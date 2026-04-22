#include "PlayerStrategySide.h"

#include "../Variant_Strategy/StrategyPlayerController.h"

class AStrategyPlayerController;

void APlayerStrategySide::TakeTurn()
{
	Super::TakeTurn();
	
	if (AStrategyPlayerController* PC = GetWorld()->GetFirstPlayerController<AStrategyPlayerController>())
	{
		PC->SetPlayerEndTurnButtonEnabled(true);
	}
}

bool APlayerStrategySide::IsHuman() const
{
	return true;	
}
bool APlayerStrategySide::IsAI() const
{
	return false;
}