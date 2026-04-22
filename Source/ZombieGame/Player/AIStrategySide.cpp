#include "AIStrategySide.h"

#include "../Variant_Strategy/StrategyGameMode.h"

void AAIStrategySide::TakeTurn()
{
	Super::TakeTurn();

	FTimerHandle TimerHandle;

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		this,
		&AAIStrategySide::OnTurnDone,
		5.0f,   // delay i sekunder
		false   // loop?
	);
}

void AAIStrategySide::OnTurnDone()
{
	UE_LOG(LogTemp, Warning, TEXT("Timer triggered!"));

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