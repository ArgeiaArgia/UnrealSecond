#include "UTPCharacterAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "../UTPPossessableCharacter.h"

UTPCharacterAnimInstance::UTPCharacterAnimInstance()
{
}

void UTPCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CachedPawnOwner = Cast<APawn>(TryGetPawnOwner());
	ResetAnimationState();
}

void UTPCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	CachedPawnOwner = Cast<APawn>(TryGetPawnOwner());
	if (!IsValid(CachedPawnOwner.Get()))
	{
		ResetAnimationState();
		return;
	}

	RefreshAnimationState(DeltaSeconds);
}

void UTPCharacterAnimInstance::RefreshAnimationState(float /*DeltaSeconds*/)
{
	const APawn* PawnOwner = CachedPawnOwner.Get();
	if (!IsValid(PawnOwner))
	{
		ResetAnimationState();
		return;
	}

	const FVector Velocity = PawnOwner->GetVelocity();
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.0f).Size();
	VerticalSpeed = Velocity.Z;
	Direction = CalculateDirection(PawnOwner, Velocity);
	bIsInAir = false;

	if (const ACharacter* CharacterOwner = Cast<ACharacter>(PawnOwner))
	{
		if (const UCharacterMovementComponent* CharacterMovement = CharacterOwner->GetCharacterMovement())
		{
			bIsInAir = CharacterMovement->IsFalling();
		}
	}

	bIsPossessed = IsPossessedByPlayer(PawnOwner);
	bIsAbilityActive = IsAbilityActive(PawnOwner);
}

bool UTPCharacterAnimInstance::IsPossessedByPlayer(const APawn* PawnOwner) const
{
	if (!PawnOwner)
	{
		return false;
	}

	if (const AUTPPossessableCharacter* PossessableCharacter = Cast<AUTPPossessableCharacter>(PawnOwner))
	{
		return PossessableCharacter->IsCurrentlyPossessed();
	}

	const AController* Controller = PawnOwner->GetController();
	return Controller && Controller->IsPlayerController();
}

bool UTPCharacterAnimInstance::IsAbilityActive(const APawn* /*PawnOwner*/) const
{
	return false;
}

void UTPCharacterAnimInstance::ResetAnimationState()
{
	GroundSpeed = 0.0f;
	Direction = 0.0f;
	VerticalSpeed = 0.0f;
	bIsInAir = false;
	bIsPossessed = false;
	bIsAbilityActive = false;
}

float UTPCharacterAnimInstance::CalculateDirection(const APawn* PawnOwner, const FVector& Velocity)
{
	if (!PawnOwner || Velocity.IsNearlyZero())
	{
		return 0.0f;
	}

	const FRotator YawRotation(0.0f, PawnOwner->GetActorRotation().Yaw, 0.0f);
	const FVector LocalVelocity = YawRotation.UnrotateVector(Velocity);
	return FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
}
