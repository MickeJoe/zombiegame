#include "WalkerEnemyAI.h"

#include "WalkerEnemyAI.h"

#include "EnemyAICandidateBuilder.h"
#include "StrategyUnit.h"
#include "Player/AIStrategySide.h"
#include "Systems/GridManager.h"
#include "Player/PlayerStrategySide.h"

void UWalkerEnemyAI::GenerateCandidates(
	AStrategyUnit* Unit,
	AGridManager* GridManager,
	APlayerStrategySide* PlayerSide,
	AAIStrategySide* EnemySide,
	TArray<FEnemyActionCandidate>& OutCandidates)
{
	Super::GenerateCandidates(Unit, GridManager, PlayerSide, EnemySide, OutCandidates);
	
	EnemyAICandidateBuilder::AddAttackCandidates(
		Unit, GridManager, PlayerSide, OutCandidates);

	EnemyAICandidateBuilder::AddMoveTowardNearestVisiblePlayerCandidate(
		Unit, GridManager, PlayerSide, OutCandidates);
}

FEnemyAIWeights UWalkerEnemyAI::GetAIWeights() const
{
	Super::GetAIWeights();
	
	FEnemyAIWeights Weights;

	Weights.Attack = 120.f;
	Weights.MoveTowardTarget = 60.f;
	Weights.DistanceToTarget = -10.f;
	Weights.CanInfect = 50.f;

	return Weights;
}

