#pragma once
#include "EnemyUnitAI.h"

#include "WalkerEnemyAI.generated.h"

class ASightManager;
class AAIStrategySide;
class AGridManager;
class APlayerStrategySide;



UCLASS(Blueprintable)
class UWalkerEnemyAI : public UEnemyUnitAI
{
	GENERATED_BODY()

public:
	
	virtual void GenerateCandidates(
		AStrategyUnit* Unit,
		AGridManager* GridManager,
		ASightManager* SightManager,
		APlayerStrategySide* PlayerSide,
		AAIStrategySide* EnemySide,
		TArray<FEnemyActionCandidate>& OutCandidates) override;

private:
	virtual FEnemyAIWeights GetAIWeights() const override;

};