// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Systems/AttackHandling/AttackStats.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"
#include "StrategyUnit.generated.h"

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

UENUM(BlueprintType)
enum class EStrategyUnitTeam : uint8
{
	Human,
	AI
};

struct FWeaponDamage
{
	int32 Damage = 0;
	int32 ArmorPierce = 0; // ignorerar armor
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
	
	int32 GetSightRange() const { return SightRange; }

	UFUNCTION(BlueprintPure, Category = "Unit Stats")
	int32 GetMaxMovement() const { return MaxMovement; }
	
	UFUNCTION(BlueprintPure, Category = "Unit Stats")
	int32 GetCurrentHealth() const { return CurrentHealth; }

	FAttackStats GetBiteAttackStats() const { return BiteAttack; }
	
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

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> StatusBarWidgetClass;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy")
	int32 MaxActionPoints = 2;

	void UseAtionPoints(int32 ActionPoints);
	int32 GetRemainingActionPoints() const;
	void ResetActionPoints();
	
	void ApplyDamage(const FWeaponDamage& WeaponDamage);
	
	void EquipWeapon(UStrategyWeaponData* WeaponData);
	
	void SetTargetBracketVisible(bool bVisible);

	FOnUnitMoveCompletedDelegate OnMoveCompleted;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Stats")
	int32 MaxMovement = 8;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Stats")
	int32 SightRange = 28;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Stats")
	int32 MaxHealth = 8;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Stats")
	int32 MaxArmor = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	FAttackStats BiteAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	FAttackStats HandAttack;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FStrategyWeaponInstance EquippedWeapon;
	
	int32 UsedActionPoints = 0;
	int32 CurrentHealth = MaxHealth;
	int32 CurrentArmor = MaxArmor;
	
	FIntPoint LastGridCell;
	bool bHasLastGridCell = false;
	
	UPROPERTY(Transient)
	AGridManager* GridManager;
	
	UPROPERTY(EditDefaultsOnly, Category="AI")
	TSubclassOf<UEnemyUnitAI> EnemyAIClass;

	UPROPERTY()
	TObjectPtr<UEnemyUnitAI> EnemyAI;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> TargetBracketWidget;	

private:
	TArray<AStrategyUnit*> GetEnemiesInRange() const;
	
	AStrategyGameMode* GetStrategyGameMode() const;	
	
};
