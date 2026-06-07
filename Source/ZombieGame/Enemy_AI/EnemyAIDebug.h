#pragma once

#include "CoreMinimal.h"

struct FEnemyActionCandidate;
struct FStrategyAttackResult;
class AStrategyUnit;

#if !UE_BUILD_SHIPPING
class FEnemyAIDebug
{
public:
	static void SetEnabled(bool bEnabled);
	static bool IsEnabled();

	static void LogTurnStart(const AStrategyUnit* Unit);
	static void LogCandidates(const AStrategyUnit* Unit, const TArray<FEnemyActionCandidate>& Candidates);
	static void LogSelectedAction(const AStrategyUnit* Unit, const FEnemyActionCandidate& Candidate);
	static void LogActionCompleted(const AStrategyUnit* Unit, const FEnemyActionCandidate& Candidate, int32 RemainingTimeUnitsBeforeSpend);
	static void LogBiteAttackResult(const AStrategyUnit* Unit, const FEnemyActionCandidate& Candidate, const FStrategyAttackResult& Result, int32 TargetHealthBefore, int32 TargetArmorBefore);
};
#endif
