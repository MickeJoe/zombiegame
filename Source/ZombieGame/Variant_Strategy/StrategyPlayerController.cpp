// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "StrategyPawn.h"
#include "Camera/CameraComponent.h"
#include "InputActionValue.h"
#include "StrategyHUD.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "StrategyUnit.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "StrategyGameMode.h"
#include "StrategyCheatManager.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Player/AIStrategySide.h"
#include "Player/PlayerStrategySide.h"
#include "Systems/GridManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/Weapon/AttackStats.h"
#include "Data/Weapon/StrategyWeaponDatabase.h"
#include "Data/Weapon/StrategyWeaponData.h"
#include "Blueprint/UserWidget.h"
#include "Systems/AttackHandling/StrategyAttackResolver.h"
#include "TargetingUI/TargetingHUDWidget.h"

#include "UI/EndTurnWidget.h"
#include "UI/PlayerUnitRosterWidget.h"
#include "UI/ShootableTargetIconBarWidget.h"
#include "UI/ShootableTargetIconWidget.h"
#include "UI/UnitActionBarWidget.h"
#include "UI/WeaponDebugSlateWidget.h"
#include "UI/WeaponInfoSlateWidget.h"
#include "UI/TargetingUI//StrategyTargetingComponent.h"
#include "ZombieGame.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	static TAutoConsoleVariable<int32> CVarStrategyPlayerOverwatchDebug(
		TEXT("zg.OverwatchDebug"),
		1,
		TEXT("Logs overwatch placement and decal projection diagnostics."));

	bool IsStrategyPlayerOverwatchDebugEnabled()
	{
		return CVarStrategyPlayerOverwatchDebug.GetValueOnGameThread() != 0;
	}
}

PRAGMA_DISABLE_OPTIMIZATION

AStrategyPlayerController::AStrategyPlayerController()
{
	// mouse cursor should always be shown
	bShowMouseCursor = true;

	GridManager = Cast<AGridManager>(
		UGameplayStatics::GetActorOfClass(this, AGridManager::StaticClass())
	);

	HighlightActor = Cast<AGridHighlightActor>(
	UGameplayStatics::GetActorOfClass(this, AGridHighlightActor::StaticClass())
	);
	
	TargetingComponent = CreateDefaultSubobject<UStrategyTargetingComponent>(
		TEXT("TargetingComponent")
	);

	CheatClass = UStrategyCheatManager::StaticClass();
}

void AStrategyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	EnsureTargetingComponent();

	if (IsLocalController() && TurnBannerWidgetClass)
	{
		TurnBannerWidget = CreateWidget<UTurnBannerWidget>(this, TurnBannerWidgetClass);
		if (TurnBannerWidget)
		{
			TurnBannerWidget->AddToViewport(100);
		}
	}

	if (EndTurnWidgetClass)
	{
		EndTurnWidget = CreateWidget<UEndTurnWidget>(this, EndTurnWidgetClass);
		if (EndTurnWidget)
		{
			EndTurnWidget->AddToViewport(1000);
			EndTurnWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			EndTurnWidget->OnEndTurnClicked.AddDynamic(
				this,
				&AStrategyPlayerController::HandleEndTurnClicked
			);
		}
	}
	
	if (UnitActionBarWidgetClass)
	{
		UnitActionBarWidget = CreateWidget<UUnitActionBarWidget>(this, UnitActionBarWidgetClass);
		if (UnitActionBarWidget)
		{
			UnitActionBarWidget->AddToViewport(1200);
			UnitActionBarWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			UnitActionBarWidget->OnUnitActionClicked.AddDynamic(
				this,
				&AStrategyPlayerController::HandleUnitActionClicked);
		}
	}

	TSubclassOf<UPlayerUnitRosterWidget> RosterWidgetClass = PlayerUnitRosterWidgetClass;
	if (!RosterWidgetClass)
	{
		RosterWidgetClass = UPlayerUnitRosterWidget::StaticClass();
	}

	PlayerUnitRosterWidget = CreateWidget<UPlayerUnitRosterWidget>(this, RosterWidgetClass);
	if (PlayerUnitRosterWidget)
	{
		PlayerUnitRosterWidget->AddToViewport(1100);
		PlayerUnitRosterWidget->SetVisibility(ESlateVisibility::Visible);
		PlayerUnitRosterWidget->SetAlignmentInViewport(FVector2D(0.0f, 0.0f));
		PlayerUnitRosterWidget->SetPositionInViewport(FVector2D(16.0f, 16.0f), false);
		PlayerUnitRosterWidget->SetDesiredSizeInViewport(FVector2D(236.0f, 360.0f));
		PlayerUnitRosterWidget->OnUnitClicked.AddDynamic(
			this,
			&AStrategyPlayerController::HandleRosterUnitClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerUnitRoster: widget creation failed"));
	}
	
	if (TargetingHUDClass)
	{
		TargetingHUD = CreateWidget<UTargetingHUDWidget>(this, TargetingHUDClass);
		if (TargetingHUD)
		{
			TargetingHUD->AddToViewport(1300);
			TargetingHUD->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (auto* GM = GetWorld()->GetAuthGameMode<AStrategyGameMode>())
	{
		GM->OnMatchReady.Broadcast();
	}

	RefreshPlayerUnitRoster();
	RefreshWeaponInfoPanel();

	GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &AStrategyPlayerController::RefreshPlayerUnitRoster));
}

void AStrategyPlayerController::HandleEndTurnClicked()
{
	if (AStrategyGameMode* GM = GetWorld()->GetAuthGameMode<AStrategyGameMode>())
	{
		GM->EndTurn();
	}
}

void AStrategyPlayerController::ShowTurnBanner(ETurnOwner TurnOwner)
{
	if (TurnBannerWidget)
	{
		TurnBannerWidget->ShowTurnBanner(TurnOwner);
	}
}

void AStrategyPlayerController::SetPlayerEndTurnButtonEnabled(bool bIsEnabledIn)
{
	EndTurnWidget->SetPlayerEndTurnButtonEnabled(bIsEnabledIn);
}

void AStrategyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only set up input on local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			// choose the context based on the input mode
			UInputMappingContext* ChosenContext = nullptr;

			switch (InputMode)
			{
			case SIM_Mouse:
				ChosenContext = MouseMappingContext;
				break;
			case SIM_Touch:
				ChosenContext = TouchMappingContext;
				break;
			}

			Subsystem->AddMappingContext(ChosenContext, 0);
		}

		// bind the input mappings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			// Camera
			EnhancedInputComponent->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::MoveCamera);
			EnhancedInputComponent->BindAction(ZoomCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::ZoomCamera);
			EnhancedInputComponent->BindAction(ResetCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::ResetCamera);

			// Mouse Interaction
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::SelectHoldStarted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::SelectHoldTriggered);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectHoldCompleted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::SelectHoldCompleted);

			EnhancedInputComponent->BindAction(SelectClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClick);

			EnhancedInputComponent->BindAction(SelectionModifierAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::SelectionModifier);
			EnhancedInputComponent->BindAction(SelectionModifierAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectionModifier);
			EnhancedInputComponent->BindAction(SelectionModifierAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::SelectionModifier);

			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::InteractHoldStarted);
			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::InteractHoldTriggered);

			EnhancedInputComponent->BindAction(InteractClickAction, ETriggerEvent::Started, this, &AStrategyPlayerController::InteractClickStarted);
			EnhancedInputComponent->BindAction(InteractClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::InteractClickCompleted);

			// Touch Interaction
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::TouchPrimaryHoldStarted);
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::TouchPrimaryHoldTriggered);
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TouchPrimaryHoldCompleted);

			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Started, this, &AStrategyPlayerController::TouchSecondaryStarted);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::TouchSecondaryTriggered);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TouchSecondaryCompleted);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::TouchSecondaryCompleted);

		}

		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AStrategyPlayerController::EscapePressed);
	}
}

void AStrategyPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalController() && bEnableWeaponDebugMenu)
	{
		const bool bIsWeaponDebugHotkeyDown = IsInputKeyDown(EKeys::I);
		if (bIsWeaponDebugHotkeyDown && !bWasWeaponDebugHotkeyDown)
		{
			ToggleWeaponDebugMenu();
		}
		bWasWeaponDebugHotkeyDown = bIsWeaponDebugHotkeyDown;
	}

	if (bIsPlacingOverwatch)
	{
		UpdateOverwatchPlacementPreview();
	}
	else
	{
		UpdateMovementPreview();
	}

	UpdateShootTargetHoverIndicator();
}

void AStrategyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// ensure we have the right pawn type
	ControlledPawn = Cast<AStrategyPawn>(InPawn);
	check(ControlledPawn);

	// set the zoom level from the pawn's camera
	DefaultZoom = CameraZoom = ControlledPawn->GetCamera()->OrthoWidth;

	// cast the HUD pointer
	StrategyHUD = Cast<AStrategyHUD>(GetHUD());
	check(StrategyHUD);
}

void AStrategyPlayerController::DragSelectUnits(const TArray<AStrategyUnit*>& Units)
{
	// do we have units in the list?
	if (Units.Num() > 0)
	{
		// ensure any previous units are deselected
		DoDeselectAllCommand();

		// select each new unit
		for (AStrategyUnit* CurrentUnit : Units)
		{
			// add the unit to the selection list
			ControlledUnits.Add(CurrentUnit);

			// select the unit
			CurrentUnit->UnitSelected();
		}

	}
	else
	{

		// release any currently selected units since nothing is on the box
		if (ControlledUnits.Num() > 0)
		{
			DoDeselectAllCommand();
		}

	}

	RefreshPlayerUnitRoster();
	RefreshWeaponInfoPanel();
}

const TArray<AStrategyUnit*>& AStrategyPlayerController::GetSelectedUnits()
{
	return ControlledUnits;
}

UStrategyTargetingComponent* AStrategyPlayerController::GetTargetingComponent()
{
	return EnsureTargetingComponent();
}

void AStrategyPlayerController::MoveCamera(const FInputActionValue& Value)
{
	FVector2D InputVector = Value.Get<FVector2D>();

	// get the forward input component vector
	FRotator ForwardRot = GetControlRotation();
	ForwardRot.Pitch = 0.0f;

	// get the right input component vector
	FRotator RightRot = GetControlRotation();
	ForwardRot.Pitch = 0.0f;
	ForwardRot.Roll = 0.0f;

	// add the forward input
	ControlledPawn->AddMovementInput(ForwardRot.RotateVector(FVector::ForwardVector), InputVector.X + InputVector.Y);

	// add the right input
	ControlledPawn->AddMovementInput(RightRot.RotateVector(FVector::RightVector), InputVector.X - InputVector.Y);

}

void AStrategyPlayerController::ZoomCamera(const FInputActionValue& Value)
{
	// scale the input and subtract from the current zoom level
	float ZoomLevel = CameraZoom - (Value.Get<float>() * ZoomScaling);

	// clamp to min/max zoom levels
	CameraZoom = FMath::Clamp(ZoomLevel, MinZoomLevel, MaxZoomLevel);

	// update the pawn's camera
	ControlledPawn->SetZoomModifier(CameraZoom);

}

void AStrategyPlayerController::ResetCamera(const FInputActionValue& Value)
{
	// reset zoom level to its initial value
	CameraZoom = DefaultZoom;

	// update the pawn's camera
	ControlledPawn->SetZoomModifier(DefaultZoom);

}

void AStrategyPlayerController::SelectHoldStarted(const FInputActionValue& Value)
{
	// save the selection start position
	StartingSelectionPosition = GetMouseLocation();

}

void AStrategyPlayerController::SelectHoldTriggered(const FInputActionValue& Value)
{

	// get the current mouse position
	FVector2D SelectionPosition = GetMouseLocation();

	// calculate the size of the selection box
	FVector2D SelectionSize = SelectionPosition - StartingSelectionPosition;

	// update the selection box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(StartingSelectionPosition, SelectionSize, SelectionPosition, true);
	}
}

