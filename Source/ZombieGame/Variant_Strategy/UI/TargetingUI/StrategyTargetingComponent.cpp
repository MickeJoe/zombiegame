#include "StrategyTargetingComponent.h"

#include "StrategyPlayerController.h"
#include "TargetingActionBarWidget.h"
#include "Variant_Strategy/StrategyUnit.h"
#include "Variant_Strategy/UI/TargetingUI/TargetingHUDWidget.h"

void UStrategyTargetingComponent::EnterFireMode(
	AStrategyUnit* InAttacker,
	const TArray<AStrategyUnit*>& InTargets)
{
	if (!InAttacker || InTargets.Num() == 0)
	{
		return;
	}
	
	GetTargetingHUDWidget()->OnCycleTargetClicked.AddDynamic(
    	this,
    	&UStrategyTargetingComponent::CycleToNextTarget);
	
	GetTargetingHUDWidget()->OnCancelClicked.AddDynamic(
		this,
		&UStrategyTargetingComponent::ExitFireMode);
	
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
		return;
	}

	CurrentTargetIndex = 0;
	bIsInFireMode = true;

	FocusCurrentTarget();
	EnterCameraView();
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

	PreviousViewTarget = PC->GetViewTarget();

	PC->SetViewTargetWithBlend(
		Attacker,
		0.25f,
		EViewTargetBlendFunction::VTBlend_Cubic
	);

	PC->bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	PC->SetInputMode(InputMode);
}

void UStrategyTargetingComponent::CycleToNextTarget()
{
	if (!bIsInFireMode || Targets.Num() == 0)
	{
		return;
	}
	
	Targets[CurrentTargetIndex]->SetTargetBracketVisible(false);

	CurrentTargetIndex =
		(CurrentTargetIndex + 1) % Targets.Num();

	FocusCurrentTarget();
}

void UStrategyTargetingComponent::ExitFireMode()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	GetTargetingHUDWidget()->OnCycleTargetClicked.RemoveAll(this);

	if (PC && PreviousViewTarget)
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

	bIsInFireMode = false;
	Attacker = nullptr;
	Targets.Reset();
	CurrentTargetIndex = INDEX_NONE;
	PreviousViewTarget = nullptr;
	
	AStrategyPlayerController* StrategyPC = Cast<AStrategyPlayerController>(PC);
	if (!StrategyPC)
	{
		return;
	}
	
	StrategyPC->HideTargetingHUD();
	StrategyPC->ShowTacticalHUD();
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