// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StrategyUnit.h"
#include "TurnBannerWidget.h"
#include "StrategyGameMode.generated.h"

/**
 *  Simple GameMode for a top down strategy game.
 */

class AStrategySpawnPoint;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchReady);

class ASightManager;
class AStrategySide;
class AAIStrategySide;
class APlayerStrategySide;
class AStrategyUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveSideChanged, AStrategySide*, NewActiveSide);

UENUM(BlueprintType)
enum class EActiveSide : uint8
{
	Player	UMETA(DisplayName = "Player"),
	AI		UMETA(DisplayName = "AI")
};

UCLASS(abstract)
class AStrategyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStrategyGameMode();
	
protected:
	void virtual BeginPlay() override;
	
	UPROPERTY()
	ASightManager* SightManager;

public:

	UPROPERTY(EditDefaultsOnly, Category="Units")
	TSubclassOf<AStrategyUnit> PlayerUnitClass;

	UPROPERTY(EditDefaultsOnly, Category="Units")
	TSubclassOf<AStrategyUnit> EnemyUnitClass;
	
	UPROPERTY(BlueprintAssignable)
	FOnMatchReady OnMatchReady;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Strategy")
	TObjectPtr<APlayerStrategySide> PlayerSide;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Strategy")
	TObjectPtr<AAIStrategySide> EnemySide;

//	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Strategy")
//	int32 ActiveSideIndex = INDEX_NONE;
	EActiveSide ActiveSide;
	
	UPROPERTY(BlueprintAssignable, Category = "Strategy")
	FOnActiveSideChanged OnActiveSideChanged;

//	UFUNCTION(BlueprintCallable, Category = "Strategy")
//	void RegisterSide(AStrategySide* Side);

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	void RegisterUnitToSide(AStrategyUnit* Unit, AStrategySide* Side);

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	AStrategySide* GetActiveSide() const;

//	UFUNCTION(BlueprintCallable, Category = "Strategy")
//	bool IsHumanPlayersTurn() const;

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	void StartMatchFlow();

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	void StartTurn();

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	void EndTurn();

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	void AdvanceToNextSide();

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	bool IsBattleOver() const;

	void SetupSpawnPoints();
	void SpawnUnits();
//	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Units")
//	TSubclassOf<AStrategyUnit> StrategyUnitClass;
private:
//	UPROPERTY(Transient)
//	TArray<AStrategyUnit*> PlayerUnitArray;

	UPROPERTY(EditDefaultsOnly, Category = "Strategy")
	TSubclassOf<APlayerStrategySide> PlayerSideClass;

	UPROPERTY(EditDefaultsOnly, Category = "Strategy")
	TSubclassOf<AAIStrategySide> EnemySideClass;	

	UPROPERTY(Transient)
	TArray<AStrategySpawnPoint*> PlayerSpawns;
	
	UPROPERTY(Transient)
	TArray<AStrategySpawnPoint*> EnemySpawns;
	
protected:
	
};
