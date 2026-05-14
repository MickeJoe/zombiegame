// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyUnit.h"
#include "../../Systems/GridManager.h"
#include "../../Enemy_AI/EnemyUnitAI.h"
#include "Player/AIStrategySide.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"
#include "Data/Weapon/StrategyWeaponData.h"
#include "AIController.h"
#include "StrategyGameMode.h"
#include "StrategyPlayerController.h"
#include "Player/StrategySide.h"
#include "UI/TargetingUI/StrategyTargetingComponent.h"
#include "UI/TargetingUI/TargetInfoWidget.h"
#include "UnitStatusBarWidget.h"
#include "Data/Unit/UnitData.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Camera/CameraComponent.h"

namespace
{
	constexpr int32 DefaultMaxActionPoints = 2;
	constexpr int32 DefaultMaxMovement = 8;
	constexpr int32 DefaultSightRange = 28;
	constexpr int32 DefaultMaxHealth = 8;
	constexpr int32 DefaultMaxArmor = 2;
}

PRAGMA_DISABLE_OPTIMIZATION

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

	if (UnitData && UnitData->StatusBarWidgetClass)
	{
		StatusBarWidgetComponent->SetWidgetClass(UnitData->StatusBarWidgetClass);
	}
	
	if (UnitData && UnitData->TargetInfoWidgetClass)
	{
		TargetInfoWidget = CreateWidget<UTargetInfoWidget>(GetWorld(), UnitData->TargetInfoWidgetClass);

		if (TargetInfoWidget)
		{
			TargetInfoWidget->AddToViewport(1000);
			TargetInfoWidget->SetTarget(this);
			TargetInfoWidget->SetVisibility(ESlateVisibility::Collapsed);
			
			
		}
	}

	if (UnitData && UnitData->DefaultWeapon && !GetEquippedFireWeapon().WeaponData && !GetEquippedMeleeWeapon().WeaponData)
	{
		EquipWeapon(UnitData->DefaultWeapon);
	}

	if (UnitData)
	{
		for (UStrategyWeaponData* DefaultWeapon : UnitData->DefaultWeapons)
		{
			EquipWeapon(DefaultWeapon);
		}
	}

	CurrentHealth = GetMaxHealth();
	CurrentArmor = GetMaxArmor();

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
	
	if (StrategyUnitTeam == EStrategyUnitTeam::AI && UnitData && UnitData->EnemyAIClass)
	{
		EnemyAI = NewObject<UEnemyUnitAI>(this, UnitData->EnemyAIClass);
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
	return GetMaxActionPoints() - UsedActionPoints;	
}

int32 AStrategyUnit::GetSightRange() const
{
	return UnitData ? UnitData->SightRange : DefaultSightRange;
}

int32 AStrategyUnit::GetMaxMovement() const
{
	return UnitData ? UnitData->MaxMovement : DefaultMaxMovement;
}

int32 AStrategyUnit::GetMaxActionPoints() const
{
	return UnitData ? UnitData->MaxActionPoints : DefaultMaxActionPoints;
}

int32 AStrategyUnit::GetMaxHealth() const
{
	return UnitData ? UnitData->MaxHealth : DefaultMaxHealth;
}

int32 AStrategyUnit::GetMaxArmor() const
{
	return UnitData ? UnitData->MaxArmor : DefaultMaxArmor;
}

FAttackStats AStrategyUnit::GetBiteAttackStats() const
{
	return UnitData ? UnitData->BiteAttack : FAttackStats();
}

const FAttackStats* AStrategyUnit::GetMeleeAttackStats() const
{
	if (const FAttackStats* WeaponAttackStats = GetEquippedMeleeWeapon().GetAttackStats())
	{
		return WeaponAttackStats;
	}

	return UnitData ? &UnitData->HandAttack : nullptr;
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

float AStrategyUnit::ApplyDamage(const FWeaponDamage& WeaponDamage)
{
	if (WeaponDamage.Damage <= 0)
	{
		return 0.0f;
	}

	const int32 EffectiveArmor = FMath::Max(CurrentArmor - WeaponDamage.ArmorPierce, 0);
	const int32 HealthDamage = FMath::Max(WeaponDamage.Damage - EffectiveArmor, 0);

	CurrentHealth = FMath::Clamp(CurrentHealth - HealthDamage, 0, GetMaxHealth());

	if (WeaponDamage.ArmorShred > 0)
	{
		CurrentArmor = FMath::Clamp(CurrentArmor - WeaponDamage.ArmorShred, 0, GetMaxArmor());
	}
	
	UpdateStatusBar();

	if (CurrentHealth <= 0)
	{
		float PlayedLength = 0.0f;

		if (UnitData && UnitData->DeathReactMontage)
		{
			PlayedLength = PlayAnimMontage(UnitData->DeathReactMontage);
			if (PlayedLength <= 0.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to play death montage for %s"), *GetName());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Play length is %f for %s"), PlayedLength, *GetName());
			}
		}

		ScheduleDeathRemoval(PlayedLength);
		return PlayedLength;
	}
	else
	{
		if (UnitData && UnitData->HitReactMontage)
		{
			const float PlayedLength = PlayAnimMontage(UnitData->HitReactMontage);
			if (PlayedLength <= 0.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to play hit react montage for %s"), *GetName());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Play length is %f for %s"), PlayedLength, *GetName());
			}

			return PlayedLength;
		}
	}

	return 0.0f;
}

