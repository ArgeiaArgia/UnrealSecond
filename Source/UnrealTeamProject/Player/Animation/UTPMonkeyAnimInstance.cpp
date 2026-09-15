#include "UTPMonkeyAnimInstance.h"

#include "../Animals/Monkey/UTPMonkeyCharacter.h"

void UTPMonkeyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	bIsAttachedToClimbRoute = false;
	ClimbDirection = 0.0f;
	ClimbSpeed = 0.0f;

	if (AUTPMonkeyCharacter* Monkey = Cast<AUTPMonkeyCharacter>(TryGetPawnOwner()))
	{
		bIsAttachedToClimbRoute = Monkey->IsAttachedToClimbRoute();
		ClimbDirection = Monkey->GetClimbInput();
		ClimbSpeed = bIsAttachedToClimbRoute ? FMath::Abs(ClimbDirection) * Monkey->GetClimbSpeed() : 0.0f;
	}
}
