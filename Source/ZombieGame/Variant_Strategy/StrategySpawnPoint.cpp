#include "StrategySpawnPoint.h"
#include "Components/ArrowComponent.h"


AStrategySpawnPoint::AStrategySpawnPoint()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(Root);
}

FTransform AStrategySpawnPoint::GetSpawnTransform() const
{
	return GetActorTransform();
}
