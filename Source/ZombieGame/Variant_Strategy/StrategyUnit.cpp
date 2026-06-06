// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyUnit.h"
#include "../../Systems/GridManager.h"
#include "../../Enemy_AI/EnemyUnitAI.h"
#include "Player/AIStrategySide.h"
#include "Systems/AttackHandling/StrategyWeaponInstance.h"
#include "Data/Item/EquippableItemData.h"
#include "Data/Item/MedicBagData.h"
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
#include "Components/PrimitiveComponent.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/ChildActorComponent.h"

namespace
{
	constexpr int32 DefaultMaxTimeUnits = 15;
	constexpr int32 DefaultMaxMovement = 8;
	constexpr int32 DefaultSightRange = 28;
	constexpr int32 DefaultMaxHealth = 8;
	constexpr int32 DefaultMaxArmor = 2;

	bool DoesWeaponMatchAttackType(const FStrategyWeaponInstance& Weapon, EStrategyWeaponAttackType AttackType)
	{
		return Weapon.WeaponData && Weapon.WeaponData->AttackType == AttackType;
	}
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
	FirstPersonCamera->SetAutoActivate(false);

	ThirdPersonTargetingCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonTargetingCamera"));
	ThirdPersonTargetingCamera->SetupAttachment(GetRootComponent());
	ThirdPersonTargetingCamera->SetRelativeLocation(ThirdPersonTargetingCameraRelativeLocation);
	ThirdPersonTargetingCamera->SetRelativeRotation(ThirdPersonTargetingCameraRelativeRotation);
	ThirdPersonTargetingCamera->bUsePawnControlRotation = false;
	ThirdPersonTargetingCamera->SetAutoActivate(false);
	
	TargetBracketWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("TargetBracketWidget"));
	TargetBracketWidget->SetupAttachment(RootComponent);

	TargetBracketWidget->SetWidgetSpace(EWidgetSpace::Screen);
	TargetBracketWidget->SetDrawAtDesiredSize(true);
	TargetBracketWidget->SetVisibility(false);
	TargetBracketWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	MeleeWeaponActorComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("MeleeWeaponActor"));
	MeleeWeaponActorComponent->SetupAttachment(GetRootComponent());
}

void AStrategyUnit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTargetingCameraSettings();
	ConfigureVisualComponentsForTacticalMovement();
	RebuildEquippedWeaponInstances();
	UpdateMeleeWeaponVisual();
}

