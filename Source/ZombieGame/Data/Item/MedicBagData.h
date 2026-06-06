#pragma once

#include "EquippableItemData.h"
#include "MedicBagData.generated.h"

UCLASS(Blueprintable)
class ZOMBIEGAME_API UMedicBagData : public UEquippableItemData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Healing", meta=(ClampMin="0"))
	int32 HealAmount = 4;
};
