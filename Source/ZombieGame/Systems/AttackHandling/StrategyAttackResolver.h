#pragma once

#include "CoreMinimal.h"
#include "Variant_Strategy/StrategyUnit.h"
#include "StrategyAttackResolver.generated.h"

struct FAttackStats;
struct FStrategyWeaponInstance;
class UUnitData;

USTRUCT(BlueprintType)
struct FStrategyAttackContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AStrategyUnit> Attacker = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AStrategyUnit> Target = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UUnitData> AttackerUnitData = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UUnitData> TargetUnitData = nullptr;

	const FStrategyWeaponInstance* EquippedWeapon = nullptr;
	const FAttackStats* AttackStats = nullptr;
};

USTRUCT(BlueprintType)
struct FStrategyAttackResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bHit = false;

	UPROPERTY(BlueprintReadOnly)
	bool bCritical = false;

	UPROPERTY(BlueprintReadOnly)
	int32 HitChance = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 CriticalChance = 0;

	UPROPERTY(BlueprintReadOnly)
	FWeaponDamage Damage;

	UPROPERTY(BlueprintReadOnly)
	float ReactionMontageDuration = 0.0f;
};

UCLASS()
class UStrategyAttackResolver : public UObject
{
	GENERATED_BODY()

public:
	static FStrategyAttackContext MakeContext(AStrategyUnit* Attacker, AStrategyUnit* Target);
	static int32 CalculateDistanceInCells(const FStrategyAttackContext& Context);
	static int32 CalculateHitChance(const FStrategyAttackContext& Context);
	static int32 CalculateCriticalChance(const FStrategyAttackContext& Context);
	static FStrategyAttackResult Resolve(const FStrategyAttackContext& Context);
	static FStrategyAttackResult ResolveAndApply(const FStrategyAttackContext& Context);
};