void AStrategyUnit::BeginPlay()
{
	Super::BeginPlay();

	ApplyTargetingCameraSettings();
	ConfigureVisualComponentsForTacticalMovement();
	RebuildEquippedWeaponInstances();
	UpdateMeleeWeaponVisual();


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

	if (UnitData && UnitData->DefaultWeapon && !PrimaryItem && !SecondaryItem)
	{
		EquipWeapon(UnitData->DefaultWeapon);
	}

	if (UnitData)
	{
		for (UStrategyWeaponData* DefaultWeapon : UnitData->DefaultWeapons)
		{
			if (DefaultWeapon
				&& !GetItemInSlot(DefaultWeapon->EquipmentSlot))
			{
				EquipWeapon(DefaultWeapon);
			}
		}

		for (UEquippableItemData* DefaultItem : UnitData->DefaultItems)
		{
			if (DefaultItem
				&& !GetItemInSlot(DefaultItem->EquipmentSlot))
			{
				EquipItem(DefaultItem);
			}
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

void AStrategyUnit::SpendTimeUnits(int32 TimeUnits)
{
	UsedTimeUnits = FMath::Clamp(UsedTimeUnits + FMath::Max(TimeUnits, 0), 0, GetMaxTimeUnits());
}

void AStrategyUnit::ResetTimeUnits()
{
	UsedTimeUnits = 0;
	ClearOverwatch();
}

int32 AStrategyUnit::GetRemainingTimeUnits() const
{
	return FMath::Max(GetMaxTimeUnits() - UsedTimeUnits, 0);
}

void AStrategyUnit::UseAtionPoints(int32 ActionPoints)
{
	SpendTimeUnits(ActionPoints);
}

void AStrategyUnit::ResetActionPoints()
{
	ResetTimeUnits();
}

int32 AStrategyUnit::GetRemainingActionPoints() const
{
	return GetRemainingTimeUnits();
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
	return GetMaxTimeUnits();
}

int32 AStrategyUnit::GetMaxTimeUnits() const
{
	return UnitData ? UnitData->MaxTimeUnits : DefaultMaxTimeUnits;
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

int32 AStrategyUnit::ApplyHealing(int32 HealAmount)
{
	if (HealAmount <= 0 || CurrentHealth <= 0)
	{
		return 0;
	}

	const int32 OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0, GetMaxHealth());
	UpdateStatusBar();
	return CurrentHealth - OldHealth;
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
	if (!GridManager || !EnemySide || GetRemainingTimeUnits() <= 0)
	{
		return false;
	}

	const FAttackStats* AttackStats = GetMeleeAttackStats();
	if (!AttackStats)
	{
		return false;
	}

	const FStrategyWeaponInstance& MeleeWeapon = GetEquippedMeleeWeapon();
	const int32 MeleeTimeUnitCost = MeleeWeapon.WeaponData
		? MeleeWeapon.WeaponData->TimeUnitCost
		: 1;
	const int32 MeleeRange = MeleeWeapon.WeaponData
		? FMath::Max(MeleeWeapon.WeaponData->Range, 1)
		: 1;
	if (GetRemainingTimeUnits() < MeleeTimeUnitCost)
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

		if (Distance <= MeleeRange)
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

	if (GetRemainingTimeUnits() <= 0)
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

	const FStrategyWeaponInstance& MeleeWeapon = GetEquippedMeleeWeapon();
	const int32 MeleeTimeUnitCost = MeleeWeapon.WeaponData
		? MeleeWeapon.WeaponData->TimeUnitCost
		: 1;
	if (GetRemainingTimeUnits() < MeleeTimeUnitCost)
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
	const int32 Range = FMath::Max(FireWeapon.WeaponData->Range, 1);

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

	const FStrategyWeaponInstance& MeleeWeapon = GetEquippedMeleeWeapon();
	const int32 MeleeTimeUnitCost = MeleeWeapon.WeaponData
		? MeleeWeapon.WeaponData->TimeUnitCost
		: 1;
	SpendTimeUnits(MeleeTimeUnitCost);
}

void AStrategyUnit::SpendWeaponAttackResources()
{
	FStrategyWeaponInstance* ActiveWeapon = ActiveWeaponSlot == EEquippableItemSlot::Primary
		? &PrimaryWeapon
		: &SecondaryWeapon;
	FStrategyWeaponInstance* FireWeapon = nullptr;
	if (DoesWeaponMatchAttackType(*ActiveWeapon, EStrategyWeaponAttackType::Fire))
	{
		FireWeapon = ActiveWeapon;
	}
	else if (DoesWeaponMatchAttackType(PrimaryWeapon, EStrategyWeaponAttackType::Fire))
	{
		FireWeapon = &PrimaryWeapon;
	}
	else if (DoesWeaponMatchAttackType(SecondaryWeapon, EStrategyWeaponAttackType::Fire))
	{
		FireWeapon = &SecondaryWeapon;
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

	SpendTimeUnits(FireWeapon->WeaponData->TimeUnitCost);

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
		&& GetRemainingTimeUnits() >= 1;
}

void AStrategyUnit::ReloadWeapon()
{
	if (!CanReload())
	{
		return;
	}

	SpendTimeUnits(1);

	FStrategyWeaponInstance* ActiveWeapon = ActiveWeaponSlot == EEquippableItemSlot::Primary
		? &PrimaryWeapon
		: &SecondaryWeapon;
	FStrategyWeaponInstance* FireWeapon = nullptr;
	if (DoesWeaponMatchAttackType(*ActiveWeapon, EStrategyWeaponAttackType::Fire))
	{
		FireWeapon = ActiveWeapon;
	}
	else if (DoesWeaponMatchAttackType(PrimaryWeapon, EStrategyWeaponAttackType::Fire))
	{
		FireWeapon = &PrimaryWeapon;
	}
	else if (DoesWeaponMatchAttackType(SecondaryWeapon, EStrategyWeaponAttackType::Fire))
	{
		FireWeapon = &SecondaryWeapon;
	}

	if (FireWeapon)
	{
		FireWeapon->CurrentAmmo = FireWeapon->GetMaxAmmo();
	}
}

bool AStrategyUnit::CanOverwatch() const
{
	const FStrategyWeaponInstance& FireWeapon = GetEquippedFireWeapon();
	return GetRemainingTimeUnits() > 0
		&& FireWeapon.WeaponData
		&& FireWeapon.GetAttackStats();
}

int32 AStrategyUnit::GetOverwatchRange() const
{
	if (const FAttackStats* AttackStats = GetEquippedFireWeapon().GetAttackStats())
	{
		return FMath::Max(GetEquippedFireWeapon().WeaponData->Range, 1);
	}

	return 1;
}

float AStrategyUnit::GetOverwatchConeAngleDegrees() const
{
	const FStrategyWeaponInstance& FireWeapon = GetEquippedFireWeapon();
	if (FireWeapon.WeaponData)
	{
		return FMath::Clamp(FireWeapon.WeaponData->OverwatchConeAngleDegrees, 1.0f, 360.0f);
	}

	return 90.0f;
}

void AStrategyUnit::EnterOverwatch(
	const FVector& Direction,
	int32 Range,
	float AngleDegrees,
	const TArray<FIntPoint>& Cells)
{
	if (!CanOverwatch())
	{
		return;
	}

	FVector FlatDirection(Direction.X, Direction.Y, 0.0f);
	if (!FlatDirection.Normalize())
	{
		FlatDirection = GetActorForwardVector();
		FlatDirection.Z = 0.0f;
		FlatDirection.Normalize();
	}

	bOverwatchActive = true;
	OverwatchDirection = FlatDirection;
	OverwatchRange = FMath::Max(Range, 1);
	OverwatchAngleDegrees = AngleDegrees;
	OverwatchCells = Cells;

	SpendTimeUnits(GetRemainingTimeUnits());
}

void AStrategyUnit::ClearOverwatch()
{
	bOverwatchActive = false;
	OverwatchRange = 0;
	OverwatchCells.Empty();
}

bool AStrategyUnit::TryFireOverwatchAt(AStrategyUnit* Target)
{
	if (!bOverwatchActive || !IsValid(Target) || Target->GetCurrentHealth() <= 0)
	{
		return false;
	}

	FStrategyWeaponInstance* ActiveWeapon = ActiveWeaponSlot == EEquippableItemSlot::Primary
		? &PrimaryWeapon
		: &SecondaryWeapon;
	FStrategyWeaponInstance* FireWeapon = nullptr;
	if (DoesWeaponMatchAttackType(*ActiveWeapon, EStrategyWeaponAttackType::Fire))
	{
		FireWeapon = ActiveWeapon;
	}
	else if (DoesWeaponMatchAttackType(PrimaryWeapon, EStrategyWeaponAttackType::Fire))
	{
		FireWeapon = &PrimaryWeapon;
	}
	else if (DoesWeaponMatchAttackType(SecondaryWeapon, EStrategyWeaponAttackType::Fire))
	{
		FireWeapon = &SecondaryWeapon;
	}

	if (!FireWeapon || !FireWeapon->WeaponData)
	{
		ClearOverwatch();
		return false;
	}

	if (FireWeapon->UsesAmmo() && FireWeapon->CurrentAmmo <= 0)
	{
		ClearOverwatch();
		return false;
	}

	const FAttackStats* AttackStats = FireWeapon->GetAttackStats();
	if (!AttackStats)
	{
		ClearOverwatch();
		return false;
	}

	FWeaponDamage Damage;
	Damage.Damage = AttackStats->Damage;
	Damage.ArmorPierce = AttackStats->ArmorPenetration;
	Damage.ArmorShred = AttackStats->ArmorShred;

	if (FireWeapon->UsesAmmo())
	{
		FireWeapon->CurrentAmmo = FMath::Max(FireWeapon->CurrentAmmo - 1, 0);
	}

	Target->ApplyDamage(Damage);
	ClearOverwatch();
	return true;
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

float AStrategyUnit::PlayMeleeAttackMontage()
{
	if (!MeleeAttackMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melee attack montage is not assigned for %s"), *GetName());
		return 0.0f;
	}

	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	GetComponents(SkeletalMeshComponents);

	USkeletalMeshComponent* CharacterMesh = nullptr;
	UAnimInstance* AnimInstance = nullptr;
	const USkeleton* MontageSkeleton = MeleeAttackMontage->GetSkeleton();

	if (!MeleeAttackMontageMeshComponentName.IsNone())
	{
		for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
		{
			if (MeshComponent && MeshComponent->GetFName() == MeleeAttackMontageMeshComponentName)
			{
				CharacterMesh = MeshComponent;
				AnimInstance = MeshComponent->GetAnimInstance();
				break;
			}
		}
	}

	for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
	{
		if (AnimInstance)
		{
			break;
		}

		if (!MeshComponent || !MeshComponent->GetAnimInstance())
		{
			continue;
		}

		const USkeleton* MeshSkeleton = MeshComponent->GetSkeletalMeshAsset()
			? MeshComponent->GetSkeletalMeshAsset()->GetSkeleton()
			: nullptr;

		if (!MontageSkeleton || MeshSkeleton == MontageSkeleton)
		{
			CharacterMesh = MeshComponent;
			AnimInstance = MeshComponent->GetAnimInstance();
			break;
		}
	}

	if (!AnimInstance)
	{
		for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
		{
			if (MeshComponent && MeshComponent->GetAnimInstance())
			{
				CharacterMesh = MeshComponent;
				AnimInstance = MeshComponent->GetAnimInstance();
				break;
			}
		}
	}

	if (!CharacterMesh || !AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot play melee montage for %s. RequestedMesh=%s Mesh=%s AnimInstance=%s Montage=%s"),
			*GetName(),
			*MeleeAttackMontageMeshComponentName.ToString(),
			*GetNameSafe(GetMesh()),
			*GetNameSafe(AnimInstance),
			*GetNameSafe(MeleeAttackMontage));
		return 0.0f;
	}

	const USkeleton* MeshSkeleton = CharacterMesh->GetSkeletalMeshAsset()
		? CharacterMesh->GetSkeletalMeshAsset()->GetSkeleton()
		: nullptr;
	if (MeshSkeleton && MontageSkeleton && MeshSkeleton != MontageSkeleton)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melee montage skeleton mismatch for %s. Mesh=%s MeshSkeleton=%s Montage=%s MontageSkeleton=%s"),
			*GetName(),
			*GetNameSafe(CharacterMesh->GetSkeletalMeshAsset()),
			*GetNameSafe(MeshSkeleton),
			*GetNameSafe(MeleeAttackMontage),
			*GetNameSafe(MontageSkeleton));
		return 0.0f;
	}

	const float PlayedLength = AnimInstance->Montage_Play(MeleeAttackMontage);
	if (PlayedLength <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to play melee attack montage for %s. Mesh=%s AnimInstance=%s Montage=%s MeshSkeleton=%s MontageSkeleton=%s"),
			*GetName(),
			*GetNameSafe(CharacterMesh->GetSkeletalMeshAsset()),
			*GetNameSafe(AnimInstance),
			*GetNameSafe(MeleeAttackMontage),
			*GetNameSafe(MeshSkeleton),
			*GetNameSafe(MontageSkeleton));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Playing melee attack montage %s on %s using mesh %s for %.2f seconds"),
			*GetNameSafe(MeleeAttackMontage),
			*GetName(),
			*GetNameSafe(CharacterMesh),
			PlayedLength);
	}

	return PlayedLength;
}

