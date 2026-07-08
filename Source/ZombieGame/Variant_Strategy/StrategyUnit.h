// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Data/Item/EquippableItemData.h"
#include "Data/Weapon/AttackStats.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"
#include "TargetingCameraMode.h"
#include "StrategyUnit.generated.h"

class UTargetInfoWidget;
class UTexture2D;
class AStrategyGameMode;
class UCameraComponent;
class AAIStrategySide;
class APlayerStrategySide;
class UStrategyWeaponData;
class UWidgetComponent;
class UEnemyUnitAI;
class AGridManager;
class AStrategySide;
class UChildActorComponent;
class USphereComponent;
class UUnitData;
class UAnimMontage;
class UEquippableItemData;
class UMedicBagData;
class UNiagaraSystem;
struct FStrategyAttackResult;
enum class EGridCoverType : uint8;

UENUM(BlueprintType)
enum class EStrategyUnitTeam : uint8
{
	Human,
	AI
};

USTRUCT(BlueprintType)
struct FWeaponDamage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Damage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ArmorPierce = 0; // ignorerar armor

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ArmorShred = 0;  // tar bort armor
};

/** Delegate to report that this unit has finished moving */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitMoveCompletedDelegate, AStrategyUnit*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGridCellChanged, AStrategyUnit*, Unit);

/**
 *  A simple strategy game unit
 *  Rather than react to inputs, it's controlled indirectly by the Strategy Player Controller
 */
UCLASS(abstract)
class AStrategyUnit : public ACharacter
{
	GENERATED_BODY()

private:

	/** Interaction range sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* InteractionRange;

protected:

	/** Cast reference to the AI Controlling this unit */
	TObjectPtr<AAIController> AIController;
	
	virtual void Tick(float DeltaTime) override;

public:

	/** Constructor */
	AStrategyUnit();
	
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

protected:

	virtual void NotifyControllerChanged() override;

public:

	/** Stops unit movement immediately */
	void StopMoving();

	/** Notifies this unit that it was selected */
	void UnitSelected();

	/** Notifies this unit that it was deselected */
	void UnitDeselected();

	/** Notifies this unit that it's been interacted with by another actor */
	void Interact(AStrategyUnit* Interactor);

	/** Attempts to move this unit to its */
	bool MoveToLocation(const FVector& Location, float AcceptanceRadius);
	
	int32 GetSightRange() const;

	UFUNCTION(BlueprintPure, Category = "Unit Stats")
	int32 GetMaxMovement() const;
	
	UFUNCTION(BlueprintPure, Category = "Unit Stats")
	int32 GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Unit Stats")
	int32 GetCurrentArmor() const { return CurrentArmor; }

	UFUNCTION(BlueprintPure, Category = "Unit Stats", meta=(DeprecatedFunction, DeprecationMessage="Use GetMaxTimeUnits instead."))
	int32 GetMaxActionPoints() const;

	UFUNCTION(BlueprintPure, Category = "Unit Stats")
	int32 GetMaxTimeUnits() const;

	UFUNCTION(BlueprintPure, Category = "Unit Stats")
	int32 GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Unit Stats")
	int32 GetMaxArmor() const;

	FAttackStats GetBiteAttackStats() const;
	const FAttackStats* GetMeleeAttackStats() const;
	const FAttackStats* GetBiteAttackStatsPtr() const;
	int32 GetBiteAttackRange() const;
	int32 GetBiteAttackTimeUnitCost() const;
	int32 GetCrouchTimeUnitCost() const;
	int32 GetCrouchHitChanceModifier(bool bHasCover, EGridCoverType CoverType) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strategy")
	TObjectPtr<AStrategySide> OwningSide = nullptr;

	void SetStrategyUnitTeam(EStrategyUnitTeam InStrategyUnitTeam);
	EStrategyUnitTeam GetStrategyUnitTeam() const;
	
	UPROPERTY(BlueprintAssignable)
	FOnGridCellChanged OnGridCellChanged;
	
	TObjectPtr<UEnemyUnitAI> GetEnemyAI() const { return EnemyAI; }
	
	void UpdateStatusBar();
	
