#pragma once

#include "CoreMinimal.h"
#include "AttackStats.generated.h"

USTRUCT(BlueprintType)
struct FAttackStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Range = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Damage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ArmorShred = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ArmorPenetration = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 AmmoCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ActionPointCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bRequiresLineOfSight = true;

	// CRIT
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0", ClampMax="1.0"))
	float CriticalProbability = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CriticalMultiplier = 1.5f;

	// INFECTION
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0", ClampMax="1.0"))
	float InfectionProbability = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float InfectionProgressionRate = 0.0f;
};