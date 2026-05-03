#include "UnitActionBarWidget.h"

#include "Components/Button.h"

PRAGMA_DISABLE_OPTIMIZATION

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

	if (Button_Hunker)
	{
		Button_Hunker->SetIsEnabled(bCanHunkerDown);
	}

	if (Button_Overwatch)
	{
		Button_Overwatch->SetIsEnabled(bCanOverwatch);
	}
}

void UUnitActionBarWidget::OnActionClicked(EPlayerUnitActionType ActionType)
{
	OnUnitActionClicked.Broadcast(ActionType);
}

PRAGMA_ENABLE_OPTIMIZATION