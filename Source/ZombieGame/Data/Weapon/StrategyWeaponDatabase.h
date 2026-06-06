#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StrategyWeaponDatabase.generated.h"

class UStrategyWeaponData;

UCLASS(BlueprintType)
class ZOMBIEGAME_API UStrategyWeaponDatabase : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapons")
	TArray<TObjectPtr<UStrategyWeaponData>> Weapons;

	UFUNCTION(BlueprintPure, Category="Weapons")
	UStrategyWeaponData* FindWeaponById(FName ItemId) const;
};
