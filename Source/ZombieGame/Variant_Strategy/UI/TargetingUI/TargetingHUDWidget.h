#pragma once

#include "CoreMinimal.h"
#include "TargetingActionBarWidget.h"
#include "Blueprint/UserWidget.h"
#include "TargetingHUDWidget.generated.h"

class UTargetingActionBarWidget;

UCLASS()
class UTargetingHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Show();
	void Hide();

	void SetTargetData(
		const FText& TargetName,
		int32 Health,
		int32 MaxHealth,
		int32 Armor,
		int32 HitChance,
		int32 CritChance
	);
	
	UPROPERTY()
	FOnTargetingActionClicked OnCycleTargetClicked;
	
	UPROPERTY()
	FOnTargetingActionClicked OnCancelClicked;
	
	UPROPERTY()
	FOnTargetingActionClicked OnFireClicked;	

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTargetingActionBarWidget> WBP_TargetingActionBar;	
	
	UFUNCTION()
	void HandleCycleTargetClicked();
	
	UFUNCTION()
	void HandleCancelClicked();
	
	UFUNCTION()
	void HandleFireClicked();	
};