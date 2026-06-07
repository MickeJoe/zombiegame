// Copyright Epic Games, Inc. All Rights Reserved.

#include "StrategyCheatManager.h"

#include "Enemy_AI/EnemyAIDebug.h"
#include "StrategyPlayerController.h"
#include "ZombieGame.h"

void UStrategyCheatManager::EnableAlwaysMeleeAttack(bool bEnable)
{
	AStrategyPlayerController* StrategyPC = Cast<AStrategyPlayerController>(GetOuterAPlayerController());
	if (!StrategyPC)
	{
		UE_LOG(LogZombieGame, Warning, TEXT("EnableAlwaysMeleeAttack failed: owner is not a StrategyPlayerController."));
		return;
	}

	StrategyPC->SetAlwaysMeleeAttackEnabled(bEnable);

	UE_LOG(
		LogZombieGame,
		Log,
		TEXT("Always melee attack cheat is now %s."),
		bEnable ? TEXT("enabled") : TEXT("disabled"));
}

void UStrategyCheatManager::EnableEnemyAIDebug(bool bEnable)
{
#if !UE_BUILD_SHIPPING
	FEnemyAIDebug::SetEnabled(bEnable);

	UE_LOG(
		LogZombieGame,
		Log,
		TEXT("Enemy AI debug cheat is now %s."),
		bEnable ? TEXT("enabled") : TEXT("disabled"));
#else
	UE_LOG(LogZombieGame, Warning, TEXT("Enemy AI debug cheat is not available in shipping builds."));
#endif
}
