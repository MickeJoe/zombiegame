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

#include "ZombieGame/Variant_Strategy/StrategyUnit.h"

PRAGMA_DISABLE_OPTIMIZATION

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

	for (AStrategySpawnPoint* Spawn : PlayerSpawns)
	{
		AStrategyUnit* Unit = GetWorld()->SpawnActor<AStrategyUnit>(
			PlayerUnitClass,
			GetProjectedSpawnTransform(Spawn),
			Params);

		if (!Unit)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player unit spawn FAILED"));
			continue;
		}

		Unit->SetStrategyUnitTeam(EStrategyUnitTeam::Human);
		PlayerSide->AddUnit(Unit);
	}

	for (AStrategySpawnPoint* Spawn : EnemySpawns)
	{
		AStrategyUnit* Unit = GetWorld()->SpawnActor<AStrategyUnit>(
			EnemyUnitClass,
			GetProjectedSpawnTransform(Spawn),
			Params);

		if (!Unit)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy unit spawn FAILED"));
			continue;
		}

		Unit->SetStrategyUnitTeam(EStrategyUnitTeam::AI);
		EnemySide->AddUnit(Unit);
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
		ActiveStrategySide->TakeTurn(GridManager, PlayerSide);
	}
	else
	{
		ActiveStrategySide->TakeTurn(GridManager, PlayerSide);
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