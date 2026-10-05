#include "UTPFlyingAnimalMovementComponent.h"

#include "UTPFlyingAnimalCharacter.h"

void UTPFlyingAnimalMovementComponent::PhysFlying(float DeltaTime, int32 Iterations)
{
	Super::PhysFlying(DeltaTime, Iterations);
	const AUTPFlyingAnimalCharacter* FlyingAnimal = Cast<AUTPFlyingAnimalCharacter>(CharacterOwner);
	if (IsFalling() && FlyingAnimal && FlyingAnimal->GetFlightState() == EUTPFlightState::Falling)
	{
		// An overlap can end the wind during Super::PhysFlying. Its final velocity
		// reconstruction must not restore the upward motion we just cancelled.
		Velocity.Z = FMath::Min(Velocity.Z, 0.0);
	}
}

void UTPFlyingAnimalMovementComponent::CalcVelocity(
	float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	const AUTPFlyingAnimalCharacter* FlyingAnimal = Cast<AUTPFlyingAnimalCharacter>(CharacterOwner);
	if (!HasValidData() || !IsFlying() || !FlyingAnimal || !FlyingAnimal->IsSoulPossessed() ||
		!FlyingAnimal->IsFlying() || HasAnimRootMotion() || DeltaTime < MIN_TICK_TIME ||
		(FlyingAnimal->GetLocalRole() == ROLE_SimulatedProxy && !bWasSimulatingRootMotion))
	{
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		return;
	}

	// CharacterMovement's flying friction and braking act on all three axes.
	// Apply them to steering only, then smoothly update vertical flight speed.
	const float PreviousVerticalVelocity = Velocity.Z;
	Velocity.Z = 0.0f;
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	const float DesiredVerticalVelocity = FlyingAnimal->GetDesiredFlightVerticalVelocity();
	Velocity.Z = FMath::FInterpTo(
		PreviousVerticalVelocity,
		DesiredVerticalVelocity,
		DeltaTime,
		FlyingAnimal->WindAcceleration);
}
