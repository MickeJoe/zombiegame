#pragma once

#include "AttackStats.h"
#include "Data/Weapon/AttackStats.h"

#include "StrategyWeaponData.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EStrategyWeaponAttackType : uint8
{
	Fire,
	Melee
};

UENUM(BlueprintType)
enum class EStrategyWeaponSlot : uint8
{
	Primary,
	Secondary
};

UCLASS(Blueprintable)
class UStrategyWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName WeaponId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	EStrategyWeaponAttackType AttackType = EStrategyWeaponAttackType::Fire;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	EStrategyWeaponSlot WeaponSlot = EStrategyWeaponSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAttackStats AttackStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overwatch", meta=(ClampMin="1.0", ClampMax="360.0", Units="deg"))
	float OverwatchConeAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxAmmo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUsesAmmo = true;
};
