// GridHighlightActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridHighlightActor.generated.h"

class UDecalComponent;
class UMaterialInterface;
class AGridManager;

UCLASS()
class AGridHighlightActor : public AActor
{
	GENERATED_BODY()

public:
	AGridHighlightActor();

	UPROPERTY(EditAnywhere, Category="Highlight")
	TObjectPtr<UMaterialInterface> ReachableDecalMaterial;

	UPROPERTY(EditAnywhere, Category="Highlight")
	FVector DecalSize = FVector(96.f, 100.f, 30.f);
	// X = projection depth, Y/Z = size on ground

	UPROPERTY(EditAnywhere, Category="Highlight")
	float SurfaceOffset = 2.f;

	UFUNCTION(BlueprintCallable, Category="Highlight")
	void ShowReachableCells(AGridManager* GridManager, const TArray<FIntPoint>& Cells);

	UFUNCTION(BlueprintCallable, Category="Highlight")
	void ClearReachableHighlights();

protected:
	UPROPERTY()
	TArray<TObjectPtr<UDecalComponent>> DecalPool;

	UDecalComponent* GetOrCreateDecal(int32 Index);
};