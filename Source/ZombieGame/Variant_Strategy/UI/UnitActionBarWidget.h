#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Player/PlayerUnitActionType.h"

#include "UnitActionBarWidget.generated.h"

class UButton;

USTRUCT(BlueprintType)
struct FUnitActionButtonData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPlayerUnitActionType ActionType = EPlayerUnitActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisabledReason;
};

UCLASS()
class UUnitActionBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable)
	void SetActions(const TArray<FUnitActionButtonData>& Actions);

	UFUNCTION(BlueprintCallable)
	void OnActionClicked(EPlayerUnitActionType ActionType);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitActionClicked, EPlayerUnitActionType, ActionType);

	UPROPERTY(BlueprintAssignable)
	FOnUnitActionClicked OnUnitActionClicked;
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Melee;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Fire;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Reload;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Hunker;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Overwatch;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Skip;
};