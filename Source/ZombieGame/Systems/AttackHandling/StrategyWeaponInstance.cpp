#include "StrategyWeaponInstance.h"

void FStrategyWeaponInstance::Init(UStrategyWeaponData* InData)
{
	check(InData);

	WeaponData = InData;

	// Init ammo
	if (WeaponData->bUsesAmmo)
	{
		CurrentAmmo = WeaponData->MaxAmmo;
	}
	else
	{
		CurrentAmmo = -1;
	}
}

bool FStrategyWeaponInstance::UsesAmmo() const
{
	return WeaponData && WeaponData->bUsesAmmo;
}

int32 FStrategyWeaponInstance::GetMaxAmmo() const
{
	return WeaponData ? WeaponData->MaxAmmo : 0;
}

const FAttackStats* FStrategyWeaponInstance::GetAttackStats() const
{
	return WeaponData ? &WeaponData->AttackStats : nullptr;
}