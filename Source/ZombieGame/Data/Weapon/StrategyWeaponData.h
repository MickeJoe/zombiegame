#pragma once

#include "CoreMinimal.h"
#include "AttackStats.h"
#include "Data/Item/EquippableItemData.h"
#include "Data/Weapon/AttackStats.h"

#include "StrategyWeaponData.generated.h"

class UNiagaraSystem;

UENUM(BlueprintType)
enum class EStrategyWeaponAttackType : uint8
{
	Fire,
	Melee,
	Bite
};

UCLASS(Blueprintable)
class UStrategyWeaponData : public UEquippableItemData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	EStrategyWeaponAttackType AttackType = EStrategyWeaponAttackType::Fire;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAttackStats AttackStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overwatch", meta=(ClampMin="1.0", ClampMax="360.0", Units="deg"))
	float OverwatchConeAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxAmmo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUsesAmmo = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	TObjectPtr<UNiagaraSystem> FireMuzzleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName FireMuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName FireMuzzleMeshComponentName = TEXT("Body");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FVector FireMuzzleFallbackOffset = FVector(90.0f, 0.0f, 90.0f);
};
