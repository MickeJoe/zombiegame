#include "StrategyTargetingComponent.h"

#include "StrategyPlayerController.h"
#include "TargetingActionBarWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Systems/AttackHandling/StrategyAttackResolver.h"
#include "Variant_Strategy/StrategyUnit.h"
#include "Variant_Strategy/UI/TargetingUI/TargetingHUDWidget.h"
#include "TargetInfoWidget.h"

UStrategyTargetingComponent::UStrategyTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

bool UStrategyTargetingComponent::EnterFireMode(
	AStrategyUnit* InAttacker,
	const TArray<AStrategyUnit*>& InTargets)
{
	return EnterAttackMode(InAttacker, InTargets, EStrategyTargetingMode::Fire);
}

bool UStrategyTargetingComponent::EnterMeleeMode(
	AStrategyUnit* InAttacker,
	const TArray<AStrategyUnit*>& InTargets)
{
	return EnterAttackMode(InAttacker, InTargets, EStrategyTargetingMode::Melee);
}

bool UStrategyTargetingComponent::EnterAttackMode(
	AStrategyUnit* InAttacker,
	const TArray<AStrategyUnit*>& InTargets,
	EStrategyTargetingMode InMode)
{
	if (!InAttacker || InTargets.Num() == 0)
	{
		return false;
	}
	
	UTargetingHUDWidget* TargetingHUD = GetTargetingHUDWidget();
	if (!TargetingHUD)
	{
		return false;
	}

	TargetingHUD->OnCycleTargetClicked.AddUniqueDynamic(
    	this,
    	&UStrategyTargetingComponent::CycleToNextTarget);
	
	TargetingHUD->OnCancelClicked.AddUniqueDynamic(
		this,
		&UStrategyTargetingComponent::ExitFireMode);

	TargetingHUD->OnFireClicked.AddUniqueDynamic(
		this,
		&UStrategyTargetingComponent::HandleFireClicked);
	
	Attacker = InAttacker;
	Targets.Reset();

	for (AStrategyUnit* Target : InTargets)
	{
		if (IsValid(Target))
		{
			Targets.Add(Target);
		}
	}

	if (Targets.Num() == 0)
	{
		return false;
	}

	CurrentTargetIndex = 0;
	TargetingMode = InMode;
	bIsInFireMode = true;

	FocusCurrentTarget();
	EnterCameraView();
	return true;
}

void UStrategyTargetingComponent::FocusCurrentTarget()
{
	if (!Attacker || !Targets.IsValidIndex(CurrentTargetIndex))
	{
		return;
	}

	AStrategyUnit* Target = Targets[CurrentTargetIndex];
	if (!Target)
	{
		return;
	}
	
	Target->SetTargetBracketVisible(true);
	Target->SetTargetInfoVisible(true);

	if (UTargetInfoWidget* TargetInfoWidget = Target->GetTargetInfoWidget())
	{
		const FStrategyAttackContext Context = TargetingMode == EStrategyTargetingMode::Melee
			? UStrategyAttackResolver::MakeContextWithAttackStats(Attacker, Target, Attacker->GetMeleeAttackStats())
			: UStrategyAttackResolver::MakeContext(Attacker, Target);
		TargetInfoWidget->SetHitChance(UStrategyAttackResolver::CalculateHitChance(Context));
		TargetInfoWidget->SetCritChance(UStrategyAttackResolver::CalculateCriticalChance(Context));
	}
	
	const FVector From = Attacker->GetActorLocation();
	const FVector To = Target->GetActorLocation();

	FRotator LookRot = (To - From).Rotation();
	LookRot.Pitch = 0.f;

	Attacker->SetActorRotation(LookRot);
}

void UStrategyTargetingComponent::EnterCameraView()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !Attacker)
	{
		return;
	}

	bChangedCameraView = false;

	if (TargetingCameraView == EStrategyTargetingCameraView::NoViewChange)
	{
		return;
	}

	PreviousViewTarget = PC->GetViewTarget();
	if (!PreviousViewTarget)
	{
		PreviousViewTarget = PC->GetPawn();
	}

	Attacker->SetTargetingCameraView(TargetingCameraView);

	PC->SetViewTargetWithBlend(
		Attacker,
		0.25f,
		EViewTargetBlendFunction::VTBlend_Cubic
	);

	bChangedCameraView = true;
	PC->bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	PC->SetInputMode(InputMode);
}

void UStrategyTargetingComponent::CycleToNextTarget()
{
	if (!bIsInFireMode || bIsResolvingAttack || Targets.Num() == 0)
	{
		return;
	}
	
	Targets[CurrentTargetIndex]->SetTargetBracketVisible(false);
	Targets[CurrentTargetIndex]->SetTargetInfoVisible(false);

	CurrentTargetIndex =
		(CurrentTargetIndex + 1) % Targets.Num();

	FocusCurrentTarget();
}

