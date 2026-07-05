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

	Unit->OnGridCellChanged.AddUniqueDynamic(this, &ASightManager::HandleUnitGridCellChanged);
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

	for (AStrategyUnit* Unit : EnemyUnits)
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
		// Keep collision enabled while hidden so characters do not fall through the level under fog of war.
		Enemy->SetActorEnableCollision(true);
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
	return TraceSightToCell(Unit, Cell);
}

bool ASightManager::TraceSightToCell(
	const AStrategyUnit* Unit,
	const FIntPoint& Cell,
	FHitResult* OutHit,
	FString* OutFailureReason) const
{
	if (!Unit || !GridManager)
	{
		if (OutFailureReason)
		{
			*OutFailureReason = FString::Printf(TEXT("Invalid input Unit=%s Grid=%s"),
				*GetNameSafe(Unit),
				*GetNameSafe(GridManager));
		}
		return false;
	}

	if (!GridManager->IsValidCell(Cell))
	{
		if (OutFailureReason)
		{
			*OutFailureReason = TEXT("Invalid grid cell");
		}
		return false;
	}

	FVector GroundLocation;
	FVector GroundNormal;

	if (!GridManager->ProjectCellToGround(Cell, GroundLocation, GroundNormal))
	{
		if (OutFailureReason)
		{
			*OutFailureReason = TEXT("ProjectCellToGround failed");
		}
		return false;
	}

	const FVector From = Unit->GetActorLocation() + FVector(0.f, 0.f, EyeHeight);
	const FVector To = GroundLocation + FVector(0.f, 0.f, EyeHeight);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SightTrace), false);
	Params.AddIgnoredActor(Unit);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GridManager);
	for (const AStrategyUnit* PlayerUnit : PlayerUnits)
	{
		if (PlayerUnit)
		{
			Params.AddIgnoredActor(PlayerUnit);
		}
	}
	for (const AStrategyUnit* EnemyUnit : EnemyUnits)
	{
		if (EnemyUnit)
		{
			Params.AddIgnoredActor(EnemyUnit);
		}
	}

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		From,
		To,
		SightTraceChannel,
		Params
	);

	if (OutHit)
	{
		*OutHit = Hit;
	}

	if (OutFailureReason)
	{
		*OutFailureReason = bHit
			? FString::Printf(TEXT("Blocked From=%s To=%s Channel=%d"),
				*From.ToCompactString(),
				*To.ToCompactString(),
				static_cast<int32>(SightTraceChannel.GetValue()))
			: FString::Printf(TEXT("Clear From=%s To=%s Channel=%d"),
				*From.ToCompactString(),
				*To.ToCompactString(),
				static_cast<int32>(SightTraceChannel.GetValue()));
	}

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



