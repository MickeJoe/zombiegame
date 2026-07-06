// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Systems/GridHighlightActor.h"
#include "UI/TurnBannerWidget.h"
#include "UI/WeaponDebugSlateWidget.h"
#include "StrategyPlayerController.generated.h"

enum class EPlayerUnitActionType : uint8;
class SWeaponInfoSlateWidget;
class SWeaponDebugSlateWidget;
struct FSlateBrush;
class UShootableTargetIconBarWidget;
class UShootableTargetIconWidget;
class SWidget;
class UTargetingHUDWidget;
class UTargetingActionBarWidget;
class UStrategyTargetingComponent;
class UStrategyWeaponData;
class UStrategyWeaponDatabase;
class UTexture2D;
class UPlayerUnitRosterWidget;
class AStrategyGameMode;
class AStrategyUnit;
class UUnitActionBarWidget;
class AGridHighlightActor;
class AGridManager;
class AStrategyPawn;
class UInputMappingContext;
class UNiagaraSystem;
struct FInputActionValue;
struct FCollisionQueryParams;
struct FHitResult;
class AStrategyHUD;
class AStrategyNPC;
class UInputAction;
class UEndTurnWidget;

/** Enum to determine the last used input type */
UENUM(BlueprintType)
enum EStrategyInputMode : uint8
{
	SIM_Mouse	UMETA(DisplayName = "Mouse"),
	SIM_Touch	UMETA(DisplayName = "Touch")
};

/**
 *  Player Controller for a top-down strategy game.
 *  Handles unit selection and commands.
 *  Implements both mouse and touch controls.
 */
UCLASS(abstract)
class AStrategyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void ShowTurnBanner(ETurnOwner TurnOwner);

	void SetPlayerEndTurnButtonEnabled(bool bIsEnabledIn);	
	void RefreshPlayerUnitRoster();

