// Copyright Epic Games, Inc. All Rights Reserved.

#include "StrategyProjectileVisual.h"

#include "Components/StaticMeshComponent.h"

AStrategyProjectileVisual::AStrategyProjectileVisual()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCanEverAffectNavigation(false);
}

void AStrategyProjectileVisual::InitializeProjectile(
	UStaticMesh* InMesh,
	const FVector& InStartLocation,
	const FVector& InEndLocation,
	float InSpeed,
	float InLifeSeconds,
	const FRotator& InMeshRelativeRotation,
	const FVector& InMeshRelativeScale)
{
	StartLocation = InStartLocation;
	EndLocation = InEndLocation;
	ElapsedTime = 0.0f;
	bInitialized = true;

	const FVector TravelVector = EndLocation - StartLocation;
	const float Distance = TravelVector.Size();
	TravelDuration = Distance > KINDA_SMALL_NUMBER
		? Distance / FMath::Max(InSpeed, 1.0f)
		: 0.05f;

	SetActorLocation(StartLocation);
	SetActorRotation(TravelVector.IsNearlyZero() ? FRotator::ZeroRotator : TravelVector.Rotation());
	SetLifeSpan(FMath::Max(InLifeSeconds, TravelDuration + 0.05f));

	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(InMesh);
		MeshComponent->SetRelativeRotation(InMeshRelativeRotation);
		MeshComponent->SetRelativeScale3D(InMeshRelativeScale);
	}
}

void AStrategyProjectileVisual::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bInitialized)
	{
		return;
	}

	ElapsedTime += DeltaSeconds;
	const float Alpha = FMath::Clamp(ElapsedTime / FMath::Max(TravelDuration, 0.001f), 0.0f, 1.0f);
	SetActorLocation(FMath::Lerp(StartLocation, EndLocation, Alpha));

	if (Alpha >= 1.0f)
	{
		Destroy();
	}
}
