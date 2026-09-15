#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UTPWindReceiverInterface.generated.h"

UINTERFACE(BlueprintType)
class UNREALTEAMPROJECT_API UTPWindReceiverInterface : public UInterface
{
	GENERATED_BODY()
};

/** Receives the current wind stream without coupling the receiver to a wind-zone class. */
class UNREALTEAMPROJECT_API ITPWindReceiverInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Wind")
	void EnterWindZone(
		AActor* WindSource,
		FVector WindDirection,
		float WindSpeed,
		float LiftStrength);

	virtual void EnterWindZone_Implementation(
		AActor* WindSource,
		FVector WindDirection,
		float WindSpeed,
		float LiftStrength)
	{
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Wind")
	void ExitWindZone(AActor* WindSource);

	virtual void ExitWindZone_Implementation(AActor* WindSource)
	{
	}
};
