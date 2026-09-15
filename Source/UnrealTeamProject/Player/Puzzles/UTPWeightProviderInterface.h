#pragma once

#include "UObject/Interface.h"
#include "UTPWeightProviderInterface.generated.h"

UINTERFACE(BlueprintType)
class UNREALTEAMPROJECT_API UTPWeightProviderInterface : public UInterface
{
	GENERATED_BODY()
};

class UNREALTEAMPROJECT_API ITPWeightProviderInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Puzzle|Weight")
	float GetWeight() const;

	virtual float GetWeight_Implementation() const
	{
		return 0.0f;
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Puzzle|Weight")
	bool CanActivateWeightPlate() const;

	virtual bool CanActivateWeightPlate_Implementation() const
	{
		return true;
	}
};
