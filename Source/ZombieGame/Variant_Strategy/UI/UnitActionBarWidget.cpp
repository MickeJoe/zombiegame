#include "UnitActionBarWidget.h"

#include "Components/Button.h"

PRAGMA_DISABLE_OPTIMIZATION

void UUnitActionBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Melee)     Button_Melee->OnClicked.AddUniqueDynamic(this, &UUnitActionBarWidget::HandleMeleeClicked);
	if (Button_Fire)      Button_Fire->OnClicked.AddUniqueDynamic(this, &UUnitActionBarWidget::HandleFireClicked);
	if (Button_Reload)    Button_Reload->OnClicked.AddUniqueDynamic(this, &UUnitActionBarWidget::HandleReloadClicked);
	if (Button_Hunker)    Button_Hunker->OnClicked.AddUniqueDynamic(this, &UUnitActionBarWidget::HandleHunkerClicked);
	if (Button_Overwatch) Button_Overwatch->OnClicked.AddUniqueDynamic(this, &UUnitActionBarWidget::HandleOverwatchClicked);
	if (Button_Skip)      Button_Skip->OnClicked.AddUniqueDynamic(this, &UUnitActionBarWidget::HandleSkipClicked);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("ActionBar initialized. ReloadButton=%s"),
		Button_Reload ? *Button_Reload->GetName() : TEXT("NULL"));
}

void UUnitActionBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Melee)     Button_Melee->SetIsEnabled(false);
	if (Button_Fire)      Button_Fire->SetIsEnabled(false);
	if (Button_Reload)    Button_Reload->SetIsEnabled(false);
	if (Button_Hunker)    Button_Hunker->SetIsEnabled(false);
	if (Button_Overwatch) Button_Overwatch->SetIsEnabled(false);
	if (Button_Skip)      Button_Skip->SetIsEnabled(false);
	
	UE_LOG(LogTemp, Warning, TEXT("ActionBar NativeConstruct called"));
}

void UUnitActionBarWidget::SetActions(const TArray<FUnitActionButtonData>& Actions)
{
	bool bCanFire = false;
	bool bCanMelee = false;
	bool bCanReload = false;
	bool bCanHunkerDown = false;
	bool bCanOverwatch = false;
	bool bCanSkip = false;

	for (const FUnitActionButtonData& Action : Actions)
	{
		switch (Action.ActionType)
		{
		case EPlayerUnitActionType::WeaponAttack:
			bCanFire = Action.bEnabled;
			break;

		case EPlayerUnitActionType::MeleeAttack:
			bCanMelee = Action.bEnabled;
			break;

		case EPlayerUnitActionType::Reload:
			bCanReload = Action.bEnabled;
			break;

		case EPlayerUnitActionType::HunkerDown:
			bCanHunkerDown = Action.bEnabled;
			break;

		case EPlayerUnitActionType::Overwatch:
			bCanOverwatch = Action.bEnabled;
			break;

		case EPlayerUnitActionType::SkipTurn:
			bCanSkip = Action.bEnabled;
			break;

		default:
			break;
		}
	}

	if (Button_Fire)
	{
		Button_Fire->SetIsEnabled(bCanFire);
	}

	if (Button_Melee)
	{
		Button_Melee->SetIsEnabled(bCanMelee);
	}

	if (Button_Reload)
	{
		Button_Reload->SetIsEnabled(bCanReload);
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("ActionBar SetActions: Fire=%d Reload=%d ReloadButton=%s"),
		bCanFire ? 1 : 0,
		bCanReload ? 1 : 0,
		Button_Reload ? *Button_Reload->GetName() : TEXT("NULL"));

	if (Button_Hunker)
	{
		Button_Hunker->SetIsEnabled(bCanHunkerDown);
	}

	if (Button_Overwatch)
	{
		Button_Overwatch->SetIsEnabled(bCanOverwatch);
	}

	if (Button_Skip)
	{
		Button_Skip->SetIsEnabled(bCanSkip);
	}
}

void UUnitActionBarWidget::OnActionClicked(EPlayerUnitActionType ActionType)
{
	OnUnitActionClicked.Broadcast(ActionType);
}

void UUnitActionBarWidget::HandleMeleeClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("ActionBar Melee pressed"));
	OnActionClicked(EPlayerUnitActionType::MeleeAttack);
}

void UUnitActionBarWidget::HandleFireClicked()
{
	OnActionClicked(EPlayerUnitActionType::WeaponAttack);
}

void UUnitActionBarWidget::HandleReloadClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("ActionBar Reload pressed"));
	OnActionClicked(EPlayerUnitActionType::Reload);
}

void UUnitActionBarWidget::HandleHunkerClicked()
{
	OnActionClicked(EPlayerUnitActionType::HunkerDown);
}

void UUnitActionBarWidget::HandleOverwatchClicked()
{
	OnActionClicked(EPlayerUnitActionType::Overwatch);
}

void UUnitActionBarWidget::HandleSkipClicked()
{
	OnActionClicked(EPlayerUnitActionType::SkipTurn);
}

PRAGMA_ENABLE_OPTIMIZATION
