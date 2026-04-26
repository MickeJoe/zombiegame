#pragma once

#include "FogOfWarActor.generated.h"

class AGridManager;

UCLASS()
class ZOMBIEGAME_API AFogOfWarActor : public AActor
{
	GENERATED_BODY()

public:
	AFogOfWarActor();

	void RefreshFog(
		AGridManager* GridManager,
		const TSet<FIntPoint>& VisibleCells,
		const TSet<FIntPoint>& ExploredCells
	);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> FogMesh;

	UPROPERTY(EditAnywhere, Category = "Fog")
	TObjectPtr<UStaticMesh> FogTileMesh;

	UPROPERTY(EditAnywhere, Category = "Fog")
	TObjectPtr<UMaterialInterface> UnexploredMaterial;

	UPROPERTY(EditAnywhere, Category = "Fog")
	TObjectPtr<UMaterialInterface> ExploredMaterial;

	UPROPERTY(EditAnywhere, Category = "Fog")
	float FogZOffset = 8.f;
};