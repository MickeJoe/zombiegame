// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Data/Weapon/AttackStats.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"
#include "TargetingCameraMode.h"
#include "StrategyUnit.generated.h"

class UTargetInfoWidget;
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
	int32 GetMaxActionPoints() const;

	UFUNCTION(BlueprintPure, Category = "Unit Stats")
	int32 GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Unit Stats")
	int32 GetMaxArmor() const;

	FAttackStats GetBiteAttackStats() const;
	const FAttackStats* GetMeleeAttackStats() const;
	
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
	float PlayMeleeAttackMontage();
	bool CanReload() const;
	void ReloadWeapon();
	bool CanOverwatch() const;
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

	void UseAtionPoints(int32 ActionPoints);
	int32 GetRemainingActionPoints() const;
	void ResetActionPoints();
	
	float ApplyDamage(const FWeaponDamage& WeaponDamage);
	
	void EquipWeapon(UStrategyWeaponData* WeaponData);
	void ClearEquippedWeapons();
	const FStrategyWeaponInstance& GetEquippedWeapon() const { return GetEquippedFireWeapon(); }
	const FStrategyWeaponInstance& GetEquippedFireWeapon() const;
	const FStrategyWeaponInstance& GetEquippedMeleeWeapon() const;
	
	UTargetInfoWidget* GetTargetInfoWidget() const { return TargetInfoWidget; }
	
	void SetTargetBracketVisible(bool bVisible);
	void SetTargetInfoVisible(bool bVisible);	
	void SetTargetingCameraView(EStrategyTargetingCameraView CameraView);
	void ClearTargetingCameraView();

	FOnUnitMoveCompletedDelegate OnMoveCompleted;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FStrategyWeaponInstance OneHandedFireWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FStrategyWeaponInstance OneHandedMeleeWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FStrategyWeaponInstance TwoHandedWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment|Visuals")
	TSubclassOf<AActor> MeleeWeaponActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment|Visuals")
	FName MeleeWeaponSocketName = TEXT("hand_rSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment|Visuals")
	FName MeleeWeaponMeshComponentName = TEXT("Body");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment|Visuals")
	TObjectPtr<UChildActorComponent> MeleeWeaponActorComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Animation")
	TObjectPtr<UAnimMontage> MeleeAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Animation")
	FName MeleeAttackMontageMeshComponentName = TEXT("Body");

	UPROPERTY(Transient)
	FStrategyWeaponInstance EmptyWeaponInstance;
	
	int32 UsedActionPoints = 0;
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
	
	FIntPoint LastGridCell;
	bool bHasLastGridCell = false;
	
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
	
	UPROPERTY()
	TObjectPtr<UTargetInfoWidget> TargetInfoWidget;

private:
	TArray<AStrategyUnit*> GetEnemiesInRange() const;
	TArray<AStrategyUnit*> GetMeleeEnemiesInRange() const;
	USkeletalMeshComponent* FindMeleeWeaponAttachMesh() const;
	void UpdateMeleeWeaponVisual();
	void ConfigureVisualComponentsForTacticalMovement();
	void ApplyTargetingCameraSettings();
	void ScheduleDeathRemoval(float DelaySeconds);
	
	AStrategyGameMode* GetStrategyGameMode() const;	
	bool bDeathRemovalScheduled = false;
	
};
