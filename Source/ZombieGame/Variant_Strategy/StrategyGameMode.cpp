// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyGameMode.h"

#include "StrategyGameMode.h"

#include "NavigationSystem.h"
#include "StrategyPlayerController.h"
#include "StrategySpawnPoint.h"
#include "Player/StrategySide.h"
#include "StrategyUnit.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerStrategySide.h"
#include "Player/AIStrategySide.h"
#include "Systems/SightManager.h"
#include "Systems/GridManager.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/Unit/UnitData.h"

#include "ZombieGame/Variant_Strategy/StrategyUnit.h"
#include "ZombieGame.h"

PRAGMA_DISABLE_OPTIMIZATION

namespace
{
	FString FormatCellForSpawnDebug(const AGridManager* GridManager, const AStrategyUnit* Unit)
	{
		if (!GridManager || !Unit)
		{
			return TEXT("(no-grid)");
		}

		const FIntPoint Cell = GridManager->WorldToGrid(Unit->GetActorLocation());
		return FString::Printf(TEXT("(%d,%d)"), Cell.X, Cell.Y);
	}

	FString DescribeUnitForSpawnDebug(const AStrategyUnit* Unit, const AGridManager* GridManager)
	{
		if (!Unit)
		{
			return TEXT("Unit=null");
		}

		USkeletalMeshComponent* MeshComponent = Unit->GetMesh();
		return FString::Printf(
			TEXT("Unit=%s Class=%s Cell=%s Loc=%s Rot=%s Hidden=%d Collision=%d MeshComp=%s Mesh=%s AnimClass=%s Materials=%d MeshVisible=%d MeshHiddenInGame=%d UnitData=%s SightRange=%d HP=%d/%d TU=%d/%d"),
			*GetNameSafe(Unit),
			*GetNameSafe(Unit->GetClass()),
			*FormatCellForSpawnDebug(GridManager, Unit),
			*Unit->GetActorLocation().ToCompactString(),
			*Unit->GetActorRotation().ToCompactString(),
			Unit->IsHidden() ? 1 : 0,
			Unit->GetActorEnableCollision() ? 1 : 0,
			*GetNameSafe(MeshComponent),
			*GetNameSafe(MeshComponent ? MeshComponent->GetSkeletalMeshAsset() : nullptr),
			*GetNameSafe(MeshComponent ? MeshComponent->GetAnimClass() : nullptr),
			MeshComponent ? MeshComponent->GetNumMaterials() : 0,
			MeshComponent && MeshComponent->IsVisible() ? 1 : 0,
			MeshComponent && MeshComponent->bHiddenInGame ? 1 : 0,
			*GetNameSafe(Unit->UnitData.Get()),
			Unit->GetSightRange(),
			Unit->GetCurrentHealth(),
			Unit->GetMaxHealth(),
			Unit->GetRemainingTimeUnits(),
			Unit->GetMaxTimeUnits());
	}
}

AStrategyGameMode::AStrategyGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStrategyGameMode::BeginPlay()
{
	Super::BeginPlay();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UWorld* World = GetWorld();
	if (!World) return;

	UE_LOG(LogZombieGame, Warning, TEXT("SpawnDebug: BeginPlay World=%s NetMode=%d GameMode=%s PlayerSideClass=%s EnemySideClass=%s PlayerUnitClass=%s EnemyUnitClass=%s"),
		*GetNameSafe(World),
		static_cast<int32>(World->GetNetMode()),
		*GetNameSafe(this),
		*GetNameSafe(PlayerSideClass.Get()),
		*GetNameSafe(EnemySideClass.Get()),
		*GetNameSafe(PlayerUnitClass.Get()),
		*GetNameSafe(EnemyUnitClass.Get()));

	PlayerSide = World->SpawnActor<APlayerStrategySide>(PlayerSideClass);
	EnemySide = World->SpawnActor<AAIStrategySide>(EnemySideClass);
	
	SightManager = Cast<ASightManager>(
		UGameplayStatics::GetActorOfClass(this, ASightManager::StaticClass())
	);
	
	GridManager = Cast<AGridManager>(
		UGameplayStatics::GetActorOfClass(this, AGridManager::StaticClass())
	);
	
	SetupSpawnPoints();
	SpawnUnits();
	UE_LOG(LogZombieGame, Warning, TEXT("SpawnDebug: Spawn complete PlayerUnits=%d EnemyUnits=%d SightManager=%s GridManager=%s"),
		PlayerSide ? PlayerSide->Units.Num() : 0,
		EnemySide ? EnemySide->Units.Num() : 0,
		*GetNameSafe(SightManager),
		*GetNameSafe(GridManager));
	SightManager->SetUnits(PlayerSide->Units, EnemySide->Units);
//	StartMatchFlow();
	OnMatchReady.AddDynamic(this, &AStrategyGameMode::StartMatchFlow);
}

