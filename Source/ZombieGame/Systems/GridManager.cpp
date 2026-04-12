#include "GridManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

AGridManager::AGridManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGridManager::BeginPlay()
{
    Super::BeginPlay();

    if (bShowGridInGame)
    {
        DrawGrid(true, -1.f);
    }
}

void AGridManager::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    FlushGridDebugLines();

    if (bShowGridInEditor)
    {
        // Kort lifetime så att gridet ritas om när actor ändras i editorn
        DrawGrid(true, 0.f);
    }
}

#if WITH_EDITOR
void AGridManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FlushGridDebugLines();

    if (bShowGridInEditor)
    {
        DrawGrid(true, 0.f);
    }
}
#endif

FVector AGridManager::GridToWorldCenter(int32 X, int32 Y) const
{
    const FVector Origin = GetActorLocation();

    return Origin + FVector(
        X * CellSize + CellSize * 0.5f,
        Y * CellSize + CellSize * 0.5f,
        GridZOffset);
}

void AGridManager::DrawGrid(bool bPersistent, float LifeTime) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector Origin = GetActorLocation() + FVector(0.f, 0.f, GridZOffset);
/*
    FVector Origin;
    FVector Extent;

    BoundsActor->GetActorBounds(false, Origin, Extent);

    FVector WorldMin = Origin - Extent;
    FVector WorldMax = Origin + Extent;
*/
    // Vertikala linjer
    for (int32 X = 0; X <= GridWidth; ++X)
    {
        const FVector Start = Origin + FVector(X * CellSize, 0.f, 0.f);
        const FVector End   = Origin + FVector(X * CellSize, GridHeight * CellSize, 0.f);

        DrawDebugLine(
            World,
            Start,
            End,
            GridColor,
            bPersistent,
            LifeTime,
            0,
            GridLineThickness);
    }

    // Horisontella linjer
    for (int32 Y = 0; Y <= GridHeight; ++Y)
    {
        const FVector Start = Origin + FVector(0.f, Y * CellSize, 0.f);
        const FVector End   = Origin + FVector(GridWidth * CellSize, Y * CellSize, 0.f);

        DrawDebugLine(
            World,
            Start,
            End,
            GridColor,
            bPersistent,
            LifeTime,
            0,
            GridLineThickness);
    }
}

void AGridManager::FlushGridDebugLines() const
{
    if (UWorld* World = GetWorld())
    {
        FlushPersistentDebugLines(World);
    }
}
FVector AGridManager::GridToWorld(const FIntPoint& Coord) const
{
    return GetActorLocation() + FVector(
        Coord.X * CellSize + CellSize * 0.5f,
        Coord.Y * CellSize + CellSize * 0.5f,
        0.f);
}

FIntPoint AGridManager::WorldToGrid(const FVector& WorldLocation) const
{
    const FVector Local = WorldLocation - GetActorLocation();

    return FIntPoint(
        FMath::FloorToInt(Local.X / CellSize),
        FMath::FloorToInt(Local.Y / CellSize));
}

bool AGridManager::ProjectCellToGround(const FIntPoint& Cell, FVector& OutLocation, FVector& OutNormal) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const FVector CellCenter = GridToWorld(Cell);

    // Starta en bit ovanför och traca nedåt
    const FVector TraceStart = CellCenter + FVector(0.f, 0.f, 2000.f);
    const FVector TraceEnd   = CellCenter - FVector(0.f, 0.f, 2000.f);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectCellToGround), false);
    Params.AddIgnoredActor(this);

    const bool bHit = World->LineTraceSingleByChannel(
        Hit,
        TraceStart,
        TraceEnd,
        ECC_Visibility,
        Params
    );

    if (!bHit)
    {
        return false;
    }

    OutLocation = Hit.ImpactPoint;
    OutNormal = Hit.ImpactNormal;
    return true;
}

void AGridManager::ClearReachableCells()
{
    for (AActor* Highlight : SpawnedHighlights)
    {
        if (Highlight)
        {
            Highlight->Destroy();
        }
    }

    SpawnedHighlights.Empty();
}

void AGridManager::ShowReachableCells(const TArray<FIntPoint>& Cells)
{
    /*
    ClearReachableCells();

    if (!ReachableHighlightClass)
    {
        return;
    }

    for (const FIntPoint& Cell : Cells)
    {
        FVector WorldPos = GridToWorld(Cell);
        WorldPos.Z += 2.0f; // slightly above ground

        AActor* Highlight = GetWorld()->SpawnActor<AActor>(
            ReachableHighlightClass,
            WorldPos,
            FRotator(-90.f, 0.f, 0.f) // depends on mesh orientation
        );

        if (Highlight)
        {
            SpawnedHighlights.Add(Highlight);
        }
    }
    */
}