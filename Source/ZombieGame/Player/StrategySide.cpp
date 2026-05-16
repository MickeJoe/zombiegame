#include "StrategySide.h"
#include "../Variant_Strategy/StrategyUnit.h"

AStrategySide::AStrategySide()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStrategySide::TakeTurn(AGridManager* GridManager, ASightManager* SightManager, APlayerStrategySide* PlayerSide)
{
	
}

void AStrategySide::AddUnit(AStrategyUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	if (!Units.Contains(Unit))
	{
		Units.Add(Unit);
		Unit->OwningSide = this;
	}
}

void AStrategySide::RemoveUnit(AStrategyUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	Units.Remove(Unit);
}

TArray<AStrategyUnit*> AStrategySide::GetAliveUnits() const
{
	TArray<AStrategyUnit*> Result;

	for (AStrategyUnit* Unit : Units)
	{
		if (IsValid(Unit) && Unit->GetCurrentHealth() > 0)
		{
			Result.Add(Unit);
		}
	}

	return Result;
}

bool AStrategySide::HasLivingUnits() const
{
	for (AStrategyUnit* Unit : Units)
	{
		if (IsValid(Unit) && Unit->GetCurrentHealth() > 0)
		{
			return true;
		}
	}

	return false;
}

bool AStrategySide::IsHuman() const
{
//	return ControllerType == EStrategySideControllerType::Human;
	return false;
}

bool AStrategySide::IsAI() const
{
//	return ControllerType == EStrategySideControllerType::AI;
	return false;
}
