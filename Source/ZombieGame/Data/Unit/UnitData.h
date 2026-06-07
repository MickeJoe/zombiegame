#pragma once

#include "CoreMinimal.h"
#include "Data/Weapon/AttackStats.h"

#include "UnitData.generated.h"

class UStrategyWeaponData;
class UEquippableItemData;
class UTargetInfoWidget;
class UEnemyUnitAI;
class UAnimMontage;
class UTexture2D;
class UUserWidget;

USTRUCT(BlueprintType)
struct FStrategyAttackAnimation
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName MeshComponentName = TEXT("Body");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName MontageSection = NAME_None;
};

USTRUCT(BlueprintType)
struct FUnitCrouchHitChanceModifiers
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crouch")
	int32 NoCover = -10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crouch")
	int32 HalfCover = -20;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crouch")
	int32 FullCover = -30;
};

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
	int32 MaxTimeUnits = 15;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats")
	int32 MaxMovement = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats")
	int32 SightRange = 28;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats")
	int32 MaxHealth = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats")
	int32 MaxArmor = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Accuracy")
	int32 Aim = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Accuracy")
	int32 WeaponSkill = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Accuracy")
	int32 CriticalChanceModifier = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Defense")
	int32 Defense = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Defense")
	int32 CriticalDefense = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Crouch", meta=(ClampMin="0"))
	int32 CrouchTimeUnitCost = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Crouch")
	FUnitCrouchHitChanceModifiers CrouchHitChanceModifiers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat", meta=(DeprecatedProperty, DeprecationMessage="Use a UStrategyWeaponData with AttackType Bite in DefaultItems instead."))
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment")
	TArray<TObjectPtr<UStrategyWeaponData>> DefaultWeapons;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment")
	TArray<TObjectPtr<UEquippableItemData>> DefaultItems;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> DeathReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Crouch")
	FStrategyAttackAnimation CrouchAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Combat")
	TMap<FName, FStrategyAttackAnimation> WeaponAttackAnimations;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Combat")
	FStrategyAttackAnimation DefaultFireAttackAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Combat")
	FStrategyAttackAnimation DefaultMeleeAttackAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Combat")
	FStrategyAttackAnimation DefaultBiteAttackAnimation;
};
