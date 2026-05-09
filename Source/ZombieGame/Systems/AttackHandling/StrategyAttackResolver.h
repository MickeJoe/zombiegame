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
	FWeaponDamage Damage;
};

UCLASS()
class UStrategyAttackResolver : public UObject
{
	GENERATED_BODY()

public:
	static FStrategyAttackContext MakeContext(AStrategyUnit* Attacker, AStrategyUnit* Target);
	static FStrategyAttackResult Resolve(const FStrategyAttackContext& Context);
	static FStrategyAttackResult ResolveAndApply(const FStrategyAttackContext& Context);
};
