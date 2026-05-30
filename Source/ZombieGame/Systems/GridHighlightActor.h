// GridHighlightActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridHighlightActor.generated.h"

class ULineBatchComponent;
class UDecalComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;
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

UENUM(BlueprintType)
enum class EGridCoverType : uint8
{
	Half,
	Full
};

USTRUCT(BlueprintType)
struct FGridCoverIndicator
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridCoverType CoverType = EGridCoverType::Half;
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

	UPROPERTY(EditAnywhere, Category="Highlight|Movement Path")
	FLinearColor MovementPathLineColor = FLinearColor(0.0f, 0.95f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category="Highlight|Movement Path", meta=(ClampMin="0.0"))
	float MovementPathLineThickness = 6.0f;

	UPROPERTY(EditAnywhere, Category="Highlight|Movement Path")
	float MovementPathLineHeightOffset = 10.0f;

	UPROPERTY(EditAnywhere, Category="Highlight|Cover")
	TObjectPtr<UTexture2D> HalfCoverTexture;

	UPROPERTY(EditAnywhere, Category="Highlight|Cover")
	TObjectPtr<UTexture2D> FullCoverTexture;

	UPROPERTY(EditAnywhere, Category="Highlight|Cover")
	TObjectPtr<UMaterialInterface> HalfCoverMaterial;

	UPROPERTY(EditAnywhere, Category="Highlight|Cover")
	TObjectPtr<UMaterialInterface> FullCoverMaterial;

	UPROPERTY(EditAnywhere, Category="Highlight|Cover")
	TObjectPtr<UMaterialInterface> CoverFallbackMaterial;

	UPROPERTY(EditAnywhere, Category="Highlight|Cover")
	TObjectPtr<UStaticMesh> CoverIconMesh;

	UPROPERTY(EditAnywhere, Category="Highlight|Cover", meta=(ClampMin="1.0"))
	float CoverIconWorldSize = 95.0f;

	UPROPERTY(EditAnywhere, Category="Highlight|Cover", meta=(ClampMin="0.0"))
	float CoverIconHeightOffset = 95.0f;

	UPROPERTY(EditAnywhere, Category="Highlight|Cover")
	float CoverIconWallOffset = 4.0f;

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
	void ShowMovementPath(const TArray<FVector>& PathPoints);
	void ClearMovementPath();
	void ShowCoverIndicators(const TArray<FGridCoverIndicator>& Indicators);
	void ClearCoverIndicators();

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

	UPROPERTY()
	TObjectPtr<ULineBatchComponent> MovementPathLineBatch;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> CoverIndicatorPool;

	UDecalComponent* GetOrCreateDecal(TArray<TObjectPtr<UDecalComponent>>& Pool, int32 Index, UMaterialInterface* Material);
	UStaticMeshComponent* GetOrCreateCoverIndicator(int32 Index);
	void ShowCells(AGridManager* GridManager, const TArray<FIntPoint>& Cells, TArray<TObjectPtr<UDecalComponent>>& Pool, UMaterialInterface* Material, bool bRequireWalkableGround);
	void ClearHighlights(TArray<TObjectPtr<UDecalComponent>>& Pool);
	void DrawBoundaryLine(ULineBatchComponent* LineBatch, const FOverwatchBoundaryLine& BoundaryLine);
	void ClearBoundaryLines(ULineBatchComponent* LineBatch);
};