void AStrategyPlayerController::SelectHoldCompleted(const FInputActionValue& Value)
{
	// reset the drag box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

void AStrategyPlayerController::SelectClick(const FInputActionValue& Value)
{
	if (bIsPlacingOverwatch)
	{
		ConfirmOverwatchPlacement();
		return;
	}

	if (GetWorld() && GetWorld()->GetRealTimeSeconds() < IgnoreSelectionInputUntilTime)
	{
		return;
	}

	/*
	if (HighlightActor)
	{
		HighlightActor->ClearReachableHighlights();
	}
*/
	if (GetLocationUnderCursor(CachedSelection))
	{
		CachedInteraction = CachedSelection;

		if (TryAttackHoveredEnemy())
		{
			return;
		}

		if (!GetHoveredStrategyUnit() && ControlledUnits.Num() > 0 && GridManager)
		{
			const FIntPoint ClickedCell = GridManager->WorldToGrid(CachedInteraction);
			AStrategyUnit* MoveUnit = ControlledUnits.Num() == 1 ? ControlledUnits[0] : TargetUnit;
			if (IsValid(MoveUnit) && ReachableCells.Contains(ClickedCell))
			{
				int32 MoveCost = 0;
				if (GridManager->TryGetMoveCostCells(MoveUnit, ClickedCell, MoveCost)
					&& MoveCost <= MoveUnit->GetRemainingTimeUnits())
				{
					if (bHasPendingMoveDestination
						&& PendingMoveCell == ClickedCell
						&& PendingMoveUnit == MoveUnit)
					{
						DoMoveUnitsCommand();
						return;
					}

					PendingMoveCell = ClickedCell;
					PendingMoveUnit = MoveUnit;
					bHasPendingMoveDestination = true;

					if (HighlightActor)
					{
						HighlightActor->ShowMovementDestination(
							GridManager,
							ClickedCell,
							MoveUnit->GetRemainingTimeUnits() - MoveCost);
					}

					return;
				}
			}
		}

		DoSelectionCommand();
	}
}

void AStrategyPlayerController::SelectionModifier(const FInputActionValue& Value)
{

	// update the selection modifier flag
	bSelectionModifier = Value.Get<bool>();
}

void AStrategyPlayerController::InteractHoldStarted(const FInputActionValue& Value)
{

	// save the starting interaction position
	StartingInteractionPosition = GetMouseLocation();
}

void AStrategyPlayerController::InteractHoldTriggered(const FInputActionValue& Value)
{

	// do a drag scroll 
	DoDragScrollCommand();
}

void AStrategyPlayerController::InteractClickStarted(const FInputActionValue& Value)
{
	if (bIsPlacingOverwatch)
	{
		return;
	}

	// reset the interaction flag
	ResetInteraction();
}

void AStrategyPlayerController::InteractClickCompleted(const FInputActionValue& Value)
{
	if (bIsPlacingOverwatch)
	{
		CancelOverwatchPlacement();
		return;
	}

	bDoubleTapActive = false;
}

void AStrategyPlayerController::TouchPrimaryHoldStarted(const FInputActionValue& Value)
{
	// save the tap press time
	LastTapPressTime = GetWorld()->GetRealTimeSeconds();

	// save the starting interaction position
	StartingInteractionPosition = Value.Get<FVector2D>();
}

void AStrategyPlayerController::TouchPrimaryHoldTriggered(const FInputActionValue& Value)
{
	// is this touch longer than a tap?
	if ((GetWorld()->GetRealTimeSeconds() - LastTapPressTime) > TouchTapMaxAllowedTime)
	{
		// if we're not doing a box select, do a drag scroll
		if (!bSelectionModifier)
		{
			DoDragScrollCommand();
		}
	}
}

void AStrategyPlayerController::TouchPrimaryHoldCompleted(const FInputActionValue& Value)
{
	// check if we're doing a tap or double tap.
	// we have to do this manually because EnhancedInput tap triggers work differently on touch inputs
	bool bTapped = false;
	bool bDoubleTapped = false;

	CheckTouchTap(bTapped, bDoubleTapped);

	// do we have a double tap?
	if (bTapped)
	{
		if (bDoubleTapped)
		{
			// ensure are we not doing a box select
			if (!bSelectionModifier)
			{
				// depending on the double tap toggle, select or deselect all units
				if (bDoubleTapActive)
				{
					DoDeselectAllCommand();
				}
				else
				{
					DoSelectAllOnScreenCommand();
				}

				// toggle the double tap flag
				bDoubleTapActive = !bDoubleTapActive;
			}
		}

	// no double tap, handle this touch input normally
	}
	else
	{

		// ensure we're not already box selecting, or were just box selecting
		if (!(bSelectionModifier || (GetWorld()->GetRealTimeSeconds() - LastBoxSelectTime) < TouchTapMaxAllowedTime))
		{
			// project the touch location and cache the selection point
			CachedInteraction = CachedSelection = ProjectTouchPointToWorldSpace();

			// do a selection action with the cached location
			DoSelectionCommand();
		}

	}

}

void AStrategyPlayerController::TouchSecondaryStarted(const FInputActionValue& Value)
{

	// raise the selection modifier flag
	bSelectionModifier = true;

	// save the starting position for the second finger
	StartingSecondFingerPosition = Value.Get<FVector2D>();
}

void AStrategyPlayerController::TouchSecondaryTriggered(const FInputActionValue& Value)
{
	// update the current position for the second finger
	CurrentSecondFingerPosition = Value.Get<FVector2D>();

	// are we box selecting, and the finger has moved enough on the touchscreen?
	if (bSelectionModifier && !StartingSecondFingerPosition.Equals(CurrentSecondFingerPosition, 10.0f))
	{
		// update the current interaction position
		CurrentInteractionPosition = CurrentSecondFingerPosition;

		// update the selection box on the HUD
		if (StrategyHUD)
		{
			const FVector2D DragSize = CurrentSecondFingerPosition - StartingSecondFingerPosition;

			StrategyHUD->DragSelectUpdate(StartingInteractionPosition, DragSize, CurrentSecondFingerPosition, true);
		}
	}
}

void AStrategyPlayerController::TouchSecondaryCompleted(const FInputActionValue& Value)
{

	// lower the selection modifier flag
	bSelectionModifier = false;

	// save the last box selection time
	LastBoxSelectTime = GetWorld()->GetRealTimeSeconds();

	// hide the selection box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

void AStrategyPlayerController::EscapePressed()
{
	if (bIsPlacingOverwatch)
	{
		CancelOverwatchPlacement();
		return;
	}

	DoDeselectAllCommand();
}

void AStrategyPlayerController::DoSelectionCommand()
{

	// do a sphere sweep to look for actors to select
	FHitResult OutHit;

	const FVector Start = CachedSelection;
	const FVector End = Start + FVector::UpVector * 350.0f;

	FCollisionShape InteractionSphere;
	InteractionSphere.SetSphere(InteractionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetPawn());
	QueryParams.bTraceComplex = true;

	GetWorld()->SweepSingleByObjectType(OutHit, Start, End, FQuat::Identity, ObjectParams, InteractionSphere, QueryParams);

	// if we're using the mouse and are not holding the selection modifier key, deselect any units first
	if (InputMode == SIM_Mouse && !bSelectionModifier)
	{

		DoDeselectAllCommand();
	}

	// did we hit a unit?
	if (OutHit.bBlockingHit)
	{

		// update the target unit
		TargetUnit = GetStrategyUnitFromHit(OutHit);

		if (TargetUnit && IsSelectableUnit(TargetUnit))
		{

			// is the unit already in the controlled list?
			if (ControlledUnits.Contains(TargetUnit))
			{

				// remove the units from the controlled list
				ControlledUnits.Remove(TargetUnit);

				// tell the unit it's been deselected
				TargetUnit->UnitDeselected();

				if (HighlightActor)
				{
					HighlightActor->ClearReachableHighlights();
				}

			}
			else
			{

				// add the unit to the controlled list
				ControlledUnits.Add(TargetUnit);

				// tell the unit it's been selected
				TargetUnit->UnitSelected();

//				UpdateMovementHighlights();

			}
		}
		else
		{
			TargetUnit = nullptr;
		}

	}
	else
	{

		// are we using touch input?
		if (InputMode == SIM_Touch)
		{
			// move all selected units to the target location
			DoMoveUnitsCommand();
		}

	}
	
	UpdateMovementHighlights();
	RefreshActionBar();
	RefreshPlayerUnitRoster();
	RefreshWeaponInfoPanel();
	RefreshShootableTargetIcons();
}

void AStrategyPlayerController::DoSelectAllOnScreenCommand()
{

	// find all NPCs currently on screen
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStrategyUnit::StaticClass(), FoundActors);

	// process each actor found
	for (AActor* CurrentActor : FoundActors)
	{
		// cast back to our unit class
		if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentActor))
		{
			// has the actor been recently rendered?
			if (CurrentActor->WasRecentlyRendered(0.2f))
			{

				// is the actor not on our controlled units list?
				if (!ControlledUnits.Contains(CurrentUnit))
				{
					// add it to the controlled units list
					ControlledUnits.Add(CurrentUnit);

					// notify it of selection
					CurrentUnit->UnitSelected();
				}
			}
		}		
	}

	RefreshPlayerUnitRoster();
	RefreshWeaponInfoPanel();
	RefreshShootableTargetIcons();
}

void AStrategyPlayerController::DoDeselectAllCommand()
{
	ClearShootableTargetIcons();

	if (HighlightActor)
	{
		HighlightActor->ClearReachableHighlights();
		HighlightActor->ClearMovementPath();
		HighlightActor->ClearSelectedCell();
		HighlightActor->ClearMovementDestination();
		HighlightActor->ClearCoverIndicators();
	}
	LastMovementPreviewCell = FIntPoint(TNumericLimits<int32>::Min(), TNumericLimits<int32>::Min());
	LastMovementPreviewUnit = nullptr;
	PendingMoveCell = FIntPoint::ZeroValue;
	PendingMoveUnit = nullptr;
	bHasPendingMoveDestination = false;

	TargetUnit = nullptr;
	
	// tell each controlled unit it's been deselected
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		// ensure the unit hasn't been destroyed
		if (IsValid(CurrentUnit))
		{
			CurrentUnit->UnitDeselected();
		}
	}

	// clear the controlled units list
	ControlledUnits.Empty();
	RefreshPlayerUnitRoster();
	RefreshWeaponInfoPanel();
	RefreshShootableTargetIcons();
}

void AStrategyPlayerController::DoDragScrollCommand()
{

	// choose the cursor position based on the input mode
	FVector2D WorkingPosition;
	
	if (InputMode == EStrategyInputMode::SIM_Mouse)
	{

		// read the mouse position
		bool bResult = GetMousePosition(WorkingPosition.X, WorkingPosition.Y);

	}
	else
	{

		// read the touch 1 position
		bool bPressed;
		GetInputTouchState(ETouchIndex::Touch1, WorkingPosition.X, WorkingPosition.Y, bPressed);

	}

	// find the difference between the starting interaction position and current coords
	const FVector2D InteractionDelta = StartingInteractionPosition - WorkingPosition;

	const FRotator CameraRot(0.0f, -45.0f, 0.0f);

	// rotate and scale the interaction delta
	const FVector ScrollDelta = CameraRot.RotateVector(FVector(InteractionDelta.X, InteractionDelta.Y, 0.0f)) * DragMultiplier;

	// apply the world offset to the controlled pawn
	ControlledPawn->AddActorWorldOffset(ScrollDelta);
}

