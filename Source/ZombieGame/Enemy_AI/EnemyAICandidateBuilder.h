#pragma once

class ASightManager;
class AAIStrategySide;
struct FEnemyActionCandidate;
class APlayerStrategySide;
class AGridManager;
class AStrategyUnit;

class EnemyAICandidateBuilder
{
public:
	static void AddAttackCandidates(
		AStrategyUnit* Unit,
		AGridManager* GridManager,
		APlayerStrategySide* PlayerSide,
		TArray<FEnemyActionCandidate>& OutCandidates);
	
	static void AddMoveToCoverCandidates(
		AStrategyUnit* Unit,
		AGridManager* GridManager,
		APlayerStrategySide* PlayerSide,
		TArray<FEnemyActionCandidate>& OutCandidates);

	static void AddMoveTowardNearestVisiblePlayerCandidate(
		AStrategyUnit* Unit,
		AGridManager* GridManager,
		ASightManager* SightManager,
		APlayerStrategySide* PlayerSide,
		TArray<FEnemyActionCandidate>& OutCandidates);

	static void AddHoldHighGroundCandidates(
		AStrategyUnit* Unit,
		AGridManager* GridManager,
		TArray<FEnemyActionCandidate>& OutCandidates);

	static void AddProtectSniperCandidates(
		AStrategyUnit* Unit,
		AGridManager* GridManager,
		AAIStrategySide* EnemySide,
		TArray<FEnemyActionCandidate>& OutCandidates);
};
