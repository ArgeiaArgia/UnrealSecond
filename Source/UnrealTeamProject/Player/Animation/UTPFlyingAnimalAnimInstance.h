#pragma once

#include "../Animals/Flying/UTPFlyingAnimalCharacter.h"
#include "UTPCharacterAnimInstance.h"
#include "UTPFlyingAnimalAnimInstance.generated.h"

/** Animation-state bridge for wind-riding flying animals such as the FlyingFox. */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API UTPFlyingAnimalAnimInstance : public UTPCharacterAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** The current gameplay flight state; use this for the flight/landing state transitions. */
	UPROPERTY(BlueprintReadOnly, Category="Animation|Flying")
	EUTPFlightState FlightState = EUTPFlightState::Perched;

	/** True while the animal is taking off, wind-riding, or gliding. */
	UPROPERTY(BlueprintReadOnly, Category="Animation|Flying")
	bool bIsFlying = false;

	/** True while the animal is gliding, including the temporary glide ability. */
	UPROPERTY(BlueprintReadOnly, Category="Animation|Flying")
	bool bIsGliding = false;

	/** True while the animal is receiving directional wind. */
	UPROPERTY(BlueprintReadOnly, Category="Animation|Flying")
	bool bIsInsideWindZone = false;

	/** True for one animation update when flight transitions into landing. */
	UPROPERTY(BlueprintReadOnly, Category="Animation|Flying|Landing")
	bool bJustLanded = false;

	/** Full three-dimensional movement speed, useful for flight animation play rate. */
	UPROPERTY(BlueprintReadOnly, Category="Animation|Flying")
	float FlightSpeed = 0.0f;

	virtual bool IsAbilityActive(const APawn* PawnOwner) const override;

private:
	bool bWasFlying = false;

	void ResetFlyingAnimationState();
};