void AStrategyPlayerController::DoMoveUnitsCommand()
{
	// set the movement goal
	FVector CurrentMoveGoal;

	if (InputMode == EStrategyInputMode::SIM_Mouse)
	{

		// set the cached interaction point as our move goal
		CurrentMoveGoal = CachedInteraction;

	}
	else
	{

		// set the cached selection as our move goal
		CurrentMoveGoal = CachedSelection;

	}

	// get the closest selected unit to the move goal. This will be our lead unit
	AStrategyUnit* Closest = GetClosestSelectedUnitToLocation(CurrentMoveGoal);

	// this will be set to true if any of the move requests fail
	bool bInteractionFailed = false;

	// process each unit in the controlled list
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (IsValid(CurrentUnit))
		{
			if (CurrentUnit->GetRemainingTimeUnits() < 1)
			{
				continue;
			}

			// stop the unit
			CurrentUnit->StopMoving();
			
			FIntPoint ClickedCell = GridManager->WorldToGrid(CurrentMoveGoal);

			if (!ReachableCells.Contains(ClickedCell))
			{
				return;
			}

			int32 MoveCost = 0;
			if (!GridManager->TryGetMoveCostCells(CurrentUnit, ClickedCell, MoveCost)
				|| MoveCost > CurrentUnit->GetRemainingTimeUnits())
			{
				return;
			}
/*
			if (!GridManager->IsCellWalkable(ClickedCell))
			{
				return;
			}
			*/
	//		FVector MoveGoal = GridManager->GridToWorld(ClickedCell);
			FVector MoveGoal;

			if (!GridManager->TryGetNavigationLocationForCell(ClickedCell, MoveGoal))
			{
				UE_LOG(LogTemp, Warning, TEXT("Clicked cell is not on navmesh"));
				return;
			}

			// move the lead unit to the goal, all other units to random navigable points around it
//			FVector MoveGoal = CurrentMoveGoal;

			if (CurrentUnit != Closest)
			{

				UNavigationSystemV1::K2_GetRandomLocationInNavigableRadius(GetWorld(), CurrentMoveGoal, MoveGoal, InteractionRadius * 0.66f);
			}

			// subscribe to the unit's move completed delegate
			CurrentUnit->OnMoveCompleted.AddDynamic(this, &AStrategyPlayerController::OnMoveCompleted);

			if (HighlightActor)
			{
				HighlightActor->ClearReachableHighlights();
				HighlightActor->ClearMovementPath();
				HighlightActor->ClearMovementDestination();
				HighlightActor->ClearCoverIndicators();
			}
			PendingMoveCell = FIntPoint::ZeroValue;
			PendingMoveUnit = nullptr;
			bHasPendingMoveDestination = false;
			
			// set up movement to the goal location
			if (!CurrentUnit->MoveToLocation(MoveGoal, 0.0f/*InteractionRadius * 0.66f*/))
			{
				// the move request failed, so flag it
				bInteractionFailed = true;
			}
			else
			{
				CurrentUnit->SpendTimeUnits(MoveCost);
				RefreshPlayerUnitRoster();
			}
		}

	}

	// play the cursor feedback depending on whether our move succeeded or not
	BP_CursorFeedback(CachedInteraction, !bInteractionFailed);

}

void AStrategyPlayerController::OnMoveCompleted(AStrategyUnit* MovedUnit)
{
	// is the unit valid?
	if (IsValid(MovedUnit))
	{
		// unsubscribe from the delegate
		MovedUnit->OnMoveCompleted.RemoveDynamic(this, &AStrategyPlayerController::OnMoveCompleted);
		
		// skip if interactions are locked
		if (!bAllowInteraction)
		{
			UpdateMovementHighlights();
			RefreshActionBar();
			RefreshPlayerUnitRoster();
			return;
		}

		// disallow additional interactions until we reset
		bAllowInteraction = false;

		// is the unit close enough to the cached interaction location?
		if(FVector::Dist2D(CachedInteraction, MovedUnit->GetActorLocation()) < InteractionRadius)
		{

			// do an overlap test to find nearby interactive objects
			TArray<FOverlapResult> OutOverlaps;

			FCollisionShape CollisionSphere;
			CollisionSphere.SetSphere(InteractionRadius);

			FCollisionObjectQueryParams ObjectParams;
			ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
			
			FCollisionQueryParams QueryParams;

			QueryParams.AddIgnoredActor(MovedUnit);

			for(const AStrategyUnit* CurSelected : ControlledUnits)
			{
				QueryParams.AddIgnoredActor(CurSelected);
			}

			if (GetWorld()->OverlapMultiByObjectType(OutOverlaps, CachedInteraction, FQuat::Identity, ObjectParams, CollisionSphere, QueryParams))
			{
				for (const FOverlapResult& CurrentOverlap : OutOverlaps)
				{
					if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentOverlap.GetActor()))
					{
						CurrentUnit->Interact(MovedUnit);
					}
				}
			}
		}

		UpdateMovementHighlights();
		RefreshActionBar();
		RefreshPlayerUnitRoster();
	}

	RefreshPlayerUnitRoster();
}

AStrategyUnit* AStrategyPlayerController::GetClosestSelectedUnitToLocation(FVector TargetLocation)
{
	// closest unit and distance
	AStrategyUnit* OutUnit = nullptr;
	float Closest = 0.0f;

	// process each unit on the list
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (CurrentUnit != nullptr)
		{
			// have we selected a unit already?
			if (OutUnit != nullptr)
			{
				// calculate the squared distance to the target location
				float Dist = FVector::DistSquared2D(TargetLocation, CurrentUnit->GetActorLocation());

				// is this unit closer?
				if (Dist < Closest)
				{
					// update the closest unit and distance
					OutUnit = CurrentUnit;
					Closest = Dist;
				}
			}
			else
			{

				// no previously selected unit, so use this one
				OutUnit = CurrentUnit;

				// initialize the closest distance
				Closest = FVector::DistSquared2D(TargetLocation, CurrentUnit->GetActorLocation());
			}
		}
		
	}

	// return the selected unit
	return OutUnit;
}

FVector2D AStrategyPlayerController::GetMouseLocation()
{
	// attempt to get the mouse position from this PC
	float MouseX, MouseY;

	if (GetMousePosition(MouseX, MouseY))
	{
		return FVector2D(MouseX, MouseY);
	}

	// return an invalid vector
	return FVector2D::ZeroVector;
}

bool AStrategyPlayerController::GetLocationUnderCursor(FVector& Location)
{
	// trace the visibility channel at the cursor location
	FHitResult OutHit;

	GetHitResultUnderCursorByChannel(SelectionTraceChannel, true, OutHit);

	// if there was a blocking hit, return the hit location
	if (OutHit.bBlockingHit)
	{
		if (const AStrategyUnit* HitUnit = GetStrategyUnitFromHit(OutHit))
		{
			Location = HitUnit->GetActorLocation();
			return true;
		}

		Location = OutHit.Location;
		return true;
	}

	return OutHit.bBlockingHit;
}

AStrategyUnit* AStrategyPlayerController::GetStrategyUnitFromHit(const FHitResult& Hit) const
{
	AActor* HitActor = Hit.GetActor();
	while (HitActor)
	{
		if (AStrategyUnit* Unit = Cast<AStrategyUnit>(HitActor))
		{
			return Unit;
		}

		HitActor = HitActor->GetAttachParentActor();
	}

	return nullptr;
}

void AStrategyPlayerController::AddAllStrategyUnitsToIgnoredActors(FCollisionQueryParams& QueryParams) const
{
	TArray<AActor*> Units;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStrategyUnit::StaticClass(), Units);

	for (AActor* Unit : Units)
	{
		QueryParams.AddIgnoredActor(Unit);
	}
}

FVector AStrategyPlayerController::ProjectTouchPointToWorldSpace()
{
	// get the touch coordinates for the first finger
	float TouchX, TouchY = 0.0f;
	bool bPressed = false;

	GetInputTouchState(ETouchIndex::Touch1, TouchX, TouchY, bPressed);

	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;

	// deproject the coords into world space
	if (DeprojectScreenPositionToWorld(TouchX, TouchY, WorldLocation, WorldDirection))
	{
		// intersect with a horizontal plane and return the resulting point
		const FPlane IntersectPlane(FVector::ZeroVector, FVector::UpVector);

		return FMath::LinePlaneIntersection(WorldLocation, WorldLocation + (WorldDirection * 100000.0f), IntersectPlane);
	}

	// failed to deproject, return a zero vector
	return FVector::ZeroVector;
}

void AStrategyPlayerController::ResetInteraction()
{
	bAllowInteraction = true;
}

void AStrategyPlayerController::CheckTouchTap(bool& bTapped, bool& bDoubleTapped)
{
	// get the current game time
	const float GameTime = GetWorld()->GetRealTimeSeconds();

	// if the player released touch before the max allowed time since press, we have a tap
	bTapped = (GameTime - LastTapPressTime) < TouchTapMaxAllowedTime;

	if (bTapped)
	{
		// we have a double tap if another tap happened before the last release time
		if ((GameTime - LastTapReleaseTime) < TouchDoubleTapMaxAllowedTime)
		{
			// increase the tap counter
			++TapCount;

		}
		else
		{

			// reset the tap counter
			TapCount = 0;
		}

	}
	else
	{

		// reset the tap counter
		TapCount = 0;
	}

	// we have a double tap if the tap count is not zero
	bDoubleTapped = TapCount >= 1;

	// save the tap release time
	LastTapReleaseTime = GameTime;
}

void AStrategyPlayerController::UpdateMovementHighlights()
{
	if (!GridManager || !HighlightActor)
	{
		return;
	}

	AStrategyUnit* HighlightUnit = ControlledUnits.Num() == 1 ? ControlledUnits[0] : TargetUnit;
	if (!IsValid(HighlightUnit) || !IsSelectableUnit(HighlightUnit))
	{
		ReachableCells.Empty();
		HighlightActor->ClearReachableHighlights();
		HighlightActor->ClearSelectedCell();
		LastMovementPreviewCell = FIntPoint(TNumericLimits<int32>::Min(), TNumericLimits<int32>::Min());
		LastMovementPreviewUnit = nullptr;
		return;
	}

	TargetUnit = HighlightUnit;
	ReachableCells.Empty();
	
	const FIntPoint UnitCell = GridManager->WorldToGrid(HighlightUnit->GetActorLocation());
	HighlightActor->ShowSelectedCell(GridManager, UnitCell);

	if (HighlightUnit->GetRemainingTimeUnits() < 1)
	{
		HighlightActor->ClearReachableHighlights();
		LastMovementPreviewCell = FIntPoint(TNumericLimits<int32>::Min(), TNumericLimits<int32>::Min());
		LastMovementPreviewUnit = nullptr;
		return;
	}

	const int32 MaxMove = HighlightUnit->GetRemainingTimeUnits();

	for (int32 Y = -MaxMove; Y <= MaxMove; ++Y)
	{
		for (int32 X = -MaxMove; X <= MaxMove; ++X)
		{
			const int32 Dist = FMath::Abs(X) + FMath::Abs(Y);
			if (Dist > MaxMove)
			{
				continue;
			}

			const FIntPoint TestCell(UnitCell.X + X, UnitCell.Y + Y);

			if (GridManager->IsCellWithinMoveRange(HighlightUnit, TestCell, MaxMove))
			{
				ReachableCells.Add(TestCell);
			}
		}
	}

	HighlightActor->ShowReachableCells(GridManager, ReachableCells);
	LastMovementPreviewCell = FIntPoint(TNumericLimits<int32>::Min(), TNumericLimits<int32>::Min());
	LastMovementPreviewUnit = nullptr;
}

