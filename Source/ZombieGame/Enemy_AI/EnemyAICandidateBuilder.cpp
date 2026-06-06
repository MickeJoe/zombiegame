#include "EnemyAICandidateBuilder.h"

#include "EnemyUnitAI.h"

#include "EnemyAIQueryHelper.h"
#include "Player/AIStrategySide.h"
#include "Player/PlayerStrategySide.h"
#include "Systems/GridManager.h"
#include "Systems/SightManager.h"
#include "Variant_Strategy/StrategyUnit.h"

namespace
{
	void AddOccupiedCells(
		const TArray<TObjectPtr<AStrategyUnit>>& Units,
		const AStrategyUnit* MovingUnit,
		const AGridManager* GridManager,
		TSet<FIntPoint>& OccupiedCells)
	{
		if (!GridManager)
		{
			return;
		}

		for (const AStrategyUnit* OtherUnit : Units)
		{
			if (!OtherUnit || OtherUnit == MovingUnit || OtherUnit->GetCurrentHealth() <= 0)
			{
				continue;
			}

			OccupiedCells.Add(GridManager->WorldToGrid(OtherUnit->GetActorLocation()));
		}
	}

	int32 GetManhattanDistance(const FIntPoint& A, const FIntPoint& B)
	{
		return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
	}
}

void EnemyAICandidateBuilder::AddBiteAttackCandidate(
	AStrategyUnit* Unit,
	AGridManager* GridManager,
	APlayerStrategySide* PlayerSide,
	TArray<FEnemyActionCandidate>& OutCandidates)
{
	if (!Unit || !GridManager || !PlayerSide)
	{
		return;
	}

	const FIntPoint UnitCell =
		GridManager->WorldToGrid(Unit->GetActorLocation());
	const FAttackStats* BiteAttackStats = Unit->GetBiteAttackStatsPtr();
	const int32 BiteRange = Unit->GetBiteAttackRange();
	const int32 BiteTimeUnitCost = Unit->GetBiteAttackTimeUnitCost();
	if (!BiteAttackStats || Unit->GetRemainingTimeUnits() < BiteTimeUnitCost)
	{
		return;
	}

	AStrategyUnit* BestTarget = nullptr;
	FIntPoint BestTargetCell = FIntPoint::ZeroValue;
	int32 BestHealth = TNumericLimits<int32>::Max();

	for (AStrategyUnit* PlayerUnit : PlayerSide->Units)
	{
		if (!PlayerUnit)
		{
			continue;
		}
/*
		if (PlayerUnit->IsDead())
		{
			continue;
		}
*/
		const FIntPoint PlayerCell =
			GridManager->WorldToGrid(PlayerUnit->GetActorLocation());

		const int32 Distance =
			FMath::Abs(PlayerCell.X - UnitCell.X) +
			FMath::Abs(PlayerCell.Y - UnitCell.Y);

		if (Distance > BiteRange)
		{
			continue;
		}

		const int32 Health = PlayerUnit->GetCurrentHealth();

		if (Health < BestHealth)
		{
			BestHealth = Health;
			BestTarget = PlayerUnit;
			BestTargetCell = PlayerCell;
		}
	}

	if (!BestTarget)
	{
		return;
	}

	FEnemyActionCandidate Candidate;
	Candidate.ActionType = EEnemyAIActionType::BiteAttack;
	Candidate.TargetUnit = BestTarget;
	Candidate.TargetCell = BestTargetCell;
	Candidate.TimeUnitCost = BiteTimeUnitCost;

	OutCandidates.Add(Candidate);
}

void EnemyAICandidateBuilder::AddMoveToCoverCandidates(
	AStrategyUnit* Unit,
	AGridManager* GridManager,
	APlayerStrategySide* PlayerSide,
	TArray<FEnemyActionCandidate>& OutCandidates)
{
	ensureMsgf(false, TEXT("AddMoveToCoverCandidates - Not implemented"));
}

void EnemyAICandidateBuilder::AddMoveTowardNearestVisiblePlayerCandidate(
    AStrategyUnit* Unit,
    AGridManager* GridManager,
    ASightManager* SightManager,
    APlayerStrategySide* PlayerSide,
	AAIStrategySide* EnemySide,
    TArray<FEnemyActionCandidate>& OutCandidates)
{
    if (!Unit || !GridManager || !SightManager || !PlayerSide || !EnemySide)
    {
        ensureMsgf(false, TEXT("AddMoveTowardNearestVisiblePlayerCandidate - Invalid input"));
        return;
    }

    AStrategyUnit* TargetUnit =
        FEnemyAIQueryHelper::FindClosestVisiblePlayerUnit(Unit, GridManager, PlayerSide);

    if (!TargetUnit)
    {
        return;
    }

    const FIntPoint CurrentCell =
        GridManager->WorldToGrid(Unit->GetActorLocation());

    const FIntPoint TargetCell =
        GridManager->WorldToGrid(TargetUnit->GetActorLocation());

    const int32 MoveRange = Unit->GetRemainingTimeUnits();
	const TSet<FIntPoint>& CandidateCells = SightManager->GetEnemyVisibleCells();
	TSet<FIntPoint> OccupiedCells;
	AddOccupiedCells(PlayerSide->Units, Unit, GridManager, OccupiedCells);
	AddOccupiedCells(EnemySide->Units, Unit, GridManager, OccupiedCells);

    bool bFoundBestCell = false;
    FIntPoint BestCell = CurrentCell;
    int32 BestManhattanDistance = TNumericLimits<int32>::Max();

    for (const FIntPoint& Cell : CandidateCells)
    {
        if (Cell == CurrentCell)
        {
            continue;
        }

    	if (GetManhattanDistance(CurrentCell, Cell) > MoveRange)
    	{
    		continue;
    	}

        // Don't move onto the player unit cell.
        if (Cell == TargetCell)
        {
            continue;
        }

    	if (OccupiedCells.Contains(Cell))
    	{
    		continue;
    	}

        if (!GridManager->IsCellWithinMoveRange(Unit, Cell, MoveRange))
        {
            continue;
        }
    	
        const int32 DistanceToTarget = GetManhattanDistance(Cell, TargetCell);

        // Best possible melee position: adjacent to target.
        if (DistanceToTarget < BestManhattanDistance)
        {
            BestManhattanDistance = DistanceToTarget;
            BestCell = Cell;
            bFoundBestCell = true;
        }
    }

    if (!bFoundBestCell)
    {
        return;
    }

    FEnemyActionCandidate Candidate;
    Candidate.ActionType = EEnemyAIActionType::MoveTowardNearestVisiblePlayer;
    Candidate.TargetCell = BestCell;
    GridManager->TryGetMoveCostCells(Unit, BestCell, Candidate.TimeUnitCost);
    Candidate.DistanceToTargetAfterMove = BestManhattanDistance;

    OutCandidates.Add(Candidate);
}

void EnemyAICandidateBuilder::AddHoldHighGroundCandidates(
	AStrategyUnit* Unit,
	AGridManager* GridManager,
	TArray<FEnemyActionCandidate>& OutCandidates)
{
	ensureMsgf(false, TEXT("AddHoldHighGroundCandidates - Not implemented"));
}

void EnemyAICandidateBuilder::AddProtectSniperCandidates(
	AStrategyUnit* Unit,
	AGridManager* GridManager,
	AAIStrategySide* EnemySide,
	TArray<FEnemyActionCandidate>& OutCandidates)
{
	ensureMsgf(false, TEXT("AddProtectSniperCandidates - Not implemented"));
}
