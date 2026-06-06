#include "Data/Weapon/StrategyWeaponDatabase.h"

#include "Data/Weapon/StrategyWeaponData.h"

UStrategyWeaponData* UStrategyWeaponDatabase::FindWeaponById(FName ItemId) const
{
	for (UStrategyWeaponData* Weapon : Weapons)
	{
		if (Weapon && Weapon->ItemId == ItemId)
		{
			return Weapon;
		}
	}

	return nullptr;
}