void AStrategyGameMode::SetupSpawnPoints()
{
	TArray<AActor*> FoundSpawnPoints;
	UGameplayStatics::GetAllActorsOfClass(this, AStrategySpawnPoint::StaticClass(), FoundSpawnPoints);

	for (AActor* Actor : FoundSpawnPoints)
	{
		if (AStrategySpawnPoint* Spawn = Cast<AStrategySpawnPoint>(Actor))
		{
			UE_LOG(LogTemp, Warning, TEXT("Actor loc: %s"), *Spawn->GetActorLocation().ToString());
//			UE_LOG(LogTemp, Warning, TEXT("Arrow loc: %s"), *Spawn->Arrow->GetComponentLocation().ToString());

			DrawDebugSphere(GetWorld(), Spawn->GetActorLocation(), 30.f, 12, FColor::Red, false, 10.f);
//			DrawDebugSphere(GetWorld(), Spawn->Arrow->GetComponentLocation(), 30.f, 12, FColor::Green, false, 10.f);
			
			if (Spawn->Side == ESpawnPointSide::Player)
			{
				PlayerSpawns.Add(Spawn);
			}
			else
			{
				EnemySpawns.Add(Spawn);
			}
		}
	}

	auto SortBySpawnOrder = [](const AStrategySpawnPoint& Left, const AStrategySpawnPoint& Right)
	{
		return Left.SpawnOrder < Right.SpawnOrder;
	};

	PlayerSpawns.Sort(SortBySpawnOrder);
	EnemySpawns.Sort(SortBySpawnOrder);

	UE_LOG(LogZombieGame, Warning, TEXT("SpawnDebug: SpawnPoints Player=%d Enemy=%d"),
		PlayerSpawns.Num(),
		EnemySpawns.Num());
}

FTransform AStrategyGameMode::GetProjectedSpawnTransform(
	const AStrategySpawnPoint* Spawn) const
{
	FTransform Transform = Spawn->GetActorTransform();

	UNavigationSystemV1* NavSys =
		FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!NavSys)
	{
		return Transform;
	}

	FNavLocation ProjectedLocation;

	const bool bProjected = NavSys->ProjectPointToNavigation(
		Spawn->GetActorLocation(),
		ProjectedLocation,
		FVector(200.f, 200.f, 1000.f)
	);

	if (bProjected)
	{
		FVector Location = ProjectedLocation.Location;

		// Bra om AStrategyUnit är Character, så kapseln inte börjar exakt i golvet
		Location.Z += 50.f;

		Transform.SetLocation(Location);
	}

	return Transform;
}

