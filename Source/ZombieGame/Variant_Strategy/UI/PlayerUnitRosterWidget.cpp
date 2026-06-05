#include "PlayerUnitRosterWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/Unit/UnitData.h"
#include "Input/Reply.h"
#include "StrategyUnit.h"
#include "Widgets/SWidget.h"

namespace
{
	const FLinearColor CardBackground(0.025f, 0.035f, 0.045f, 0.84f);
	const FLinearColor CardSelectedBackground(0.025f, 0.09f, 0.12f, 0.92f);
	const FLinearColor PortraitFallback(0.06f, 0.09f, 0.11f, 1.0f);
	const FLinearColor TextWhite(0.92f, 0.95f, 0.98f, 1.0f);
	const FLinearColor TextMuted(0.74f, 0.78f, 0.82f, 1.0f);
	const FLinearColor AccentBlue(0.0f, 0.72f, 1.0f, 1.0f);
	const FLinearColor AccentYellow(1.0f, 0.72f, 0.02f, 1.0f);
	const FLinearColor HealthGreen(0.18f, 0.95f, 0.38f, 1.0f);
	const FLinearColor HealthEmpty(0.18f, 0.95f, 0.38f, 0.20f);
	const FLinearColor ActionBlue(0.0f, 0.74f, 1.0f, 1.0f);
	const FLinearColor ActionEmpty(0.0f, 0.74f, 1.0f, 0.20f);

	void SetTextStyle(UTextBlock* TextBlock, const FLinearColor& Color, int32 FontSize)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetColorAndOpacity(Color);
		TextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor::Black);

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
	}

	void AddSegmentBar(
		UWidgetTree* WidgetTree,
		UVerticalBox* Parent,
		int32 CurrentValue,
		int32 MaxValue,
		const FLinearColor& FilledColor,
		const FLinearColor& EmptyColor)
	{
		if (!WidgetTree || !Parent || MaxValue <= 0)
		{
			return;
		}

		USizeBox* BarHeight = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		BarHeight->SetHeightOverride(8.0f);

		UHorizontalBox* Bar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		BarHeight->AddChild(Bar);

		const int32 ClampedCurrent = FMath::Clamp(CurrentValue, 0, MaxValue);
		for (int32 Index = 0; Index < MaxValue; ++Index)
		{
			UImage* Segment = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Segment->SetColorAndOpacity(Index < ClampedCurrent ? FilledColor : EmptyColor);

			UHorizontalBoxSlot* SegmentSlot = Bar->AddChildToHorizontalBox(Segment);
			if (SegmentSlot)
			{
				SegmentSlot->SetPadding(FMargin(1.0f, 0.0f));
				SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}

		UVerticalBoxSlot* BarSlot = Parent->AddChildToVerticalBox(BarHeight);
		if (BarSlot)
		{
			BarSlot->SetPadding(FMargin(0.0f, 1.0f, 0.0f, 3.0f));
			BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}
}

void UPlayerUnitRosterCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && (!Border_Background || !Image_Icon || !Text_Name || !Text_Health || !Text_ActionPoints))
	{
		BuildDefaultLayout();
	}
}

void UPlayerUnitRosterCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);

	if (WidgetTree && (!Border_Background || !Image_Icon || !Text_Name || !Text_Health || !Text_ActionPoints))
	{
		BuildDefaultLayout();
	}

	Refresh();
}

TSharedRef<SWidget> UPlayerUnitRosterCardWidget::RebuildWidget()
{
	if (WidgetTree && (!Border_Background || !Image_Icon || !Text_Name || !Text_Health || !Text_ActionPoints))
	{
		BuildDefaultLayout();
	}

	return Super::RebuildWidget();
}

void UPlayerUnitRosterCardWidget::SetEntry(const FPlayerUnitRosterEntry& InEntry)
{
	Entry = InEntry;
	Refresh();
}

FReply UPlayerUnitRosterCardWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && IsValid(Entry.Unit))
	{
		OnUnitClicked.Broadcast(Entry.Unit);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPlayerUnitRosterCardWidget::BuildDefaultLayout()
{
	Border_Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Background"));
	WidgetTree->RootWidget = Border_Background;
	Border_Background->SetPadding(FMargin(5.0f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_Row"));
	Border_Background->SetContent(Row);

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_Icon"));
	IconSize->SetWidthOverride(64.0f);
	IconSize->SetHeightOverride(64.0f);

	Image_Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_Icon"));
	IconSize->AddChild(Image_Icon);

	UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconSize);
	if (IconSlot)
	{
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	UVerticalBox* TextStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_TextStack"));
	UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextStack);
	if (TextSlot)
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	Text_Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Name"));
	TextStack->AddChildToVerticalBox(Text_Name);

	UHorizontalBox* HealthRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_Health"));
	TextStack->AddChildToVerticalBox(HealthRow);

	Text_Health = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Health"));
	Text_Health->SetText(FText::FromString(TEXT("HP")));
	if (UHorizontalBoxSlot* HealthLabelSlot = HealthRow->AddChildToHorizontalBox(Text_Health))
	{
		HealthLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		HealthLabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	Text_HealthValue = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_HealthValue"));
	HealthRow->AddChildToHorizontalBox(Text_HealthValue);

	UHorizontalBox* ActionPointRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_ActionPoints"));
	TextStack->AddChildToVerticalBox(ActionPointRow);

	Text_ActionPoints = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_ActionPoints"));
	Text_ActionPoints->SetText(FText::FromString(TEXT("TU")));
	if (UHorizontalBoxSlot* ActionLabelSlot = ActionPointRow->AddChildToHorizontalBox(Text_ActionPoints))
	{
		ActionLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		ActionLabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	Text_ActionPointsValue = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_ActionPointsValue"));
	ActionPointRow->AddChildToHorizontalBox(Text_ActionPointsValue);
}

void UPlayerUnitRosterCardWidget::Refresh()
{
	if (Border_Background)
	{
		Border_Background->SetBrushColor(Entry.bSelected ? CardSelectedBackground : CardBackground);
	}

	if (Image_Icon)
	{
		if (Entry.Icon)
		{
			Image_Icon->SetBrushFromTexture(Entry.Icon);
			Image_Icon->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			Image_Icon->SetColorAndOpacity(PortraitFallback);
		}
	}

	if (Text_Name)
	{
		Text_Name->SetText(Entry.DisplayName.IsEmpty() ? FText::FromString(TEXT("UNIT")) : Entry.DisplayName);
		SetTextStyle(Text_Name, AccentBlue, 15);
	}

	if (Text_Health)
	{
		Text_Health->SetText(FText::FromString(TEXT("HP")));
		SetTextStyle(Text_Health, TextWhite, 13);
	}

	if (Text_HealthValue)
	{
		Text_HealthValue->SetText(FText::Format(
			NSLOCTEXT("PlayerUnitRoster", "HealthValueFormat", "{0}/{1}"),
			FText::AsNumber(Entry.CurrentHealth),
			FText::AsNumber(Entry.MaxHealth)));
		SetTextStyle(Text_HealthValue, AccentYellow, 13);
	}
	else if (Text_Health)
	{
		Text_Health->SetText(FText::Format(
			NSLOCTEXT("PlayerUnitRoster", "HealthFormat", "HP {0}/{1}"),
			FText::AsNumber(Entry.CurrentHealth),
			FText::AsNumber(Entry.MaxHealth)));
	}

	if (Text_ActionPoints)
	{
		Text_ActionPoints->SetText(FText::FromString(TEXT("TU")));
		SetTextStyle(Text_ActionPoints, TextWhite, 13);
	}

	if (Text_ActionPointsValue)
	{
		Text_ActionPointsValue->SetText(FText::Format(
			NSLOCTEXT("PlayerUnitRoster", "ActionPointValueFormat", "{0}/{1}"),
			FText::AsNumber(Entry.CurrentTimeUnits),
			FText::AsNumber(Entry.MaxTimeUnits)));
		SetTextStyle(Text_ActionPointsValue, AccentYellow, 13);
	}
	else if (Text_ActionPoints)
	{
		Text_ActionPoints->SetText(FText::Format(
			NSLOCTEXT("PlayerUnitRoster", "ActionPointFormat", "TU {0}/{1}"),
			FText::AsNumber(Entry.CurrentTimeUnits),
			FText::AsNumber(Entry.MaxTimeUnits)));
	}
}

void UPlayerUnitRosterWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !VerticalBox_Units)
	{
		BuildDefaultLayout();
	}
}

void UPlayerUnitRosterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);

	if (WidgetTree && !VerticalBox_Units)
	{
		BuildDefaultLayout();
	}
}

