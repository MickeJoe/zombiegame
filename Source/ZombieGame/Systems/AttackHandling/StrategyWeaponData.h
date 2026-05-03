#pragma once

#include "AttackStats.h"

#include "StrategyWeaponData.generated.h"

UCLASS(Blueprintable)
class UStrategyWeaponData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName WeaponId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAttackStats AttackStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxAmmo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUsesAmmo = true;
};