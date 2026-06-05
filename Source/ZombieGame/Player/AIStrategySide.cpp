#include "AIStrategySide.h"

#include "PlayerStrategySide.h"
#include "../Variant_Strategy/StrategyGameMode.h"
#include "../Variant_Strategy/StrategyUnit.h"
#include "Enemy_AI/WalkerEnemyAI.h"
#include "Systems/SightManager.h"

void AAIStrategySide::TakeTurn(AGridManager* GridManager, ASightManager* SightManager, APlayerStrategySide* PlayerSide)
{
	Super::TakeTurn(GridManager, SightManager, PlayerSide);
	
	CachedPlayerSide = PlayerSide;
	CachedGridManager = GridManager;
	CachedSightManager = SightManager;
	
	CurrentUnitIndex = 0;
	ActiveTurnUnits = GetAliveUnits();
	SightManager->UpdateEnemySight();
	StartNextEnemyUnitTurn();
}

void AAIStrategySide::OnEnemyUnitTurnDone(AStrategyUnit* Unit)
{
	StartNextEnemyUnitTurn();
}

void AAIStrategySide::StartNextEnemyUnitTurn()
{
	while (ActiveTurnUnits.IsValidIndex(CurrentUnitIndex))
	{
		AStrategyUnit* Unit = ActiveTurnUnits[CurrentUnitIndex];

		++CurrentUnitIndex;

		if (!IsValid(Unit) || Unit->GetCurrentHealth() <= 0 || !Unit->GetEnemyAI())
		{
			continue;
		}

		Unit->ResetTimeUnits();

		Unit->GetEnemyAI()->TakeTurn(
			Unit,
			CachedGridManager,
			CachedSightManager,
			CachedPlayerSide,
			this);

		return;
	}

	OnTurnDone();
}

void AAIStrategySide::OnTurnDone()
{
	ActiveTurnUnits.Reset();
	CurrentUnitIndex = 0;

	if (AStrategyGameMode* GM = GetWorld()->GetAuthGameMode<AStrategyGameMode>())
	{
		GM->EndTurn();
	}
}

bool AAIStrategySide::IsHuman() const
{
	return false;	
}
bool AAIStrategySide::IsAI() const
{
	return true;
}
