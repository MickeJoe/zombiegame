// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "StrategyCheatManager.generated.h"

UCLASS()
class ZOMBIEGAME_API UStrategyCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(exec)
	void EnableAlwaysMeleeAttack(bool bEnable = true);

	UFUNCTION(exec)
	void EnableEnemyAIDebug(bool bEnable = true);
};
