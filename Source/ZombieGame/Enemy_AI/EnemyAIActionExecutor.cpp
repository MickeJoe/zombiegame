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
	case EEnemyAIActionType::BiteAttack:
		ExecuteBiteAttack(Unit, Candidate, OwnerAI);
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

void FEnemyAIActionExecutor::ExecuteBiteAttack(
	AStrategyUnit* Unit,
	const FEnemyActionCandidate& Candidate,
	UEnemyUnitAI* OwnerAI)
{
	FWeaponDamage Damage;
	Damage.Damage = 3;
	Damage.ArmorPierce = false;
	Damage.ArmorShred = false;
	
	Unit->ApplyDamage(Damage);
	OwnerAI->OnMoveCompleted(Unit);
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