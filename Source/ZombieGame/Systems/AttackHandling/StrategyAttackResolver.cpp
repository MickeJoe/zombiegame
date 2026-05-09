#include "StrategyAttackResolver.h"

#include "Data/Weapon/AttackStats.h"
#include "Data/Weapon/StrategyWeaponData.h"
#include "Data/Unit/UnitData.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"

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

FStrategyAttackResult UStrategyAttackResolver::Resolve(const FStrategyAttackContext& Context)
{
	FStrategyAttackResult Result;

	if (!Context.Attacker || !Context.Target || !Context.AttackStats)
	{
		return Result;
	}

	Result.bHit = true;
	Result.Damage.Damage = Context.AttackStats->Damage;
	Result.Damage.ArmorPierce = Context.AttackStats->ArmorPenetration;
	Result.Damage.ArmorShred = Context.AttackStats->ArmorShred;

	return Result;
}

FStrategyAttackResult UStrategyAttackResolver::ResolveAndApply(const FStrategyAttackContext& Context)
{
	FStrategyAttackResult Result = Resolve(Context);

	if (Result.bHit && Context.Target)
	{
		Context.Target->ApplyDamage(Result.Damage);
	}

	return Result;
}
