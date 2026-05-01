#pragma once
#include "StrategySide.h"

#include "AIStrategySide.generated.h"

UCLASS()
class AAIStrategySide : public AStrategySide
{
	GENERATED_BODY()

public:

	virtual void TakeTurn(AGridManager* GridManager, APlayerStrategySide* PlayerSide) override;
	void OnEnemyUnitTurnDone(AStrategyUnit* Unit);

	virtual bool IsHuman() const override;
	virtual bool IsAI() const override;
	
protected:
	UFUNCTION()
	void OnTurnDone();
	
	void StartNextEnemyUnitTurn();
	
	int32 CurrentUnitIndex;
	
	UPROPERTY()
	TObjectPtr<AGridManager> CachedGridManager = nullptr;

	UPROPERTY()
	TObjectPtr<APlayerStrategySide> CachedPlayerSide = nullptr;
};