#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "UTPFlyingAnimalMovementComponent.generated.h"

/** Keeps wind and glide velocity independent of horizontal flight braking. */
UCLASS()
class UNREALTEAMPROJECT_API UTPFlyingAnimalMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual void PhysFlying(float DeltaTime, int32 Iterations) override;
};