protected:

	/** Strategy Pawn associated with this controller */
	TObjectPtr<AStrategyPawn> ControlledPawn;

	/** Strategy HUD associated with this controller */
	TObjectPtr<AStrategyHUD> StrategyHUD;

	/** Determines the chosen input type */
	UPROPERTY(EditAnywhere, Category="Input")
	TEnumAsByte<EStrategyInputMode> InputMode = SIM_Mouse;

	/** Input mapping context to use with mouse input */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* MouseMappingContext;

	/** Input mapping context to use with touch input */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* TouchMappingContext;

	/** If true, the player is adding or removing units to the selected units list */
	bool bSelectionModifier = false;

	/** If true, double-tap touch select all mode is active */
	bool bDoubleTapActive = false;

	/** If true, allow the player to interact with game objects */
	bool bAllowInteraction = true;

	/** Input Action for moving the camera */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveCameraAction;

	/** Input Action for zooming the camera */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ZoomCameraAction;

	/** Input Action for resetting the camera to its default position */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ResetCameraAction;

	/** Input Action for select and click */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectClickAction;

	/** Input Action for select press and hold */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectHoldAction;

	/** Input Action for click interaction */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractClickAction;

	/** Input Action for interaction press and hold */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractHoldAction;

	/** Input Action for modifying selection mode */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectionModifierAction;

	/** Input Action for primary touch hold */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TouchPrimaryHoldAction;

	/** Input Action for secondary touch */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TouchSecondaryAction;

	/** Max distance to look for nearby units when doing a click or touch interaction */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float InteractionRadius = 250.0f;

	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 500, Units = "cm"))
	float ShootTargetHoverTraceRadius = 45.0f;

	/** Max distance between the starting and current position of the second touch finger to be considered a box selection */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000))
	float MinSecondFingerDistanceForBoxSelect = 10.0f;

	/** Saves the world location of the last initiated interaction */
	FVector CachedInteraction;

	/** Saves the world location of the last initiated unit selection */
	FVector CachedSelection;

	/** Saves the world location where the player started a press and hold interaction */
	FVector2D StartingInteractionPosition;

	/** Saves the current world location of the player's cursor in press and hold interaction */
	FVector2D CurrentInteractionPosition;

	/** Saves the starting world location of a player's cursor in a press and hold selection box */
	FVector2D StartingSelectionPosition;

	/** Saves the starting location of a two-finger touch interaction (pinch) */
	FVector2D StartingSecondFingerPosition;

	/** Saves the current location of a two-finger touch interaction (pinch) */
	FVector2D CurrentSecondFingerPosition;

	/** Current camera zoom level */
	float CameraZoom;

	/** Default camera zoom level */
	float DefaultZoom;

	/** Minimum allowed camera zoom level */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float MinZoomLevel = 1000.0f;

	/** Maximum allowed camera zoom level */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float MaxZoomLevel = 2500.0f;

	/** Scales zoom inputs by this value */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 1000))
	float ZoomScaling = 100.0f;

	/** Degrees per second used when rotating the strategy camera with keyboard input */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 720))
	float CameraRotationSpeed = 90.0f;

	/** Affects how fast the camera moves while dragging with the mouse */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float DragMultiplier = 0.1f;

	/** Trace channel to use for selection trace checks */
	UPROPERTY(EditAnywhere, Category = "Selection")
	TEnumAsByte<ETraceTypeQuery> SelectionTraceChannel;

	/** Currently selected unit */
	AStrategyUnit* TargetUnit = nullptr;

	/** Currently selected unit list */
	TArray<AStrategyUnit*> ControlledUnits;

	///////////////////////////////////
	// Touchscreen enhanced input workaround

	/** Game time when the player last started tapping the touchscreen */
	float LastTapPressTime = 0.0f;

	/** Game time when the player last ended tapping the touchscreen */
	float LastTapReleaseTime = 0.0f;

	/** Number of successive times the player has tapped the touchscreen */
	int32 TapCount = 0;

	/** Game time when the player last completed a box select on the touchscreen */
	float LastBoxSelectTime = 0.0f;

	float IgnoreSelectionInputUntilTime = 0.0f;

	/** Max time between touch press and release to be considered a tap */
	UPROPERTY(EditAnywhere, Category = "Touch Input", meta = (ClampMin = 0, ClampMax = 1, Units="s"))
	float TouchTapMaxAllowedTime = 0.15f;

	/** Max time between touchscreen taps to be considered a double tap */
	UPROPERTY(EditAnywhere, Category = "Touch Input", meta = (ClampMin = 0, ClampMax = 1, Units="s"))
	float TouchDoubleTapMaxAllowedTime = 0.4f;

public:

	/** Constructor */
	AStrategyPlayerController();

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;

	virtual void PlayerTick(float DeltaTime) override;

	/** Pawn initialization */
	virtual void OnPossess(APawn* InPawn);

public:

	/** Updates selected units from the HUD's drag select box */
	void DragSelectUnits(const TArray<AStrategyUnit*>& Units);

	/** Passes the list of selected units */
	const TArray<AStrategyUnit*>& GetSelectedUnits();
	
	UStrategyTargetingComponent* GetTargetingComponent();
	UTargetingHUDWidget* GetTargetingHUDWidget() const { return TargetingHUD; }
	
	void RemoveTacticalHUD() const;
	void ShowTacticalHUD();
	void RestoreTacticalView(float BlendTime = 0.25f);
	
	void ShowTargetingHUD();
	void HideTargetingHUD();
	void RefreshWeaponInfoPanel();
	void SuppressSelectionInputBriefly();
	void RefreshLockedOverwatchHighlights();
	void ClearShootableTargetIcons();
	void ClearShootTargetHoverIndicator();

