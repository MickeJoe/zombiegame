#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EquippableItemData.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EEquippableItemSlot : uint8
{
	Primary,
	Secondary
};

UCLASS(Blueprintable, Abstract)
class ZOMBIEGAME_API UEquippableItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	EEquippableItemSlot EquipmentSlot = EEquippableItemSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment", meta=(ClampMin="0"))
	int32 Range = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment", meta=(ClampMin="0", DisplayName="Time Unit Cost (TU)", ToolTip="How many time units this item costs to use."))
	int32 TimeUnitCost = 1;
};
