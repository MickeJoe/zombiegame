#pragma once

#include "CoreMinimal.h"

class AGridManager;
class AStrategyUnit;
class UEnemyUnitAI;
struct FEnemyActionCandidate;

class FEnemyAIActionExecutor
{
public:
	static void Execute(
		AGridManager* GridManager,
		AStrategyUnit* Unit,
		const FEnemyActionCandidate& Candidate,
		UEnemyUnitAI* OwnerAI);

private:
	static void ExecuteBiteAttack(
		AStrategyUnit* Unit,
		const FEnemyActionCandidate& Candidate,
		UEnemyUnitAI* OwnerAI);

	static void ExecuteMove(
		AGridManager* GridManager,
		AStrategyUnit* Unit,
		const FIntPoint& Cell,
		UEnemyUnitAI* OwnerAI);
	
};