void AStrategyPlayerController::UpdateMovementPreview()
{
	if (!GridManager || !HighlightActor || ReachableCells.Num() == 0)
	{
		if (HighlightActor && !bHasPendingMoveDestination)
		{
			HighlightActor->ClearMovementDestination();
		}
		return;
	}

	AStrategyUnit* PreviewUnit = ControlledUnits.Num() == 1 ? ControlledUnits[0] : TargetUnit;
	if (!IsValid(PreviewUnit) || !IsSelectableUnit(PreviewUnit) || PreviewUnit->GetRemainingTimeUnits() < 1)
	{
		HighlightActor->ClearMovementPath();
		if (!bHasPendingMoveDestination)
		{
			HighlightActor->ClearMovementDestination();
		}
		HighlightActor->ClearCoverIndicators();
		if (LastMovementPreviewCell != FIntPoint(TNumericLimits<int32>::Min(), TNumericLimits<int32>::Min()))
		{
			RefreshShootableTargetIcons();
		}
		LastMovementPreviewCell = FIntPoint(TNumericLimits<int32>::Min(), TNumericLimits<int32>::Min());
		LastMovementPreviewUnit = nullptr;
		return;
	}

	FVector CursorLocation;
	if (!GetLocationUnderCursor(CursorLocation))
	{
		HighlightActor->ClearMovementPath();
		if (!bHasPendingMoveDestination)
		{
			HighlightActor->ClearMovementDestination();
		}
		HighlightActor->ClearCoverIndicators();
		if (LastMovementPreviewCell != FIntPoint(TNumericLimits<int32>::Min(), TNumericLimits<int32>::Min()))
		{
			RefreshShootableTargetIcons();
		}
		LastMovementPreviewCell = FIntPoint(TNumericLimits<int32>::Min(), TNumericLimits<int32>::Min());
		LastMovementPreviewUnit = nullptr;
		return;
	}

	const FIntPoint HoveredCell = GridManager->WorldToGrid(CursorLocation);
	if (!ReachableCells.Contains(HoveredCell))
	{
		HighlightActor->ClearMovementPath();
		if (!bHasPendingMoveDestination)
		{
			HighlightActor->ClearMovementDestination();
		}
		HighlightActor->ClearCoverIndicators();
		if (LastMovementPreviewCell != FIntPoint(TNumericLimits<int32>::Min(), TNumericLimits<int32>::Min()))
		{
			RefreshShootableTargetIcons();
		}
		LastMovementPreviewCell = FIntPoint(TNumericLimits<int32>::Min(), TNumericLimits<int32>::Min());
		LastMovementPreviewUnit = nullptr;
		return;
	}

	if (LastMovementPreviewCell == HoveredCell && LastMovementPreviewUnit == PreviewUnit)
	{
		return;
	}

	LastMovementPreviewCell = HoveredCell;
	LastMovementPreviewUnit = PreviewUnit;

	TArray<FVector> PathPoints;
	if (BuildMovementPathPreview(PreviewUnit, HoveredCell, PathPoints))
	{
		HighlightActor->ShowMovementPath(PathPoints);
	}
	else
	{
		HighlightActor->ClearMovementPath();
	}

	HighlightActor->ShowCoverIndicators(BuildCoverIndicatorsForCell(HoveredCell));

	const FIntPoint UnitCell = GridManager->WorldToGrid(PreviewUnit->GetActorLocation());
	int32 MovementTimeUnitCost = 0;
	if (HoveredCell != UnitCell)
	{
		GridManager->TryGetMoveCostCells(PreviewUnit, HoveredCell, MovementTimeUnitCost);
	}

	RefreshShootableTargetIconsForCell(PreviewUnit, HoveredCell, MovementTimeUnitCost);
}

void AStrategyPlayerController::BeginOverwatchPlacement(AStrategyUnit* Unit)
{
	if (!IsValid(Unit) || !Unit->CanOverwatch())
	{
		if (IsStrategyPlayerOverwatchDebugEnabled())
		{
			UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: BeginOverwatchPlacement blocked Unit=%s IsValid=%d CanOverwatch=%d"),
				*GetNameSafe(Unit),
				IsValid(Unit),
				IsValid(Unit) ? Unit->CanOverwatch() : false);
		}
		return;
	}

	if (IsStrategyPlayerOverwatchDebugEnabled())
	{
		UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: BeginOverwatchPlacement Unit=%s TU=%d Range=%d HighlightActor=%s Grid=%s ConeAngle=%.1f"),
			*GetNameSafe(Unit),
			Unit->GetRemainingTimeUnits(),
			Unit->GetOverwatchRange(),
			*GetNameSafe(HighlightActor),
			*GetNameSafe(GridManager),
			Unit->GetOverwatchConeAngleDegrees());
	}

	OverwatchPlacementUnit = Unit;
	bIsPlacingOverwatch = true;
	OverwatchPreviewCells.Empty();

	if (HighlightActor)
	{
		HighlightActor->ClearReachableHighlights();
		HighlightActor->ClearMovementPath();
		HighlightActor->ClearSelectedCell();
		HighlightActor->ClearMovementDestination();
		HighlightActor->ClearCoverIndicators();
	}

	UpdateOverwatchPlacementPreview();
}

void AStrategyPlayerController::UpdateOverwatchPlacementPreview()
{
	if (!bIsPlacingOverwatch || !IsValid(OverwatchPlacementUnit) || !GridManager || !HighlightActor)
	{
		if (IsStrategyPlayerOverwatchDebugEnabled())
		{
			UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: Preview skipped bPlacing=%d Unit=%s Grid=%s HighlightActor=%s"),
				bIsPlacingOverwatch,
				*GetNameSafe(OverwatchPlacementUnit),
				*GetNameSafe(GridManager),
				*GetNameSafe(HighlightActor));
		}
		return;
	}

	FVector AimLocation;
	if (!GetLocationUnderCursor(AimLocation))
	{
		if (IsStrategyPlayerOverwatchDebugEnabled())
		{
			UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: Preview cursor trace failed TraceChannel=%d"),
				static_cast<int32>(SelectionTraceChannel.GetValue()));
		}
		return;
	}

	OverwatchPreviewCells = BuildOverwatchConeCells(OverwatchPlacementUnit, AimLocation);
	const FOverwatchBoundaryLine BoundaryLine = MakeOverwatchBoundaryLine(
		OverwatchPlacementUnit,
		OverwatchPlacementDirection,
		OverwatchPreviewCells);

	if (IsStrategyPlayerOverwatchDebugEnabled())
	{
		UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: Preview Aim=%s Direction=%s Cells=%d Range=%d"),
			*AimLocation.ToCompactString(),
			*OverwatchPlacementDirection.ToCompactString(),
			OverwatchPreviewCells.Num(),
			OverwatchPlacementUnit->GetOverwatchRange());
	}
	HighlightActor->ShowOverwatchPreviewCells(GridManager, OverwatchPreviewCells);
	HighlightActor->ShowOverwatchPreviewBoundaryLine(BoundaryLine);
}

void AStrategyPlayerController::ConfirmOverwatchPlacement()
{
	if (!bIsPlacingOverwatch || !IsValid(OverwatchPlacementUnit))
	{
		if (IsStrategyPlayerOverwatchDebugEnabled())
		{
			UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: Confirm failed, no valid placement Unit=%s bPlacing=%d"),
				*GetNameSafe(OverwatchPlacementUnit),
				bIsPlacingOverwatch);
		}
		CancelOverwatchPlacement();
		return;
	}

	FVector AimLocation;
	if (!GetLocationUnderCursor(AimLocation))
	{
		if (IsStrategyPlayerOverwatchDebugEnabled())
		{
			UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: Confirm cursor trace failed"));
		}
		CancelOverwatchPlacement();
		return;
	}

	OverwatchPreviewCells = BuildOverwatchConeCells(OverwatchPlacementUnit, AimLocation);
	if (OverwatchPreviewCells.Num() == 0)
	{
		if (IsStrategyPlayerOverwatchDebugEnabled())
		{
			UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: Confirm produced zero cells Aim=%s Unit=%s"),
				*AimLocation.ToCompactString(),
				*GetNameSafe(OverwatchPlacementUnit));
		}
		CancelOverwatchPlacement();
		return;
	}

	if (IsStrategyPlayerOverwatchDebugEnabled())
	{
		UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: Confirm Unit=%s APBefore=%d Aim=%s Direction=%s Cells=%d Range=%d"),
			*GetNameSafe(OverwatchPlacementUnit),
			OverwatchPlacementUnit->GetRemainingTimeUnits(),
			*AimLocation.ToCompactString(),
			*OverwatchPlacementDirection.ToCompactString(),
			OverwatchPreviewCells.Num(),
			OverwatchPlacementUnit->GetOverwatchRange());
	}

	OverwatchPlacementUnit->EnterOverwatch(
		OverwatchPlacementDirection,
		OverwatchPlacementUnit->GetOverwatchRange(),
		OverwatchPlacementUnit->GetOverwatchConeAngleDegrees(),
		OverwatchPreviewCells);

	if (HighlightActor)
	{
		HighlightActor->ClearOverwatchPreviewHighlights();
		RefreshLockedOverwatchHighlights();
	}

	OverwatchPlacementUnit = nullptr;
	bIsPlacingOverwatch = false;
	OverwatchPreviewCells.Empty();

	RefreshActionBar();
	RefreshPlayerUnitRoster();
	RefreshWeaponInfoPanel();
}

void AStrategyPlayerController::CancelOverwatchPlacement()
{
	if (IsStrategyPlayerOverwatchDebugEnabled())
	{
		UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: Cancel placement Unit=%s Cells=%d"),
			*GetNameSafe(OverwatchPlacementUnit),
			OverwatchPreviewCells.Num());
	}

	if (HighlightActor)
	{
		HighlightActor->ClearOverwatchPreviewHighlights();
	}

	OverwatchPlacementUnit = nullptr;
	bIsPlacingOverwatch = false;
	OverwatchPreviewCells.Empty();
}

void AStrategyPlayerController::RefreshLockedOverwatchHighlights()
{
	if (!HighlightActor || !GridManager)
	{
		return;
	}

	TArray<FIntPoint> LockedCells;
	TArray<FOverwatchBoundaryLine> BoundaryLines;
	if (AStrategyGameMode* GameMode = GetStrategyGameMode())
	{
		if (GameMode->GetPlayerSide())
		{
			for (AStrategyUnit* Unit : GameMode->GetPlayerSide()->GetAliveUnits())
			{
				if (IsValid(Unit) && Unit->IsOverwatchActive())
				{
					LockedCells.Append(Unit->GetOverwatchCells());
					BoundaryLines.Add(MakeOverwatchBoundaryLine(
						Unit,
						Unit->GetOverwatchDirection(),
						Unit->GetOverwatchCells()));
				}
			}
		}

		if (GameMode->GetEnemySide())
		{
			for (AStrategyUnit* Unit : GameMode->GetEnemySide()->GetAliveUnits())
			{
				if (IsValid(Unit) && Unit->IsOverwatchActive())
				{
					LockedCells.Append(Unit->GetOverwatchCells());
					BoundaryLines.Add(MakeOverwatchBoundaryLine(
						Unit,
						Unit->GetOverwatchDirection(),
						Unit->GetOverwatchCells()));
				}
			}
		}
	}

	HighlightActor->ShowOverwatchCells(GridManager, LockedCells);
	HighlightActor->ShowOverwatchBoundaryLines(BoundaryLines);
}

