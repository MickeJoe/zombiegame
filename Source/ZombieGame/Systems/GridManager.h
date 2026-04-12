#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridHighlightActor.h"
#include "GridManager.generated.h"

class AStrategyUnit;

class UNavigationSystemV1;
class ANavigationData;

UCLASS()
class AGridManager : public AActor
{
	GENERATED_BODY()

public:
	AGridManager();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	float CellSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 GridWidth = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 GridHeight = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Debug")
	bool bShowGridInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Debug")
	bool bShowGridInGame = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Debug")
	float GridZOffset = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Debug")
	FColor GridColor = FColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Debug")
	float GridLineThickness = 1.5f;

	FVector GridToWorldCenter(int32 X, int32 Y) const;

	FIntPoint WorldToGrid(const FVector& WorldLocation) const;
	FVector GridToWorld(const FIntPoint& GridCoord) const;

	UFUNCTION(BlueprintCallable, Category="Grid")
	bool ProjectCellToGround(const FIntPoint& Cell, FVector& OutLocation, FVector& OutNormal) const;

	bool TryGetNavigationDataForCell(
		const FIntPoint& Cell,
		UNavigationSystemV1*& OutNavSys,
		const ANavigationData*& OutNavData,
		FNavLocation& OutProjectedEnd
	) const;



	bool IsCellOnNavMesh(
		const FIntPoint& Cell,
		FNavLocation* OutLocation
	) const;
/*
	bool IsCellReachableFromUnit(
		const AStrategyUnit* Unit,
		const FIntPoint& Cell
	) const;
*/
	bool IsCellWithinMoveRange(
		const AStrategyUnit* Unit,
		const FIntPoint& Cell,
		int32 MaxMoveCells
	) const;
	
private:
	void DrawGrid(bool bPersistent, float LifeTime) const;
	void FlushGridDebugLines() const;
	
	UPROPERTY()
	TArray<AActor*> SpawnedHighlights;
};