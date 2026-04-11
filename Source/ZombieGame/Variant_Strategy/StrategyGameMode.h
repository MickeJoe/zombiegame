// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StrategyUnit.h"
#include "StrategyGameMode.generated.h"

/**
 *  Simple GameMode for a top down strategy game.
 */
UCLASS(abstract)
class AStrategyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	void BeginPlay();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Units")
	TSubclassOf<AStrategyUnit> StrategyUnitClass;
private:


	UPROPERTY(Transient)
	TArray<AStrategyUnit*> PlayerUnitArray;
};
