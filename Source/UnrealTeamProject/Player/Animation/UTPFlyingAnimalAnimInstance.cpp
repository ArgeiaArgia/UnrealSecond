#include "UTPFlyingAnimalAnimInstance.h"

void UTPFlyingAnimalAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ResetFlyingAnimationState();
	bWasAirborne = false;
}

void UTPFlyingAnimalAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const AUTPFlyingAnimalCharacter* FlyingAnimal =
		Cast<AUTPFlyingAnimalCharacter>(TryGetPawnOwner());
	if (!IsValid(FlyingAnimal))
	{
		ResetFlyingAnimationState();
		bWasAirborne = false;
		return;
	}

	FlightState = FlyingAnimal->GetFlightState();
	bIsFlying = FlyingAnimal->IsFlying();
	bIsGliding = FlyingAnimal->IsGliding();
	bIsInsideWindZone = FlyingAnimal->IsInsideWindZone();
	FlightSpeed = FlyingAnimal->GetVelocity().Size();

	// This pulse drives a transition into the landing state. The landing state
	// itself remains active until FlightState returns to Perched.
	bJustLanded = bWasAirborne && FlightState == EUTPFlightState::Landing;
	bWasAirborne = bIsFlying || FlightState == EUTPFlightState::Falling;
}

bool UTPFlyingAnimalAnimInstance::IsAbilityActive(const APawn* PawnOwner) const
{
	if (const AUTPFlyingAnimalCharacter* FlyingAnimal = Cast<AUTPFlyingAnimalCharacter>(PawnOwner))
	{
		return FlyingAnimal->IsFlightAbilityActive();
	}

	return false;
}

void UTPFlyingAnimalAnimInstance::ResetFlyingAnimationState()
{
	FlightState = EUTPFlightState::Perched;
	bIsFlying = false;
	bIsGliding = false;
	bIsInsideWindZone = false;
	bJustLanded = false;
	FlightSpeed = 0.0f;
}
