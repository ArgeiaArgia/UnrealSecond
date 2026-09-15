#pragma once

#include "UTPCharacterAnimInstance.h"
#include "UTPFrogAnimInstance.generated.h"

class APawn;

/** Animation state bridge for the frog's charge jump and landing transitions. */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API UTPFrogAnimInstance : public UTPCharacterAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** True while the player is holding the frog's jump input and building jump power. */
	UPROPERTY(BlueprintReadOnly, Category="Animation|Frog|Jump")
	bool bIsChargingJump = false;

	/** Normalized charge amount from 0.0 to 1.0; use this to drive a charge pose or blend. */
	UPROPERTY(BlueprintReadOnly, Category="Animation|Frog|Jump")
	float JumpChargeRatio = 0.0f;

	/** True for one animation update after an airborne frog touches the ground. */
	UPROPERTY(BlueprintReadOnly, Category="Animation|Frog|Landing")
	bool bJustLanded = false;

	/** True while the frog is within a shallow-water volume. */
	UPROPERTY(BlueprintReadOnly, Category="Animation|Frog|Water")
	bool bIsInShallowWater = false;

	virtual bool IsAbilityActive(const APawn* PawnOwner) const override;

private:
	bool bWasInAir = false;

	void ResetFrogAnimationState();
};
