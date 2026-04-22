// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyGameMode.h"

#include "StrategyGameMode.h"

#include "StrategyPlayerController.h"
#include "StrategySpawnPoint.h"
#include "Player/StrategySide.h"
#include "StrategyUnit.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

#include "ZombieGame/Variant_Strategy/StrategyUnit.h"

PRAGMA_DISABLE_OPTIMIZATION

AStrategyGameMode::AStrategyGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStrategyGameMode::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundSides;
	UGameplayStatics::GetAllActorsOfClass(this, AStrategySide::StaticClass(), FoundSides);

	for (AActor* Actor : FoundSides)
	{
		if (AStrategySide* Side = Cast<AStrategySide>(Actor))
		{
			RegisterSide(Side);
		}
	}

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SetupSpawnPoints();
	SpawnUnits();
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

void AStrategyGameMode::SpawnUnits()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (AStrategySpawnPoint* Spawn : PlayerSpawns)
	{
		AStrategyUnit* Unit = GetWorld()->SpawnActor<AStrategyUnit>(
			PlayerUnitClass,
			Spawn->GetActorTransform(),
			Params);

		Unit->SetStrategyUnitTeam(EStrategyUnitTeam::Human);

		UE_LOG(LogTemp, Warning, TEXT("Player unit spawned: %s"),
			Unit ? *Unit->GetName() : TEXT("FAILED"));
	}

	for (AStrategySpawnPoint* Spawn : EnemySpawns)
	{
		AStrategyUnit* Unit = GetWorld()->SpawnActor<AStrategyUnit>(
			EnemyUnitClass,
			Spawn->GetActorTransform(),
			Params);

		Unit->SetStrategyUnitTeam(EStrategyUnitTeam::AI);

		UE_LOG(LogTemp, Warning, TEXT("Enemy unit spawned: %s"),
			Unit ? *Unit->GetName() : TEXT("FAILED"));
	}
}

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
	if (!Sides.IsValidIndex(ActiveSideIndex))
	{
		return nullptr;
	}

	return Sides[ActiveSideIndex];
}

bool AStrategyGameMode::IsHumanPlayersTurn() const
{
	const AStrategySide* ActiveSide = GetActiveSide();
	return ActiveSide && ActiveSide->IsHuman();
}

void AStrategyGameMode::StartMatchFlow()
{
	if (Sides.Num() == 0)
	{
		return;
	}

	ActiveSideIndex = 0;
	StartTurn();
}

void AStrategyGameMode::StartTurn()
{
	AStrategySide* ActiveSide = GetActiveSide();
	if (!ActiveSide)
	{
		return;
	}

	if (AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PC->ShowTurnBanner(ActiveSide->IsAI() ? ETurnOwner::Enemy : ETurnOwner::Player);
	}

	OnActiveSideChanged.Broadcast(ActiveSide);
	ActiveSide->TakeTurn();
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
	if (Sides.Num() == 0)
	{
		ActiveSideIndex = INDEX_NONE;
		return;
	}

	for (int32 Offset = 1; Offset <= Sides.Num(); ++Offset)
	{
		const int32 NextIndex = (ActiveSideIndex + Offset) % Sides.Num();
		AStrategySide* Candidate = Sides[NextIndex];

		if (IsValid(Candidate) /*&& Candidate->HasLivingUnits()*/)
		{
			ActiveSideIndex = NextIndex;
			return;
		}
	}
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