#include "EnemyUnitAI.h"
#include "StrategyUnit.h"
#include "Systems/GridManager.h"
#include "Player/PlayerStrategySide.h"

#include "WalkerEnemyAI.h"
#include "StrategyUnit.h"
#include "Player/AIStrategySide.h"
#include "Systems/GridManager.h"
#include "Player/PlayerStrategySide.h"

#include "EnemyUnitAI.h"

#include "EnemyAIActionExecutor.h"
#include "Kismet/GameplayStatics.h"
#include "Systems/GridManager.h"
#include "StrategyGameMode.h"
#include "Player/PlayerStrategySide.h"
#include "StrategyUnit.h"

void UEnemyUnitAI::TakeTurn(
	AStrategyUnit* Unit,
	AGridManager* GridManager,
	ASightManager* SightManager,
	APlayerStrategySide* PlayerSide,
	AAIStrategySide* EnemySide)
{
	OwnerUnit = Unit;
	CachedGridManager = GridManager;
	CachedSightManager = SightManager;
	CachedPlayerSide = PlayerSide;
	CachedEnemySide = EnemySide;

	ExecuteNextAction();
}

void UEnemyUnitAI::ExecuteNextAction()
{
	if (!OwnerUnit || OwnerUnit->GetRemainingActionPoints() <= 0)
	{
		FinishUnitTurn();
		return;
	}

	TArray<FEnemyActionCandidate> Candidates;

	GenerateCandidates(
		OwnerUnit,
		CachedGridManager,
		CachedSightManager,
		CachedPlayerSide,
		CachedEnemySide,
		Candidates);

	for (FEnemyActionCandidate& Candidate : Candidates)
	{
		Candidate.Score = ScoreCandidate(
			OwnerUnit,
			Candidate,
			CachedGridManager,
			CachedPlayerSide,
			CachedEnemySide);
	}

	const FEnemyActionCandidate* BestAction = FindBestCandidate(Candidates);

	if (!BestAction)
	{
		FinishUnitTurn();
		return;
	}

	CurrentAction = *BestAction;

	FEnemyAIActionExecutor::Execute(
		CachedGridManager,
		OwnerUnit,
		CurrentAction,
		this);
}

void UEnemyUnitAI::GenerateCandidates(
	AStrategyUnit* Unit,
	AGridManager* GridManager,
	ASightManager* SightManager,
	APlayerStrategySide* PlayerSide,
	AAIStrategySide* EnemySide,
	TArray<FEnemyActionCandidate>& OutCandidates)
{
	
}

const FEnemyActionCandidate* UEnemyUnitAI::FindBestCandidate(
	const TArray<FEnemyActionCandidate>& Candidates)
{
	const FEnemyActionCandidate* Best = nullptr;
	float BestScore = -FLT_MAX;

	for (const FEnemyActionCandidate& Candidate : Candidates)
	{
		if (!Best ||
			Candidate.Score > BestScore ||
			(FMath::IsNearlyEqual(Candidate.Score, BestScore) && FMath::RandBool()))
		{
			BestScore = Candidate.Score;
			Best = &Candidate;
		}
	}

	return Best;
}

float UEnemyUnitAI::ScoreCandidate(
	AStrategyUnit* Unit,
	const FEnemyActionCandidate& Candidate,
	AGridManager* GridManager,
	APlayerStrategySide* PlayerSide,
	AAIStrategySide* EnemySide) const
{
	const FEnemyAIWeights Weights = GetAIWeights();

	float Score = 0.f;

	switch (Candidate.ActionType)
	{
	case EEnemyAIActionType::BiteAttack:
		Score += Weights.BiteAttack;
/*
		if (Candidate.bCanInfect)
		{
			Score += Weights.CanInfect;
		}
		*/
		break;

	case EEnemyAIActionType::MoveTowardNearestVisiblePlayer:
		Score += Weights.MoveTowardTarget;
		Score += Candidate.DistanceToTargetAfterMove * Weights.DistanceToTarget;
		break;

	default:
		ensureMsgf(false, TEXT("ScoreCandidate - Unsupported ActionType"));
		break;
	}

	return Score;
}

FEnemyAIWeights UEnemyUnitAI::GetAIWeights() const
{
	FEnemyAIWeights Weights;
	
	return Weights;
}

void UEnemyUnitAI::OnMoveCompleted(AStrategyUnit* MovedUnit)
{
	if (MovedUnit != OwnerUnit)
	{
		return;
	}

	if (!OwnerUnit)
	{
		return;
	}

	OwnerUnit->UseAtionPoints(CurrentAction.ActionPointCost);

	ExecuteNextAction();
}

void UEnemyUnitAI::FinishUnitTurn()
{
	if (CachedEnemySide)
	{
		CachedEnemySide->OnEnemyUnitTurnDone(OwnerUnit);
	}

	OwnerUnit = nullptr;
	CachedGridManager = nullptr;
	CachedPlayerSide = nullptr;
	CachedEnemySide = nullptr;
}


/*

AStrategyUnit* UEnemyUnitAI::FindClosestVisiblePlayerUnit() const
{
	if (!OwnerUnit || !GridManager || PlayerUnitArray.Num() == 0)
	{
		return nullptr;
	}

	const FIntPoint EnemyCell =
		GridManager->WorldToGrid(OwnerUnit->GetActorLocation());

	const int32 VisionRange = OwnerUnit->GetSightRange();

	TArray<FIntPoint> VisibleCells;
	GridManager->GetCellsInRange(EnemyCell, VisionRange, VisibleCells);

	const TSet<FIntPoint> VisibleCellSet(VisibleCells);

	AStrategyUnit* BestTarget = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AStrategyUnit* PlayerUnit : PlayerUnitArray)
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

		const float DistSq =
			FVector::DistSquared(
				OwnerUnit->GetActorLocation(),
				PlayerUnit->GetActorLocation()
			);

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = PlayerUnit;
		}
	}

	return BestTarget;
}

FIntPoint UEnemyUnitAI::FindBestMoveCellTowardsTarget(
	const AStrategyUnit* TargetUnit,
	int32 MaxMoveCells
) const
{
	const FIntPoint EnemyCell =
		GridManager->WorldToGrid(OwnerUnit->GetActorLocation());

	const FIntPoint TargetCell =
		GridManager->WorldToGrid(TargetUnit->GetActorLocation());

	TArray<FIntPoint> CandidateCells;
	GridManager->GetCellsInRange(EnemyCell, MaxMoveCells, CandidateCells);

	FIntPoint BestCell = EnemyCell;
	int32 BestDistance = TNumericLimits<int32>::Max();

	for (const FIntPoint& Cell : CandidateCells)
	{
		if (!GridManager->IsCellWithinMoveRange(OwnerUnit, Cell, MaxMoveCells))
		{
			continue;
		}

		// Undvik att försöka flytta in på spelarens cell.
		if (Cell == TargetCell)
		{
			continue;
		}

		const int32 DistanceToTarget =
			FMath::Abs(Cell.X - TargetCell.X) +
			FMath::Abs(Cell.Y - TargetCell.Y);

		if (DistanceToTarget < BestDistance)
		{
			BestDistance = DistanceToTarget;
			BestCell = Cell;
		}
	}

	return BestCell;
}
*/