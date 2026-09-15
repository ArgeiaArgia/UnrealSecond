#pragma once

#include "UTPCharacterAnimInstance.h"
#include "UTPMonkeyAnimInstance.generated.h"

UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API UTPMonkeyAnimInstance : public UTPCharacterAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category="Animation|Monkey")
	bool bIsAttachedToClimbRoute = false;

	UPROPERTY(BlueprintReadOnly, Category="Animation|Monkey")
	float ClimbDirection = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Animation|Monkey")
	float ClimbSpeed = 0.0f;
};
