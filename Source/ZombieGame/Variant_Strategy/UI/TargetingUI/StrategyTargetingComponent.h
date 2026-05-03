#pragma once

#include "StrategyTargetingComponent.generated.h"

class UTargetingHUDWidget;
class UTargetingActionBarWidget;
class AStrategyUnit;

UCLASS()
class UStrategyTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void EnterFireMode(AStrategyUnit* InAttacker, const TArray<AStrategyUnit*>& InTargets);
	bool IsInFireMode() const { return bIsInFireMode; }

private:
	void FocusCurrentTarget();
	void EnterCameraView();
	
	UFUNCTION()
	void CycleToNextTarget();
	
	UFUNCTION()
	void ExitFireMode();
	
	UTargetingHUDWidget* GetTargetingHUDWidget();

	UPROPERTY()
	TObjectPtr<AStrategyUnit> Attacker;

	UPROPERTY()
	TArray<TObjectPtr<AStrategyUnit>> Targets;
	
	UPROPERTY()
	TObjectPtr<AActor> PreviousViewTarget;

	int32 CurrentTargetIndex = INDEX_NONE;
	bool bIsInFireMode = false;
};