// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyUnit.h"
#include "../../Systems/GridManager.h"
#include "../../Enemy_AI/EnemyUnitAI.h"
#include "Player/AIStrategySide.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"
#include "Systems/AttackHandling/StrategyWeaponData.h"
#include "AIController.h"
#include "StrategyGameMode.h"
#include "StrategyPlayerController.h"
#include "UI/TargetingUI/StrategyTargetingComponent.h"
#include "UnitStatusBarWidget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Camera/CameraComponent.h"

AStrategyUnit::AStrategyUnit()
{
	PrimaryActorTick.bCanEverTick = true;
	
	GridManager = Cast<AGridManager>(
	UGameplayStatics::GetActorOfClass(this, AGridManager::StaticClass())
	);
	
	// ensure this unit has a valid AI controller to handle move requests
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// create the interaction range sphere
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Range"));
	InteractionRange->SetupAttachment(RootComponent);

	InteractionRange->SetSphereRadius(100.0f);
	InteractionRange->SetCollisionProfileName(FName("OverlapAllDynamic"));

	// configure movement
	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 1000.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.0f;
	GetCharacterMovement()->PerchRadiusThreshold = 20.0f;
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 150.0f;
	GetCharacterMovement()->AvoidanceWeight = 1.0f;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->SetFixedBrakingDistance(200.0f);
	GetCharacterMovement()->SetFixedBrakingDistance(true);
	
	StatusBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusBarWidget"));
	StatusBarWidgetComponent->SetupAttachment(RootComponent);

	StatusBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	StatusBarWidgetComponent->SetDrawSize(FVector2D(120.0f, 22.0f));
	StatusBarWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	StatusBarWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetRootComponent());

	// Justera i editorn sen
	FirstPersonCamera->SetRelativeLocation(FVector(30.f, 0.f, 90.f));
	FirstPersonCamera->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	FirstPersonCamera->bUsePawnControlRotation = false;
	
	TargetBracketWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("TargetBracketWidget"));
	TargetBracketWidget->SetupAttachment(RootComponent);

	TargetBracketWidget->SetWidgetSpace(EWidgetSpace::Screen);
	TargetBracketWidget->SetDrawAtDesiredSize(true);
	TargetBracketWidget->SetVisibility(false);
	TargetBracketWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));	
}

void AStrategyUnit::BeginPlay()
{
	Super::BeginPlay();

	if (StatusBarWidgetClass)
	{
		StatusBarWidgetComponent->SetWidgetClass(StatusBarWidgetClass);
	}

	CurrentHealth = MaxHealth;
	CurrentArmor = MaxArmor;

	UpdateStatusBar();
}

void AStrategyUnit::NotifyControllerChanged()
{
	// validate and save a copy of the AI controller reference
	AIController = Cast<AAIController>(Controller);
	
	if (AIController)
	{
		// subscribe to the move finished handler on the path following component
		UPathFollowingComponent* PFComp = AIController->GetPathFollowingComponent();
		if (PFComp)
		{
			PFComp->OnRequestFinished.AddUObject(this, &AStrategyUnit::OnMoveFinished);
		}
	}
}

void AStrategyUnit::StopMoving()
{
	// use the character movement component to stop movement
	GetCharacterMovement()->StopMovementImmediately();
}

void AStrategyUnit::UnitSelected()
{
	if (StrategyUnitTeam != EStrategyUnitTeam::Human)
	{
		return;
	}
	
	// pass control to BP
	BP_UnitSelected();
}

void AStrategyUnit::UnitDeselected()
{
	// pass control to BP
	BP_UnitDeselected();
}

void AStrategyUnit::Interact(AStrategyUnit* Interactor)
{
	// ensure the interactor is valid
	if (IsValid(Interactor))
	{
		// rotate towards the actor we're interacting with
		SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Interactor->GetActorLocation()));

		// signal the interactor to play its interaction behavior
		Interactor->BP_InteractionBehavior(this);

		// play our own interaction behavior
		BP_InteractionBehavior(Interactor);
	}
	
}