protected:

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UTurnBannerWidget> TurnBannerWidgetClass;

	UPROPERTY()
	TObjectPtr<UTurnBannerWidget> TurnBannerWidget;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UEndTurnWidget> EndTurnWidgetClass;

	UPROPERTY()
	TObjectPtr<UEndTurnWidget> EndTurnWidget;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUnitActionBarWidget> UnitActionBarWidgetClass;

	UPROPERTY()
	TObjectPtr<UUnitActionBarWidget> UnitActionBarWidget;

	UPROPERTY(EditDefaultsOnly, Category="UI|Targeting")
	TSubclassOf<UShootableTargetIconBarWidget> ShootableTargetIconBarWidgetClass;

	UPROPERTY()
	TObjectPtr<UShootableTargetIconBarWidget> ShootableTargetIconBarWidget;

	UPROPERTY(EditDefaultsOnly, Category="UI|Targeting")
	TSubclassOf<UShootableTargetIconWidget> ShootableTargetIconWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI|Targeting")
	FVector2D ShootableTargetIconBarOffset = FVector2D(0.0f, -158.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI|Targeting")
	FVector2D ShootableTargetIconSize = FVector2D(34.0f, 34.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI|Targeting")
	float ShootableTargetIconSpacing = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI|Targeting")
	TObjectPtr<UTexture2D> ShootTargetHoverHitChanceIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI|Targeting")
	TObjectPtr<UTexture2D> ShootTargetHoverActionPointIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI|Targeting")
	FVector2D ShootTargetHoverIconSize = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UPlayerUnitRosterWidget> PlayerUnitRosterWidgetClass;

	UPROPERTY()
	TObjectPtr<UPlayerUnitRosterWidget> PlayerUnitRosterWidget;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Targeting", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStrategyTargetingComponent> TargetingComponent;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UTargetingHUDWidget> TargetingHUDClass;

	UPROPERTY()
	TObjectPtr<UTargetingHUDWidget> TargetingHUD;

	UFUNCTION()
	void HandleEndTurnClicked();
	
	/** Moves the camera by the given input */
	void MoveCamera(const FInputActionValue& Value);

	/** Changes the camera zoom level by the given input */
	void ZoomCamera(const FInputActionValue& Value);

	/** Rotates the camera around the vertical axis */
	void RotateCamera(float Direction);

	/** Resets the camera to its initial value */
	void ResetCamera(const FInputActionValue& Value);

	/** Start a select and hold input */
	void SelectHoldStarted(const FInputActionValue& Value);
	
	/** Select and hold input triggered */
	void SelectHoldTriggered(const FInputActionValue& Value);

	/** Select and hold input completed */
	void SelectHoldCompleted(const FInputActionValue& Value);

	/** Select click action */
	void SelectClick(const FInputActionValue& Value);

	/** Presses or releases the selection modifier key */
	void SelectionModifier(const FInputActionValue& Value);

	/** Starts an interaction hold input */
	void InteractHoldStarted(const FInputActionValue& Value);

	/** Interaction hold input triggered */
	void InteractHoldTriggered(const FInputActionValue& Value);

	/** Interaction click input started */
	void InteractClickStarted(const FInputActionValue& Value);

	/** Interaction click input completed */
	void InteractClickCompleted(const FInputActionValue& Value);

	/** Touch primary finger hold started */
	void TouchPrimaryHoldStarted(const FInputActionValue& Value);

	/** Touch primary finger hold triggered */
	void TouchPrimaryHoldTriggered(const FInputActionValue& Value);

	/** Touch primary finger hold completed */
	void TouchPrimaryHoldCompleted(const FInputActionValue& Value);

	/** Touch secondary finger started */
	void TouchSecondaryStarted(const FInputActionValue& Value);

	/** Touch secondary finger triggered */
	void TouchSecondaryTriggered(const FInputActionValue& Value);

	/** Touch secondary finger completed */
	void TouchSecondaryCompleted(const FInputActionValue& Value);

	void EscapePressed();

	/** Attempt to select or deselect units at the cached location */
	void DoSelectionCommand();

	/** Select all units currently on screen */
	void DoSelectAllOnScreenCommand();

	/** Deselect all controlled units */
	void DoDeselectAllCommand();

	/** Drag scroll the camera */
	void DoDragScrollCommand();

	/** Move all selected units */
	void DoMoveUnitsCommand();

	/** Called when a unit move is completed */
	UFUNCTION()
	void OnMoveCompleted(AStrategyUnit* MovedUnit);

	/** Sorts all controlled units based on their distance to the provided world location */
	AStrategyUnit* GetClosestSelectedUnitToLocation(FVector TargetLocation);

	/** Calculates and returns the current mouse location */
	FVector2D GetMouseLocation();

	/** Attempts to get the world location under the cursor, returns true if successful */
	bool GetLocationUnderCursor(FVector& Location);
	AStrategyUnit* GetStrategyUnitFromHit(const FHitResult& Hit) const;
	void AddAllStrategyUnitsToIgnoredActors(FCollisionQueryParams& QueryParams) const;

	/** Projects the current touch location into world space */
	FVector ProjectTouchPointToWorldSpace();

	/** Spawns the positive cursor effect */
	UFUNCTION(BlueprintImplementableEvent, Category="Cursor", meta = (DisplayName="Cursor Feedback"))
	void BP_CursorFeedback(FVector Location, bool bPositive);

	/** Resets the interaction flag */
	void ResetInteraction();

	/** Detects taps and double taps for mobile platforms. */
	void CheckTouchTap(bool& bTapped, bool& bDoubleTapped);
	
	void UpdateMovementHighlights();
	void UpdateMovementPreview();
	bool IsSelectableUnit(const AStrategyUnit* Unit) const;
	bool IsPointerOverBlockingUI() const;
	void RefreshShootableTargetIcons();
	void RefreshShootableTargetIconsForCell(AStrategyUnit* Unit, const FIntPoint& SourceCell, int32 MovementTimeUnitCost);
	void UpdateShootTargetHoverIndicator();
	bool TryAttackHoveredEnemy();
	bool TrySelectOrUseMedicBag();
	bool TryExecuteDirectAttack(AStrategyUnit* Attacker, AStrategyUnit* Target);
	void BeginOverwatchPlacement(AStrategyUnit* Unit);
	void UpdateOverwatchPlacementPreview();
	void ConfirmOverwatchPlacement();
	void CancelOverwatchPlacement();
	TArray<FIntPoint> BuildOverwatchConeCells(const AStrategyUnit* Unit, const FVector& AimLocation);
	bool HasOverwatchLineOfSight(const AStrategyUnit* Unit, const FIntPoint& Cell) const;
	FOverwatchBoundaryLine MakeOverwatchBoundaryLine(const AStrategyUnit* Unit, const FVector& Direction, const TArray<FIntPoint>& Cells) const;
	bool BuildMovementPathPreview(const AStrategyUnit* Unit, const FIntPoint& TargetCell, TArray<FVector>& OutPathPoints) const;
	TArray<FGridCoverIndicator> BuildCoverIndicatorsForCell(const FIntPoint& Cell) const;
	bool GetCoverTypeForDirection(const FIntPoint& Cell, const FIntPoint& Direction, EGridCoverType& OutCoverType) const;
	
	UFUNCTION()
	void HandleUnitActionClicked(EPlayerUnitActionType ActionType);

	UFUNCTION()
	void HandleRosterUnitClicked(AStrategyUnit* Unit);

	void SelectRosterUnit(AStrategyUnit* Unit);
	void CenterCameraOnUnit(const AStrategyUnit* Unit);
public:
	void RefreshActionBar();
	void SetAlwaysMeleeAttackEnabled(bool bEnabled);
	bool IsAlwaysMeleeAttackEnabled() const { return bAlwaysMeleeAttackEnabled; }

	UFUNCTION(BlueprintCallable, Exec, Category="Debug|Weapons")
	void ToggleWeaponDebugMenu();

	UFUNCTION(BlueprintCallable, Exec, Category="Debug|Weapons")
	void ShowWeaponDebugMenu();

	UFUNCTION(BlueprintCallable, Exec, Category="Debug|Weapons")
	void HideWeaponDebugMenu();

	UFUNCTION(BlueprintCallable, Exec, Category="Debug|Weapons")
	void DebugEquipWeapon(FName ItemId);

	UFUNCTION(BlueprintCallable, Exec, Category="Debug|Levels")
	void DebugOpenLevel(FName LevelPackageName);
protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	TObjectPtr<AGridHighlightActor> HighlightActor;

	UPROPERTY(Transient)
	TArray<FIntPoint> ReachableCells;

	UPROPERTY(Transient)
	FIntPoint LastMovementPreviewCell = FIntPoint::ZeroValue;

	UPROPERTY(Transient)
	TObjectPtr<AStrategyUnit> LastMovementPreviewUnit = nullptr;

	UPROPERTY(Transient)
	FIntPoint PendingMoveCell = FIntPoint::ZeroValue;

	UPROPERTY(Transient)
	TObjectPtr<AStrategyUnit> PendingMoveUnit = nullptr;

	UPROPERTY(Transient)
	bool bHasPendingMoveDestination = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cover")
	float CoverHalfTraceHeight = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cover")
	float CoverFullTraceHeight = 165.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cover")
	float CoverIndicatorInset = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overwatch")
	float OverwatchLineOfSightHeightOffset = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overwatch", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OverwatchMinGroundNormalZ = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Debug|Weapons")
	TObjectPtr<UStrategyWeaponDatabase> WeaponDatabase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Debug|Weapons")
	bool bEnableWeaponDebugMenu = true;

	UPROPERTY(Transient)
	TObjectPtr<AStrategyUnit> OverwatchPlacementUnit = nullptr;

	UPROPERTY(Transient)
	FVector OverwatchPlacementDirection = FVector::ForwardVector;

	UPROPERTY(Transient)
	TArray<FIntPoint> OverwatchPreviewCells;

	bool bIsPlacingOverwatch = false;
	bool bAlwaysMeleeAttackEnabled = false;
	bool bWasWeaponDebugHotkeyDown = false;
	
	UPROPERTY(Transient)
	AGridManager* GridManager;
	
private:
	AStrategyGameMode* GetStrategyGameMode() const;
	UStrategyTargetingComponent* EnsureTargetingComponent();
	void EnsureWeaponInfoSlateWidget();
	void UpdateWeaponInfoSlateWidget(AStrategyUnit* SelectedUnit);
	void EnsureWeaponDebugSlateWidget();
	void UpdateWeaponDebugSlateWidget();
	void HandleDebugWeaponPicked(UStrategyWeaponData* WeaponData);
	void HandleDebugLevelPicked(FName LevelPackageName);
	TArray<AStrategyUnit*> GetWeaponDebugTargetUnits() const;
	TArray<UStrategyWeaponData*> GetDebugWeapons() const;
	TArray<FDebugLevelEntry> GetDebugLevels() const;
	bool IsDebugLevelAllowed(FName LevelPackageName) const;
	UStrategyWeaponData* FindDebugWeapon(FName ItemId) const;
	TArray<AStrategyUnit*> GetShootableTargetsFromCell(AStrategyUnit* Unit, const FIntPoint& SourceCell, int32 MovementTimeUnitCost) const;
	AStrategyUnit* GetHoveredStrategyUnit();
	void ClearPendingMedicBagTarget();

	TSharedPtr<SWeaponInfoSlateWidget> WeaponInfoSlateWidget;
	TSharedPtr<SWeaponDebugSlateWidget> WeaponDebugSlateWidget;
	TSharedPtr<SWidget> ShootableTargetIconSlateWidget;
	TArray<TSharedPtr<FSlateBrush>> ShootableTargetIconBrushes;
	TSharedPtr<SWidget> ShootTargetHoverSlateWidget;
	TSharedPtr<FSlateBrush> ShootTargetHoverHitChanceIconBrush;
	TSharedPtr<FSlateBrush> ShootTargetHoverActionPointIconBrush;
	TObjectPtr<AStrategyUnit> LastShootTargetHoverUnit = nullptr;
	TObjectPtr<AStrategyUnit> PendingMedicBagTarget = nullptr;
	
};
