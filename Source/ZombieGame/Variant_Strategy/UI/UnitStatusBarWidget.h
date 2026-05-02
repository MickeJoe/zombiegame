#pragma once

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UnitStatusBarWidget.generated.h"

class UHorizontalBox;
class UImage;

UCLASS()
class ZOMBIEGAME_API UUnitStatusBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHealthAndArmor(
		int32 CurrentHealth,
		int32 MaxHealth,
		int32 CurrentArmor,
		int32 MaxArmor);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HorizontalBox_HealthChunks;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HorizontalBox_ArmorChunks;

private:
	void RebuildChunks(
		UHorizontalBox* Box,
		int32 CurrentValue,
		int32 MaxValue,
		const FLinearColor& FilledColor,
		const FLinearColor& EmptyColor);
};