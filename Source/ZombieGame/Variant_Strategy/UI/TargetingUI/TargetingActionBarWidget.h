#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetingActionBarWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetingActionClicked);

UCLASS()
class UTargetingActionBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FOnTargetingActionClicked OnFireClicked;

	UPROPERTY()
	FOnTargetingActionClicked OnCancelClicked;

	UPROPERTY()
	FOnTargetingActionClicked OnCycleTargetClicked;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Fire;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Cancel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_CycleTarget;

private:
	UFUNCTION()
	void HandleFireClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleCycleTargetClicked();
};
