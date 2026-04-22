#include "TurnBannerWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UTurnBannerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Hidden);
}

void UTurnBannerWidget::ShowTurnBanner(ETurnOwner TurnOwner)
{
	if (!Image_Banner /*|| !Text_Turn*/)
	{
		return;
	}

	switch (TurnOwner)
	{
	case ETurnOwner::Player:
		//Text_Turn->SetText(FText::FromString(TEXT("PLAYER TURN")));

		if (PlayerTurnTexture)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(PlayerTurnTexture);
			Image_Banner->SetBrush(Brush);
		}
		break;

	case ETurnOwner::Enemy:
		//Text_Turn->SetText(FText::FromString(TEXT("ENEMY TURN")));

		if (EnemyTurnTexture)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(EnemyTurnTexture);
			Image_Banner->SetBrush(Brush);
		}
		break;
	}

	SetVisibility(ESlateVisibility::Visible);
/*
	if (FadeAnim)
	{
		PlayAnimation(FadeAnim, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f);
	}
	*/
}