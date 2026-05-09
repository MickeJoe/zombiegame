// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Data/Weapon/AttackStats.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"
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
class USphereComponent;
class UUnitData;

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strategy")
	TObjectPtr<AStrategySide> OwningSide = nullptr;

	void SetStrategyUnitTeam(EStrategyUnitTeam InStrategyUnitTeam);
	EStrategyUnitTeam GetStrategyUnitTeam() const;
	
	UPROPERTY(BlueprintAssignable)
	FOnGridCellChanged OnGridCellChanged;
	
	TObjectPtr<UEnemyUnitAI> GetEnemyAI() const { return EnemyAI; }
	
	void UpdateStatusBar();
	
	bool CanWeaponAttack(AAIStrategySide* EnemySide) const;
	void StartWeaponAttackMode();

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
	
	void ApplyDamage(const FWeaponDamage& WeaponDamage);
	
	void EquipWeapon(UStrategyWeaponData* WeaponData);
	const FStrategyWeaponInstance& GetEquippedWeapon() const { return EquippedWeapon; }
	
	UTargetInfoWidget* GetTargetInfoWidget() const { return TargetInfoWidget; }
	
	void SetTargetBracketVisible(bool bVisible);
	void SetTargetInfoVisible(bool bVisible);	

	FOnUnitMoveCompletedDelegate OnMoveCompleted;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FStrategyWeaponInstance EquippedWeapon;
	
	int32 UsedActionPoints = 0;
	int32 CurrentHealth = 0;
	int32 CurrentArmor = 0;
	
	FIntPoint LastGridCell;
	bool bHasLastGridCell = false;
	
	UPROPERTY(Transient)
	AGridManager* GridManager;
	
	UPROPERTY()
	TObjectPtr<UEnemyUnitAI> EnemyAI;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> TargetBracketWidget;
	
	UPROPERTY()
	TObjectPtr<UTargetInfoWidget> TargetInfoWidget;

private:
	TArray<AStrategyUnit*> GetEnemiesInRange() const;
	
	AStrategyGameMode* GetStrategyGameMode() const;	
	
};
