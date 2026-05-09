#pragma once

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetInfoWidget.generated.h"

class UTextBlock;
class UUnitStatusBarWidget;
class AStrategyUnit;

UCLASS()
class UTargetInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetTarget(AStrategyUnit* InTarget);

	UFUNCTION(BlueprintCallable)
	void SetHitChance(int32 InHitChance);

	UFUNCTION(BlueprintCallable)
	void SetCritChance(int32 InCritChance);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_TargetName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_HitChance;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CritChance;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUnitStatusBarWidget> UnitStatusBar;

private:
	UPROPERTY()
	TObjectPtr<AStrategyUnit> Target;
};