void AStrategyGameMode::SpawnUnits()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const int32 PlayerUnitCount = PlayerUnitClasses.Num() > 0
		? FMath::Min(PlayerSpawns.Num(), PlayerUnitClasses.Num())
		: PlayerSpawns.Num();

	if (PlayerUnitClasses.Num() > PlayerSpawns.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Only %d of %d configured player unit classes can spawn because there are %d player spawn points"),
			PlayerUnitCount,
			PlayerUnitClasses.Num(),
			PlayerSpawns.Num());
	}

	for (int32 Index = 0; Index < PlayerUnitCount; ++Index)
	{
		AStrategySpawnPoint* Spawn = PlayerSpawns[Index];
		TSubclassOf<AStrategyUnit> UnitClass = PlayerUnitClasses.Num() > 0
			? PlayerUnitClasses[Index]
			: PlayerUnitClass;

		if (!UnitClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player unit spawn skipped: no unit class at index %d"), Index);
			continue;
		}

		AStrategyUnit* Unit = GetWorld()->SpawnActor<AStrategyUnit>(
			UnitClass,
			GetProjectedSpawnTransform(Spawn),
			Params);

		if (!Unit)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player unit spawn FAILED"));
			continue;
		}

		Unit->SetStrategyUnitTeam(EStrategyUnitTeam::Human);
		if (DefaultWeaponData
			&& !Unit->GetEquippedFireWeapon().WeaponData
			&& !Unit->GetEquippedMeleeWeapon().WeaponData)
		{
			Unit->EquipWeapon(DefaultWeaponData);
		}
		PlayerSide->AddUnit(Unit);
		BindUnitOverwatchMovement(Unit);

		UE_LOG(LogZombieGame, Warning, TEXT("SpawnDebug: Player spawned Index=%d Spawn=%s SpawnLoc=%s UnitClass=%s %s"),
			Index,
			*GetNameSafe(Spawn),
			*Spawn->GetActorLocation().ToCompactString(),
			*GetNameSafe(UnitClass.Get()),
			*DescribeUnitForSpawnDebug(Unit, GridManager));

		if (!Unit->GetMesh() || !Unit->GetMesh()->GetSkeletalMeshAsset())
		{
			UE_LOG(LogZombieGame, Error, TEXT("SpawnDebug: Player %s has no skeletal mesh after spawn. ConfiguredClass=%s"),
				*GetNameSafe(Unit),
				*GetNameSafe(UnitClass.Get()));
		}
	}

	const int32 EnemyUnitCount = EnemyUnitClasses.Num() > 0
		? FMath::Min(EnemySpawns.Num(), EnemyUnitClasses.Num())
		: EnemySpawns.Num();

	if (EnemyUnitClasses.Num() > EnemySpawns.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Only %d of %d configured enemy unit classes can spawn because there are %d enemy spawn points"),
			EnemyUnitCount,
			EnemyUnitClasses.Num(),
			EnemySpawns.Num());
	}

	for (int32 Index = 0; Index < EnemyUnitCount; ++Index)
	{
		AStrategySpawnPoint* Spawn = EnemySpawns[Index];
		TSubclassOf<AStrategyUnit> UnitClass = EnemyUnitClasses.Num() > 0
			? EnemyUnitClasses[Index]
			: EnemyUnitClass;

		if (!UnitClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy unit spawn skipped: no unit class at index %d"), Index);
			continue;
		}

		AStrategyUnit* Unit = GetWorld()->SpawnActor<AStrategyUnit>(
			UnitClass,
			GetProjectedSpawnTransform(Spawn),
			Params);

		if (!Unit)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy unit spawn FAILED"));
			continue;
		}

		Unit->SetStrategyUnitTeam(EStrategyUnitTeam::AI);
		EnemySide->AddUnit(Unit);
		BindUnitOverwatchMovement(Unit);

		UE_LOG(LogZombieGame, Warning, TEXT("SpawnDebug: Enemy spawned Index=%d Spawn=%s SpawnLoc=%s UnitClass=%s %s"),
			Index,
			*GetNameSafe(Spawn),
			*Spawn->GetActorLocation().ToCompactString(),
			*GetNameSafe(UnitClass.Get()),
			*DescribeUnitForSpawnDebug(Unit, GridManager));

		if (!Unit->GetMesh() || !Unit->GetMesh()->GetSkeletalMeshAsset())
		{
			UE_LOG(LogZombieGame, Error, TEXT("SpawnDebug: Enemy %s has no skeletal mesh after spawn. ConfiguredClass=%s"),
				*GetNameSafe(Unit),
				*GetNameSafe(UnitClass.Get()));
		}
	}
}
/*
void AStrategyGameMode::RegisterSide(AStrategySide* Side)
{
	if (!Side)
	{
		return;
	}

	if (!Sides.Contains(Side))
	{
		Sides.Add(Side);
	}
}
*/
void AStrategyGameMode::RegisterUnitToSide(AStrategyUnit* Unit, AStrategySide* Side)
{
	if (!Unit || !Side)
	{
		return;
	}

	Unit->OwningSide = Side;
	Side->AddUnit(Unit);
	BindUnitOverwatchMovement(Unit);
}

