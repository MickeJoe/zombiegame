#pragma once

#include "StrategySpawnPoint.generated.h"

class UArrowComponent;

UENUM(BlueprintType)
enum class ESpawnPointSide : uint8
{
	Player,
	Enemy
};

UCLASS()
class AStrategySpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AStrategySpawnPoint();

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Spawn")
	ESpawnPointSide Side = ESpawnPointSide::Player;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Spawn")
	int32 SpawnOrder = 0;

	UFUNCTION(BlueprintCallable)
	FTransform GetSpawnTransform() const;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArrowComponent> Arrow;
};