// GridHighlightActor.cpp

#include "GridHighlightActor.h"
#include "Components/DecalComponent.h"
#include "Components/LineBatchComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GridManager.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "ZombieGame.h"

namespace
{
	static TAutoConsoleVariable<int32> CVarOverwatchDebug(
		TEXT("zg.OverwatchDebug"),
		1,
		TEXT("Logs overwatch placement and decal projection diagnostics."));

	bool IsOverwatchDebugEnabled()
	{
		return CVarOverwatchDebug.GetValueOnGameThread() != 0;
	}
}

AGridHighlightActor::AGridHighlightActor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshFinder.Succeeded())
	{
		CoverIconMesh = PlaneMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMaterialFinder(TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (FallbackMaterialFinder.Succeeded())
	{
		CoverFallbackMaterial = FallbackMaterialFinder.Object;
	}

	OverwatchBoundaryLineBatch = CreateDefaultSubobject<ULineBatchComponent>(TEXT("OverwatchBoundaryLines"));
	OverwatchBoundaryLineBatch->SetupAttachment(RootComponent);

	OverwatchPreviewBoundaryLineBatch = CreateDefaultSubobject<ULineBatchComponent>(TEXT("OverwatchPreviewBoundaryLines"));
	OverwatchPreviewBoundaryLineBatch->SetupAttachment(RootComponent);

	MovementPathLineBatch = CreateDefaultSubobject<ULineBatchComponent>(TEXT("MovementPathLines"));
	MovementPathLineBatch->SetupAttachment(RootComponent);
}

UDecalComponent* AGridHighlightActor::GetOrCreateDecal(
	TArray<TObjectPtr<UDecalComponent>>& Pool,
	int32 Index,
	UMaterialInterface* Material)
{
	while (Pool.Num() <= Index)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(this);
		if (!Decal)
		{
			return nullptr;
		}

		Decal->SetupAttachment(RootComponent);
		Decal->RegisterComponent();
		Decal->SetVisibility(false);
		Decal->SetHiddenInGame(true);
		Decal->DecalSize = DecalSize;

		if (Material)
		{
			Decal->SetDecalMaterial(Material);
		}

		Pool.Add(Decal);
	}

	return Pool[Index];
}

UStaticMeshComponent* AGridHighlightActor::GetOrCreateCoverIndicator(int32 Index)
{
	while (CoverIndicatorPool.Num() <= Index)
	{
		UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this);
		if (!MeshComponent)
		{
			return nullptr;
		}

		MeshComponent->SetupAttachment(RootComponent);
		MeshComponent->RegisterComponent();
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetCastShadow(false);
		MeshComponent->SetVisibility(false);
		MeshComponent->SetHiddenInGame(true);

		if (CoverIconMesh)
		{
			MeshComponent->SetStaticMesh(CoverIconMesh);
		}

		CoverIndicatorPool.Add(MeshComponent);
	}

	return CoverIndicatorPool[Index];
}

void AGridHighlightActor::ClearReachableHighlights()
{
	ClearHighlights(DecalPool);
	ClearMovementPath();
	ClearCoverIndicators();
}

void AGridHighlightActor::ClearOverwatchHighlights()
{
	ClearHighlights(OverwatchDecalPool);
	ClearOverwatchBoundaryLines();
}

void AGridHighlightActor::ClearOverwatchPreviewHighlights()
{
	ClearHighlights(OverwatchPreviewDecalPool);
	ClearOverwatchPreviewBoundaryLines();
}

void AGridHighlightActor::ClearHighlights(TArray<TObjectPtr<UDecalComponent>>& Pool)
{
	for (UDecalComponent* Decal : Pool)
	{
		if (!Decal)
		{
			continue;
		}

		Decal->SetVisibility(false);
		Decal->SetHiddenInGame(true);
	}
}

void AGridHighlightActor::ShowReachableCells(AGridManager* GridManager, const TArray<FIntPoint>& Cells)
{
	ShowCells(GridManager, Cells, DecalPool, ReachableDecalMaterial, false);
}

void AGridHighlightActor::ShowOverwatchCells(AGridManager* GridManager, const TArray<FIntPoint>& Cells)
{
	if (IsOverwatchDebugEnabled())
	{
		UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: ShowOverwatchCells Actor=%s Grid=%s Material=%s Cells=%d Pool=%d"),
			*GetNameSafe(this),
			*GetNameSafe(GridManager),
			*GetNameSafe(OverwatchDecalMaterial),
			Cells.Num(),
			OverwatchDecalPool.Num());
	}

	ShowCells(GridManager, Cells, OverwatchDecalPool, OverwatchDecalMaterial, true);
}