bool AStrategyUnit::MoveToLocation(const FVector& Location, float AcceptanceRadius)
{
	// ensure we have a valid AI Controller
	if (AIController)
	{
		// set up the AI Move Request
		FAIMoveRequest MoveReq;

		MoveReq.SetGoalLocation(Location);
		MoveReq.SetAcceptanceRadius(AcceptanceRadius);
		MoveReq.SetAllowPartialPath(true);
		MoveReq.SetUsePathfinding(true);
		MoveReq.SetProjectGoalLocation(true);
		MoveReq.SetRequireNavigableEndLocation(true);
		MoveReq.SetNavigationFilter(AIController->GetDefaultNavigationFilterClass());
		MoveReq.SetCanStrafe(false);

		// request a move to the AI Controller
		FNavPathSharedPtr FollowedPath;
		const FPathFollowingRequestResult ResultData = AIController->MoveTo(MoveReq, &FollowedPath);
		
		// check the move result
		switch (ResultData.Code)
		{
			// failed. Return false
			case EPathFollowingRequestResult::Failed:

				return false;
				break;

			// already at goal. Return true and call the move completed delegate
			case EPathFollowingRequestResult::AlreadyAtGoal:

				OnMoveCompleted.Broadcast(this);
				return true;
				break;

			// move successfully scheduled. Return true
			case EPathFollowingRequestResult::RequestSuccessful:

				return true;
				break;
		}
	}

	// the move could not be completed
	return false;
}

void AStrategyUnit::OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	// call the delegate
	OnMoveCompleted.Broadcast(this);
}

void AStrategyUnit::SetStrategyUnitTeam(EStrategyUnitTeam InStrategyUnitTeam)
{
	StrategyUnitTeam = InStrategyUnitTeam;
	
	if (StrategyUnitTeam == EStrategyUnitTeam::AI)
	{
		EnemyAI = NewObject<UEnemyUnitAI>(this, EnemyAIClass);
	}
}

EStrategyUnitTeam AStrategyUnit::GetStrategyUnitTeam() const
{
	return StrategyUnitTeam;
}

void AStrategyUnit::UseAtionPoints(int32 ActionPoints)
{
	UsedActionPoints += ActionPoints;
}

void AStrategyUnit::ResetActionPoints()
{
	UsedActionPoints = 0;
}

int32 AStrategyUnit::GetRemainingActionPoints() const
{
	return MaxActionPoints - UsedActionPoints;	
}

void AStrategyUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GridManager)
	{
		return;
	}

	const FIntPoint CurrentCell = GridManager->WorldToGrid(GetActorLocation());

	if (!bHasLastGridCell)
	{
		LastGridCell = CurrentCell;
		bHasLastGridCell = true;
		return;
	}

	if (CurrentCell != LastGridCell)
	{
		LastGridCell = CurrentCell;
		OnGridCellChanged.Broadcast(this);
	}
}

void AStrategyUnit::ApplyDamage(const FWeaponDamage& WeaponDamage)
{
	if (WeaponDamage.Damage <= 0)
	{
		return;
	}

	const int32 EffectiveArmor = FMath::Max(CurrentArmor - WeaponDamage.ArmorPierce, 0);
	const int32 HealthDamage = FMath::Max(WeaponDamage.Damage - EffectiveArmor, 0);

	CurrentHealth = FMath::Clamp(CurrentHealth - HealthDamage, 0, MaxHealth);

	if (WeaponDamage.ArmorShred > 0)
	{
		CurrentArmor = FMath::Clamp(CurrentArmor - WeaponDamage.ArmorShred, 0, MaxArmor);
	}

	UpdateStatusBar();

	if (CurrentHealth <= 0)
	{
		// Die();
	}
}

