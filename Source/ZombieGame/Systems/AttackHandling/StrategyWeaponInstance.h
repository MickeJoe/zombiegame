#pragma once

#include "CoreMinimal.h"
#include "StrategyWeaponData.h"
#include "StrategyWeaponInstance.generated.h"

USTRUCT(BlueprintType)
struct FStrategyWeaponInstance
{
	GENERATED_BODY()

public:
	void Init(UStrategyWeaponData* InData);	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStrategyWeaponData> WeaponData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentAmmo = 0;

public:
	bool UsesAmmo() const;
	int32 GetMaxAmmo() const;
	const FAttackStats* GetAttackStats() const;
};