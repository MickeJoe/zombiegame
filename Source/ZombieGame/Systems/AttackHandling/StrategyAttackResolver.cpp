#include "StrategyAttackResolver.h"

#include "Data/Weapon/AttackStats.h"
#include "Data/Weapon/StrategyWeaponData.h"
#include "Data/Unit/UnitData.h"
#include "Kismet/GameplayStatics.h"
#include "Systems/GridManager.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"

namespace
{
	const FRangeChanceModifier* FindRangeModifier(const FAttackStats& AttackStats, int32 DistanceInCells)
	{
		for (const FRangeChanceModifier& Modifier : AttackStats.RangeChanceModifiers)
		{
			if (DistanceInCells >= Modifier.MinRange && DistanceInCells <= Modifier.MaxRange)
			{
				return &Modifier;
			}
		}

		return nullptr;
	}
}

FStrategyAttackContext UStrategyAttackResolver::MakeContext(AStrategyUnit* Attacker, AStrategyUnit* Target)
{
	FStrategyAttackContext Context;
	Context.Attacker = Attacker;
	Context.Target = Target;

	if (Attacker)
	{
		Context.AttackerUnitData = Attacker->UnitData;
		Context.EquippedWeapon = &Attacker->GetEquippedWeapon();
		Context.AttackStats = Context.EquippedWeapon->GetAttackStats();
	}

	if (Target)
	{
		Context.TargetUnitData = Target->UnitData;
	}

	return Context;
}

FStrategyAttackContext UStrategyAttackResolver::MakeContextFromCell(
	AStrategyUnit* Attacker,
	AStrategyUnit* Target,
	const FIntPoint& AttackerCell)
{
	FStrategyAttackContext Context = MakeContext(Attacker, Target);
	Context.bUseOverrideAttackerCell = true;
	Context.OverrideAttackerCell = AttackerCell;
	return Context;
}

FStrategyAttackContext UStrategyAttackResolver::MakeContextWithAttackStats(
	AStrategyUnit* Attacker,
	AStrategyUnit* Target,
	const FAttackStats* AttackStats)
{
	FStrategyAttackContext Context = MakeContext(Attacker, Target);
	Context.AttackStats = AttackStats;
	return Context;
}

int32 UStrategyAttackResolver::CalculateDistanceInCells(const FStrategyAttackContext& Context)
{
	if (!Context.Attacker || !Context.Target)
	{
		return 0;
	}

	if (const UWorld* World = Context.Attacker->GetWorld())
	{
		if (const AGridManager* GridManager = Cast<AGridManager>(
			UGameplayStatics::GetActorOfClass(World, AGridManager::StaticClass())))
		{
			const FIntPoint AttackerCell = Context.bUseOverrideAttackerCell
				? Context.OverrideAttackerCell
				: GridManager->WorldToGrid(Context.Attacker->GetActorLocation());
			const FIntPoint TargetCell = GridManager->WorldToGrid(Context.Target->GetActorLocation());

			return FMath::Abs(AttackerCell.X - TargetCell.X) + FMath::Abs(AttackerCell.Y - TargetCell.Y);
		}
	}

	constexpr float FallbackCellSize = 100.0f;
	return FMath::RoundToInt(FVector::Dist2D(Context.Attacker->GetActorLocation(), Context.Target->GetActorLocation()) / FallbackCellSize);
}

int32 UStrategyAttackResolver::CalculateHitChance(const FStrategyAttackContext& Context)
{
	if (!Context.AttackStats)
	{
		return 0;
	}

	const int32 DistanceInCells = CalculateDistanceInCells(Context);
	int32 HitChance = Context.AttackStats->BaseHitChance;

	if (Context.AttackerUnitData)
	{
		HitChance += Context.AttackerUnitData->Aim;
		HitChance += Context.AttackerUnitData->WeaponSkill;
	}

	if (const FRangeChanceModifier* RangeModifier = FindRangeModifier(*Context.AttackStats, DistanceInCells))
	{
		HitChance += RangeModifier->HitModifier;
	}

	if (Context.TargetUnitData)
	{
		HitChance -= Context.TargetUnitData->Defense;
	}

	const int32 MinHitChance = FMath::Clamp(Context.AttackStats->MinimumHitChance, 0, 100);
	const int32 MaxHitChance = FMath::Clamp(Context.AttackStats->MaximumHitChance, MinHitChance, 100);

	return FMath::Clamp(HitChance, MinHitChance, MaxHitChance);
}

int32 UStrategyAttackResolver::CalculateCriticalChance(const FStrategyAttackContext& Context)
{
	if (!Context.AttackStats)
	{
		return 0;
	}

	const int32 DistanceInCells = CalculateDistanceInCells(Context);
	int32 CriticalChance = FMath::RoundToInt(Context.AttackStats->CriticalProbability * 100.0f);

	if (Context.AttackerUnitData)
	{
		CriticalChance += Context.AttackerUnitData->CriticalChanceModifier;
	}

	if (const FRangeChanceModifier* RangeModifier = FindRangeModifier(*Context.AttackStats, DistanceInCells))
	{
		CriticalChance += RangeModifier->CriticalModifier;
	}

	if (Context.TargetUnitData)
	{
		CriticalChance -= Context.TargetUnitData->CriticalDefense;
	}

	return FMath::Clamp(CriticalChance, 0, 100);
}

FStrategyAttackResult UStrategyAttackResolver::Resolve(const FStrategyAttackContext& Context)
{
	FStrategyAttackResult Result;

	if (!Context.Attacker || !Context.Target || !Context.AttackStats)
	{
		return Result;
	}

	Result.HitChance = CalculateHitChance(Context);
	Result.CriticalChance = CalculateCriticalChance(Context);
	Result.bHit = FMath::RandRange(1, 100) <= Result.HitChance;

	if (!Result.bHit)
	{
		return Result;
	}

	Result.bCritical = FMath::RandRange(1, 100) <= Result.CriticalChance;
	Result.Damage.Damage = Context.AttackStats->Damage;

	if (Result.bCritical)
	{
		Result.Damage.Damage = FMath::RoundToInt(Result.Damage.Damage * Context.AttackStats->CriticalMultiplier);
	}

	Result.Damage.ArmorPierce = Context.AttackStats->ArmorPenetration;
	Result.Damage.ArmorShred = Context.AttackStats->ArmorShred;

	return Result;
}

FStrategyAttackResult UStrategyAttackResolver::ResolveAndApply(const FStrategyAttackContext& Context)
{
	FStrategyAttackResult Result = Resolve(Context);

	if (Result.bHit && Context.Target)
	{
		Result.ReactionMontageDuration = Context.Target->ApplyDamage(Result.Damage);
	}

	return Result;
}