TArray<FIntPoint> AStrategyPlayerController::BuildOverwatchConeCells(const AStrategyUnit* Unit, const FVector& AimLocation)
{
	TArray<FIntPoint> Cells;
	if (!Unit || !GridManager)
	{
		if (IsStrategyPlayerOverwatchDebugEnabled())
		{
			UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: BuildCone aborted Unit=%s Grid=%s"),
				*GetNameSafe(Unit),
				*GetNameSafe(GridManager));
		}
		return Cells;
	}

	const FVector UnitLocation = Unit->GetActorLocation();
	FVector Direction(AimLocation.X - UnitLocation.X, AimLocation.Y - UnitLocation.Y, 0.0f);
	if (!Direction.Normalize())
	{
		Direction = Unit->GetActorForwardVector();
		Direction.Z = 0.0f;
		Direction.Normalize();
	}

	OverwatchPlacementDirection = Direction;

	const FIntPoint UnitCell = GridManager->WorldToGrid(UnitLocation);
	const int32 Range = FMath::Max(Unit->GetOverwatchRange(), 1);
	const float OverwatchConeAngleDegrees = Unit->GetOverwatchConeAngleDegrees();
	const float HalfAngleRadians = FMath::DegreesToRadians(OverwatchConeAngleDegrees * 0.5f);
	const float MinDot = FMath::Cos(HalfAngleRadians);
	int32 InvalidCells = 0;
	int32 OutOfRangeCells = 0;
	int32 OutsideAngleCells = 0;
	int32 BlockedCells = 0;

	for (int32 Y = -Range; Y <= Range; ++Y)
	{
		for (int32 X = -Range; X <= Range; ++X)
		{
			if (X == 0 && Y == 0)
			{
				continue;
			}

			const FIntPoint TestCell(UnitCell.X + X, UnitCell.Y + Y);
			if (!GridManager->IsValidCell(TestCell))
			{
				++InvalidCells;
				continue;
			}

			const FVector CellLocation = GridManager->GridToWorld(TestCell);
			FVector ToCell(CellLocation.X - UnitLocation.X, CellLocation.Y - UnitLocation.Y, 0.0f);
			const float DistanceInCells = ToCell.Size() / GridManager->CellSize;
			if (DistanceInCells > Range || !ToCell.Normalize())
			{
				++OutOfRangeCells;
				continue;
			}

			if (FVector::DotProduct(Direction, ToCell) >= MinDot)
			{
				if (HasOverwatchLineOfSight(Unit, TestCell))
				{
					Cells.Add(TestCell);
				}
				else
				{
					++BlockedCells;
				}
			}
			else
			{
				++OutsideAngleCells;
			}
		}
	}

	if (IsStrategyPlayerOverwatchDebugEnabled())
	{
		UE_LOG(LogZombieGame, Warning, TEXT("OverwatchDebug: BuildCone Unit=%s UnitCell=%s Aim=%s Direction=%s Range=%d Angle=%.1f Cells=%d Invalid=%d OutOfRange=%d OutsideAngle=%d Blocked=%d"),
			*GetNameSafe(Unit),
			*UnitCell.ToString(),
			*AimLocation.ToCompactString(),
			*Direction.ToCompactString(),
			Range,
			OverwatchConeAngleDegrees,
			Cells.Num(),
			InvalidCells,
			OutOfRangeCells,
			OutsideAngleCells,
			BlockedCells);
	}

	return Cells;
}

bool AStrategyPlayerController::HasOverwatchLineOfSight(const AStrategyUnit* Unit, const FIntPoint& Cell) const
{
	if (!Unit || !GridManager || !GetWorld())
	{
		return false;
	}

	FVector TargetLocation;
	FVector TargetNormal;
	if (!GridManager->ProjectCellToGround(Cell, TargetLocation, TargetNormal))
	{
		return false;
	}

	if (TargetNormal.Z < OverwatchMinGroundNormalZ)
	{
		return false;
	}

	const FVector Start = Unit->GetActorLocation() + FVector(0.0f, 0.0f, OverwatchLineOfSightHeightOffset);
	const FVector End = TargetLocation + FVector(0.0f, 0.0f, OverwatchLineOfSightHeightOffset);

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OverwatchLineOfSight), false);
	QueryParams.AddIgnoredActor(Unit);
	AddAllStrategyUnitsToIgnoredActors(QueryParams);

	return !GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		QueryParams);
}

bool AStrategyPlayerController::BuildMovementPathPreview(
	const AStrategyUnit* Unit,
	const FIntPoint& TargetCell,
	TArray<FVector>& OutPathPoints) const
{
	OutPathPoints.Reset();
	if (!Unit || !GridManager || !GetWorld())
	{
		return false;
	}

	FVector TargetLocation;
	if (!GridManager->TryGetNavigationLocationForCell(TargetCell, TargetLocation))
	{
		return false;
	}

	UNavigationPath* NavigationPath = UNavigationSystemV1::FindPathToLocationSynchronously(
		GetWorld(),
		Unit->GetActorLocation(),
		TargetLocation,
		const_cast<AStrategyUnit*>(Unit));

	if (!NavigationPath || NavigationPath->PathPoints.Num() < 2)
	{
		return false;
	}

	OutPathPoints = NavigationPath->PathPoints;
	return true;
}

TArray<FGridCoverIndicator> AStrategyPlayerController::BuildCoverIndicatorsForCell(const FIntPoint& Cell) const
{
	TArray<FGridCoverIndicator> Indicators;
	if (!GridManager)
	{
		return Indicators;
	}

	FVector GroundLocation;
	if (!GridManager->TryGetNavigationLocationForCell(Cell, GroundLocation))
	{
		return Indicators;
	}

	const FIntPoint Directions[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	const float HalfCell = GridManager->CellSize * 0.5f;
	for (const FIntPoint& Direction : Directions)
	{
		EGridCoverType CoverType = EGridCoverType::Half;
		if (!GetCoverTypeForDirection(Cell, Direction, CoverType))
		{
			continue;
		}

		const FVector DirectionVector(
			static_cast<float>(Direction.X),
			static_cast<float>(Direction.Y),
			0.0f);

		FGridCoverIndicator Indicator;
		Indicator.CoverType = CoverType;
		Indicator.Direction = DirectionVector;
		Indicator.Location = GroundLocation + DirectionVector * FMath::Max(0.0f, HalfCell - CoverIndicatorInset);
		Indicators.Add(Indicator);
	}

	return Indicators;
}

bool AStrategyPlayerController::GetCoverTypeForDirection(
	const FIntPoint& Cell,
	const FIntPoint& Direction,
	EGridCoverType& OutCoverType) const
{
	if (!GridManager || !GetWorld() || Direction == FIntPoint::ZeroValue)
	{
		return false;
	}

	const FVector CellCenter = GridManager->GridToWorld(Cell);
	const FVector DirectionVector(
		static_cast<float>(Direction.X),
		static_cast<float>(Direction.Y),
		0.0f);
	const FVector TraceEndBase = CellCenter + DirectionVector * GridManager->CellSize;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CoverTrace), false);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetPawn());
	AddAllStrategyUnitsToIgnoredActors(QueryParams);

	auto IsBlockedAtHeight = [this, &QueryParams, CellCenter, TraceEndBase](float Height)
	{
		FHitResult Hit;
		return GetWorld()->LineTraceSingleByChannel(
			Hit,
			CellCenter + FVector(0.0f, 0.0f, Height),
			TraceEndBase + FVector(0.0f, 0.0f, Height),
			ECC_Visibility,
			QueryParams);
	};

	if (!IsBlockedAtHeight(CoverHalfTraceHeight))
	{
		return false;
	}

	OutCoverType = IsBlockedAtHeight(CoverFullTraceHeight)
		? EGridCoverType::Full
		: EGridCoverType::Half;
	return true;
}

FOverwatchBoundaryLine AStrategyPlayerController::MakeOverwatchBoundaryLine(
	const AStrategyUnit* Unit,
	const FVector& Direction,
	const TArray<FIntPoint>& Cells) const
{
	FOverwatchBoundaryLine BoundaryLine;
	if (!Unit || !GridManager || Cells.Num() == 0)
	{
		return BoundaryLine;
	}

	FVector FlatDirection(Direction.X, Direction.Y, 0.0f);
	if (!FlatDirection.Normalize())
	{
		FlatDirection = Unit->GetActorForwardVector();
		FlatDirection.Z = 0.0f;
		FlatDirection.Normalize();
	}

	const FVector Origin = Unit->GetActorLocation();
	const float HalfCell = GridManager->CellSize * 0.5f;

	bool bHasLeft = false;
	bool bHasRight = false;
	float LeftDistanceSq = 0.0f;
	float RightDistanceSq = 0.0f;
	float LeftSignedAngle = TNumericLimits<float>::Lowest();
	float RightSignedAngle = TNumericLimits<float>::Max();
	FVector LeftEnd = Origin;
	FVector RightEnd = Origin;

	auto GetBoundaryCorner = [this, Origin, FlatDirection, HalfCell](const FIntPoint& Cell, bool bLeftSide)
	{
		const FVector Center = GridManager->GridToWorld(Cell);
		const FVector Corners[] =
		{
			Center + FVector(-HalfCell, -HalfCell, 0.0f),
			Center + FVector(HalfCell, -HalfCell, 0.0f),
			Center + FVector(HalfCell, HalfCell, 0.0f),
			Center + FVector(-HalfCell, HalfCell, 0.0f)
		};

		FVector BestCorner = Corners[0];
		float BestSignedAngle = bLeftSide ? TNumericLimits<float>::Lowest() : TNumericLimits<float>::Max();
		float BestDistanceSq = 0.0f;

		for (const FVector& Corner : Corners)
		{
			FVector ToCorner(Corner.X - Origin.X, Corner.Y - Origin.Y, 0.0f);
			const float DistanceSq = ToCorner.SizeSquared();
			if (DistanceSq <= KINDA_SMALL_NUMBER || !ToCorner.Normalize())
			{
				continue;
			}

			const float SignedAngle = FMath::Atan2(
				FVector::CrossProduct(FlatDirection, ToCorner).Z,
				FVector::DotProduct(FlatDirection, ToCorner));

			const bool bBetterLeft = bLeftSide
				&& (SignedAngle > BestSignedAngle
					|| (FMath::IsNearlyEqual(SignedAngle, BestSignedAngle) && DistanceSq > BestDistanceSq));
			const bool bBetterRight = !bLeftSide
				&& (SignedAngle < BestSignedAngle
					|| (FMath::IsNearlyEqual(SignedAngle, BestSignedAngle) && DistanceSq > BestDistanceSq));

			if (bBetterLeft || bBetterRight)
			{
				BestSignedAngle = SignedAngle;
				BestDistanceSq = DistanceSq;
				BestCorner = Corner;
			}
		}

		return BestCorner;
	};

	for (const FIntPoint& Cell : Cells)
	{
		const FVector Center = GridManager->GridToWorld(Cell);
		FVector ToCell(Center.X - Origin.X, Center.Y - Origin.Y, 0.0f);
		const float DistanceSq = ToCell.SizeSquared();
		if (DistanceSq <= KINDA_SMALL_NUMBER || !ToCell.Normalize())
		{
			continue;
		}

		const float SignedAngle = FMath::Atan2(
			FVector::CrossProduct(FlatDirection, ToCell).Z,
			FVector::DotProduct(FlatDirection, ToCell));

		if (SignedAngle >= 0.0f
			&& (!bHasLeft
				|| SignedAngle > LeftSignedAngle
				|| (FMath::IsNearlyEqual(SignedAngle, LeftSignedAngle) && DistanceSq > LeftDistanceSq)))
		{
			bHasLeft = true;
			LeftSignedAngle = SignedAngle;
			LeftDistanceSq = DistanceSq;
			LeftEnd = GetBoundaryCorner(Cell, true);
		}

		if (SignedAngle <= 0.0f
			&& (!bHasRight
				|| SignedAngle < RightSignedAngle
				|| (FMath::IsNearlyEqual(SignedAngle, RightSignedAngle) && DistanceSq > RightDistanceSq)))
		{
			bHasRight = true;
			RightSignedAngle = SignedAngle;
			RightDistanceSq = DistanceSq;
			RightEnd = GetBoundaryCorner(Cell, false);
		}
	}

	BoundaryLine.Origin = Origin;
	BoundaryLine.LeftEnd = bHasLeft ? LeftEnd : Origin;
	BoundaryLine.RightEnd = bHasRight ? RightEnd : Origin;
	return BoundaryLine;
}

bool AStrategyPlayerController::IsSelectableUnit(const AStrategyUnit* Unit) const
{
	return Unit && Unit->GetStrategyUnitTeam() == EStrategyUnitTeam::Human;
}

