#pragma once

#include "../../UTPPossessableCharacter.h"
#include "../../Puzzles/UTPWeightProviderInterface.h"
#include "UTPTurtleCharacter.generated.h"

UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPTurtleCharacter
	: public AUTPPossessableCharacter
	, public ITPWeightProviderInterface
{
	GENERATED_BODY()

public:
	AUTPTurtleCharacter();

	virtual void BeginPlay() override;
	virtual void OnPossessedBySoul_Implementation(APawn* SoulPawn) override;
	virtual void OnReleasedFromSoul_Implementation(APawn* SoulPawn) override;
	virtual float GetWeight_Implementation() const override;
	virtual bool CanActivateWeightPlate_Implementation() const override;

	UFUNCTION(BlueprintPure, Category="Animal|Turtle")
	bool IsBracing() const { return bIsBracing; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Turtle", meta=(ClampMin="0.0"))
	float BraceMass = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Turtle")
	bool bIsBracing = false;
};
