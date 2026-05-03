#pragma once

UENUM(BlueprintType)
enum class EPlayerUnitActionType : uint8
{
	None,

	MeleeAttack,
	WeaponAttack,
	Reload,
	HunkerDown,
	Overwatch,
	SkipTurn,

	ThrowGrenade,
	UseItem,
	Interact,
	SwitchWeapon
};