#include "SightManager.h"

#include "GridManager.h"
#include "StrategyUnit.h"
#include "../Variant_Strategy/UI/FogOfWarActor.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"

#include "ZombieGame.h"

namespace
{
	FString FormatCell(const FIntPoint& Cell)
	{
		return FString::Printf(TEXT("(%d,%d)"), Cell.X, Cell.Y);
	}

	FString DescribeUnitForSightDebug(const AStrategyUnit* Unit)
	{
		if (!Unit)
		{
			return TEXT("Unit=null");
		}

		USkeletalMeshComponent* MeshComponent = Unit->GetMesh();
		return FString::Printf(
			TEXT("Unit=%s Class=%s Loc=%s Hidden=%d ActorHidden=%d Collision=%d MeshComp=%s Mesh=%s AnimClass=%s Materials=%d MeshVisible=%d MeshHiddenInGame=%d"),
			*GetNameSafe(Unit),
			*GetNameSafe(Unit->GetClass()),
			*Unit->GetActorLocation().ToCompactString(),
			Unit->IsHidden() ? 1 : 0,
			Unit->IsHidden() ? 1 : 0,
			Unit->GetActorEnableCollision() ? 1 : 0,
			*GetNameSafe(MeshComponent),
			*GetNameSafe(MeshComponent ? MeshComponent->GetSkeletalMeshAsset() : nullptr),
			*GetNameSafe(MeshComponent ? MeshComponent->GetAnimClass() : nullptr),
			MeshComponent ? MeshComponent->GetNumMaterials() : 0,
			MeshComponent && MeshComponent->IsVisible() ? 1 : 0,
			MeshComponent && MeshComponent->bHiddenInGame ? 1 : 0);
	}
}

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

	UE_LOG(LogZombieGame, Warning, TEXT("SightDebug: SetUnits Players=%d Enemies=%d Grid=%s Fog=%s FogDisabled=%d"),
		PlayerUnits.Num(),
		EnemyUnits.Num(),
		*GetNameSafe(GridManager),
		*GetNameSafe(FogOfWarActor),
		bFogDisabled ? 1 : 0);

	for (AStrategyUnit* Unit : PlayerUnits)
	{
		RegisterUnit(Unit);
		UE_LOG(LogZombieGame, Warning, TEXT("SightDebug: Player registered %s"),
			*DescribeUnitForSightDebug(Unit));
	}

	for (AStrategyUnit* Unit : EnemyUnits)
	{
		UE_LOG(LogZombieGame, Warning, TEXT("SightDebug: Enemy registered %s"),
			*DescribeUnitForSightDebug(Unit));
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

		UE_LOG(LogZombieGame, Warning, TEXT("SightDebug: UnitSight %s Cell=%s SightRange=%d CellsInRange=%d VisibleTotal=%d ExploredTotal=%d"),
			*GetNameSafe(Unit),
			*FormatCell(UnitCell),
			SightRange,
			CellsInRange.Num(),
			OutVisibleCells.Num(),
			OutExploredCells.Num());
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

		UE_LOG(LogZombieGame, Warning, TEXT("SightDebug: EnemyVisibility Before VisibleCell=%d Cell=%s VisibleCells=%d %s"),
			bVisible ? 1 : 0,
			*FormatCell(EnemyCell),
			VisibleCells.Num(),
			*DescribeUnitForSightDebug(Enemy));

		if (!bVisible)
		{
			for (const AStrategyUnit* PlayerUnit : PlayerUnits)
			{
				if (!PlayerUnit)
				{
					continue;
				}

				FHitResult Hit;
				FString FailureReason;
				const bool bPlayerCanSeeEnemyCell = TraceSightToCell(PlayerUnit, EnemyCell, &Hit, &FailureReason);
				const FIntPoint PlayerCell = GridManager->WorldToGrid(PlayerUnit->GetActorLocation());
				UE_LOG(LogZombieGame, Warning, TEXT("SightDebug: EnemyHiddenReason Enemy=%s EnemyCell=%s Player=%s PlayerCell=%s CanSeeEnemyCell=%d Reason=%s HitActor=%s HitComponent=%s HitLocation=%s"),
					*GetNameSafe(Enemy),
					*FormatCell(EnemyCell),
					*GetNameSafe(PlayerUnit),
					*FormatCell(PlayerCell),
					bPlayerCanSeeEnemyCell ? 1 : 0,
					*FailureReason,
					*GetNameSafe(Hit.GetActor()),
					*GetNameSafe(Hit.GetComponent()),
					*Hit.Location.ToCompactString());
			}
		}

		Enemy->SetActorHiddenInGame(!bVisible);
		// Keep collision enabled while hidden so characters do not fall through the level under fog of war.
		Enemy->SetActorEnableCollision(true);

		UE_LOG(LogZombieGame, Warning, TEXT("SightDebug: EnemyVisibility After VisibleCell=%d Cell=%s %s"),
			bVisible ? 1 : 0,
			*FormatCell(EnemyCell),
			*DescribeUnitForSightDebug(Enemy));
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



