#include "UTPFrogAnimInstance.h"

#include "../Animals/Frog/UTPFrogCharacter.h"

void UTPFrogAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ResetFrogAnimationState();
	bWasInAir = bIsInAir;
}

void UTPFrogAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AUTPFrogCharacter* Frog = Cast<AUTPFrogCharacter>(TryGetPawnOwner());
	if (!IsValid(Frog))
	{
		ResetFrogAnimationState();
		bWasInAir = false;
		return;
	}

	bIsChargingJump = Frog->IsChargingJump();
	JumpChargeRatio = Frog->GetJumpChargeRatio();
	bIsInShallowWater = Frog->IsInShallowWater();

	// This pulse is consumed by the AnimBP transition into a landing state.
	// The landing state itself should remain active until its animation finishes.
	bJustLanded = bWasInAir && !bIsInAir;
	bWasInAir = bIsInAir;
}

bool UTPFrogAnimInstance::IsAbilityActive(const APawn* PawnOwner) const
{
	if (const AUTPFrogCharacter* Frog = Cast<AUTPFrogCharacter>(PawnOwner))
	{
		return Frog->IsChargingJump();
	}

	return false;
}

void UTPFrogAnimInstance::ResetFrogAnimationState()
{
	bIsChargingJump = false;
	JumpChargeRatio = 0.0f;
	bJustLanded = false;
	bIsInShallowWater = false;
}
