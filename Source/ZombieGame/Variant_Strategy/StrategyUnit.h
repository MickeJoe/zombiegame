// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "StrategyUnit.generated.h"

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

	UFUNCTION(BlueprintPure, Category = "Movement")
	int32 GetMaxMovement() const { return MaxMovement; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strategy")
	TObjectPtr<AStrategySide> OwningSide = nullptr;

	void SetStrategyUnitTeam(EStrategyUnitTeam InStrategyUnitTeam);
	EStrategyUnitTeam GetStrategyUnitTeam() const;
	
	UPROPERTY(BlueprintAssignable)
	FOnGridCellChanged OnGridCellChanged;
	
	TObjectPtr<UEnemyUnitAI> GetEnemyAI() const { return EnemyAI; }

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

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy")
	int32 MaxActionPoints = 2;

	void UseAtionPoints(int32 ActionPoints);
	int32 GetRemainingActionPoints() const;
	void ResetActionPoints();

	FOnUnitMoveCompletedDelegate OnMoveCompleted;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Stats")
	int32 MaxMovement = 8;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Stats")
	int32 SightRange = 12;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Stats")
	int32 MaxHealth = 50;
	
	int32 UsedActionPoints = 0;
	int32 CurrentHealth = MaxHealth;
	
	FIntPoint LastGridCell;
	bool bHasLastGridCell = false;
	
	UPROPERTY(Transient)
	AGridManager* GridManager;
	
	UPROPERTY(EditDefaultsOnly, Category="AI")
	TSubclassOf<UEnemyUnitAI> EnemyAIClass;

	UPROPERTY()
	TObjectPtr<UEnemyUnitAI> EnemyAI;
};
