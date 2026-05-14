#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "StrategyTargetingComponent.generated.h"

class UTargetingHUDWidget;
class UTargetingActionBarWidget;
class AStrategyUnit;

UENUM()
enum class EStrategyTargetingMode : uint8
{
	Fire,
	Melee
};

UCLASS()
class UStrategyTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStrategyTargetingComponent();
	
	bool EnterFireMode(AStrategyUnit* InAttacker, const TArray<AStrategyUnit*>& InTargets);
	bool EnterMeleeMode(AStrategyUnit* InAttacker, const TArray<AStrategyUnit*>& InTargets);
	bool IsInFireMode() const { return bIsInFireMode; }

	UFUNCTION(BlueprintCallable)
	void RequestExitFireMode();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


private:
	bool EnterAttackMode(AStrategyUnit* InAttacker, const TArray<AStrategyUnit*>& InTargets, EStrategyTargetingMode InMode);
	void FocusCurrentTarget();
	void EnterCameraView();
	
	UFUNCTION()
	void CycleToNextTarget();
	
	UFUNCTION()
	void ExitFireMode();

	UFUNCTION()
	void CompleteDelayedExitFireMode();

	UFUNCTION()
	void HandleFireClicked();

	void ExitFireModeAfterDelay(float DelaySeconds);
	
	UTargetingHUDWidget* GetTargetingHUDWidget();

	UPROPERTY()
	TObjectPtr<AStrategyUnit> Attacker;

	UPROPERTY()
	TArray<TObjectPtr<AStrategyUnit>> Targets;
	
	UPROPERTY()
	TObjectPtr<AActor> PreviousViewTarget;

	int32 CurrentTargetIndex = INDEX_NONE;
	EStrategyTargetingMode TargetingMode = EStrategyTargetingMode::Fire;
	bool bIsInFireMode = false;
	bool bIsResolvingAttack = false;
	FTimerHandle ExitFireModeTimerHandle;
};
