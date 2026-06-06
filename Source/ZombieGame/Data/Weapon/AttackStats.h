#pragma once

#include "CoreMinimal.h"
#include "AttackStats.generated.h"

USTRUCT(BlueprintType)
struct FRangeChanceModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 MinRange = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 MaxRange = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 HitModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CriticalModifier = 0;
};

USTRUCT(BlueprintType)
struct FAttackStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Accuracy", meta=(ClampMin="0", ClampMax="100"))
	int32 BaseHitChance = 65;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Accuracy", meta=(ClampMin="0", ClampMax="100"))
	int32 MinimumHitChance = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Accuracy", meta=(ClampMin="0", ClampMax="100"))
	int32 MaximumHitChance = 95;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Accuracy")
	TArray<FRangeChanceModifier> RangeChanceModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Damage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ArmorShred = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ArmorPenetration = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 AmmoCost = 0;

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