void AStrategyGameMode::BindUnitOverwatchMovement(AStrategyUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	Unit->OnGridCellChanged.AddUniqueDynamic(this, &AStrategyGameMode::HandleUnitGridCellChanged);
}

AStrategySide* AStrategyGameMode::GetOpposingSideForUnit(const AStrategyUnit* Unit) const
{
	if (!Unit)
	{
		return nullptr;
	}

	return Unit->GetStrategyUnitTeam() == EStrategyUnitTeam::Human
		? Cast<AStrategySide>(EnemySide)
		: Cast<AStrategySide>(PlayerSide);
}

void AStrategyGameMode::HandleUnitGridCellChanged(AStrategyUnit* Unit)
{
	if (!IsValid(Unit) || Unit->GetCurrentHealth() <= 0 || !GridManager)
	{
		return;
	}

	AStrategySide* OpposingSide = GetOpposingSideForUnit(Unit);
	if (!OpposingSide)
	{
		return;
	}

	const FIntPoint UnitCell = GridManager->WorldToGrid(Unit->GetActorLocation());
	for (AStrategyUnit* OverwatchUnit : OpposingSide->GetAliveUnits())
	{
		if (!IsValid(OverwatchUnit)
			|| OverwatchUnit->GetCurrentHealth() <= 0
			|| !OverwatchUnit->IsOverwatchActive()
			|| !OverwatchUnit->GetOverwatchCells().Contains(UnitCell))
		{
			continue;
		}

		UE_LOG(LogZombieGame, Log, TEXT("Overwatch: %s fires at %s entering cell %s"),
			*GetNameSafe(OverwatchUnit),
			*GetNameSafe(Unit),
			*UnitCell.ToString());

		if (OverwatchUnit->TryFireOverwatchAt(Unit))
		{
			if (AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetWorld()->GetFirstPlayerController()))
			{
				PC->RefreshLockedOverwatchHighlights();
				PC->RefreshWeaponInfoPanel();
			}
		}

		if (!IsValid(Unit) || Unit->GetCurrentHealth() <= 0)
		{
			break;
		}
	}
}

AStrategySide* AStrategyGameMode::GetActiveSide() const
{
	if (ActiveSide == EActiveSide::Player)
	{
		return PlayerSide;
	}
	return EnemySide;
}

void AStrategyGameMode::StartMatchFlow()
{
	ActiveSide = EActiveSide::Player;
	StartTurn();
}

void AStrategyGameMode::StartTurn()
{
	AStrategySide* ActiveStrategySide = GetActiveSide();
	if (!ActiveStrategySide)
	{
		return;
	}

	if (AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PC->ShowTurnBanner(ActiveStrategySide->IsAI() ? ETurnOwner::Enemy : ETurnOwner::Player);
	}

	OnActiveSideChanged.Broadcast(ActiveStrategySide);
	if (ActiveSide == EActiveSide::Player)
	{
		ActiveStrategySide->TakeTurn(GridManager, SightManager, PlayerSide);
	}
	else
	{
		ActiveStrategySide->TakeTurn(GridManager, SightManager, PlayerSide);
	}
}

void AStrategyGameMode::EndTurn()
{
	if (IsBattleOver())
	{
		return;
	}

	AdvanceToNextSide();
	StartTurn();
}

void AStrategyGameMode::AdvanceToNextSide()
{
	if (ActiveSide == EActiveSide::Player)
	{
		ActiveSide = EActiveSide::AI;
		return;
	}
	ActiveSide = EActiveSide::Player;
}

bool AStrategyGameMode::IsBattleOver() const
{
	return false;
	/*
	int32 LivingSideCount = 0;

	for (AStrategySide* Side : Sides)
	{
		if (IsValid(Side) && Side->HasLivingUnits())
		{
			++LivingSideCount;
		}
	}

	return LivingSideCount <= 1;
	*/
}

PRAGMA_ENABLE_OPTIMIZATION
