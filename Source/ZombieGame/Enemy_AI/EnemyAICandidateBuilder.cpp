#include "EnemyAICandidateBuilder.h"

#include "EnemyUnitAI.h"

#include "EnemyAIQueryHelper.h"
#include "Player/AIStrategySide.h"
#include "Player/PlayerStrategySide.h"
#include "Systems/GridManager.h"
#include "Systems/SightManager.h"
#include "Variant_Strategy/StrategyUnit.h"
#include "Engine/World.h"

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

	FIntPoint GetDominantGridDirection(const FIntPoint& FromCell, const FIntPoint& ToCell)
	{
		const int32 DeltaX = ToCell.X - FromCell.X;
		const int32 DeltaY = ToCell.Y - FromCell.Y;

		if (DeltaX == 0 && DeltaY == 0)
		{
			return FIntPoint::ZeroValue;
		}

		if (FMath::Abs(DeltaX) >= FMath::Abs(DeltaY))
		{
			return FIntPoint(DeltaX > 0 ? 1 : -1, 0);
		}

		return FIntPoint(0, DeltaY > 0 ? 1 : -1);
	}

	int32 GetCoverScoreAgainstPlayer(
		const AStrategyUnit* Unit,
		const AGridManager* GridManager,
		const FIntPoint& CoverCell,
		const AStrategyUnit* PlayerUnit)
	{
		if (!Unit || !GridManager || !PlayerUnit || !Unit->GetWorld())
		{
			return 0;
		}

		const FIntPoint PlayerCell = GridManager->WorldToGrid(PlayerUnit->GetActorLocation());
		const FIntPoint Direction = GetDominantGridDirection(CoverCell, PlayerCell);
		if (Direction == FIntPoint::ZeroValue)
		{
			return 0;
		}

		const FVector CellCenter = GridManager->GridToWorld(CoverCell);
		const FVector DirectionVector(
			static_cast<float>(Direction.X),
			static_cast<float>(Direction.Y),
			0.0f);
		const FVector TraceEndBase = CellCenter + DirectionVector * GridManager->CellSize;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyAICoverTrace), false);
		QueryParams.AddIgnoredActor(Unit);
		QueryParams.AddIgnoredActor(PlayerUnit);

		auto IsBlockedAtHeight = [Unit, &QueryParams, CellCenter, TraceEndBase](float Height)
		{
			FHitResult Hit;
			return Unit->GetWorld()->LineTraceSingleByChannel(
				Hit,
				CellCenter + FVector(0.0f, 0.0f, Height),
				TraceEndBase + FVector(0.0f, 0.0f, Height),
				ECC_Visibility,
				QueryParams);
		};

		constexpr float HalfCoverTraceHeight = 80.0f;
		constexpr float FullCoverTraceHeight = 165.0f;
		if (!IsBlockedAtHeight(HalfCoverTraceHeight))
		{
			return 0;
		}

		return IsBlockedAtHeight(FullCoverTraceHeight) ? 2 : 1;
	}

	int32 GetBestCoverScoreAgainstPlayers(
		const AStrategyUnit* Unit,
		const AGridManager* GridManager,
		const APlayerStrategySide* PlayerSide,
		const FIntPoint& CoverCell,
		AStrategyUnit*& OutBestTarget)
	{
		int32 BestCoverScore = 0;
		int32 BestDistance = TNumericLimits<int32>::Max();
		OutBestTarget = nullptr;

		if (!GridManager || !PlayerSide)
		{
			return 0;
		}

		for (AStrategyUnit* PlayerUnit : PlayerSide->Units)
		{
			if (!PlayerUnit || PlayerUnit->GetCurrentHealth() <= 0)
			{
				continue;
			}

			const int32 CoverScore = GetCoverScoreAgainstPlayer(Unit, GridManager, CoverCell, PlayerUnit);
			if (CoverScore <= 0)
			{
				continue;
			}

			const FIntPoint PlayerCell = GridManager->WorldToGrid(PlayerUnit->GetActorLocation());
			const int32 Distance = GetManhattanDistance(CoverCell, PlayerCell);
			if (CoverScore > BestCoverScore || (CoverScore == BestCoverScore && Distance < BestDistance))
			{
				BestCoverScore = CoverScore;
				BestDistance = Distance;
				OutBestTarget = PlayerUnit;
			}
		}

		return BestCoverScore;
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

void EnemyAICandidateBuilder::AddCrouchCandidate(
	AStrategyUnit* Unit,
	AGridManager* GridManager,
	APlayerStrategySide* PlayerSide,
	TArray<FEnemyActionCandidate>& OutCandidates)
{
	if (!Unit || !Unit->CanCrouch())
	{
		return;
	}

	FEnemyActionCandidate Candidate;
	Candidate.ActionType = EEnemyAIActionType::Crouch;
	Candidate.TimeUnitCost = Unit->GetCrouchTimeUnitCost();
	if (GridManager && PlayerSide)
	{
		Candidate.TargetCell = GridManager->WorldToGrid(Unit->GetActorLocation());
		AStrategyUnit* CoverTarget = nullptr;
		Candidate.CoverScore = GetBestCoverScoreAgainstPlayers(Unit, GridManager, PlayerSide, Candidate.TargetCell, CoverTarget);
		Candidate.TargetUnit = CoverTarget;
	}

	OutCandidates.Add(Candidate);
}

void EnemyAICandidateBuilder::AddMoveToCoverCandidates(
	AStrategyUnit* Unit,
	AGridManager* GridManager,
	APlayerStrategySide* PlayerSide,
	AAIStrategySide* EnemySide,
	TArray<FEnemyActionCandidate>& OutCandidates)
{
	if (!Unit || !GridManager || !PlayerSide || !EnemySide)
	{
		return;
	}

	const FIntPoint CurrentCell = GridManager->WorldToGrid(Unit->GetActorLocation());
	const int32 AvailableMoveRange = FMath::Max(0, Unit->GetRemainingTimeUnits() - Unit->GetCrouchTimeUnitCost());
	if (AvailableMoveRange <= 0)
	{
		return;
	}

	TArray<FIntPoint> CandidateCells;
	GridManager->GetCellsInRange(CurrentCell, AvailableMoveRange, CandidateCells);

	TSet<FIntPoint> OccupiedCells;
	AddOccupiedCells(PlayerSide->Units, Unit, GridManager, OccupiedCells);
	AddOccupiedCells(EnemySide->Units, Unit, GridManager, OccupiedCells);

	for (const FIntPoint& Cell : CandidateCells)
	{
		if (Cell == CurrentCell || OccupiedCells.Contains(Cell))
		{
			continue;
		}

		int32 MoveCost = 0;
		if (!GridManager->TryGetMoveCostCells(Unit, Cell, MoveCost) || MoveCost > AvailableMoveRange)
		{
			continue;
		}

		AStrategyUnit* CoverTarget = nullptr;
		const int32 CoverScore = GetBestCoverScoreAgainstPlayers(Unit, GridManager, PlayerSide, Cell, CoverTarget);
		if (CoverScore <= 0)
		{
			continue;
		}

		FEnemyActionCandidate Candidate;
		Candidate.ActionType = EEnemyAIActionType::MoveToCover;
		Candidate.TargetCell = Cell;
		Candidate.TargetUnit = CoverTarget;
		Candidate.CoverScore = CoverScore;
		Candidate.TimeUnitCost = MoveCost;

		if (CoverTarget)
		{
			const FIntPoint TargetCell = GridManager->WorldToGrid(CoverTarget->GetActorLocation());
			Candidate.DistanceToTargetAfterMove = GetManhattanDistance(Cell, TargetCell);
		}

		OutCandidates.Add(Candidate);
	}
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
