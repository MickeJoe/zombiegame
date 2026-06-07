#pragma once
#include "EnemyUnitAI.h"

#include "StalkerEnemyAI.generated.h"

class ASightManager;
class AAIStrategySide;
class AGridManager;
class APlayerStrategySide;



UCLASS(Blueprintable)
class UStalkerEnemyAI : public UEnemyUnitAI
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
