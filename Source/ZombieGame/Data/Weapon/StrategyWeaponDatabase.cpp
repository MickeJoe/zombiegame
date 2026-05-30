#include "Data/Weapon/StrategyWeaponDatabase.h"

#include "Data/Weapon/StrategyWeaponData.h"

UStrategyWeaponData* UStrategyWeaponDatabase::FindWeaponById(FName WeaponId) const
{
	for (UStrategyWeaponData* Weapon : Weapons)
	{
		if (Weapon && Weapon->WeaponId == WeaponId)
		{
			return Weapon;
		}
	}

	return nullptr;
}
