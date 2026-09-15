#pragma once

#include "UObject/Interface.h"
#include "UTPShallowWaterInterface.generated.h"

UINTERFACE(BlueprintType)
class UNREALTEAMPROJECT_API UTPShallowWaterInterface : public UInterface
{
	GENERATED_BODY()
};

/** Contract used by shallow-water zones so puzzle actors stay decoupled from frog C++ types. */
class UNREALTEAMPROJECT_API ITPShallowWaterInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Water")
	bool CanTraverseWater(AActor* Actor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Water")
	void EnterWater(AActor* Actor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Water")
	void ExitWater(AActor* Actor);
};
