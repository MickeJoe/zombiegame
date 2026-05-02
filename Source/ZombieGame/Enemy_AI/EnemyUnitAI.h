#pragma once

#include "EnemyUnitAI.generated.h"

class AAIStrategySide;
class AGridManager;
class APlayerStrategySide;
class ASightManager;
class AStrategyUnit;

UENUM()
enum class EEnemyAIActionType : uint8
{
	Attack,
	MoveToCover,
	MoveTowardNearestVisiblePlayer,
	HoldHighGround,
	ProtectSniper,
	SpreadOut,
	Wait
};

struct FEnemyActionCandidate
{
	EEnemyAIActionType ActionType;
	FIntPoint TargetCell;
	TObjectPtr<AStrategyUnit> TargetUnit = nullptr;
	int32 DistanceToTargetAfterMove = 0;
	float Score = 0.f;
	int32 ActionPointCost = 1;
};

struct FEnemyAIWeights
{
	float Attack = 0.0f;
	float Cover  = 0.0f;
	float HighGround = 0.0f;
	float Distance = 0.0f;
	float Exposed = 0.0f;
	float ProtectSniper = 0.0f;
	float Infect = 0.0f;
	float MoveTowardTarget = 0.0f;
	float DistanceToTarget = 0.0f;
	float CanInfect = 0.0f;
	float DistanceToTargetAfterMove = 0.0f;
};

UCLASS(Abstract, Blueprintable)
class UEnemyUnitAI : public UObject
{
	GENERATED_BODY()

public:
	void TakeTurn(
		AStrategyUnit* Unit,
		AGridManager* GridManager,
		ASightManager* SightManager,
		APlayerStrategySide* PlayerSide,
		AAIStrategySide* EnemySide);
	
	UFUNCTION()
	void OnMoveCompleted(AStrategyUnit* MovedUnit);	
	
protected:
	virtual void GenerateCandidates(
		AStrategyUnit* Unit,
		AGridManager* GridManager,
		ASightManager* SightManager,
		APlayerStrategySide* PlayerSide,
		AAIStrategySide* EnemySide,
		TArray<FEnemyActionCandidate>& OutCandidates);
	
	static const FEnemyActionCandidate* FindBestCandidate(
		const TArray<FEnemyActionCandidate>& Candidates);
	
	float ScoreCandidate(
		AStrategyUnit* Unit,
		const FEnemyActionCandidate& Candidate,
		AGridManager* GridManager,
		APlayerStrategySide* PlayerSide,
		AAIStrategySide* EnemySide) const;
	
	virtual FEnemyAIWeights GetAIWeights() const;
	
	void FinishUnitTurn();
	void ExecuteNextAction();
/*	
	AStrategyUnit* FindClosestVisiblePlayerUnit() const;
	
	FIntPoint FindBestMoveCellTowardsTarget(
		const AStrategyUnit* TargetUnit,
		int32 MaxMoveCells
	) const;
*/	
//	UPROPERTY()
//	TObjectPtr<AGridManager> GridManager;

//	UPROPERTY()
//	TArray<AStrategyUnit*> PlayerUnitArray;

	UPROPERTY()
	TObjectPtr<AStrategyUnit> OwnerUnit;	
	
	FEnemyActionCandidate CurrentAction;
	
	AGridManager* CachedGridManager;
	ASightManager* CachedSightManager;
	APlayerStrategySide* CachedPlayerSide;
	AAIStrategySide* CachedEnemySide;
};