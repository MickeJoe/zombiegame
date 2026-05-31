#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShootableTargetIconBarWidget.generated.h"

class AStrategyUnit;
class UHorizontalBox;
class UShootableTargetIconWidget;

UCLASS()
class UShootableTargetIconBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category="Targeting")
	void SetTargets(
		const TArray<AStrategyUnit*>& Targets,
		TSubclassOf<UShootableTargetIconWidget> IconWidgetClass);

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UHorizontalBox> IconBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targeting")
	FMargin IconPadding = FMargin(4.0f, 0.0f);
};