TSharedRef<SWidget> UPlayerUnitRosterWidget::RebuildWidget()
{
	if (WidgetTree && !VerticalBox_Units)
	{
		BuildDefaultLayout();
	}

	return Super::RebuildWidget();
}

void UPlayerUnitRosterWidget::SetUnits(
	const TArray<AStrategyUnit*>& Units,
	const TArray<AStrategyUnit*>& SelectedUnits)
{
	if (!VerticalBox_Units)
	{
		if (WidgetTree)
		{
			BuildDefaultLayout();
		}
		else
		{
			return;
		}
	}

	VerticalBox_Units->ClearChildren();

	if (Units.Num() <= 0)
	{
		if (!Text_EmptyState && WidgetTree)
		{
			Text_EmptyState = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_EmptyState"));
			Text_EmptyState->SetText(NSLOCTEXT("PlayerUnitRoster", "EmptyState", "NO PLAYER UNITS"));
			SetTextStyle(Text_EmptyState, AccentBlue, 16);
		}

		if (Text_EmptyState)
		{
			VerticalBox_Units->AddChildToVerticalBox(Text_EmptyState);
		}

		return;
	}

	for (AStrategyUnit* Unit : Units)
	{
		if (!IsValid(Unit) || Unit->GetCurrentHealth() <= 0)
		{
			continue;
		}

		TSubclassOf<UPlayerUnitRosterCardWidget> WidgetClass = CardWidgetClass;
		if (!WidgetClass)
		{
			WidgetClass = UPlayerUnitRosterCardWidget::StaticClass();
		}

		UPlayerUnitRosterCardWidget* Card = CreateWidget<UPlayerUnitRosterCardWidget>(GetOwningPlayer(), WidgetClass);
		if (!Card)
		{
			continue;
		}

		FPlayerUnitRosterEntry Entry;
		Entry.Unit = Unit;
		Entry.DisplayName = Unit->UnitData && !Unit->UnitData->DisplayName.IsEmpty()
			? Unit->UnitData->DisplayName
			: FText::FromName(Unit->GetFName());
		Entry.Icon = Unit->UnitData ? Unit->UnitData->Icon : nullptr;
		Entry.CurrentHealth = Unit->GetCurrentHealth();
		Entry.MaxHealth = Unit->GetMaxHealth();
		Entry.CurrentTimeUnits = Unit->GetRemainingTimeUnits();
		Entry.MaxTimeUnits = Unit->GetMaxTimeUnits();
		Entry.bSelected = SelectedUnits.Contains(Unit);

		Card->SetEntry(Entry);
		Card->OnUnitClicked.AddDynamic(this, &UPlayerUnitRosterWidget::HandleCardUnitClicked);

		UVerticalBoxSlot* CardSlot = VerticalBox_Units->AddChildToVerticalBox(Card);
		if (CardSlot)
		{
			CardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
			CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}
}

void UPlayerUnitRosterWidget::HandleCardUnitClicked(AStrategyUnit* Unit)
{
	OnUnitClicked.Broadcast(Unit);
}

void UPlayerUnitRosterWidget::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_Root"));
	WidgetTree->RootWidget = Root;

	Border_RosterBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_RosterBackground"));
	Border_RosterBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	Border_RosterBackground->SetPadding(FMargin(0.0f));

	VerticalBox_Units = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_Units"));
	Border_RosterBackground->SetContent(VerticalBox_Units);

	UCanvasPanelSlot* RosterSlot = Root->AddChildToCanvas(Border_RosterBackground);
	if (RosterSlot)
	{
		RosterSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		RosterSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		RosterSlot->SetPosition(FVector2D(0.0f, 0.0f));
		RosterSlot->SetSize(FVector2D(236.0f, 360.0f));
	}

	Text_EmptyState = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_EmptyState"));
	Text_EmptyState->SetText(NSLOCTEXT("PlayerUnitRoster", "EmptyState", "NO PLAYER UNITS"));
	SetTextStyle(Text_EmptyState, AccentBlue, 16);
	VerticalBox_Units->AddChildToVerticalBox(Text_EmptyState);
}
