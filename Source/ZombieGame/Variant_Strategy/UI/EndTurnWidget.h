#pragma once

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EndTurnWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndTurnClicked);

UCLASS()
class UEndTurnWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnEndTurnClicked OnEndTurnClicked;

	void SetPlayerEndTurnButtonEnabled(bool bIsEnabledIn);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_EndTurn;
	
	UFUNCTION()
	void HandleEndTurnClicked();
};