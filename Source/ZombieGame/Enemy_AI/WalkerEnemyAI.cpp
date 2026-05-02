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
	ASightManager* SightManager,
	APlayerStrategySide* PlayerSide,
	AAIStrategySide* EnemySide,
	TArray<FEnemyActionCandidate>& OutCandidates)
{
	Super::GenerateCandidates(Unit, GridManager, SightManager, PlayerSide, EnemySide, OutCandidates);
	
	EnemyAICandidateBuilder::AddBiteAttackCandidate(
		Unit, GridManager, PlayerSide, OutCandidates);

	EnemyAICandidateBuilder::AddMoveTowardNearestVisiblePlayerCandidate(
		Unit, GridManager, SightManager, PlayerSide, OutCandidates);
}

FEnemyAIWeights UWalkerEnemyAI::GetAIWeights() const
{
	Super::GetAIWeights();
	
	FEnemyAIWeights Weights;

	Weights.BiteAttack = 120.f;
	Weights.MoveTowardTarget = 60.f;
	Weights.DistanceToTarget = -10.f;
	Weights.CanInfect = 50.f;

	return Weights;
}

