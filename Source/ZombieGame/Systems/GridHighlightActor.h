// GridHighlightActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridHighlightActor.generated.h"

class ULineBatchComponent;
class UDecalComponent;
class UMaterialInterface;
class AGridManager;

USTRUCT(BlueprintType)
struct FOverwatchBoundaryLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LeftEnd = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector RightEnd = FVector::ZeroVector;
};

UCLASS()
class AGridHighlightActor : public AActor
{
	GENERATED_BODY()

public:
	AGridHighlightActor();

	UPROPERTY(EditAnywhere, Category="Highlight")
	TObjectPtr<UMaterialInterface> ReachableDecalMaterial;

	UPROPERTY(EditAnywhere, Category="Highlight|Overwatch")
	TObjectPtr<UMaterialInterface> OverwatchDecalMaterial;

	UPROPERTY(EditAnywhere, Category="Highlight|Overwatch")
	FLinearColor OverwatchEdgeLineColor = FLinearColor(0.0f, 0.9f, 0.85f, 1.0f);

	UPROPERTY(EditAnywhere, Category="Highlight|Overwatch", meta=(ClampMin="0.0"))
	float OverwatchEdgeLineThickness = 8.0f;

	UPROPERTY(EditAnywhere, Category="Highlight|Overwatch")
	float OverwatchEdgeLineHeightOffset = 12.0f;

	UPROPERTY(EditAnywhere, Category="Highlight|Overwatch", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OverwatchMinGroundNormalZ = 0.65f;

	UPROPERTY(EditAnywhere, Category="Highlight")
	FVector DecalSize = FVector(96.f, 100.f, 30.f);
	// X = projection depth, Y/Z = size on ground

	UPROPERTY(EditAnywhere, Category="Highlight")
	float SurfaceOffset = 2.f;

	UFUNCTION(BlueprintCallable, Category="Highlight")
	void ShowReachableCells(AGridManager* GridManager, const TArray<FIntPoint>& Cells);

	UFUNCTION(BlueprintCallable, Category="Highlight")
	void ClearReachableHighlights();

	UFUNCTION(BlueprintCallable, Category="Highlight|Overwatch")
	void ShowOverwatchCells(AGridManager* GridManager, const TArray<FIntPoint>& Cells);

	UFUNCTION(BlueprintCallable, Category="Highlight|Overwatch")
	void ClearOverwatchHighlights();

	UFUNCTION(BlueprintCallable, Category="Highlight|Overwatch")
	void ShowOverwatchPreviewCells(AGridManager* GridManager, const TArray<FIntPoint>& Cells);

	UFUNCTION(BlueprintCallable, Category="Highlight|Overwatch")
	void ClearOverwatchPreviewHighlights();

	void ShowOverwatchBoundaryLines(const TArray<FOverwatchBoundaryLine>& BoundaryLines);
	void ClearOverwatchBoundaryLines();
	void ShowOverwatchPreviewBoundaryLine(const FOverwatchBoundaryLine& BoundaryLine);
	void ClearOverwatchPreviewBoundaryLines();

protected:
	UPROPERTY()
	TArray<TObjectPtr<UDecalComponent>> DecalPool;

	UPROPERTY()
	TArray<TObjectPtr<UDecalComponent>> OverwatchDecalPool;

	UPROPERTY()
	TArray<TObjectPtr<UDecalComponent>> OverwatchPreviewDecalPool;

	UPROPERTY()
	TObjectPtr<ULineBatchComponent> OverwatchBoundaryLineBatch;

	UPROPERTY()
	TObjectPtr<ULineBatchComponent> OverwatchPreviewBoundaryLineBatch;

	UDecalComponent* GetOrCreateDecal(TArray<TObjectPtr<UDecalComponent>>& Pool, int32 Index, UMaterialInterface* Material);
	void ShowCells(AGridManager* GridManager, const TArray<FIntPoint>& Cells, TArray<TObjectPtr<UDecalComponent>>& Pool, UMaterialInterface* Material, bool bRequireWalkableGround);
	void ClearHighlights(TArray<TObjectPtr<UDecalComponent>>& Pool);
	void DrawBoundaryLine(ULineBatchComponent* LineBatch, const FOverwatchBoundaryLine& BoundaryLine);
	void ClearBoundaryLines(ULineBatchComponent* LineBatch);
};
