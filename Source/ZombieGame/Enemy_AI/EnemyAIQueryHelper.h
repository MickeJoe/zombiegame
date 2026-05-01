#pragma once

#include "CoreMinimal.h"

class AStrategyUnit;
class AGridManager;
class APlayerStrategySide;

class FEnemyAIQueryHelper
{
public:
	static AStrategyUnit* FindClosestVisiblePlayerUnit(
		const AStrategyUnit* EnemyUnit,
		const AGridManager* GridManager,
		const APlayerStrategySide* PlayerSide);
};