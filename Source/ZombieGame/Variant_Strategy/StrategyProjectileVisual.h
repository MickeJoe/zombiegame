// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyProjectileVisual.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class ZOMBIEGAME_API AStrategyProjectileVisual : public AActor
{
	GENERATED_BODY()

public:
	AStrategyProjectileVisual();

	virtual void Tick(float DeltaSeconds) override;

	void InitializeProjectile(
		UStaticMesh* InMesh,
		const FVector& InStartLocation,
		const FVector& InEndLocation,
		float InSpeed,
		float InLifeSeconds,
		const FRotator& InMeshRelativeRotation,
		const FVector& InMeshRelativeScale);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	float TravelDuration = 0.1f;
	float ElapsedTime = 0.0f;
	bool bInitialized = false;
};
