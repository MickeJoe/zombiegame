// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyGameMode.h"

#include "ZombieGame/Variant_Strategy/StrategyUnit.h"

void AStrategyGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
/*
	AStrategyUnit* Unit1 = World->SpawnActor<AStrategyUnit>(StrategyUnitClass, FVector(0, 0, 100), FRotator::ZeroRotator, Params);
	AStrategyUnit* Unit2 = World->SpawnActor<AStrategyUnit>(StrategyUnitClass, FVector(300, 0, 100), FRotator::ZeroRotator, Params);

	PlayerUnitArray.Add(Unit1);
	PlayerUnitArray.Add(Unit2);
*/
}