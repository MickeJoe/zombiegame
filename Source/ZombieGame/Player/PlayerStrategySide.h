#pragma once
#include "StrategySide.h"

#include "PlayerStrategySide.generated.h"

UCLASS()
class APlayerStrategySide : public AStrategySide
{
	GENERATED_BODY()

public:

	virtual void TakeTurn() override;
	
	virtual bool IsHuman() const override;
	virtual bool IsAI() const override;
};