void AGridHighlightActor::ShowOverwatchPreviewCells(AGridManager* GridManager, const TArray<FIntPoint>& Cells)
{
	if (IsOverwatchDebugEnabled())
	{
		UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: ShowOverwatchPreviewCells Actor=%s Grid=%s Material=%s Cells=%d Pool=%d"),
			*GetNameSafe(this),
			*GetNameSafe(GridManager),
			*GetNameSafe(OverwatchDecalMaterial),
			Cells.Num(),
			OverwatchPreviewDecalPool.Num());
	}

	ShowCells(GridManager, Cells, OverwatchPreviewDecalPool, OverwatchDecalMaterial, true);
}

void AGridHighlightActor::ShowOverwatchBoundaryLines(const TArray<FOverwatchBoundaryLine>& BoundaryLines)
{
	ClearOverwatchBoundaryLines();

	for (const FOverwatchBoundaryLine& BoundaryLine : BoundaryLines)
	{
		DrawBoundaryLine(OverwatchBoundaryLineBatch, BoundaryLine);
	}
}

void AGridHighlightActor::ClearOverwatchBoundaryLines()
{
	ClearBoundaryLines(OverwatchBoundaryLineBatch);
}

void AGridHighlightActor::ShowOverwatchPreviewBoundaryLine(const FOverwatchBoundaryLine& BoundaryLine)
{
	ClearOverwatchPreviewBoundaryLines();
	DrawBoundaryLine(OverwatchPreviewBoundaryLineBatch, BoundaryLine);
}

void AGridHighlightActor::ClearOverwatchPreviewBoundaryLines()
{
	ClearBoundaryLines(OverwatchPreviewBoundaryLineBatch);
}

void AGridHighlightActor::ShowMovementPath(const TArray<FVector>& PathPoints)
{
	ClearMovementPath();

	if (!MovementPathLineBatch || PathPoints.Num() < 2)
	{
		return;
	}

	for (int32 i = 1; i < PathPoints.Num(); ++i)
	{
		const FVector Start = PathPoints[i - 1] + FVector(0.0f, 0.0f, MovementPathLineHeightOffset);
		const FVector End = PathPoints[i] + FVector(0.0f, 0.0f, MovementPathLineHeightOffset);

		MovementPathLineBatch->DrawLine(
			Start,
			End,
			MovementPathLineColor,
			0,
			MovementPathLineThickness);
	}
}

void AGridHighlightActor::ClearMovementPath()
{
	ClearBoundaryLines(MovementPathLineBatch);
}

void AGridHighlightActor::ShowCoverIndicators(const TArray<FGridCoverIndicator>& Indicators)
{
	ClearCoverIndicators();

	for (int32 i = 0; i < Indicators.Num(); ++i)
	{
		UStaticMeshComponent* MeshComponent = GetOrCreateCoverIndicator(i);
		if (!MeshComponent)
		{
			continue;
		}

		UMaterialInterface* Material = Indicators[i].CoverType == EGridCoverType::Full
			? FullCoverMaterial.Get()
			: HalfCoverMaterial.Get();
		if (!Material)
		{
			Material = CoverFallbackMaterial.Get();
		}

		if (!Material || !CoverIconMesh)
		{
			continue;
		}

		FVector CoverDirection(Indicators[i].Direction.X, Indicators[i].Direction.Y, 0.0f);
		if (!CoverDirection.Normalize())
		{
			CoverDirection = FVector::ForwardVector;
		}

		const FVector VisibleNormal = -CoverDirection;
		const FVector CoverTangent = FVector::CrossProduct(FVector::UpVector, VisibleNormal).GetSafeNormal();
		const FRotator CoverRotation = FRotationMatrix::MakeFromXZ(CoverTangent, VisibleNormal).Rotator();
		const float PlaneMeshSize = 100.0f;
		const float CoverScale = CoverIconWorldSize / PlaneMeshSize;

		MeshComponent->SetStaticMesh(CoverIconMesh);
		MeshComponent->SetMaterial(0, Material);
		MeshComponent->SetWorldLocation(
			Indicators[i].Location
			+ VisibleNormal * CoverIconWallOffset
			+ FVector(0.0f, 0.0f, CoverIconHeightOffset));
		MeshComponent->SetWorldRotation(CoverRotation);
		MeshComponent->SetWorldScale3D(FVector(CoverScale));
		MeshComponent->SetHiddenInGame(false);
		MeshComponent->SetVisibility(true);
	}
}

