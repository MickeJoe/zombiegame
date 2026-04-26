#include "FogOfWarActor.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "DrawDebugHelpers.h" // tmp

#include "../../Systems/GridManager.h"

AFogOfWarActor::AFogOfWarActor()
{
	PrimaryActorTick.bCanEverTick = false;

	FogMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FogMesh"));
	RootComponent = FogMesh;

	FogMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FogMesh->SetCanEverAffectNavigation(false);
}

void AFogOfWarActor::RefreshFog(
	AGridManager* GridManager,
	const TSet<FIntPoint>& VisibleCells,
	const TSet<FIntPoint>& ExploredCells
)
{
	if (!GridManager || !FogTileMesh)
	{
		return;
	}

	FogMesh->ClearInstances();
	FogMesh->SetStaticMesh(FogTileMesh);

	// Första versionen: använd en material-slot.
	// Börja med UnexploredMaterial. Sen kan vi dela upp i två mesh components.
	if (UnexploredMaterial)
	{
		FogMesh->SetMaterial(0, UnexploredMaterial);
	}

	for (int32 Y = 0; Y < GridManager->GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridManager->GridWidth; ++X)
		{
			const FIntPoint Cell(X, Y);

			if (VisibleCells.Contains(Cell))
			{
				continue;
			}

			FVector GroundLocation;
			FVector GroundNormal;

			if (!GridManager->ProjectCellToGround(Cell, GroundLocation, GroundNormal))
			{
				continue;
			}

			FTransform Transform = FTransform::Identity;
			Transform.SetLocation(GroundLocation + FVector(0.f, 0.f, FogZOffset));

			const float Scale = GridManager->CellSize / 100.f;
			Transform.SetScale3D(FVector(Scale, Scale, 1.f));

			// tmp
			/*
			DrawDebugSphere(
				GetWorld(),
				GroundLocation + FVector(0.f, 0.f, FogZOffset),
				25.f,
				12,
				FColor::Red,
				true,
				10.f
			);
			*/

			FogMesh->AddInstance(Transform, true);
		}
	}
}