void AStrategyPlayerController::HandleUnitActionClicked(EPlayerUnitActionType ActionType)
{
	SuppressSelectionInputBriefly();

	AStrategyUnit* SelectedUnit = ControlledUnits.Num() == 1 ? ControlledUnits[0] : TargetUnit;
	if (!SelectedUnit && ControlledUnits.Num() > 0)
	{
		SelectedUnit = ControlledUnits[0];
	}
	
	if (!SelectedUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melee action ignored: no selected unit"));
		return;
	}

	switch (ActionType)
	{
	case EPlayerUnitActionType::MeleeAttack:
		UE_LOG(LogTemp, Warning, TEXT("Melee action clicked for %s"), *GetNameSafe(SelectedUnit));
		SelectedUnit->StartMeleeAttackMode();
		break;

	case EPlayerUnitActionType::WeaponAttack:
		SelectedUnit->StartWeaponAttackMode();
		break;

	case EPlayerUnitActionType::Reload:
		SelectedUnit->ReloadWeapon();
		RefreshActionBar();
		RefreshPlayerUnitRoster();
		RefreshWeaponInfoPanel();
		break;

	case EPlayerUnitActionType::HunkerDown:
//		SelectedUnit->HunkerDown();
		RefreshActionBar();
		break;

	case EPlayerUnitActionType::Overwatch:
		BeginOverwatchPlacement(SelectedUnit);
		RefreshActionBar();
		break;

	case EPlayerUnitActionType::SkipTurn:
		SelectedUnit->SpendTimeUnits(SelectedUnit->GetRemainingTimeUnits());
		RefreshActionBar();
		RefreshPlayerUnitRoster();
		RefreshWeaponInfoPanel();
		break;

	default:
		break;
	}
}

void AStrategyPlayerController::HandleRosterUnitClicked(AStrategyUnit* Unit)
{
	SelectRosterUnit(Unit);
	CenterCameraOnUnit(Unit);
}

void AStrategyPlayerController::SelectRosterUnit(AStrategyUnit* Unit)
{
	if (!IsSelectableUnit(Unit))
	{
		return;
	}

	DoDeselectAllCommand();

	TargetUnit = Unit;
	ControlledUnits.Add(Unit);
	Unit->UnitSelected();

	UpdateMovementHighlights();
	RefreshActionBar();
	RefreshPlayerUnitRoster();
	RefreshWeaponInfoPanel();
	RefreshShootableTargetIcons();
}

void AStrategyPlayerController::CenterCameraOnUnit(const AStrategyUnit* Unit)
{
	if (!Unit || !ControlledPawn)
	{
		return;
	}

	const FVector CurrentPawnLocation = ControlledPawn->GetActorLocation();
	const FVector UnitLocation = Unit->GetActorLocation();

	ControlledPawn->SetActorLocation(FVector(
		UnitLocation.X,
		UnitLocation.Y,
		CurrentPawnLocation.Z));
}

void AStrategyPlayerController::RefreshActionBar()
{
	AStrategyUnit* SelectedUnit = ControlledUnits.Num() == 1 ? ControlledUnits[0] : TargetUnit;
	if (!IsValid(SelectedUnit) || SelectedUnit->GetCurrentHealth() <= 0)
	{
		if (UnitActionBarWidget)
		{
			UnitActionBarWidget->SetActions({});
		}
		RefreshShootableTargetIcons();
		return;
	}
	
	if (!UnitActionBarWidget)
	{
		RefreshShootableTargetIcons();
		return;
	}
	
	AStrategyGameMode* GameMode = GetStrategyGameMode();
	if (!ensureMsgf(GameMode, TEXT("GameMode is null in AStrategyPlayerController::RefreshActionBar")))
	{
		return;
	}

	AAIStrategySide* EnemySide = GameMode->GetEnemySide();
	if (!ensureMsgf(EnemySide, TEXT("EnemySide is null in AStrategyPlayerController::RefreshActionBar")))
	{
		return;
	}
	
	TArray<FUnitActionButtonData> Actions;
	
	Actions.Add({
		EPlayerUnitActionType::Reload,
		FText::FromString("Reload"),
		nullptr,
		SelectedUnit->CanReload(),
		FText::FromString("Weapon is full or no TU")
	});

	Actions.Add({
		EPlayerUnitActionType::Overwatch,
		FText::FromString("Overwatch"),
		nullptr,
		SelectedUnit->CanOverwatch(),
		FText::FromString("No TU")
	});

	Actions.Add({
		EPlayerUnitActionType::SkipTurn,
		FText::FromString("Skip"),
		nullptr,
		SelectedUnit->GetRemainingTimeUnits() > 0,
		FText::FromString("No TU")
	});

/*
	Actions.Add({
		EPlayerUnitActionType::HunkerDown,
		FText::FromString("Hunker"),
		nullptr,
		SelectedUnit->CanHunkerDown(),
		FText::FromString("No TU")
	});

*/
	UnitActionBarWidget->SetActions(Actions);
	RefreshShootableTargetIcons();
	
}

void AStrategyPlayerController::SetAlwaysMeleeAttackEnabled(bool bEnabled)
{
	bAlwaysMeleeAttackEnabled = bEnabled;
	RefreshActionBar();
}

void AStrategyPlayerController::RefreshPlayerUnitRoster()
{
	if (!PlayerUnitRosterWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerUnitRoster: refresh skipped, widget is null"));
		return;
	}

	AStrategyGameMode* GameMode = GetStrategyGameMode();
	if (!GameMode || !GameMode->GetPlayerSide())
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerUnitRoster: refresh skipped, player side is not ready"));
		return;
	}

	const TArray<AStrategyUnit*> AliveUnits = GameMode->GetPlayerSide()->GetAliveUnits();
	PlayerUnitRosterWidget->SetUnits(AliveUnits, ControlledUnits);
}

void AStrategyPlayerController::ClearShootableTargetIcons()
{
	if (ShootableTargetIconBarWidget)
	{
		ShootableTargetIconBarWidget->SetTargets({}, ShootableTargetIconWidgetClass);
	}

	if (ShootableTargetIconSlateWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ShootableTargetIconSlateWidget.ToSharedRef());
	}

	ShootableTargetIconSlateWidget.Reset();
	ShootableTargetIconBrushes.Reset();
}

void AStrategyPlayerController::ClearShootTargetHoverIndicator()
{
	if (ShootTargetHoverSlateWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ShootTargetHoverSlateWidget.ToSharedRef());
	}

	ShootTargetHoverSlateWidget.Reset();
	ShootTargetHoverHitChanceIconBrush.Reset();
	ShootTargetHoverActionPointIconBrush.Reset();
	LastShootTargetHoverUnit = nullptr;

	if (HighlightActor)
	{
		HighlightActor->ClearHoveredEnemyCell();
	}
}

AStrategyUnit* AStrategyPlayerController::GetHoveredStrategyUnit()
{
	FHitResult OutHit;
	GetHitResultUnderCursorByChannel(SelectionTraceChannel, true, OutHit);

	if (OutHit.bBlockingHit)
	{
		if (AStrategyUnit* HitUnit = GetStrategyUnitFromHit(OutHit))
		{
			return HitUnit;
		}
	}

	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!GetMousePosition(MouseX, MouseY)
		|| !DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		return nullptr;
	}

	const FVector Start = WorldLocation;
	const FVector End = Start + WorldDirection * 100000.0f;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetPawn());
	QueryParams.bTraceComplex = false;

	TArray<FHitResult> HitResults;
	GetWorld()->SweepMultiByObjectType(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(ShootTargetHoverTraceRadius),
		QueryParams);

	for (const FHitResult& Hit : HitResults)
	{
		if (AStrategyUnit* Unit = GetStrategyUnitFromHit(Hit))
		{
			return Unit;
		}
	}

	return nullptr;
}

void AStrategyPlayerController::UpdateShootTargetHoverIndicator()
{
	if (!GEngine || !GEngine->GameViewport || !GridManager)
	{
		ClearShootTargetHoverIndicator();
		return;
	}

	if (TargetingComponent && TargetingComponent->IsInFireMode())
	{
		ClearShootTargetHoverIndicator();
		return;
	}

	AStrategyUnit* SelectedUnit = ControlledUnits.Num() == 1 ? ControlledUnits[0] : TargetUnit;
	if (!IsValid(SelectedUnit) && ControlledUnits.Num() > 0)
	{
		SelectedUnit = ControlledUnits[0];
	}

	AStrategyUnit* HoveredUnit = GetHoveredStrategyUnit();
	if (!IsValid(SelectedUnit)
		|| !IsSelectableUnit(SelectedUnit)
		|| SelectedUnit->GetCurrentHealth() <= 0
		|| !IsValid(HoveredUnit)
		|| HoveredUnit->GetCurrentHealth() <= 0
		|| HoveredUnit->GetStrategyUnitTeam() != EStrategyUnitTeam::AI)
	{
		ClearShootTargetHoverIndicator();
		return;
	}

	if (HighlightActor)
	{
		HighlightActor->ShowHoveredEnemyCell(
			GridManager,
			GridManager->WorldToGrid(HoveredUnit->GetActorLocation()));
	}

	FIntPoint SourceCell = GridManager->WorldToGrid(SelectedUnit->GetActorLocation());
	int32 MovementTimeUnitCost = 0;

	const bool bUsePendingMoveSource =
		bHasPendingMoveDestination
		&& PendingMoveUnit == SelectedUnit
		&& (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift));

	if (bUsePendingMoveSource)
	{
		int32 PendingMoveCost = 0;
		if (GridManager->TryGetMoveCostCells(SelectedUnit, PendingMoveCell, PendingMoveCost))
		{
			SourceCell = PendingMoveCell;
			MovementTimeUnitCost = PendingMoveCost;
		}
	}

	const TArray<AStrategyUnit*> ShootableTargets =
		GetShootableTargetsFromCell(SelectedUnit, SourceCell, MovementTimeUnitCost);
	if (!ShootableTargets.Contains(HoveredUnit))
	{
		ClearShootTargetHoverIndicator();
		return;
	}

	const FStrategyAttackContext Context = bUsePendingMoveSource
		? UStrategyAttackResolver::MakeContextFromCell(SelectedUnit, HoveredUnit, SourceCell)
		: UStrategyAttackResolver::MakeContext(SelectedUnit, HoveredUnit);
	if (!Context.AttackStats)
	{
		ClearShootTargetHoverIndicator();
		return;
	}

	const int32 HitChance = UStrategyAttackResolver::CalculateHitChance(Context);
	const int32 RemainingTimeUnitsAfterShot =
		FMath::Max(
			SelectedUnit->GetRemainingTimeUnits()
			- MovementTimeUnitCost
			- Context.AttackStats->TimeUnitCost,
			0);

	FVector2D ScreenPos;
	if (!UGameplayStatics::ProjectWorldToScreen(this, HoveredUnit->GetActorLocation(), ScreenPos))
	{
		ClearShootTargetHoverIndicator();
		return;
	}

	ShootTargetHoverHitChanceIconBrush = MakeShared<FSlateBrush>();
	ShootTargetHoverHitChanceIconBrush->ImageSize = ShootTargetHoverIconSize;
	ShootTargetHoverHitChanceIconBrush->DrawAs = ESlateBrushDrawType::Image;
	ShootTargetHoverHitChanceIconBrush->SetResourceObject(ShootTargetHoverHitChanceIcon);

	ShootTargetHoverActionPointIconBrush = MakeShared<FSlateBrush>();
	ShootTargetHoverActionPointIconBrush->ImageSize = ShootTargetHoverIconSize;
	ShootTargetHoverActionPointIconBrush->DrawAs = ESlateBrushDrawType::Image;
	ShootTargetHoverActionPointIconBrush->SetResourceObject(ShootTargetHoverActionPointIcon);

	const FString HitText = FString::Printf(TEXT("%d%%"), HitChance);
	const FString TimeUnitText = FString::Printf(TEXT("%d"), RemainingTimeUnitsAfterShot);
	const float WidgetWidth = 116.0f;
	const float WidgetHeight = 30.0f;

	if (ShootTargetHoverSlateWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ShootTargetHoverSlateWidget.ToSharedRef());
	}

	ShootTargetHoverSlateWidget =
		SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.0f, 0.0f))
		.Alignment(FVector2D(0.5f, 1.0f))
		.Offset(FMargin(ScreenPos.X, ScreenPos.Y - 92.0f, WidgetWidth, WidgetHeight))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
			[
				SNew(SBox)
				.WidthOverride(ShootTargetHoverIconSize.X)
				.HeightOverride(ShootTargetHoverIconSize.Y)
				[
					SNew(SImage)
					.Image(ShootTargetHoverHitChanceIcon ? ShootTargetHoverHitChanceIconBrush.Get() : FCoreStyle::Get().GetBrush(TEXT("Icons.Warning")))
					.ColorAndOpacity(ShootTargetHoverHitChanceIcon ? FLinearColor::White : FLinearColor(0.95f, 0.2f, 0.15f, 1.0f))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 10.0f, 0.0f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(HitText))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.9f, 1.0f)))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				.ShadowOffset(FVector2D(1.0f, 1.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
			[
				SNew(SBox)
				.WidthOverride(ShootTargetHoverIconSize.X)
				.HeightOverride(ShootTargetHoverIconSize.Y)
				[
					SNew(SImage)
					.Image(ShootTargetHoverActionPointIcon ? ShootTargetHoverActionPointIconBrush.Get() : FCoreStyle::Get().GetBrush(TEXT("Icons.Warning")))
					.ColorAndOpacity(ShootTargetHoverActionPointIcon ? FLinearColor::White : FLinearColor(0.95f, 0.2f, 0.15f, 1.0f))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TimeUnitText))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.95f, 0.3f, 1.0f)))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				.ShadowOffset(FVector2D(1.0f, 1.0f))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f))
			]
		];

	GEngine->GameViewport->AddViewportWidgetContent(ShootTargetHoverSlateWidget.ToSharedRef(), 1220);
	LastShootTargetHoverUnit = HoveredUnit;
}

