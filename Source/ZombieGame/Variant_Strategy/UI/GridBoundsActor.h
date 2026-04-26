#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridBoundsActor.generated.h"

class UBoxComponent;

UCLASS()
class ZOMBIEGAME_API AGridBoundsActor : public AActor
{
	GENERATED_BODY()

public:
	AGridBoundsActor();

	FVector GetGridOrigin() const;
	FVector GetGridCenter() const;
	FVector GetGridExtents() const;

	int32 GetGridWidth(float CellSize) const;
	int32 GetGridHeight(float CellSize) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UBoxComponent> Bounds;
};