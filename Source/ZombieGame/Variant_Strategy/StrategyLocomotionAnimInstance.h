// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "StrategyLocomotionAnimInstance.generated.h"

/**
 * Anim instance for strategy units whose visible mesh may not report pawn velocity.
 * Computes speed from the owning skeletal mesh component's world-space movement.
 */
UCLASS()
class ZOMBIEGAME_API UStrategyLocomotionAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bUsePreviewSpeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ClampMin = "0.0"))
	float PreviewSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Speed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion", meta = (ClampMin = "0.0"))
	float StopSpeedThreshold = 3.0f;

private:
	FVector PreviousLocation = FVector::ZeroVector;
	bool bHasPreviousLocation = false;
};
