#include "GridBoundsActor.h"

#include "Components/BoxComponent.h"

AGridBoundsActor::AGridBoundsActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	RootComponent = Bounds;
	
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetCanEverAffectNavigation(false);

	Bounds->SetBoxExtent(FVector(1000.f, 1000.f, 100.f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetHiddenInGame(true);
}

FVector AGridBoundsActor::GetGridCenter() const
{
	return Bounds->GetComponentLocation();
}

FVector AGridBoundsActor::GetGridExtents() const
{
	return Bounds->GetScaledBoxExtent();
}

FVector AGridBoundsActor::GetGridOrigin() const
{
	const FVector Center = GetGridCenter();
	const FVector Extents = GetGridExtents();

	return FVector(
		Center.X - Extents.X,
		Center.Y - Extents.Y,
		Center.Z
	);
}

int32 AGridBoundsActor::GetGridWidth(float CellSize) const
{
	if (CellSize <= 0.f)
	{
		return 0;
	}

	const FVector Extents = GetGridExtents();
	return FMath::FloorToInt((Extents.X * 2.f) / CellSize);
}

int32 AGridBoundsActor::GetGridHeight(float CellSize) const
{
	if (CellSize <= 0.f)
	{
		return 0;
	}

	const FVector Extents = GetGridExtents();
	return FMath::FloorToInt((Extents.Y * 2.f) / CellSize);
}