bool AStrategyPlayerController::TryAttackHoveredEnemy()
{
	AStrategyUnit* SelectedUnit = ControlledUnits.Num() == 1 ? ControlledUnits[0] : TargetUnit;
	if (!IsValid(SelectedUnit) && ControlledUnits.Num() > 0)
	{
		SelectedUnit = ControlledUnits[0];
	}

	AStrategyUnit* HoveredUnit = GetHoveredStrategyUnit();
	if (!IsValid(SelectedUnit)
		|| !IsSelectableUnit(SelectedUnit)
		|| SelectedUnit->GetCurrentHealth() <= 0
		|| !IsValid(HoveredUnit)
		|| HoveredUnit->GetCurrentHealth() <= 0
		|| HoveredUnit->GetStrategyUnitTeam() != EStrategyUnitTeam::AI)
	{
		return false;
	}

	return TryExecuteDirectAttack(SelectedUnit, HoveredUnit);
}

bool AStrategyPlayerController::TryExecuteDirectAttack(AStrategyUnit* Attacker, AStrategyUnit* Target)
{
	if (!GridManager || !IsValid(Attacker) || !IsValid(Target))
	{
		return false;
	}

	const FIntPoint AttackerCell = GridManager->WorldToGrid(Attacker->GetActorLocation());
	if (GetShootableTargetsFromCell(Attacker, AttackerCell, 0).Contains(Target))
	{
		const FStrategyAttackContext Context = UStrategyAttackResolver::MakeContext(Attacker, Target);
		if (!Context.AttackStats)
		{
			return false;
		}

		UStrategyAttackResolver::ResolveAndApply(Context);
		Attacker->SpendWeaponAttackResources();
		ClearShootTargetHoverIndicator();
		RefreshActionBar();
		RefreshPlayerUnitRoster();
		RefreshWeaponInfoPanel();
		RefreshShootableTargetIcons();
		UpdateMovementHighlights();
		return true;
	}

	const FAttackStats* MeleeAttackStats = Attacker->GetMeleeAttackStats();
	if (!MeleeAttackStats || Attacker->GetRemainingTimeUnits() < MeleeAttackStats->TimeUnitCost)
	{
		return false;
	}

	const FIntPoint TargetCell = GridManager->WorldToGrid(Target->GetActorLocation());
	const int32 Distance =
		FMath::Abs(AttackerCell.X - TargetCell.X)
		+ FMath::Abs(AttackerCell.Y - TargetCell.Y);
	if (Distance > MeleeAttackStats->Range)
	{
		return false;
	}

	const FStrategyAttackContext Context =
		UStrategyAttackResolver::MakeContextWithAttackStats(Attacker, Target, MeleeAttackStats);
	UStrategyAttackResolver::ResolveAndApply(Context);
	Attacker->PlayMeleeAttackMontage();
	Attacker->SpendMeleeAttackResources();
	ClearShootTargetHoverIndicator();
	RefreshActionBar();
	RefreshPlayerUnitRoster();
	RefreshWeaponInfoPanel();
	RefreshShootableTargetIcons();
	UpdateMovementHighlights();
	return true;
}

void AStrategyPlayerController::RefreshShootableTargetIcons()
{
	ClearShootableTargetIcons();

	if (TargetingComponent && TargetingComponent->IsInFireMode())
	{
		return;
	}

	AStrategyUnit* SelectedUnit = ControlledUnits.Num() == 1 ? ControlledUnits[0] : TargetUnit;
	if (!IsValid(SelectedUnit) && ControlledUnits.Num() > 0)
	{
		SelectedUnit = ControlledUnits[0];
	}

	if (!IsValid(SelectedUnit)
		|| !IsSelectableUnit(SelectedUnit)
		|| SelectedUnit->GetCurrentHealth() <= 0)
	{
		return;
	}

	const FIntPoint SourceCell = GridManager
		? GridManager->WorldToGrid(SelectedUnit->GetActorLocation())
		: FIntPoint::ZeroValue;
	const TArray<AStrategyUnit*> Targets = GetShootableTargetsFromCell(SelectedUnit, SourceCell, 0);

	if (Targets.Num() == 0 || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TSharedRef<SHorizontalBox> IconRow = SNew(SHorizontalBox);

	for (AStrategyUnit* Target : Targets)
	{
		UTexture2D* IconTexture = Target ? Target->GetShootableTargetIconTexture() : nullptr;

		TSharedPtr<FSlateBrush> IconBrush = MakeShared<FSlateBrush>();
		IconBrush->ImageSize = ShootableTargetIconSize;
		IconBrush->DrawAs = ESlateBrushDrawType::Image;
		IconBrush->SetResourceObject(IconTexture);
		ShootableTargetIconBrushes.Add(IconBrush);

		IconRow->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(ShootableTargetIconSpacing * 0.5f, 0.0f))
		[
			SNew(SBox)
			.WidthOverride(ShootableTargetIconSize.X)
			.HeightOverride(ShootableTargetIconSize.Y)
			[
				SNew(SImage)
				.Image(IconTexture ? IconBrush.Get() : FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.ColorAndOpacity(IconTexture ? FLinearColor::White : FLinearColor(1.0f, 0.0f, 0.0f, 0.85f))
			]
		];
	}

	const float BarWidth =
		Targets.Num() * ShootableTargetIconSize.X
		+ FMath::Max(Targets.Num() - 1, 0) * ShootableTargetIconSpacing;
	const float BarHeight = ShootableTargetIconSize.Y;

	ShootableTargetIconSlateWidget =
		SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.5f, 1.0f))
		.Alignment(FVector2D(0.5f, 1.0f))
		.Offset(FMargin(
			ShootableTargetIconBarOffset.X,
			ShootableTargetIconBarOffset.Y,
			BarWidth,
			BarHeight))
		[
			IconRow
		];

	GEngine->GameViewport->AddViewportWidgetContent(ShootableTargetIconSlateWidget.ToSharedRef(), 1210);
}

void AStrategyPlayerController::RefreshShootableTargetIconsForCell(
	AStrategyUnit* Unit,
	const FIntPoint& SourceCell,
	int32 MovementTimeUnitCost)
{
	ClearShootableTargetIcons();

	if (TargetingComponent && TargetingComponent->IsInFireMode())
	{
		return;
	}

	if (!IsValid(Unit)
		|| !IsSelectableUnit(Unit)
		|| Unit->GetCurrentHealth() <= 0)
	{
		return;
	}

	const TArray<AStrategyUnit*> Targets = GetShootableTargetsFromCell(Unit, SourceCell, MovementTimeUnitCost);
	if (Targets.Num() == 0 || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TSharedRef<SHorizontalBox> IconRow = SNew(SHorizontalBox);

	for (AStrategyUnit* Target : Targets)
	{
		UTexture2D* IconTexture = Target ? Target->GetShootableTargetIconTexture() : nullptr;

		TSharedPtr<FSlateBrush> IconBrush = MakeShared<FSlateBrush>();
		IconBrush->ImageSize = ShootableTargetIconSize;
		IconBrush->DrawAs = ESlateBrushDrawType::Image;
		IconBrush->SetResourceObject(IconTexture);
		ShootableTargetIconBrushes.Add(IconBrush);

		IconRow->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(ShootableTargetIconSpacing * 0.5f, 0.0f))
		[
			SNew(SBox)
			.WidthOverride(ShootableTargetIconSize.X)
			.HeightOverride(ShootableTargetIconSize.Y)
			[
				SNew(SImage)
				.Image(IconTexture ? IconBrush.Get() : FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.ColorAndOpacity(IconTexture ? FLinearColor::White : FLinearColor(1.0f, 0.0f, 0.0f, 0.85f))
			]
		];
	}

	const float BarWidth =
		Targets.Num() * ShootableTargetIconSize.X
		+ FMath::Max(Targets.Num() - 1, 0) * ShootableTargetIconSpacing;
	const float BarHeight = ShootableTargetIconSize.Y;

	ShootableTargetIconSlateWidget =
		SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(0.5f, 1.0f))
		.Alignment(FVector2D(0.5f, 1.0f))
		.Offset(FMargin(
			ShootableTargetIconBarOffset.X,
			ShootableTargetIconBarOffset.Y,
			BarWidth,
			BarHeight))
		[
			IconRow
		];

	GEngine->GameViewport->AddViewportWidgetContent(ShootableTargetIconSlateWidget.ToSharedRef(), 1210);
}

TArray<AStrategyUnit*> AStrategyPlayerController::GetShootableTargetsFromCell(
	AStrategyUnit* Unit,
	const FIntPoint& SourceCell,
	int32 MovementTimeUnitCost) const
{
	TArray<AStrategyUnit*> Targets;

	if (!GridManager || !IsValid(Unit))
	{
		return Targets;
	}

	const FStrategyWeaponInstance& FireWeapon = Unit->GetEquippedFireWeapon();
	const FAttackStats* AttackStats = FireWeapon.GetAttackStats();
	if (!FireWeapon.WeaponData || !AttackStats)
	{
		return Targets;
	}

	const int32 RemainingTimeUnitsAfterMove = Unit->GetRemainingTimeUnits() - FMath::Max(MovementTimeUnitCost, 0);
	if (RemainingTimeUnitsAfterMove < AttackStats->TimeUnitCost)
	{
		return Targets;
	}

	const int32 AmmoCost = FireWeapon.UsesAmmo()
		? FMath::Max(AttackStats->AmmoCost, 1)
		: 0;
	if (FireWeapon.UsesAmmo() && FireWeapon.CurrentAmmo < AmmoCost)
	{
		return Targets;
	}

	AStrategyGameMode* GameMode = GetStrategyGameMode();
	AAIStrategySide* EnemySide = GameMode ? GameMode->GetEnemySide() : nullptr;
	if (!EnemySide)
	{
		return Targets;
	}

	for (AStrategyUnit* Target : EnemySide->Units)
	{
		if (!IsValid(Target)
			|| Target->GetCurrentHealth() <= 0
			|| Target->GetStrategyUnitTeam() != EStrategyUnitTeam::AI)
		{
			continue;
		}

		const FIntPoint TargetCell = GridManager->WorldToGrid(Target->GetActorLocation());
		const int32 Distance =
			FMath::Abs(SourceCell.X - TargetCell.X)
			+ FMath::Abs(SourceCell.Y - TargetCell.Y);

		if (Distance <= AttackStats->Range)
		{
			Targets.Add(Target);
		}
	}

	return Targets;
}