USkeletalMeshComponent* AStrategyUnit::FindMeleeWeaponAttachMesh() const
{
	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	GetComponents(SkeletalMeshComponents);

	if (!MeleeWeaponMeshComponentName.IsNone())
	{
		for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
		{
			if (MeshComponent
				&& MeshComponent->GetFName() == MeleeWeaponMeshComponentName
				&& (MeleeWeaponSocketName.IsNone() || MeshComponent->DoesSocketExist(MeleeWeaponSocketName)))
			{
				return MeshComponent;
			}
		}
	}

	for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
	{
		if (MeshComponent && MeshComponent->DoesSocketExist(MeleeWeaponSocketName))
		{
			return MeshComponent;
		}
	}

	return GetMesh();
}

void AStrategyUnit::UpdateMeleeWeaponVisual()
{
	if (!MeleeWeaponActorComponent)
	{
		return;
	}

	MeleeWeaponActorComponent->SetChildActorClass(MeleeWeaponActorClass);

	USkeletalMeshComponent* AttachMesh = FindMeleeWeaponAttachMesh();
	if (!AttachMesh)
	{
		return;
	}

	MeleeWeaponActorComponent->AttachToComponent(
		AttachMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		MeleeWeaponSocketName);
}

void AStrategyUnit::ConfigureVisualComponentsForTacticalMovement()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents(PrimitiveComponents);

	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		if (!Component || Component == Capsule)
		{
			continue;
		}

		Component->SetCanEverAffectNavigation(false);
	}
}