void AGridHighlightActor::ClearCoverIndicators()
{
	for (UStaticMeshComponent* MeshComponent : CoverIndicatorPool)
	{
		if (!MeshComponent)
		{
			continue;
		}

		MeshComponent->SetVisibility(false);
		MeshComponent->SetHiddenInGame(true);
	}
}

void AGridHighlightActor::ShowCells(
	AGridManager* GridManager,
	const TArray<FIntPoint>& Cells,
	TArray<TObjectPtr<UDecalComponent>>& Pool,
	UMaterialInterface* Material,
	bool bRequireWalkableGround)
{
	ClearHighlights(Pool);

	if (!GridManager || !Material)
	{
		if (IsOverwatchDebugEnabled())
		{
			UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: ShowCells aborted Grid=%s Material=%s Cells=%d"),
				*GetNameSafe(GridManager),
				*GetNameSafe(Material),
				Cells.Num());
		}
		return;
	}

	int32 VisibleDecals = 0;
	int32 ProjectionFailures = 0;
	int32 RejectedSurfaceNormals = 0;

	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		UDecalComponent* Decal = GetOrCreateDecal(Pool, i, Material);
		if (!Decal)
		{
			continue;
		}

		FVector GroundLocation;
		FVector GroundNormal;
		if (bRequireWalkableGround)
		{
			if (!GridManager->TryGetNavigationLocationForCell(Cells[i], GroundLocation))
			{
				++ProjectionFailures;
				continue;
			}

			GroundNormal = FVector::UpVector;
		}
		else if (!GridManager->ProjectCellToGround(Cells[i], GroundLocation, GroundNormal))
		{
			++ProjectionFailures;
			continue;
		}

		if (bRequireWalkableGround && GroundNormal.Z < OverwatchMinGroundNormalZ)
		{
			++RejectedSurfaceNormals;
			continue;
		}

		// Flytta ut decalen lite från marken så den inte z-fightas
		const FVector FinalLocation = GroundLocation + GroundNormal * SurfaceOffset;

		// Decals projicerar längs sin lokala X-axel, så vi låter X peka "ned" mot ytan
		const FRotator DecalRotation = FRotationMatrix::MakeFromX(-GroundNormal).Rotator();
		
		DrawDebugBox(
			GetWorld(),
	GroundLocation,
	FVector(GridManager->CellSize * 0.5f),
	FColor::Green,
	false,
	5.f
);

		Decal->SetWorldLocation(FinalLocation);
		Decal->SetWorldRotation(DecalRotation);
//		Decal->DecalSize = DecalSize;
		const float HalfSize = GridManager->CellSize * 0.5f;

		Decal->DecalSize = FVector(
			GridManager->CellSize,
			HalfSize,
			HalfSize
		);
		
		Decal->SetDecalMaterial(Material);
		Decal->SetHiddenInGame(false);
		Decal->SetVisibility(true);
		++VisibleDecals;
	}

	if (IsOverwatchDebugEnabled() && Material == OverwatchDecalMaterial)
	{
		UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: ShowCells finished Cells=%d VisibleDecals=%d ProjectionFailures=%d RejectedNormals=%d Material=%s"),
			Cells.Num(),
			VisibleDecals,
			ProjectionFailures,
			RejectedSurfaceNormals,
			*GetNameSafe(Material));
	}
}

void AGridHighlightActor::DrawBoundaryLine(ULineBatchComponent* LineBatch, const FOverwatchBoundaryLine& BoundaryLine)
{
	if (!LineBatch)
	{
		return;
	}

	const FVector Origin = BoundaryLine.Origin + FVector(0.0f, 0.0f, OverwatchEdgeLineHeightOffset);
	const FVector LeftEnd = BoundaryLine.LeftEnd + FVector(0.0f, 0.0f, OverwatchEdgeLineHeightOffset);
	const FVector RightEnd = BoundaryLine.RightEnd + FVector(0.0f, 0.0f, OverwatchEdgeLineHeightOffset);

	LineBatch->DrawLine(
		Origin,
		LeftEnd,
		OverwatchEdgeLineColor,
		0,
		OverwatchEdgeLineThickness);

	LineBatch->DrawLine(
		Origin,
		RightEnd,
		OverwatchEdgeLineColor,
		0,
		OverwatchEdgeLineThickness);
}

void AGridHighlightActor::ClearBoundaryLines(ULineBatchComponent* LineBatch)
{
	if (!LineBatch)
	{
		return;
	}

	LineBatch->BatchedLines.Reset();
	LineBatch->MarkRenderStateDirty();
}
