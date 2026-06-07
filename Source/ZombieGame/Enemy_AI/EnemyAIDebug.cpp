#include "EnemyAIDebug.h"

#if !UE_BUILD_SHIPPING

#include "EnemyUnitAI.h"
#include "StrategyUnit.h"
#include "ZombieGame.h"
#include "Engine/Engine.h"
#include "Systems/AttackHandling/StrategyAttackResolver.h"

namespace
{
	bool bEnemyAIDebugEnabled = false;

	FString GetActionName(EEnemyAIActionType ActionType)
	{
		switch (ActionType)
		{
		case EEnemyAIActionType::BiteAttack:
			return TEXT("BiteAttack");
		case EEnemyAIActionType::Crouch:
			return TEXT("Crouch");
		case EEnemyAIActionType::MoveToCover:
			return TEXT("MoveToCover");
		case EEnemyAIActionType::MoveTowardNearestVisiblePlayer:
			return TEXT("MoveTowardNearestVisiblePlayer");
		case EEnemyAIActionType::HoldHighGround:
			return TEXT("HoldHighGround");
		case EEnemyAIActionType::ProtectSniper:
			return TEXT("ProtectSniper");
		case EEnemyAIActionType::SpreadOut:
			return TEXT("SpreadOut");
		case EEnemyAIActionType::Wait:
			return TEXT("Wait");
		default:
			return TEXT("Unknown");
		}
	}

	void EmitDebugMessage(const FString& Message)
	{
		UE_LOG(LogZombieGame, Log, TEXT("[EnemyAI] %s"), *Message);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				INDEX_NONE,
				5.0f,
				FColor::Orange,
				FString::Printf(TEXT("EnemyAI: %s"), *Message));
		}
	}

	FString DescribeCandidate(const FEnemyActionCandidate& Candidate)
	{
		return FString::Printf(
			TEXT("%s Score=%.1f TUCost=%d TargetCell=(%d,%d) DistAfter=%d Target=%s"),
			*GetActionName(Candidate.ActionType),
			Candidate.Score,
			Candidate.TimeUnitCost,
			Candidate.TargetCell.X,
			Candidate.TargetCell.Y,
			Candidate.DistanceToTargetAfterMove,
			*GetNameSafe(Candidate.TargetUnit));
	}
}

void FEnemyAIDebug::SetEnabled(bool bEnabled)
{
	bEnemyAIDebugEnabled = bEnabled;
	EmitDebugMessage(FString::Printf(TEXT("debug %s"), bEnemyAIDebugEnabled ? TEXT("enabled") : TEXT("disabled")));
}

bool FEnemyAIDebug::IsEnabled()
{
	return bEnemyAIDebugEnabled;
}

void FEnemyAIDebug::LogTurnStart(const AStrategyUnit* Unit)
{
	if (!bEnemyAIDebugEnabled || !Unit)
	{
		return;
	}

	EmitDebugMessage(FString::Printf(
		TEXT("%s turn start. HP=%d/%d TU=%d/%d"),
		*GetNameSafe(Unit),
		Unit->GetCurrentHealth(),
		Unit->GetMaxHealth(),
		Unit->GetRemainingTimeUnits(),
		Unit->GetMaxTimeUnits()));
}

void FEnemyAIDebug::LogCandidates(const AStrategyUnit* Unit, const TArray<FEnemyActionCandidate>& Candidates)
{
	if (!bEnemyAIDebugEnabled || !Unit)
	{
		return;
	}

	EmitDebugMessage(FString::Printf(
		TEXT("%s generated %d candidate(s). RemainingTU=%d"),
		*GetNameSafe(Unit),
		Candidates.Num(),
		Unit->GetRemainingTimeUnits()));

	for (const FEnemyActionCandidate& Candidate : Candidates)
	{
		EmitDebugMessage(FString::Printf(
			TEXT("%s candidate: %s"),
			*GetNameSafe(Unit),
			*DescribeCandidate(Candidate)));
	}
}

void FEnemyAIDebug::LogSelectedAction(const AStrategyUnit* Unit, const FEnemyActionCandidate& Candidate)
{
	if (!bEnemyAIDebugEnabled || !Unit)
	{
		return;
	}

	EmitDebugMessage(FString::Printf(
		TEXT("%s selected: %s. RemainingTU=%d"),
		*GetNameSafe(Unit),
		*DescribeCandidate(Candidate),
		Unit->GetRemainingTimeUnits()));
}

void FEnemyAIDebug::LogActionCompleted(
	const AStrategyUnit* Unit,
	const FEnemyActionCandidate& Candidate,
	int32 RemainingTimeUnitsBeforeSpend)
{
	if (!bEnemyAIDebugEnabled || !Unit)
	{
		return;
	}

	EmitDebugMessage(FString::Printf(
		TEXT("%s completed %s. TU %d -> %d (cost %d)"),
		*GetNameSafe(Unit),
		*GetActionName(Candidate.ActionType),
		RemainingTimeUnitsBeforeSpend,
		Unit->GetRemainingTimeUnits(),
		Candidate.TimeUnitCost));
}

void FEnemyAIDebug::LogBiteAttackResult(
	const AStrategyUnit* Unit,
	const FEnemyActionCandidate& Candidate,
	const FStrategyAttackResult& Result,
	int32 TargetHealthBefore,
	int32 TargetArmorBefore)
{
	if (!bEnemyAIDebugEnabled || !Unit || !Candidate.TargetUnit)
	{
		return;
	}

	const int32 TargetHealthAfter = Candidate.TargetUnit->GetCurrentHealth();
	const int32 TargetArmorAfter = Candidate.TargetUnit->GetCurrentArmor();

	EmitDebugMessage(FString::Printf(
		TEXT("%s bite -> %s. Hit=%s Crit=%s HitChance=%d CritChance=%d RolledDamage=%d AppliedHealthDamage=%d Armor %d->%d TargetHP %d->%d"),
		*GetNameSafe(Unit),
		*GetNameSafe(Candidate.TargetUnit),
		Result.bHit ? TEXT("yes") : TEXT("no"),
		Result.bCritical ? TEXT("yes") : TEXT("no"),
		Result.HitChance,
		Result.CriticalChance,
		Result.Damage.Damage,
		FMath::Max(TargetHealthBefore - TargetHealthAfter, 0),
		TargetArmorBefore,
		TargetArmorAfter,
		TargetHealthBefore,
		TargetHealthAfter));
}

#endif
