#include "EnemyAIActionExecutor.h"

#include "EnemyAIDebug.h"
#include "StrategyUnit.h"
#include "EnemyUnitAI.h"
#include "StrategyPlayerController.h"
#include "Systems/GridManager.h"
#include "Systems/AttackHandling/StrategyAttackResolver.h"
#include "TimerManager.h"

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

	case EEnemyAIActionType::Crouch:
		ExecuteCrouch(Unit, OwnerAI);
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
	if (!IsValid(Unit) || !IsValid(Candidate.TargetUnit))
	{
		ensureMsgf(false, TEXT("ExecuteBiteAttack - Invalid attacker or target"));
		if (OwnerAI)
		{
			OwnerAI->OnMoveCompleted(Unit);
		}
		return;
	}

	const FAttackStats* AttackStats = Unit->GetBiteAttackStatsPtr();
	if (!AttackStats || Unit->GetRemainingTimeUnits() < Unit->GetBiteAttackTimeUnitCost())
	{
		if (OwnerAI)
		{
			OwnerAI->OnMoveCompleted(Unit);
		}
		return;
	}

	const FStrategyAttackContext Context =
		UStrategyAttackResolver::MakeContextWithAttackStats(Unit, Candidate.TargetUnit, AttackStats);
	Unit->FaceTargetForAttack(Candidate.TargetUnit);
	const int32 TargetHealthBefore = Candidate.TargetUnit->GetCurrentHealth();
	const int32 TargetArmorBefore = Candidate.TargetUnit->GetCurrentArmor();
	const FStrategyAttackResult Result = UStrategyAttackResolver::ResolveAndApply(Context);
#if !UE_BUILD_SHIPPING
	FEnemyAIDebug::LogBiteAttackResult(Unit, Candidate, Result, TargetHealthBefore, TargetArmorBefore);
#endif
	const float AttackMontageDuration = Unit->PlayBiteAttackMontage();

	if (AStrategyPlayerController* PC = Unit->GetWorld()
		? Unit->GetWorld()->GetFirstPlayerController<AStrategyPlayerController>()
		: nullptr)
	{
		PC->RefreshPlayerUnitRoster();
	}

	if (OwnerAI)
	{
		const float ActionDelay = FMath::Max3(AttackMontageDuration, Result.ReactionMontageDuration, 0.1f);
		FTimerHandle BiteActionCompleteTimerHandle;
		Unit->GetWorldTimerManager().SetTimer(
			BiteActionCompleteTimerHandle,
			FTimerDelegate::CreateUObject(OwnerAI, &UEnemyUnitAI::OnMoveCompleted, Unit),
			ActionDelay,
			false);
	}
}

void FEnemyAIActionExecutor::ExecuteCrouch(
	AStrategyUnit* Unit,
	UEnemyUnitAI* OwnerAI)
{
	if (!IsValid(Unit))
	{
		return;
	}

	const float CrouchMontageDuration = Unit->EnterCrouch();
	if (OwnerAI)
	{
		const float ActionDelay = FMath::Max(CrouchMontageDuration, 0.1f);
		FTimerHandle CrouchActionCompleteTimerHandle;
		Unit->GetWorldTimerManager().SetTimer(
			CrouchActionCompleteTimerHandle,
			FTimerDelegate::CreateUObject(OwnerAI, &UEnemyUnitAI::CompleteActionAndFinishTurn, Unit),
			ActionDelay,
			false);
	}
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
