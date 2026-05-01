#include "EnemyAIQueryHelper.h"


// EnemyAIQueryHelper.cpp

#include "EnemyAIQueryHelper.h"

#include "StrategyUnit.h"
#include "Systems/GridManager.h"
#include "Player/PlayerStrategySide.h"

AStrategyUnit* FEnemyAIQueryHelper::FindClosestVisiblePlayerUnit(
	const AStrategyUnit* EnemyUnit,
	const AGridManager* GridManager,
	const APlayerStrategySide* PlayerSide)
{
	if (!EnemyUnit || !GridManager || !PlayerSide)
	{
		return nullptr;
	}

	const TArray<TObjectPtr<AStrategyUnit>>& PlayerUnits = PlayerSide->Units;

	if (PlayerUnits.Num() == 0)
	{
		return nullptr;
	}

	const FIntPoint EnemyCell =
		GridManager->WorldToGrid(EnemyUnit->GetActorLocation());

	const int32 VisionRange = EnemyUnit->GetSightRange();

	TArray<FIntPoint> VisibleCells;
	GridManager->GetCellsInRange(EnemyCell, VisionRange, VisibleCells);

	const TSet<FIntPoint> VisibleCellSet(VisibleCells);

	AStrategyUnit* BestTarget = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AStrategyUnit* PlayerUnit : PlayerUnits)
	{
		if (!PlayerUnit)
		{
			continue;
		}

		const FIntPoint PlayerCell =
			GridManager->WorldToGrid(PlayerUnit->GetActorLocation());

		if (!VisibleCellSet.Contains(PlayerCell))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(
			EnemyUnit->GetActorLocation(),
			PlayerUnit->GetActorLocation());

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = PlayerUnit;
		}
	}

	return BestTarget;
}