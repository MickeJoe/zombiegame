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
	SetVisibility(ESlateVisibility::HitTestInvisible);

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

	Text_Health = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Health"));
	TextStack->AddChildToVerticalBox(Text_Health);

	Text_ActionPoints = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_ActionPoints"));
	TextStack->AddChildToVerticalBox(Text_ActionPoints);
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
		SetTextStyle(Text_Name, TextWhite, 15);
	}

	if (Text_Health)
	{
		Text_Health->SetText(FText::Format(
			NSLOCTEXT("PlayerUnitRoster", "HealthFormat", "HP {0}/{1}"),
			FText::AsNumber(Entry.CurrentHealth),
			FText::AsNumber(Entry.MaxHealth)));
		SetTextStyle(Text_Health, TextMuted, 13);
	}

	if (Text_ActionPoints)
	{
		Text_ActionPoints->SetText(FText::Format(
			NSLOCTEXT("PlayerUnitRoster", "ActionPointFormat", "AP {0}/{1}"),
			FText::AsNumber(Entry.CurrentActionPoints),
			FText::AsNumber(Entry.MaxActionPoints)));
		SetTextStyle(Text_ActionPoints, AccentBlue, 13);
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
	SetVisibility(ESlateVisibility::HitTestInvisible);

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

		UBorder* RowBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		RowBackground->SetPadding(FMargin(5.0f));
		RowBackground->SetBrushColor(SelectedUnits.Contains(Unit) ? CardSelectedBackground : CardBackground);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		RowBackground->SetContent(Row);

		USizeBox* PortraitBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		PortraitBox->SetWidthOverride(64.0f);
		PortraitBox->SetHeightOverride(64.0f);

		UImage* PortraitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (Unit->UnitData && Unit->UnitData->Icon)
		{
			PortraitImage->SetBrushFromTexture(Unit->UnitData->Icon);
			PortraitImage->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			PortraitImage->SetColorAndOpacity(PortraitFallback);
		}
		PortraitBox->AddChild(PortraitImage);

		UHorizontalBoxSlot* PortraitSlot = Row->AddChildToHorizontalBox(PortraitBox);
		if (PortraitSlot)
		{
			PortraitSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			PortraitSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			PortraitSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBox* InfoStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UHorizontalBoxSlot* InfoSlot = Row->AddChildToHorizontalBox(InfoStack);
		if (InfoSlot)
		{
			InfoSlot->SetVerticalAlignment(VAlign_Center);
			InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		const FText UnitName = Unit->UnitData && !Unit->UnitData->DisplayName.IsEmpty()
			? Unit->UnitData->DisplayName
			: FText::FromName(Unit->GetFName());

		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		NameText->SetText(UnitName);
		SetTextStyle(NameText, TextWhite, 15);
		InfoStack->AddChildToVerticalBox(NameText);

		UTextBlock* HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HealthText->SetText(FText::Format(
			NSLOCTEXT("PlayerUnitRoster", "HealthFormat", "HP {0}/{1}"),
			FText::AsNumber(Unit->GetCurrentHealth()),
			FText::AsNumber(Unit->GetMaxHealth())));
		SetTextStyle(HealthText, TextMuted, 13);
		InfoStack->AddChildToVerticalBox(HealthText);

		AddSegmentBar(
			WidgetTree,
			InfoStack,
			Unit->GetCurrentHealth(),
			Unit->GetMaxHealth(),
			HealthGreen,
			HealthEmpty);

		UTextBlock* ActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ActionText->SetText(FText::Format(
			NSLOCTEXT("PlayerUnitRoster", "ActionPointFormat", "AP {0}/{1}"),
			FText::AsNumber(Unit->GetRemainingActionPoints()),
			FText::AsNumber(Unit->GetMaxActionPoints())));
		SetTextStyle(ActionText, AccentBlue, 13);
		InfoStack->AddChildToVerticalBox(ActionText);

		AddSegmentBar(
			WidgetTree,
			InfoStack,
			Unit->GetRemainingActionPoints(),
			Unit->GetMaxActionPoints(),
			ActionBlue,
			ActionEmpty);

		UVerticalBoxSlot* CardSlot = VerticalBox_Units->AddChildToVerticalBox(RowBackground);
		if (CardSlot)
		{
			CardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
			CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}
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