	bool CanMeleeAttack(AAIStrategySide* EnemySide) const;
	bool CanWeaponAttack(AAIStrategySide* EnemySide) const;
	void SpendMeleeAttackResources();
	void SpendWeaponAttackResources();
	void StartMeleeAttackMode();
	void StartWeaponAttackMode();
	void FaceTargetForAttack(const AStrategyUnit* Target);
	float PlayWeaponAttackMontage(const FStrategyWeaponInstance& Weapon);
	void PlayWeaponMuzzleEffect(const FStrategyWeaponInstance& Weapon);
	void PlayWeaponProjectileVisual(const FStrategyWeaponInstance& Weapon, const AStrategyUnit* Target, const FStrategyAttackResult& Result);
	float PlayMeleeAttackMontage();
	float PlayBiteAttackMontage();
	bool CanReload() const;
	void ReloadWeapon();
	bool CanOverwatch() const;
	bool CanCrouch() const;
	float EnterCrouch();
	bool IsCrouching() const { return bIsCrouching; }
	int32 GetOverwatchRange() const;
	float GetOverwatchConeAngleDegrees() const;
	void EnterOverwatch(const FVector& Direction, int32 Range, float AngleDegrees, const TArray<FIntPoint>& Cells);
	void ClearOverwatch();
	bool TryFireOverwatchAt(AStrategyUnit* Target);
	bool IsOverwatchActive() const { return bOverwatchActive; }
	const FVector& GetOverwatchDirection() const { return OverwatchDirection; }
	int32 GetActiveOverwatchRange() const { return OverwatchRange; }
	float GetOverwatchAngleDegrees() const { return OverwatchAngleDegrees; }
	const TArray<FIntPoint>& GetOverwatchCells() const { return OverwatchCells; }

protected:

	/** called by the AI controller when this unit has finished moving */
	void OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);

	EStrategyUnitTeam StrategyUnitTeam;

