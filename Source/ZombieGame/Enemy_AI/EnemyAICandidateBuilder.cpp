#include "EnemyAICandidateBuilder.h"

#include "EnemyUnitAI.h"

#include "EnemyAIQueryHelper.h"
#include "Systems/GridManager.h"
#include "Variant_Strategy/StrategyUnit.h"

void EnemyAICandidateBuilder::AddAttackCandidates(
	AStrategyUnit* Unit,
	AGridManager* GridManager,
	APlayerStrategySide* PlayerSide,
	TArray<FEnemyActionCandidate>& OutCandidates)
{
	ensureMsgf(false, TEXT("AddAttackCandidates - Not implemented"));
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
    APlayerStrategySide* PlayerSide,
    TArray<FEnemyActionCandidate>& OutCandidates)
{
    if (!Unit || !GridManager || !PlayerSide)
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

    const int32 MoveRange = Unit->GetMaxMovement();

    TArray<FIntPoint> CandidateCells;
    GridManager->GetCellsInRange(CurrentCell, MoveRange, CandidateCells);

    bool bFoundBestCell = false;
    FIntPoint BestCell = CurrentCell;
    int32 BestManhattanDistance = TNumericLimits<int32>::Max();

    for (const FIntPoint& Cell : CandidateCells)
    {
        if (Cell == CurrentCell)
        {
            continue;
        }

        // Don't move onto the player unit cell.
        if (Cell == TargetCell)
        {
            continue;
        }

        if (!GridManager->IsCellWithinMoveRange(Unit, Cell, MoveRange))
        {
            continue;
        }

        const int32 DistanceToTarget =
            FMath::Abs(Cell.X - TargetCell.X) +
            FMath::Abs(Cell.Y - TargetCell.Y);

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
    Candidate.ActionPointCost = 1;
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