void UStrategyTargetingComponent::ExitFireMode()
{
	if (bIsResolvingAttack)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExitFireModeTimerHandle);
	}

	if (UTargetingHUDWidget* TargetingHUD = GetTargetingHUDWidget())
	{
		TargetingHUD->OnCycleTargetClicked.RemoveAll(this);
		TargetingHUD->OnCancelClicked.RemoveAll(this);
		TargetingHUD->OnFireClicked.RemoveAll(this);
	}

	AStrategyPlayerController* StrategyPC = Cast<AStrategyPlayerController>(PC);

	if (StrategyPC)
	{
		StrategyPC->SuppressSelectionInputBriefly();
		if (bChangedCameraView)
		{
			StrategyPC->RestoreTacticalView();
		}
	}
	else if (PC && PreviousViewTarget && bChangedCameraView)
	{
		PC->SetViewTargetWithBlend(
			PreviousViewTarget,
			0.25f,
			EViewTargetBlendFunction::VTBlend_Cubic
		);
	}

	if (PC)
	{
		PC->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}

	for (AStrategyUnit* Target : Targets)
	{
		if (IsValid(Target))
		{
			Target->SetTargetBracketVisible(false);
			Target->SetTargetInfoVisible(false);
		}
	}

	if (Attacker)
	{
		Attacker->ClearTargetingCameraView();
	}

	bIsInFireMode = false;
	bIsResolvingAttack = false;
	bChangedCameraView = false;
	TargetingMode = EStrategyTargetingMode::Fire;
	Attacker = nullptr;
	Targets.Reset();
	CurrentTargetIndex = INDEX_NONE;
	PreviousViewTarget = nullptr;
	
	if (!StrategyPC)
	{
		return;
	}
	
	StrategyPC->HideTargetingHUD();
	StrategyPC->ShowTacticalHUD();
}

void UStrategyTargetingComponent::RequestExitFireMode()
{
	ExitFireMode();
}

void UStrategyTargetingComponent::CompleteDelayedExitFireMode()
{
	bIsResolvingAttack = false;
	ExitFireMode();
}

void UStrategyTargetingComponent::HandleFireClicked()
{
	if (!bIsInFireMode || bIsResolvingAttack || !Attacker || !Targets.IsValidIndex(CurrentTargetIndex))
	{
		return;
	}

	AStrategyUnit* Target = Targets[CurrentTargetIndex];
	if (!Target)
	{
		return;
	}

	const FStrategyAttackContext Context = TargetingMode == EStrategyTargetingMode::Melee
		? UStrategyAttackResolver::MakeContextWithAttackStats(Attacker, Target, Attacker->GetMeleeAttackStats())
		: UStrategyAttackResolver::MakeContext(Attacker, Target);
	if (!Context.AttackStats)
	{
		return;
	}

	Attacker->FaceTargetForAttack(Target);
	const FStrategyAttackResult Result = UStrategyAttackResolver::ResolveAndApply(Context);
	if (TargetingMode == EStrategyTargetingMode::Melee)
	{
		Attacker->PlayMeleeAttackMontage();
		Attacker->SpendMeleeAttackResources();
	}
	else
	{
		const FStrategyWeaponInstance& FireWeapon = Attacker->GetEquippedFireWeapon();
		Attacker->PlayWeaponAttackMontage(FireWeapon);
		Attacker->PlayWeaponProjectileVisual(FireWeapon, Target, Result);
		Attacker->SpendWeaponAttackResources();
	}

	if (AStrategyPlayerController* StrategyPC = Cast<AStrategyPlayerController>(GetOwner()))
	{
		StrategyPC->SuppressSelectionInputBriefly();
		StrategyPC->RefreshWeaponInfoPanel();
	}

	bIsResolvingAttack = true;
	ExitFireModeAfterDelay(Result.ReactionMontageDuration);
}

void UStrategyTargetingComponent::ExitFireModeAfterDelay(float DelaySeconds)
{
	if (DelaySeconds <= 0.0f)
	{
		bIsResolvingAttack = false;
		ExitFireMode();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ExitFireModeTimerHandle,
			this,
			&UStrategyTargetingComponent::CompleteDelayedExitFireMode,
			DelaySeconds,
			false);
	}
	else
	{
		bIsResolvingAttack = false;
		ExitFireMode();
	}
}

UTargetingHUDWidget* UStrategyTargetingComponent::GetTargetingHUDWidget()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return nullptr;
	}

	AStrategyPlayerController* StrategyPC = Cast<AStrategyPlayerController>(PC);
	if (!StrategyPC)
	{
		return nullptr;
	}
	
	return StrategyPC->GetTargetingHUDWidget();
}

void UStrategyTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Targets.IsValidIndex(CurrentTargetIndex))
	{
		return;
	}
	
	if (!Targets[CurrentTargetIndex]->GetTargetInfoWidget())
	{
		return;
	}

	AStrategyUnit* Target = Targets[CurrentTargetIndex];
	if (!Target)
	{
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	FVector2D ScreenPos;
	if (UGameplayStatics::ProjectWorldToScreen(PC, Target->GetActorLocation(), ScreenPos))
	{
		Target->GetTargetInfoWidget()->SetPositionInViewport(ScreenPos + FVector2D(-60.f, -200.f));
	}
}
