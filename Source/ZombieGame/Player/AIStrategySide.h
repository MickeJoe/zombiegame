#pragma once
#include "StrategySide.h"

#include "AIStrategySide.generated.h"

UCLASS()
class AAIStrategySide : public AStrategySide
{
	GENERATED_BODY()

public:

	virtual void TakeTurn() override;

	virtual bool IsHuman() const override;
	virtual bool IsAI() const override;
	
protected:
	UFUNCTION()
	void OnTurnDone();
};