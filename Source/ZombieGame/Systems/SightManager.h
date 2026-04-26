#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SightManager.generated.h"

class AFogOfWarActor;
class AGridManager;
class AStrategyUnit;

UCLASS()
class ZOMBIEGAME_API ASightManager : public AActor
{
	GENERATED_BODY()

public:
	ASightManager();

	virtual void BeginPlay() override;
	
	void RegisterUnit(AStrategyUnit* Unit);
	void SetUnits(
		const TArray<AStrategyUnit*>& InPlayerUnits,
		const TArray<AStrategyUnit*>& InEnemyUnits
	);

	void UpdatePlayerSight();
	void UpdateEnemyVisibility() const;

	bool IsCellVisible(const FIntPoint& Cell) const;
	bool IsCellExplored(const FIntPoint& Cell) const;
	bool CanSeeCell(const AStrategyUnit* Unit, const FIntPoint& Cell) const;

	const TSet<FIntPoint>& GetVisibleCells() const { return VisibleCells; }
	const TSet<FIntPoint>& GetExploredCells() const { return ExploredCells; }
	
protected:
	UPROPERTY()
	AFogOfWarActor* FogOfWarActor;
	
	UPROPERTY(EditAnywhere, Category = "Sight")
	bool bFogDisabled = false;

private:
	UPROPERTY(EditAnywhere, Category = "Sight")
	int32 DefaultSightRange = 6;

	UPROPERTY(EditAnywhere, Category = "Sight")
	float EyeHeight = 100.f;

	UPROPERTY(EditAnywhere, Category = "Sight")
	TEnumAsByte<ECollisionChannel> SightTraceChannel = ECC_Visibility;
	
	UPROPERTY()
	TArray<AStrategyUnit*> PlayerUnits;
	UPROPERTY()
	TArray<AStrategyUnit*> EnemyUnits;

	UPROPERTY()
	TObjectPtr<AGridManager> GridManager;
	
	UFUNCTION()
	void HandleUnitGridCellChanged(AStrategyUnit* Unit);
	
	void UpdateSightAndFog();
	void RefreshFog();

	TSet<FIntPoint> VisibleCells;
	TSet<FIntPoint> ExploredCells;

	void FindGridManager();
	int32 GetSightRangeForUnit(const AStrategyUnit* Unit) const;
};