void AStrategyUnit::ApplyTargetingCameraSettings()
{
	if (ThirdPersonTargetingCamera)
	{
		ThirdPersonTargetingCamera->SetRelativeLocation(ThirdPersonTargetingCameraRelativeLocation);
		ThirdPersonTargetingCamera->SetRelativeRotation(ThirdPersonTargetingCameraRelativeRotation);
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
	const int32 Range = FMath::Max(FireWeapon.WeaponData->Range, 1);

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
	const FStrategyWeaponInstance& MeleeWeapon = GetEquippedMeleeWeapon();
	const int32 Range = MeleeWeapon.WeaponData
		? FMath::Max(MeleeWeapon.WeaponData->Range, 1)
		: 1;

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

void AStrategyUnit::EquipItem(UEquippableItemData* ItemData)
{
	if (!ItemData)
	{
		return;
	}

	FStrategyWeaponInstance* WeaponSlot = ItemData->EquipmentSlot == EEquippableItemSlot::Primary
		? &PrimaryWeapon
		: &SecondaryWeapon;
	TObjectPtr<UEquippableItemData>& ItemSlot = ItemData->EquipmentSlot == EEquippableItemSlot::Primary
		? PrimaryItem
		: SecondaryItem;

	ItemSlot = ItemData;
	*WeaponSlot = FStrategyWeaponInstance();

	if (UStrategyWeaponData* WeaponData = Cast<UStrategyWeaponData>(ItemData))
	{
		WeaponSlot->Init(WeaponData);
	}
}

void AStrategyUnit::RebuildEquippedWeaponInstances()
{
	PrimaryWeapon = FStrategyWeaponInstance();
	SecondaryWeapon = FStrategyWeaponInstance();

	if (UStrategyWeaponData* PrimaryWeaponData = Cast<UStrategyWeaponData>(PrimaryItem))
	{
		PrimaryWeapon.Init(PrimaryWeaponData);
	}

	if (UStrategyWeaponData* SecondaryWeaponData = Cast<UStrategyWeaponData>(SecondaryItem))
	{
		SecondaryWeapon.Init(SecondaryWeaponData);
	}
}

void AStrategyUnit::EquipWeapon(UStrategyWeaponData* WeaponData)
{
	EquipItem(WeaponData);
}

void AStrategyUnit::ClearEquippedWeapons()
{
	PrimaryItem = nullptr;
	SecondaryItem = nullptr;
	PrimaryWeapon = FStrategyWeaponInstance();
	SecondaryWeapon = FStrategyWeaponInstance();
}

void AStrategyUnit::SetActiveWeaponSlot(EEquippableItemSlot WeaponSlot)
{
	ActiveWeaponSlot = WeaponSlot;
}

const FStrategyWeaponInstance& AStrategyUnit::GetEquippedWeapon() const
{
	return GetWeaponInSlot(ActiveWeaponSlot);
}

UEquippableItemData* AStrategyUnit::GetEquippedItem() const
{
	return GetItemInSlot(ActiveWeaponSlot);
}

UEquippableItemData* AStrategyUnit::GetItemInSlot(EEquippableItemSlot ItemSlot) const
{
	return ItemSlot == EEquippableItemSlot::Primary
		? PrimaryItem
		: SecondaryItem;
}

const FStrategyWeaponInstance& AStrategyUnit::GetWeaponInSlot(EEquippableItemSlot WeaponSlot) const
{
	return WeaponSlot == EEquippableItemSlot::Primary
		? PrimaryWeapon
		: SecondaryWeapon;
}

const FStrategyWeaponInstance& AStrategyUnit::GetEquippedFireWeapon() const
{
	const FStrategyWeaponInstance& ActiveWeapon = GetWeaponInSlot(ActiveWeaponSlot);
	if (DoesWeaponMatchAttackType(ActiveWeapon, EStrategyWeaponAttackType::Fire))
	{
		return ActiveWeapon;
	}

	if (DoesWeaponMatchAttackType(PrimaryWeapon, EStrategyWeaponAttackType::Fire))
	{
		return PrimaryWeapon;
	}

	if (DoesWeaponMatchAttackType(SecondaryWeapon, EStrategyWeaponAttackType::Fire))
	{
		return SecondaryWeapon;
	}

	return EmptyWeaponInstance;
}

const FStrategyWeaponInstance& AStrategyUnit::GetEquippedMeleeWeapon() const
{
	const FStrategyWeaponInstance& ActiveWeapon = GetWeaponInSlot(ActiveWeaponSlot);
	if (DoesWeaponMatchAttackType(ActiveWeapon, EStrategyWeaponAttackType::Melee))
	{
		return ActiveWeapon;
	}

	if (DoesWeaponMatchAttackType(PrimaryWeapon, EStrategyWeaponAttackType::Melee))
	{
		return PrimaryWeapon;
	}

	if (DoesWeaponMatchAttackType(SecondaryWeapon, EStrategyWeaponAttackType::Melee))
	{
		return SecondaryWeapon;
	}

	return EmptyWeaponInstance;
}

UMedicBagData* AStrategyUnit::GetEquippedMedicBag() const
{
	return Cast<UMedicBagData>(GetEquippedItem());
}

bool AStrategyUnit::CanUseMedicBagOn(const AStrategyUnit* Target) const
{
	const UMedicBagData* MedicBag = GetEquippedMedicBag();
	if (!GridManager || !MedicBag || !IsValid(Target))
	{
		return false;
	}

	if (GetCurrentHealth() <= 0
		|| Target->GetCurrentHealth() <= 0
		|| Target->GetCurrentHealth() >= Target->GetMaxHealth()
		|| Target->GetStrategyUnitTeam() != GetStrategyUnitTeam()
		|| GetRemainingTimeUnits() < MedicBag->TimeUnitCost)
	{
		return false;
	}

	const FIntPoint MyCell = GridManager->WorldToGrid(GetActorLocation());
	const FIntPoint TargetCell = GridManager->WorldToGrid(Target->GetActorLocation());
	const int32 Distance =
		FMath::Abs(MyCell.X - TargetCell.X) +
		FMath::Abs(MyCell.Y - TargetCell.Y);

	return Distance <= FMath::Max(MedicBag->Range, 0);
}

bool AStrategyUnit::UseMedicBagOn(AStrategyUnit* Target)
{
	UMedicBagData* MedicBag = GetEquippedMedicBag();
	if (!MedicBag || !CanUseMedicBagOn(Target))
	{
		return false;
	}

	const int32 HealedAmount = Target->ApplyHealing(MedicBag->HealAmount);
	if (HealedAmount <= 0)
	{
		return false;
	}

	SpendTimeUnits(MedicBag->TimeUnitCost);
	return true;
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

void AStrategyUnit::SetTargetingCameraView(EStrategyTargetingCameraView CameraView)
{
	ClearTargetingCameraView();

	switch (CameraView)
	{
	case EStrategyTargetingCameraView::FirstPerson:
		if (FirstPersonCamera)
		{
			FirstPersonCamera->SetActive(true);
		}
		break;

	case EStrategyTargetingCameraView::ThirdPerson:
		if (ThirdPersonTargetingCamera)
		{
			ThirdPersonTargetingCamera->SetActive(true);
		}
		break;

	case EStrategyTargetingCameraView::NoViewChange:
	default:
		break;
	}
}

void AStrategyUnit::ClearTargetingCameraView()
{
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetActive(false);
	}

	if (ThirdPersonTargetingCamera)
	{
		ThirdPersonTargetingCamera->SetActive(false);
	}
}

PRAGMA_ENABLE_OPTIMIZATION
