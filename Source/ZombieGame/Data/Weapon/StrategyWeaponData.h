#pragma once

#include "CoreMinimal.h"
#include "AttackStats.h"
#include "Data/Item/EquippableItemData.h"
#include "Data/Weapon/AttackStats.h"

#include "StrategyWeaponData.generated.h"

class UNiagaraSystem;
class AActor;
class UAnimMontage;
class UAnimSequenceBase;
class UStaticMesh;

UENUM(BlueprintType)
enum class EStrategyWeaponAttackType : uint8
{
	Fire,
	Melee,
	Bite
};

UCLASS(Blueprintable)
class UStrategyWeaponData : public UEquippableItemData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	EStrategyWeaponAttackType AttackType = EStrategyWeaponAttackType::Fire;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAttackStats AttackStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overwatch", meta=(ClampMin="1.0", ClampMax="360.0", Units="deg"))
	float OverwatchConeAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxAmmo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUsesAmmo = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	TObjectPtr<UNiagaraSystem> FireMuzzleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName FireMuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName FireMuzzleMeshComponentName = TEXT("Body");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FVector FireMuzzleFallbackOffset = FVector(90.0f, 0.0f, 90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|Projectile")
	TSubclassOf<AActor> ProjectileVisualActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|Projectile")
	TObjectPtr<UStaticMesh> ProjectileMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|Projectile", meta=(ClampMin="1.0", Units="cm/s"))
	float ProjectileSpeed = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|Projectile", meta=(ClampMin="0.05", Units="s"))
	float ProjectileLifeSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|Projectile")
	FRotator ProjectileMeshRelativeRotation = FRotator(90.0f, 0.0f, 90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|Projectile")
	FVector ProjectileMeshRelativeScale = FVector(0.25f, 0.25f, 0.25f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|Projectile", meta=(Units="cm"))
	float ProjectileChestHitHeight = 105.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|Projectile", meta=(Units="cm"))
	float ProjectileCriticalHitHeight = 165.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|Projectile", meta=(Units="cm"))
	float ProjectileMissLateralOffset = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX|Projectile", meta=(Units="cm"))
	float ProjectileMissPastTargetDistance = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	TSubclassOf<AActor> EquippedActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	FName EquippedAttachSocketName = TEXT("hand_rSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	FName EquippedAttachMeshComponentName = TEXT("Body");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	FTransform EquippedAttachOffset = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation|Hold Pose")
	TObjectPtr<UAnimMontage> EquippedHoldPoseMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation|Hold Pose")
	TObjectPtr<UAnimSequenceBase> EquippedHoldPoseAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation|Hold Pose")
	FName EquippedHoldPoseSlotName = TEXT("DefaultSlot");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation|Hold Pose")
	FName EquippedHoldPoseMeshComponentName = TEXT("Body");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation|Hold Pose")
	FName EquippedHoldPoseSection = NAME_None;
};
