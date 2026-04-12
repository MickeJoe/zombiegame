#include "GridManager.h"
#include "StrategyUnit.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

// Nav
#include "NavigationSystem.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "AI/NavigationSystemBase.h"

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

bool AGridManager::IsCellOnNavMesh(
    const FIntPoint& Cell,
    FNavLocation* OutLocation
) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!NavSys)
    {
        return false;
    }

    const FVector WorldPos = GridToWorld(Cell);
    const FVector QueryExtent(50.f, 50.f, 200.f);

    FNavLocation Projected;

    if (!NavSys->ProjectPointToNavigation(WorldPos, Projected, QueryExtent))
    {
        return false;
    }

    if (OutLocation)
    {
        *OutLocation = Projected;
    }

    return true;
}

bool AGridManager::TryGetNavigationDataForCell(
    const FIntPoint& Cell,
    UNavigationSystemV1*& OutNavSys,
    const ANavigationData*& OutNavData,
    FNavLocation& OutProjectedEnd
) const
{
    OutNavSys = nullptr;
    OutNavData = nullptr;

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!NavSys)
    {
        return false;
    }

    if (!IsCellOnNavMesh(Cell, &OutProjectedEnd))
    {
        return false;
    }

    const ANavigationData* NavData = NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
    if (!NavData)
    {
        return false;
    }

    OutNavSys = NavSys;
    OutNavData = NavData;
    return true;
}
/*
bool AGridManager::IsCellReachableFromUnit(
    const AStrategyUnit* Unit,
    const FIntPoint& Cell
) const
{
    if (!Unit)
    {
        return false;
    }

    UNavigationSystemV1* NavSys = nullptr;
    const ANavigationData* NavData = nullptr;
    FNavLocation ProjectedEnd;

    if (!TryGetNavigationDataForCell(Cell, NavSys, NavData, ProjectedEnd))
    {
        return false;
    }

    const FVector Start = Unit->GetActorLocation();
    FPathFindingQuery Query(Unit, *NavData, Start, ProjectedEnd.Location);

    return NavSys->TestPathSync(Query, EPathFindingMode::Regular);
}
*/
/*
bool AGridManager::IsCellWithinMoveRange(
    const AStrategyUnit* Unit,
    const FIntPoint& Cell,
    int32 MaxMoveCells
) const
{
    if (!Unit)
    {
        return false;
    }

    UNavigationSystemV1* NavSys = nullptr;
    const ANavigationData* NavData = nullptr;
    FNavLocation ProjectedEnd;

    if (!TryGetNavigationDataForCell(Cell, NavSys, NavData, ProjectedEnd))
    {
        return false;
    }

    const FVector Start = Unit->GetActorLocation();

    FVector::FReal PathLength = 0.0;
    const ENavigationQueryResult::Type Result = NavSys->GetPathLength(
        Start,
        ProjectedEnd.Location,
        PathLength,
        NavData
    );

    if (Result != ENavigationQueryResult::Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MoveRange] Path failed for Cell (%d,%d)"), Cell.X, Cell.Y);
        return false;
    }

    const float MaxMoveDistanceWorld = static_cast<float>(MaxMoveCells) * CellSize;
    const bool bWithinRange = PathLength <= MaxMoveDistanceWorld;

    UE_LOG(LogTemp, Warning,
        TEXT("[MoveRange] UnitPos=%s Cell=(%d,%d) CellWorld=%s PathLength=%.1f MaxWorld=%.1f Result=%s"),
        *Start.ToString(),
        Cell.X, Cell.Y,
        *ProjectedEnd.Location.ToString(),
        PathLength,
        MaxMoveDistanceWorld,
        bWithinRange ? TEXT("YES") : TEXT("NO")
    );

    return bWithinRange;
}
*/
bool AGridManager::IsCellWithinMoveRange(
    const AStrategyUnit* Unit,
    const FIntPoint& Cell,
    int32 MaxMoveCells
) const
{
    if (!Unit)
    {
        return false;
    }

    UNavigationSystemV1* NavSys = nullptr;
    const ANavigationData* NavData = nullptr;
    FNavLocation ProjectedEnd;

    if (!TryGetNavigationDataForCell(Cell, NavSys, NavData, ProjectedEnd))
    {
        return false;
    }

    const FVector Start = Unit->GetActorLocation();

    // 🔴 1. Först: finns det en riktig path?
    FPathFindingQuery Query(Unit, *NavData, Start, ProjectedEnd.Location);

    if (!NavSys->TestPathSync(Query))
    {
        return false; // ← STOPPA här
    }

    // 🔵 2. Sen: kolla längd
    FVector::FReal PathLength = 0.0;
    const auto Result = NavSys->GetPathLength(
        Start,
        ProjectedEnd.Location,
        PathLength,
        NavData
    );

    if (Result != ENavigationQueryResult::Success)
    {
        return false;
    }

    const float MaxMoveDistanceWorld = MaxMoveCells * CellSize;

    return PathLength <= MaxMoveDistanceWorld;
}