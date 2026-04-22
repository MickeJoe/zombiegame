#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TurnBannerWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class UWidgetAnimation;

UENUM(BlueprintType)
enum class ETurnOwner : uint8
{
	Player,
	Enemy
};

UCLASS()
class UTurnBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ShowTurnBanner(ETurnOwner TurnOwner);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Banner;

//	UPROPERTY(meta = (BindWidget))
//	TObjectPtr<UTextBlock> Text_Turn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Turn Banner")
	TObjectPtr<UTexture2D> PlayerTurnTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Turn Banner")
	TObjectPtr<UTexture2D> EnemyTurnTexture;

//	UPROPERTY(Transient, meta = (BindWidgetAnim))
//	TObjectPtr<UWidgetAnimation> FadeAnim;
};