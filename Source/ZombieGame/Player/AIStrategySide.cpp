#include "AIStrategySide.h"

#include "PlayerStrategySide.h"
#include "../Variant_Strategy/StrategyGameMode.h"
#include "Enemy_AI/WalkerEnemyAI.h"

void AAIStrategySide::TakeTurn(AGridManager* GridManager, APlayerStrategySide* PlayerSide)
{
	Super::TakeTurn(GridManager, PlayerSide);
	
	CachedPlayerSide = PlayerSide;
	CachedGridManager = GridManager;
	
	CurrentUnitIndex = 0;
	StartNextEnemyUnitTurn();
}

void AAIStrategySide::OnEnemyUnitTurnDone(AStrategyUnit* Unit)
{
	StartNextEnemyUnitTurn();
}

void AAIStrategySide::StartNextEnemyUnitTurn()
{
	while (Units.IsValidIndex(CurrentUnitIndex))
	{
		AStrategyUnit* Unit = Units[CurrentUnitIndex];

		++CurrentUnitIndex;

		if (!Unit || !Unit->GetEnemyAI())
		{
			continue;
		}

		Unit->ResetActionPoints();

		Unit->GetEnemyAI()->TakeTurn(
			Unit,
			CachedGridManager,
			CachedPlayerSide,
			this);

		return;
	}

	OnTurnDone();
}

void AAIStrategySide::OnTurnDone()
{
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