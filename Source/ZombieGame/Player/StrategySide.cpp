#include "StrategySide.h"
#include "../Variant_Strategy/StrategyUnit.h"

AStrategySide::AStrategySide()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStrategySide::TakeTurn()
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
		if (IsValid(Unit))
		{
			// Byt mot din egen IsDead-logik senare
			Result.Add(Unit);
		}
	}

	return Result;
}

bool AStrategySide::HasLivingUnits() const
{
	for (AStrategyUnit* Unit : Units)
	{
		if (IsValid(Unit))
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