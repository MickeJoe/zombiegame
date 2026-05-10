#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerUnitRosterWidget.generated.h"

class AStrategyUnit;
class UBorder;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

USTRUCT(BlueprintType)
struct FPlayerUnitRosterEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AStrategyUnit> Unit = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentHealth = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxHealth = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentActionPoints = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxActionPoints = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bSelected = false;
};

UCLASS()
class ZOMBIEGAME_API UPlayerUnitRosterCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable)
	void SetEntry(const FPlayerUnitRosterEntry& InEntry);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Background;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Health;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ActionPoints;

private:
	FPlayerUnitRosterEntry Entry;

	void BuildDefaultLayout();
	void Refresh();
};

UCLASS()
class ZOMBIEGAME_API UPlayerUnitRosterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable)
	void SetUnits(const TArray<AStrategyUnit*>& Units, const TArray<AStrategyUnit*>& SelectedUnits);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayerUnitRosterCardWidget> CardWidgetClass;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_Units;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_RosterBackground;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_EmptyState;

private:
	void BuildDefaultLayout();
};
