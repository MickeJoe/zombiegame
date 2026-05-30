// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TargetingCameraMode.generated.h"

UENUM(BlueprintType)
enum class EStrategyTargetingCameraView : uint8
{
	NoViewChange UMETA(DisplayName = "No View Change"),
	FirstPerson UMETA(DisplayName = "1st Party Targeting"),
	ThirdPerson UMETA(DisplayName = "3rd Party Targeting")
};