bool AStrategyUnit::CanWeaponAttack(AAIStrategySide* EnemySide) const
{
	if (!GridManager || !EnemySide)
	{
		return false;
	}

	// 1. AP check
	if (GetRemainingActionPoints() <= 0)
	{
		return false;
	}

	// 2. Ammo check
	if (!EquippedWeapon.WeaponData)
	{
		return false;
	}

	if (EquippedWeapon.UsesAmmo() && EquippedWeapon.CurrentAmmo <= 0)
	{
		return false;
	}

	// 3. Range check
	const FIntPoint MyCell = GridManager->WorldToGrid(GetActorLocation());
	const int32 Range = EquippedWeapon.WeaponData->AttackStats.Range;

	for (AStrategyUnit* Enemy : EnemySide->Units) // eller EnemySide->Units
	{
		if (!Enemy)
		{
			continue;
		}

		const FIntPoint EnemyCell = GridManager->WorldToGrid(Enemy->GetActorLocation());

		const int32 Distance = FMath::Abs(MyCell.X - EnemyCell.X) +
							   FMath::Abs(MyCell.Y - EnemyCell.Y);

		if (Distance <= Range)
		{
			return true;
		}
	}

	return false;
}

void AStrategyUnit::StartWeaponAttackMode()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	AStrategyPlayerController* StrategyPC = Cast<AStrategyPlayerController>(PC);
	if (!StrategyPC)
	{
		return;
	}

	UStrategyTargetingComponent* Targeting = StrategyPC->GetTargetingComponent();
	if (!Targeting)
	{
		return;
	}
	
	StrategyPC->RemoveTacticalHUD();
	Targeting->EnterFireMode(this, GetEnemiesInRange());
	StrategyPC->ShowTargetingHUD();
}

TArray<AStrategyUnit*> AStrategyUnit::GetEnemiesInRange() const
{
	TArray<AStrategyUnit*> Result;
	
	AStrategyGameMode* GameMode = GetStrategyGameMode();
	if (!ensureMsgf(GameMode, TEXT("GameMode is null in AStrategyPlayerController::RefreshActionBar")))
	{
		return Result;
	}

	AAIStrategySide* EnemySide = GameMode->GetEnemySide();
	if (!ensureMsgf(EnemySide, TEXT("EnemySide is null in AStrategyPlayerController::RefreshActionBar")))
	{
		return Result;
	}
	
	if (!GridManager || !EquippedWeapon.WeaponData)
	{
		return Result;
	}

	const FIntPoint MyCell = GridManager->WorldToGrid(GetActorLocation());
	const int32 Range = EquippedWeapon.WeaponData->AttackStats.Range;

	for (AStrategyUnit* Enemy : EnemySide->Units)
	{
		if (!Enemy)
		{
			continue;
		}

		const FIntPoint EnemyCell = GridManager->WorldToGrid(Enemy->GetActorLocation());

		const int32 Distance =
			FMath::Abs(MyCell.X - EnemyCell.X) +
			FMath::Abs(MyCell.Y - EnemyCell.Y);

		if (Distance <= Range)
		{
			Result.Add(Enemy);
		}
	}

	return Result;
}

void AStrategyUnit::UpdateStatusBar()
{
	if (!StatusBarWidgetComponent)
	{
		return;
	}

	UUnitStatusBarWidget* StatusWidget =
		Cast<UUnitStatusBarWidget>(StatusBarWidgetComponent->GetUserWidgetObject());

	if (!StatusWidget)
	{
		return;
	}

	StatusWidget->SetHealthAndArmor(
		CurrentHealth,
		MaxHealth,
		CurrentArmor,
		MaxArmor);
}

void AStrategyUnit::EquipWeapon(UStrategyWeaponData* WeaponData)
{
	EquippedWeapon.Init(WeaponData);
}

AStrategyGameMode* AStrategyUnit::GetStrategyGameMode() const
{
	UWorld* World = GetWorld();
	return World ? Cast<AStrategyGameMode>(World->GetAuthGameMode()) : nullptr;
}

void AStrategyUnit::SetTargetBracketVisible(bool bVisible)
{
	if (TargetBracketWidget)
	{
		TargetBracketWidget->SetVisibility(bVisible);
	}
}