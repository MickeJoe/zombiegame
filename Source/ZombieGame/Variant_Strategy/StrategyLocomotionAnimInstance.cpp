// Copyright Epic Games, Inc. All Rights Reserved.

#include "StrategyLocomotionAnimInstance.h"

#include "Components/SkeletalMeshComponent.h"

void UStrategyLocomotionAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (!MeshComponent)
	{
		Speed = 0.0f;
		bHasPreviousLocation = false;
		return;
	}

	PreviousLocation = MeshComponent->GetComponentLocation();
	bHasPreviousLocation = true;
}

void UStrategyLocomotionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (bUsePreviewSpeed)
	{
		Speed = PreviewSpeed;
		return;
	}

	if (DeltaSeconds <= UE_SMALL_NUMBER)
	{
		Speed = 0.0f;
		return;
	}

	const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (!MeshComponent)
	{
		Speed = 0.0f;
		bHasPreviousLocation = false;
		return;
	}

	const FVector CurrentLocation = MeshComponent->GetComponentLocation();

	if (!bHasPreviousLocation)
	{
		PreviousLocation = CurrentLocation;
		bHasPreviousLocation = true;
		Speed = 0.0f;
		return;
	}

	const FVector DeltaLocation = CurrentLocation - PreviousLocation;
	Speed = FVector(DeltaLocation.X, DeltaLocation.Y, 0.0f).Size() / DeltaSeconds;

	if (Speed < StopSpeedThreshold)
	{
		Speed = 0.0f;
	}

	PreviousLocation = CurrentLocation;
}
