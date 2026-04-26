// GridHighlightActor.cpp

#include "GridHighlightActor.h"
#include "Components/DecalComponent.h"
#include "GridManager.h"
#include "Engine/World.h"

AGridHighlightActor::AGridHighlightActor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

UDecalComponent* AGridHighlightActor::GetOrCreateDecal(int32 Index)
{
	while (DecalPool.Num() <= Index)
	{
		const int32 NewIndex = DecalPool.Num();

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

		if (ReachableDecalMaterial)
		{
			Decal->SetDecalMaterial(ReachableDecalMaterial);
		}

		DecalPool.Add(Decal);
	}

	return DecalPool[Index];
}

void AGridHighlightActor::ClearReachableHighlights()
{
	for (UDecalComponent* Decal : DecalPool)
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
	ClearReachableHighlights();

	if (!GridManager || !ReachableDecalMaterial)
	{
		return;
	}

	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		UDecalComponent* Decal = GetOrCreateDecal(i);
		if (!Decal)
		{
			continue;
		}

		FVector GroundLocation;
		FVector GroundNormal;
		if (!GridManager->ProjectCellToGround(Cells[i], GroundLocation, GroundNormal))
		{
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
		
		Decal->SetDecalMaterial(ReachableDecalMaterial);
		Decal->SetHiddenInGame(false);
		Decal->SetVisibility(true);
	}
}