protected:

	/** Blueprint handler for strategy game selection */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Unit Selected"))
	void BP_UnitSelected();

	/** Blueprint handler for strategy game deselection */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Unit Deselected"))
	void BP_UnitDeselected();

	/** Blueprint handler for strategy game interactions */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Interaction Behavior"))
	void BP_InteractionBehavior(AStrategyUnit* Interactor);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI")
	TObjectPtr<UWidgetComponent> StatusBarWidgetComponent;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Unit Data")
	TObjectPtr<UUnitData> UnitData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI", meta=(DisplayName="Enemy AI Class Override"))
	TSubclassOf<UEnemyUnitAI> EnemyAIClassOverride;

	void SpendTimeUnits(int32 TimeUnits);
	int32 GetRemainingTimeUnits() const;
	void ResetTimeUnits();

	void UseAtionPoints(int32 ActionPoints);
	int32 GetRemainingActionPoints() const;
	void ResetActionPoints();
	
	float ApplyDamage(const FWeaponDamage& WeaponDamage);
	int32 ApplyHealing(int32 HealAmount);
	
	void EquipItem(UEquippableItemData* ItemData);
	void EquipWeapon(UStrategyWeaponData* WeaponData);
	void ClearEquippedWeapons();
	void SetActiveWeaponSlot(EEquippableItemSlot WeaponSlot);
	EEquippableItemSlot GetActiveWeaponSlot() const { return ActiveWeaponSlot; }
	UEquippableItemData* GetEquippedItem() const;
	UEquippableItemData* GetItemInSlot(EEquippableItemSlot ItemSlot) const;
	const FStrategyWeaponInstance& GetEquippedWeapon() const;
	const FStrategyWeaponInstance& GetWeaponInSlot(EEquippableItemSlot WeaponSlot) const;
	const FStrategyWeaponInstance& GetEquippedFireWeapon() const;
	const FStrategyWeaponInstance& GetEquippedMeleeWeapon() const;
	const FStrategyWeaponInstance& GetEquippedBiteWeapon() const;
	UMedicBagData* GetEquippedMedicBag() const;
	bool CanUseMedicBagOn(const AStrategyUnit* Target) const;
	bool UseMedicBagOn(AStrategyUnit* Target);
	
	UTargetInfoWidget* GetTargetInfoWidget() const { return TargetInfoWidget; }
	
	TArray<AStrategyUnit*> GetEnemiesInRange() const;
	UTexture2D* GetShootableTargetIconTexture() const { return ShootableTargetIconTexture; }
	void SetTargetBracketVisible(bool bVisible);
	void SetTargetInfoVisible(bool bVisible);	
	void SetTargetingCameraView(EStrategyTargetingCameraView CameraView);
	void ClearTargetingCameraView();

	FOnUnitMoveCompletedDelegate OnMoveCompleted;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(DisplayName="Primary Weapon / Item"))
	TObjectPtr<UEquippableItemData> PrimaryItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(DisplayName="Secondary Weapon / Item"))
	TObjectPtr<UEquippableItemData> SecondaryItem;

	UPROPERTY(Transient)
	FStrategyWeaponInstance PrimaryWeapon;

	UPROPERTY(Transient)
	FStrategyWeaponInstance SecondaryWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	EEquippableItemSlot ActiveWeaponSlot = EEquippableItemSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment|Visuals")
	TSubclassOf<AActor> MeleeWeaponActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment|Visuals")
	FName MeleeWeaponSocketName = TEXT("hand_rSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment|Visuals")
	FName MeleeWeaponMeshComponentName = TEXT("Body");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment|Visuals")
	TObjectPtr<UChildActorComponent> MeleeWeaponActorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment|Visuals")
	TObjectPtr<UChildActorComponent> EquippedWeaponActorComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Animation")
	TObjectPtr<UAnimMontage> MeleeAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Animation")
	FName MeleeAttackMontageMeshComponentName = TEXT("Body");

	UPROPERTY(Transient)
	FStrategyWeaponInstance EmptyWeaponInstance;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveWeaponHoldPoseMontage;
	
	int32 UsedTimeUnits = 0;
	int32 CurrentHealth = 0;
	int32 CurrentArmor = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Overwatch")
	bool bOverwatchActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Overwatch")
	FVector OverwatchDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Overwatch")
	int32 OverwatchRange = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Overwatch")
	float OverwatchAngleDegrees = 135.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Overwatch")
	TArray<FIntPoint> OverwatchCells;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crouch")
	bool bIsCrouching = false;
	
	FIntPoint LastGridCell;
	bool bHasLastGridCell = false;
	bool bInitializingDefaultEquipment = false;
	int32 PendingWeaponHoldPoseRetries = 0;
	
	UPROPERTY(Transient)
	AGridManager* GridManager;
	
	UPROPERTY()
	TObjectPtr<UEnemyUnitAI> EnemyAI;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat Camera")
	TObjectPtr<UCameraComponent> ThirdPersonTargetingCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat Camera|Third Person")
	FVector ThirdPersonTargetingCameraRelativeLocation = FVector(-340.0f, 170.0f, 145.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat Camera|Third Person")
	FRotator ThirdPersonTargetingCameraRelativeRotation = FRotator(-10.0f, -14.0f, 0.0f);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> TargetBracketWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Targeting")
	TObjectPtr<UTexture2D> ShootableTargetIconTexture;
	
	UPROPERTY()
	TObjectPtr<UTargetInfoWidget> TargetInfoWidget;

private:
	void RebuildEquippedWeaponInstances();
	TArray<AStrategyUnit*> GetMeleeEnemiesInRange() const;
	UAnimMontage* ResolveWeaponAttackMontage(const FStrategyWeaponInstance& Weapon, EStrategyWeaponAttackType FallbackAttackType, FName& OutMeshComponentName, FName& OutMontageSection) const;
	float PlayResolvedAttackMontage(UAnimMontage* Montage, FName AttackMeshComponentName, FName MontageSection, const TCHAR* LogContext);
	bool TryGetWeaponMuzzleTransform(const UStrategyWeaponData* WeaponData, FTransform& OutMuzzleTransform) const;
	USkeletalMeshComponent* FindWeaponMuzzleEffectMesh(const UStrategyWeaponData* WeaponData) const;
	USkeletalMeshComponent* FindMeleeWeaponAttachMesh() const;
	USkeletalMeshComponent* FindEquippedWeaponAttachMesh(const UStrategyWeaponData* WeaponData) const;
	void UpdateMeleeWeaponVisual();
	void UpdateEquippedWeaponVisual();
	void UpdateEquippedWeaponHoldPose();
	void ScheduleEquippedWeaponHoldPoseUpdate(int32 RetryCount = 0, float DelaySeconds = 0.25f);
	void StopWeaponHoldPose();
	void ConfigureVisualComponentsForTacticalMovement();
	void ApplyTargetingCameraSettings();
	void ScheduleDeathRemoval(float DelaySeconds);
	
	AStrategyGameMode* GetStrategyGameMode() const;	
	bool bDeathRemovalScheduled = false;
	
};
