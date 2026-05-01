#include "EnemyAIActionExecutor.h"

#include "StrategyUnit.h"
#include "EnemyUnitAI.h"
#include "Systems/GridManager.h"

void FEnemyAIActionExecutor::Execute(
	AGridManager* GridManager,
	AStrategyUnit* Unit,
	const FEnemyActionCandidate& Candidate,
	UEnemyUnitAI* OwnerAI)
{
	switch (Candidate.ActionType)
	{
	case EEnemyAIActionType::Attack:
		ExecuteAttack(Unit, Candidate);
		break;

	case EEnemyAIActionType::MoveToCover:
		ExecuteMove(GridManager, Unit, Candidate.TargetCell, OwnerAI);
		break;

	case EEnemyAIActionType::MoveTowardNearestVisiblePlayer:
		ExecuteMove(GridManager, Unit, Candidate.TargetCell, OwnerAI);
		break;

	case EEnemyAIActionType::ProtectSniper:
		ExecuteMove(GridManager, Unit, Candidate.TargetCell, OwnerAI);
		break;

	case EEnemyAIActionType::Wait:
		break;

	default:
		ensureMsgf(false, TEXT("Execute - Unknown ActionType"));
		break;
	}
}

void FEnemyAIActionExecutor::ExecuteAttack(
	AStrategyUnit* Unit,
	const FEnemyActionCandidate& Candidate)
{
	ensureMsgf(false, TEXT("ExecuteAttack - Not Implemented"));
/*	
	if (Candidate.AttackTarget)
	{

	}
	*/
}

void FEnemyAIActionExecutor::ExecuteMove(
	AGridManager* GridManager,
	AStrategyUnit* Unit,
	const FIntPoint& Cell,
	UEnemyUnitAI* OwnerAI)
{
	if (!Unit)
	{
		ensureMsgf(false, TEXT("ExecuteMove - Unit is null"));
		return;
	}
	
	FVector MoveGoal;
	if (!GridManager->TryGetNavigationLocationForCell(Cell, MoveGoal))
	{
		ensureMsgf(false, TEXT("ExecuteMove - Failed to get move goal"));
		return;
	}

	Unit->OnMoveCompleted.RemoveDynamic(
		OwnerAI,
		&UEnemyUnitAI::OnMoveCompleted
	);

	Unit->OnMoveCompleted.AddDynamic(
		OwnerAI,
		&UEnemyUnitAI::OnMoveCompleted
	);	
	
	if (!Unit->MoveToLocation(MoveGoal, 0.0f))
	{
		ensureMsgf(false, TEXT("ExecuteMove - Failed to Move unit"));
		return;
	}
}
/*
void FEnemyAIActionExecutor::OnMoveCompleted(AStrategyUnit* MovedUnit)
{
	
}
*/