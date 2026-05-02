#include "SightManager.h"

#include "GridManager.h"
#include "StrategyUnit.h"
#include "../Variant_Strategy/UI/FogOfWarActor.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ASightManager::ASightManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASightManager::BeginPlay()
{
	Super::BeginPlay();
	
	FogOfWarActor = Cast<AFogOfWarActor>(
		UGameplayStatics::GetActorOfClass(this, AFogOfWarActor::StaticClass())
	);

	FindGridManager();
}

void ASightManager::RegisterUnit(AStrategyUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	Unit->OnGridCellChanged.AddDynamic(this, &ASightManager::HandleUnitGridCellChanged);
}

void ASightManager::HandleUnitGridCellChanged(AStrategyUnit* Unit)
{
	UpdateSightAndFog();
}

void ASightManager::SetUnits(
	const TArray<AStrategyUnit*>& InPlayerUnits,
	const TArray<AStrategyUnit*>& InEnemyUnits)
{
	PlayerUnits = InPlayerUnits;
	EnemyUnits = InEnemyUnits;

	for (AStrategyUnit* Unit : PlayerUnits)
	{
		RegisterUnit(Unit);
	}

	UpdateSightAndFog();
}

void ASightManager::UpdateSightAndFog()
{
	UpdatePlayerSight();
	UpdateEnemyVisibility();
	RefreshFog();
}

void ASightManager::FindGridManager()
{
	GridManager = Cast<AGridManager>(
		UGameplayStatics::GetActorOfClass(this, AGridManager::StaticClass())
	);
}

void ASightManager::UpdatePlayerSight()
{
	UpdateSightForUnits(PlayerUnits, VisibleCells, ExploredCells);
}

void ASightManager::UpdateEnemySight()
{
	UpdateSightForUnits(EnemyUnits, EnemyVisibleCells, EnemyExploredCells);
}

void ASightManager::UpdateSightForUnits(
	const TArray<AStrategyUnit*>& Units,
	TSet<FIntPoint>& OutVisibleCells,
	TSet<FIntPoint>& OutExploredCells)
{
	if (!GridManager)
	{
		FindGridManager();
	}

	if (!GridManager)
	{
		return;
	}

	OutVisibleCells.Empty();

	for (AStrategyUnit* Unit : Units)
	{
		if (!Unit)
		{
			continue;
		}

		const FIntPoint UnitCell = GridManager->WorldToGrid(Unit->GetActorLocation());
		const int32 SightRange = GetSightRangeForUnit(Unit);

		TArray<FIntPoint> CellsInRange;
		GridManager->GetCellsInRange(UnitCell, SightRange, CellsInRange);

		for (const FIntPoint& Cell : CellsInRange)
		{
			if (CanSeeCell(Unit, Cell))
			{
				OutVisibleCells.Add(Cell);
				OutExploredCells.Add(Cell);
			}
		}
	}
}

void ASightManager::UpdateEnemyVisibility() const
{
	if (!GridManager)
	{
		return;
	}
	
	if (bFogDisabled)
	{
		return;
	}

	for (AStrategyUnit* Enemy : EnemyUnits)
	{
		if (!Enemy)
		{
			continue;
		}

		const FIntPoint EnemyCell = GridManager->WorldToGrid(Enemy->GetActorLocation());
		const bool bVisible = IsCellVisible(EnemyCell);

		Enemy->SetActorHiddenInGame(!bVisible);
		Enemy->SetActorEnableCollision(bVisible);
	}
}

bool ASightManager::IsCellVisible(const FIntPoint& Cell) const
{
	return VisibleCells.Contains(Cell);
}

bool ASightManager::IsCellExplored(const FIntPoint& Cell) const
{
	return ExploredCells.Contains(Cell);
}

bool ASightManager::CanSeeCell(const AStrategyUnit* Unit, const FIntPoint& Cell) const
{
	if (!Unit || !GridManager)
	{
		return false;
	}

	if (!GridManager->IsValidCell(Cell))
	{
		return false;
	}

	FVector GroundLocation;
	FVector GroundNormal;

	if (!GridManager->ProjectCellToGround(Cell, GroundLocation, GroundNormal))
	{
		return false;
	}

	const FVector From = Unit->GetActorLocation() + FVector(0.f, 0.f, EyeHeight);
	const FVector To = GroundLocation + FVector(0.f, 0.f, EyeHeight);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SightTrace), false);
	Params.AddIgnoredActor(Unit);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GridManager);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		From,
		To,
		SightTraceChannel,
		Params
	);

	return !bHit;
}

int32 ASightManager::GetSightRangeForUnit(const AStrategyUnit* Unit) const
{
	if (!Unit)
	{
		return DefaultSightRange;
	}

	return Unit->GetSightRange();
}

void ASightManager::RefreshFog()
{
	if (bFogDisabled)
	{
		return;
	}
	
	if (!FogOfWarActor || !GridManager)
	{
		return;
	}

	FogOfWarActor->RefreshFog(
		GridManager,
		VisibleCells,
		ExploredCells
	);
};