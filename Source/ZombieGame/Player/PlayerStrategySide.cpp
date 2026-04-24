#include "PlayerStrategySide.h"

#include "../Variant_Strategy/StrategyPlayerController.h"
#include "../Variant_Strategy/StrategyUnit.h"

class AStrategyPlayerController;

void APlayerStrategySide::TakeTurn()
{
	Super::TakeTurn();
	
	if (AStrategyPlayerController* PC = GetWorld()->GetFirstPlayerController<AStrategyPlayerController>())
	{
		PC->SetPlayerEndTurnButtonEnabled(true);
	}

	for (TObjectPtr<AStrategyUnit> UnitPtr : Units)
	{
		UnitPtr->ResetActionPoints();
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