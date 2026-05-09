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

void UStrategyTargetingComponent::EnterFireMode(
	AStrategyUnit* InAttacker,
	const TArray<AStrategyUnit*>& InTargets)
{
	if (!InAttacker || InTargets.Num() == 0)
	{
		return;
	}
	
	UTargetingHUDWidget* TargetingHUD = GetTargetingHUDWidget();
	if (!TargetingHUD)
	{
		return;
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
	Target->SetTargetInfoVisible(true);
	
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
	Targets[CurrentTargetIndex]->SetTargetInfoVisible(false);

	CurrentTargetIndex =
		(CurrentTargetIndex + 1) % Targets.Num();

	FocusCurrentTarget();
}

void UStrategyTargetingComponent::ExitFireMode()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());

	if (UTargetingHUDWidget* TargetingHUD = GetTargetingHUDWidget())
	{
		TargetingHUD->OnCycleTargetClicked.RemoveAll(this);
		TargetingHUD->OnCancelClicked.RemoveAll(this);
		TargetingHUD->OnFireClicked.RemoveAll(this);
	}

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

	for (AStrategyUnit* Target : Targets)
	{
		if (IsValid(Target))
		{
			Target->SetTargetBracketVisible(false);
			Target->SetTargetInfoVisible(false);
		}
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

void UStrategyTargetingComponent::HandleFireClicked()
{
	if (!bIsInFireMode || !Attacker || !Targets.IsValidIndex(CurrentTargetIndex))
	{
		return;
	}

	AStrategyUnit* Target = Targets[CurrentTargetIndex];
	if (!Target)
	{
		return;
	}

	const FStrategyAttackContext Context = UStrategyAttackResolver::MakeContext(Attacker, Target);
	UStrategyAttackResolver::ResolveAndApply(Context);

	ExitFireMode();
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