AStrategyGameMode* AStrategyPlayerController::GetStrategyGameMode() const
{
	UWorld* World = GetWorld();
	return World ? Cast<AStrategyGameMode>(World->GetAuthGameMode()) : nullptr;
}

UStrategyTargetingComponent* AStrategyPlayerController::EnsureTargetingComponent()
{
	if (TargetingComponent)
	{
		return TargetingComponent;
	}

	TargetingComponent = NewObject<UStrategyTargetingComponent>(
		this,
		UStrategyTargetingComponent::StaticClass(),
		TEXT("TargetingComponentRuntime"));

	if (TargetingComponent)
	{
		AddInstanceComponent(TargetingComponent);
		TargetingComponent->RegisterComponent();
		UE_LOG(LogTemp, Warning, TEXT("StrategyPlayerController recreated missing TargetingComponent at runtime."));
	}

	return TargetingComponent;
}

void AStrategyPlayerController::RemoveTacticalHUD() const
{
	if (EndTurnWidget)
	{
		EndTurnWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UnitActionBarWidget)
	{
		UnitActionBarWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (TurnBannerWidget)
	{
		TurnBannerWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AStrategyPlayerController::ShowTacticalHUD()
{
	RestoreTacticalView();

	if (EndTurnWidget)
	{
		EndTurnWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (UnitActionBarWidget)
	{
		UnitActionBarWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (TurnBannerWidget)
	{
		TurnBannerWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (PlayerUnitRosterWidget)
	{
		PlayerUnitRosterWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	RefreshActionBar();
	RefreshPlayerUnitRoster();
	RefreshWeaponInfoPanel();
	RefreshShootableTargetIcons();
}

void AStrategyPlayerController::RestoreTacticalView(float BlendTime)
{
	if (!ControlledPawn)
	{
		ControlledPawn = Cast<AStrategyPawn>(GetPawn());
	}

	if (ControlledPawn && GetViewTarget() != ControlledPawn)
	{
		SetViewTargetWithBlend(
			ControlledPawn,
			BlendTime,
			EViewTargetBlendFunction::VTBlend_Cubic);
	}

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI TacticalInputMode;
	TacticalInputMode.SetHideCursorDuringCapture(false);
	TacticalInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(TacticalInputMode);
}

void AStrategyPlayerController::ShowTargetingHUD()
{
	ClearShootableTargetIcons();

	if (TargetingHUD)
	{
		TargetingHUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	RefreshWeaponInfoPanel();

	if (EndTurnWidget)
	{
		EndTurnWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UnitActionBarWidget)
	{
		UnitActionBarWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	CurrentMouseCursor = EMouseCursor::Default;

	FInputModeGameAndUI TargetingInputMode;
	TargetingInputMode.SetHideCursorDuringCapture(false);
	TargetingInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(TargetingInputMode);
}

void AStrategyPlayerController::HideTargetingHUD()
{
	if (TargetingHUD)
	{
		TargetingHUD->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (EndTurnWidget)
	{
		EndTurnWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (UnitActionBarWidget)
	{
		UnitActionBarWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	RefreshWeaponInfoPanel();
}

void AStrategyPlayerController::RefreshWeaponInfoPanel()
{
	AStrategyUnit* SelectedUnit = ControlledUnits.Num() == 1 ? ControlledUnits[0] : TargetUnit;
	if (!IsValid(SelectedUnit) || SelectedUnit->GetCurrentHealth() <= 0)
	{
		SelectedUnit = nullptr;
	}

	EnsureWeaponInfoSlateWidget();
	UpdateWeaponInfoSlateWidget(SelectedUnit);
	UpdateWeaponDebugSlateWidget();
}

void AStrategyPlayerController::SuppressSelectionInputBriefly()
{
	if (GetWorld())
	{
		IgnoreSelectionInputUntilTime = GetWorld()->GetRealTimeSeconds() + 0.15f;
	}
}

void AStrategyPlayerController::EnsureWeaponInfoSlateWidget()
{
	if (WeaponInfoSlateWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	WeaponInfoSlateWidget = SNew(SWeaponInfoSlateWidget);

	GEngine->GameViewport->AddViewportWidgetContent(WeaponInfoSlateWidget.ToSharedRef(), 5000);
}

void AStrategyPlayerController::UpdateWeaponInfoSlateWidget(AStrategyUnit* SelectedUnit)
{
	if (WeaponInfoSlateWidget.IsValid())
	{
		WeaponInfoSlateWidget->SetUnit(SelectedUnit);
	}
}

void AStrategyPlayerController::ToggleWeaponDebugMenu()
{
	if (!bEnableWeaponDebugMenu)
	{
		return;
	}

	EnsureWeaponDebugSlateWidget();
	if (!WeaponDebugSlateWidget.IsValid())
	{
		return;
	}

	const EVisibility CurrentVisibility = WeaponDebugSlateWidget->GetVisibility();
	if (CurrentVisibility == EVisibility::Visible)
	{
		HideWeaponDebugMenu();
	}
	else
	{
		ShowWeaponDebugMenu();
	}
}

void AStrategyPlayerController::ShowWeaponDebugMenu()
{
	if (!bEnableWeaponDebugMenu)
	{
		return;
	}

	EnsureWeaponDebugSlateWidget();
	if (WeaponDebugSlateWidget.IsValid())
	{
		UpdateWeaponDebugSlateWidget();
		WeaponDebugSlateWidget->SetVisibility(EVisibility::Visible);
	}
}

void AStrategyPlayerController::HideWeaponDebugMenu()
{
	if (WeaponDebugSlateWidget.IsValid())
	{
		WeaponDebugSlateWidget->SetVisibility(EVisibility::Collapsed);
	}
}

void AStrategyPlayerController::DebugEquipWeapon(FName WeaponId)
{
	UStrategyWeaponData* Weapon = FindDebugWeapon(WeaponId);
	if (!Weapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("DebugEquipWeapon failed: no weapon with id '%s'"), *WeaponId.ToString());
		return;
	}

	HandleDebugWeaponPicked(Weapon);
}

void AStrategyPlayerController::EnsureWeaponDebugSlateWidget()
{
	if (WeaponDebugSlateWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	GEngine->GameViewport->AddViewportWidgetContent(
		SNew(SConstraintCanvas)
		+ SConstraintCanvas::Slot()
		.Anchors(FAnchors(1.0f, 0.0f))
		.Alignment(FVector2D(1.0f, 0.0f))
		.Offset(FMargin(-24.0f, 180.0f, 320.0f, 520.0f))
		[
			SAssignNew(WeaponDebugSlateWidget, SWeaponDebugSlateWidget)
			.OnWeaponPicked(FOnDebugWeaponPicked::CreateUObject(this, &AStrategyPlayerController::HandleDebugWeaponPicked))
		],
		6000);

	WeaponDebugSlateWidget->SetVisibility(EVisibility::Collapsed);
	UpdateWeaponDebugSlateWidget();
}

void AStrategyPlayerController::UpdateWeaponDebugSlateWidget()
{
	if (WeaponDebugSlateWidget.IsValid())
	{
		WeaponDebugSlateWidget->SetContext(GetWeaponDebugTargetUnits(), GetDebugWeapons());
	}
}

void AStrategyPlayerController::HandleDebugWeaponPicked(UStrategyWeaponData* WeaponData)
{
	const TArray<AStrategyUnit*> Units = GetWeaponDebugTargetUnits();
	if (Units.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Weapon debug equip ignored: no player units selected"));
		return;
	}

	for (AStrategyUnit* Unit : Units)
	{
		if (IsValid(Unit))
		{
			if (WeaponData)
			{
				Unit->EquipWeapon(WeaponData);
			}
			else
			{
				Unit->ClearEquippedWeapons();
			}
		}
	}

	RefreshActionBar();
	RefreshPlayerUnitRoster();
	RefreshWeaponInfoPanel();

	UE_LOG(LogTemp, Warning, TEXT("Weapon debug %s on %d unit(s)"),
		WeaponData ? *FString::Printf(TEXT("equipped %s"), *GetNameSafe(WeaponData)) : TEXT("cleared weapons"),
		Units.Num());
}

TArray<AStrategyUnit*> AStrategyPlayerController::GetWeaponDebugTargetUnits() const
{
	TArray<AStrategyUnit*> Units;

	for (AStrategyUnit* Unit : ControlledUnits)
	{
		if (IsValid(Unit) && IsSelectableUnit(Unit) && Unit->GetCurrentHealth() > 0)
		{
			Units.Add(Unit);
		}
	}

	if (Units.Num() == 0 && IsValid(TargetUnit) && IsSelectableUnit(TargetUnit) && TargetUnit->GetCurrentHealth() > 0)
	{
		Units.Add(TargetUnit);
	}

	return Units;
}

TArray<UStrategyWeaponData*> AStrategyPlayerController::GetDebugWeapons() const
{
	TArray<UStrategyWeaponData*> Weapons;
	TSet<UStrategyWeaponData*> SeenWeapons;

	auto AddWeapon = [&Weapons, &SeenWeapons](UStrategyWeaponData* Weapon)
	{
		if (Weapon && !SeenWeapons.Contains(Weapon))
		{
			SeenWeapons.Add(Weapon);
			Weapons.Add(Weapon);
		}
	};

	if (WeaponDatabase)
	{
		for (UStrategyWeaponData* Weapon : WeaponDatabase->Weapons)
		{
			AddWeapon(Weapon);
		}
	}

	if (Weapons.Num() == 0)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetDataList;
		AssetRegistryModule.Get().GetAssetsByClass(UStrategyWeaponData::StaticClass()->GetClassPathName(), AssetDataList, true);

		for (const FAssetData& AssetData : AssetDataList)
		{
			AddWeapon(Cast<UStrategyWeaponData>(AssetData.GetAsset()));
		}
	}

	if (Weapons.Num() == 0)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetDataList;
		AssetRegistryModule.Get().GetAssetsByPath(FName(TEXT("/Game")), AssetDataList, true);

		for (const FAssetData& AssetData : AssetDataList)
		{
			FString NativeClass;
			FString PrimaryAssetType;
			AssetData.GetTagValue(FName(TEXT("NativeClass")), NativeClass);
			AssetData.GetTagValue(FName(TEXT("PrimaryAssetType")), PrimaryAssetType);

			const bool bLooksLikeWeaponData =
				AssetData.AssetClassPath.GetAssetName() == UStrategyWeaponData::StaticClass()->GetFName()
				|| NativeClass.Contains(TEXT("/Script/ZombieGame.StrategyWeaponData"))
				|| PrimaryAssetType == TEXT("StrategyWeaponData");

			if (bLooksLikeWeaponData)
			{
				AddWeapon(Cast<UStrategyWeaponData>(AssetData.GetAsset()));
			}
		}
	}

	if (Weapons.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Weapon debug menu found no StrategyWeaponData assets. Assign a StrategyWeaponDatabase asset or verify DA weapons are saved."));
	}

	Weapons.Sort([](const UStrategyWeaponData& Left, const UStrategyWeaponData& Right)
	{
		const FString LeftName = Left.DisplayName.IsEmpty() ? Left.WeaponId.ToString() : Left.DisplayName.ToString();
		const FString RightName = Right.DisplayName.IsEmpty() ? Right.WeaponId.ToString() : Right.DisplayName.ToString();
		return LeftName < RightName;
	});

	return Weapons;
}

UStrategyWeaponData* AStrategyPlayerController::FindDebugWeapon(FName WeaponId) const
{
	if (WeaponDatabase)
	{
		if (UStrategyWeaponData* Weapon = WeaponDatabase->FindWeaponById(WeaponId))
		{
			return Weapon;
		}
	}

	for (UStrategyWeaponData* Weapon : GetDebugWeapons())
	{
		if (Weapon && Weapon->WeaponId == WeaponId)
		{
			return Weapon;
		}
	}

	return nullptr;
}

PRAGMA_ENABLE_OPTIMIZATION
