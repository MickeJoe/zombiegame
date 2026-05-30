// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyHUD.h"
#include "StrategyUnit.h"
#include "StrategyPlayerController.h"
#include "Kismet/GameplayStatics.h"

void AStrategyHUD::BeginPlay()
{
	Super::BeginPlay();
}

void AStrategyHUD::DragSelectUpdate(FVector2D Start, FVector2D WidthAndHeight, FVector2D CurrentPosition, bool bDraw)
{
	// copy the selection box data
	bDrawBox = bDraw;
	BoxStart = Start;
	BoxSize = WidthAndHeight;
	BoxCurrentPosition = CurrentPosition;

}

void AStrategyHUD::DrawHUD()
{
	// draw all debug information, etc.
	Super::DrawHUD();

	// ensure we have a valid player controller
	if (AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetOwningPlayerController()))
	{
		// draw the selection box
		if (bDrawBox)
		{
			DrawRect(SelectionBoxColor, BoxStart.X, BoxStart.Y, BoxSize.X, BoxSize.Y);

			// Project unit origins instead of actor bounds so visual mesh offsets do not affect selection.
			TArray<AStrategyUnit*> BoxedUnits;
			TArray<AActor*> FoundActors;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStrategyUnit::StaticClass(), FoundActors);

			const float MinX = FMath::Min(BoxStart.X, BoxCurrentPosition.X);
			const float MaxX = FMath::Max(BoxStart.X, BoxCurrentPosition.X);
			const float MinY = FMath::Min(BoxStart.Y, BoxCurrentPosition.Y);
			const float MaxY = FMath::Max(BoxStart.Y, BoxCurrentPosition.Y);

			for (AActor* Actor : FoundActors)
			{
				AStrategyUnit* Unit = Cast<AStrategyUnit>(Actor);
				if (!Unit || Unit->GetStrategyUnitTeam() != EStrategyUnitTeam::Human)
				{
					continue;
				}

				FVector2D ScreenLocation;
				if (PC->ProjectWorldLocationToScreen(Unit->GetActorLocation(), ScreenLocation)
					&& ScreenLocation.X >= MinX
					&& ScreenLocation.X <= MaxX
					&& ScreenLocation.Y >= MinY
					&& ScreenLocation.Y <= MaxY)
				{
					BoxedUnits.Add(Unit);
				}
			}

			// update the unit selection on the player controller
			PC->DragSelectUnits(BoxedUnits);
		}

		// get the currently selected units
		TArray<AStrategyUnit*> SelectedUnits = PC->GetSelectedUnits();

	}

}
