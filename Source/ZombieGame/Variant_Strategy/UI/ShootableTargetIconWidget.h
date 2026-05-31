#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShootableTargetIconWidget.generated.h"

class UImage;
class UTexture2D;

UCLASS()
class UShootableTargetIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category="Targeting")
	void SetTargetIcon(UTexture2D* IconTexture);

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> TargetImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targeting")
	FVector2D IconSize = FVector2D(32.0f, 32.0f);
};
