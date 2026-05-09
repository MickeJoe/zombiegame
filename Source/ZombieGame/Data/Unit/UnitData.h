#pragma once

#include "Data/Weapon/AttackStats.h"

#include "UnitData.generated.h"

class UStrategyWeaponData;
class UTargetInfoWidget;
class UEnemyUnitAI;

UCLASS(BlueprintType)
class UUnitData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats")
	int32 MaxActionPoints = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats")
	int32 MaxMovement = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats")
	int32 SightRange = 28;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats")
	int32 MaxHealth = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats")
	int32 MaxArmor = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat")
	FAttackStats BiteAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat")
	FAttackStats HandAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI")
	TSubclassOf<UEnemyUnitAI> EnemyAIClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> StatusBarWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UTargetInfoWidget> TargetInfoWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment")
	TObjectPtr<UStrategyWeaponData> DefaultWeapon;
};