void AStrategyUnit::ScheduleDeathRemoval(float DelaySeconds)
{
	if (bDeathRemovalScheduled)
	{
		return;
	}

	bDeathRemovalScheduled = true;

	StopMoving();
	GetCharacterMovement()->DisableMovement();

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetTargetBracketVisible(false);
	SetTargetInfoVisible(false);

	if (StatusBarWidgetComponent)
	{
		StatusBarWidgetComponent->SetVisibility(false);
	}

	if (OwningSide)
	{
		OwningSide->RemoveUnit(this);
		OwningSide = nullptr;
	}

	SetLifeSpan(FMath::Max(DelaySeconds, 0.1f));
}

bool AStrategyUnit::CanMeleeAttack(AAIStrategySide* EnemySide) const
{
	if (!GridManager || !EnemySide || GetRemainingActionPoints() <= 0)
	{
		return false;
	}

	const FAttackStats* AttackStats = GetMeleeAttackStats();
	if (!AttackStats)
	{
		return false;
	}

	if (GetRemainingActionPoints() < AttackStats->ActionPointCost)
	{
		return false;
	}

	const FIntPoint MyCell = GridManager->WorldToGrid(GetActorLocation());

	for (AStrategyUnit* Enemy : EnemySide->Units)
	{
		if (!IsValid(Enemy) || Enemy->GetCurrentHealth() <= 0)
		{
			continue;
		}

		const FIntPoint EnemyCell = GridManager->WorldToGrid(Enemy->GetActorLocation());
		const int32 Distance =
			FMath::Abs(MyCell.X - EnemyCell.X) +
			FMath::Abs(MyCell.Y - EnemyCell.Y);

		if (Distance <= AttackStats->Range)
		{
			return true;
		}
	}

	return false;
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

	const FStrategyWeaponInstance& FireWeapon = GetEquippedFireWeapon();

	// 2. Ammo check
	if (!FireWeapon.WeaponData)
	{
		return false;
	}

	const FAttackStats* AttackStats = FireWeapon.GetAttackStats();
	if (!AttackStats)
	{
		return false;
	}

	if (GetRemainingActionPoints() < AttackStats->ActionPointCost)
	{
		return false;
	}

	const int32 AmmoCost = FireWeapon.UsesAmmo()
		? FMath::Max(AttackStats->AmmoCost, 1)
		: 0;
	if (FireWeapon.UsesAmmo() && FireWeapon.CurrentAmmo < AmmoCost)
	{
		return false;
	}

	// 3. Range check
	const FIntPoint MyCell = GridManager->WorldToGrid(GetActorLocation());
	const int32 Range = AttackStats->Range;

	for (AStrategyUnit* Enemy : EnemySide->Units) // eller EnemySide->Units
	{
		if (!Enemy || Enemy->GetCurrentHealth() <= 0)
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

void AStrategyUnit::SpendMeleeAttackResources()
{
	const FAttackStats* AttackStats = GetMeleeAttackStats();
	if (!AttackStats)
	{
		return;
	}

	UseAtionPoints(AttackStats->ActionPointCost);
}

void AStrategyUnit::SpendWeaponAttackResources()
{
	FStrategyWeaponInstance* FireWeapon = nullptr;
	if (TwoHandedWeapon.WeaponData && TwoHandedWeapon.WeaponData->AttackType == EStrategyWeaponAttackType::Fire)
	{
		FireWeapon = &TwoHandedWeapon;
	}
	else if (OneHandedFireWeapon.WeaponData)
	{
		FireWeapon = &OneHandedFireWeapon;
	}

	if (!FireWeapon)
	{
		return;
	}

	const FAttackStats* AttackStats = FireWeapon->GetAttackStats();
	if (!AttackStats)
	{
		return;
	}

	UseAtionPoints(AttackStats->ActionPointCost);

	if (FireWeapon->UsesAmmo())
	{
		const int32 AmmoCost = FMath::Max(AttackStats->AmmoCost, 1);
		FireWeapon->CurrentAmmo = FMath::Max(FireWeapon->CurrentAmmo - AmmoCost, 0);
	}
}

bool AStrategyUnit::CanReload() const
{
	const FStrategyWeaponInstance& FireWeapon = GetEquippedFireWeapon();
	return FireWeapon.UsesAmmo()
		&& FireWeapon.CurrentAmmo < FireWeapon.GetMaxAmmo()
		&& GetRemainingActionPoints() >= 1;
}

void AStrategyUnit::ReloadWeapon()
{
	if (!CanReload())
	{
		return;
	}

	UseAtionPoints(1);

	FStrategyWeaponInstance* FireWeapon = nullptr;
	if (TwoHandedWeapon.WeaponData && TwoHandedWeapon.WeaponData->AttackType == EStrategyWeaponAttackType::Fire)
	{
		FireWeapon = &TwoHandedWeapon;
	}
	else if (OneHandedFireWeapon.WeaponData)
	{
		FireWeapon = &OneHandedFireWeapon;
	}

	if (FireWeapon)
	{
		FireWeapon->CurrentAmmo = FireWeapon->GetMaxAmmo();
	}
}

void AStrategyUnit::StartMeleeAttackMode()
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

	if (Targeting->EnterMeleeMode(this, GetMeleeEnemiesInRange()))
	{
		StrategyPC->RemoveTacticalHUD();
		StrategyPC->ShowTargetingHUD();
	}
	else
	{
		StrategyPC->ShowTacticalHUD();
	}
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
	
	if (Targeting->EnterFireMode(this, GetEnemiesInRange()))
	{
		StrategyPC->RemoveTacticalHUD();
		StrategyPC->ShowTargetingHUD();
	}
	else
	{
		StrategyPC->ShowTacticalHUD();
	}
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
	
	const FStrategyWeaponInstance& FireWeapon = GetEquippedFireWeapon();
	if (!GridManager || !FireWeapon.WeaponData)
	{
		return Result;
	}

	const FAttackStats* AttackStats = FireWeapon.GetAttackStats();
	if (!AttackStats)
	{
		return Result;
	}

	const FIntPoint MyCell = GridManager->WorldToGrid(GetActorLocation());
	const int32 Range = AttackStats->Range;

	for (AStrategyUnit* Enemy : EnemySide->Units)
	{
		if (!Enemy || Enemy->GetCurrentHealth() <= 0)
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

TArray<AStrategyUnit*> AStrategyUnit::GetMeleeEnemiesInRange() const
{
	TArray<AStrategyUnit*> Result;

	AStrategyGameMode* GameMode = GetStrategyGameMode();
	if (!ensureMsgf(GameMode, TEXT("GameMode is null in AStrategyUnit::GetMeleeEnemiesInRange")))
	{
		return Result;
	}

	AAIStrategySide* EnemySide = GameMode->GetEnemySide();
	if (!ensureMsgf(EnemySide, TEXT("EnemySide is null in AStrategyUnit::GetMeleeEnemiesInRange")))
	{
		return Result;
	}

	const FAttackStats* AttackStats = GetMeleeAttackStats();
	if (!GridManager || !AttackStats)
	{
		return Result;
	}

	const FIntPoint MyCell = GridManager->WorldToGrid(GetActorLocation());
	const int32 Range = AttackStats->Range;

	for (AStrategyUnit* Enemy : EnemySide->Units)
	{
		if (!Enemy || Enemy->GetCurrentHealth() <= 0)
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
		GetMaxHealth(),
		CurrentArmor,
		GetMaxArmor());
}

void AStrategyUnit::EquipWeapon(UStrategyWeaponData* WeaponData)
{
	if (!WeaponData)
	{
		return;
	}

	if (WeaponData->Handedness == EStrategyWeaponHandedness::TwoHanded)
	{
		OneHandedFireWeapon = FStrategyWeaponInstance();
		OneHandedMeleeWeapon = FStrategyWeaponInstance();
		TwoHandedWeapon.Init(WeaponData);
		return;
	}

	TwoHandedWeapon = FStrategyWeaponInstance();

	if (WeaponData->AttackType == EStrategyWeaponAttackType::Fire)
	{
		OneHandedFireWeapon.Init(WeaponData);
	}
	else
	{
		OneHandedMeleeWeapon.Init(WeaponData);
	}
}

const FStrategyWeaponInstance& AStrategyUnit::GetEquippedFireWeapon() const
{
	if (TwoHandedWeapon.WeaponData && TwoHandedWeapon.WeaponData->AttackType == EStrategyWeaponAttackType::Fire)
	{
		return TwoHandedWeapon;
	}

	if (OneHandedFireWeapon.WeaponData)
	{
		return OneHandedFireWeapon;
	}

	return EmptyWeaponInstance;
}

const FStrategyWeaponInstance& AStrategyUnit::GetEquippedMeleeWeapon() const
{
	if (TwoHandedWeapon.WeaponData && TwoHandedWeapon.WeaponData->AttackType == EStrategyWeaponAttackType::Melee)
	{
		return TwoHandedWeapon;
	}

	if (OneHandedMeleeWeapon.WeaponData)
	{
		return OneHandedMeleeWeapon;
	}

	return EmptyWeaponInstance;
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

		if (UWidget* TargetBracket = TargetBracketWidget->GetWidget())
		{
			TargetBracket->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void AStrategyUnit::SetTargetInfoVisible(bool bVisible)
{
	if (TargetInfoWidget)
	{
		TargetInfoWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

PRAGMA_ENABLE_OPTIMIZATION
