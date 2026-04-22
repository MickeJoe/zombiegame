#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategySide.generated.h"

class AStrategyUnit;
/*
UENUM(BlueprintType)
enum class EStrategySideControllerType : uint8
{
	Human,
	AI
};
*/
UCLASS()
class AStrategySide : public AActor
{
	GENERATED_BODY()

public:
	AStrategySide();

	virtual void TakeTurn();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strategy")
	FName SideId = NAME_None;

//	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Strategy")
//	EStrategySideControllerType ControllerType = EStrategySideControllerType::Human;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Strategy")
	TArray<TObjectPtr<AStrategyUnit>> Units;

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	void AddUnit(AStrategyUnit* Unit);

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	void RemoveUnit(AStrategyUnit* Unit);

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	TArray<AStrategyUnit*> GetAliveUnits() const;

	UFUNCTION(BlueprintCallable, Category = "Strategy")
	bool HasLivingUnits() const;

//	UFUNCTION(BlueprintCallable, Category = "Strategy")
	virtual bool IsHuman() const;

//	UFUNCTION(BlueprintCallable, Category = "Strategy")
	virtual bool IsAI() const;
};