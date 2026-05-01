#pragma once
#include "StrategySide.h"

#include "PlayerStrategySide.generated.h"

UCLASS()
class APlayerStrategySide : public AStrategySide
{
	GENERATED_BODY()

public:

	virtual void TakeTurn(AGridManager* GridManager, APlayerStrategySide* PlayerSide) override;
	
	virtual bool IsHuman() const override;
	virtual bool IsAI() const override;
};