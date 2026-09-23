#include "UTPSharedCameraRig.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

namespace
{
	constexpr float DefaultCameraPitchDegrees = -60.0f;
	constexpr float MinimumCameraPitchDegrees = -80.0f;
	constexpr float MaximumCameraPitchDegrees = -35.0f;
}

AUTPSharedCameraRig::AUTPSharedCameraRig()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(Root);
	SpringArmComponent->TargetArmLength = 450.0f;
	SpringArmComponent->bUsePawnControlRotation = false;
	SpringArmComponent->bEnableCameraLag = bEnableFollowLag;
	SpringArmComponent->CameraLagSpeed = FollowLagSpeed;
	SpringArmComponent->SetUsingAbsoluteRotation(false);
	SpringArmComponent->bInheritPitch = true;
	SpringArmComponent->bInheritYaw = true;
	SpringArmComponent->bInheritRoll = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;
}

void AUTPSharedCameraRig::Initialize(APlayerController* InOwningPlayerController)
{
	OwningPlayerController = InOwningPlayerController;

	// The shared camera starts above the character and stays within its
	// overhead viewing range while the player looks around.
	const float InitialYaw = InOwningPlayerController
		? InOwningPlayerController->GetControlRotation().Yaw
		: GetActorRotation().Yaw;
	SetActorRotation(FRotator(DefaultCameraPitchDegrees, InitialYaw, 0.0f));
}

void AUTPSharedCameraRig::SetFollowTarget(APawn* InFollowTarget, bool bSnapToTarget)
{
	CancelPossessionTransition();

	if (FollowTarget.IsValid())
	{
		PrimaryActorTick.RemovePrerequisite(FollowTarget.Get(), FollowTarget->PrimaryActorTick);
	}

	FollowTarget = InFollowTarget;

	if (FollowTarget.IsValid())
	{
		// Always update after the target's movement has completed.
		PrimaryActorTick.AddPrerequisite(FollowTarget.Get(), FollowTarget->PrimaryActorTick);

		// Only the very first target is allowed to establish the rig's location.
		// Body changes retain this one rig and are eased by Tick().
		if (bSnapToTarget)
		{
			SetActorLocation(FollowTarget->GetActorLocation() + FollowOffset);
		}
	}
}

void AUTPSharedCameraRig::BeginPossessionTransition(APawn* InTransitionTarget, float InDuration)
{
	if (!IsValid(InTransitionTarget))
	{
		return;
	}

	PossessionTransitionTarget = InTransitionTarget;
	PossessionTransitionStartLocation = GetActorLocation();
	PossessionTransitionElapsed = 0.0f;
	PossessionTransitionDuration = FMath::Max(0.0f, InDuration);
	bPossessionTransitionActive = PossessionTransitionDuration > KINDA_SMALL_NUMBER;

	if (!bPossessionTransitionActive)
	{
		SetFollowTarget(InTransitionTarget);
	}
}

void AUTPSharedCameraRig::CancelPossessionTransition()
{
	bPossessionTransitionActive = false;
	PossessionTransitionTarget = nullptr;
	PossessionTransitionElapsed = 0.0f;
	PossessionTransitionDuration = 0.0f;
}

bool AUTPSharedCameraRig::IsPossessionTransitionActive() const
{
	return bPossessionTransitionActive;
}

FRotator AUTPSharedCameraRig::GetCameraRigRotation() const
{
	return GetActorRotation();
}

void AUTPSharedCameraRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FRotator ControlRotation = OwningPlayerController.IsValid()
		? OwningPlayerController->GetControlRotation()
		: GetActorRotation();
	const FRotator TargetRotation(
		FMath::Clamp(ControlRotation.Pitch, MinimumCameraPitchDegrees, MaximumCameraPitchDegrees),
		ControlRotation.Yaw,
		0.0f);

	SpringArmComponent->bEnableCameraLag = bEnableFollowLag;
	SpringArmComponent->CameraLagSpeed = FollowLagSpeed;

	if (bPossessionTransitionActive && PossessionTransitionTarget.IsValid())
	{
		PossessionTransitionElapsed = FMath::Min(
			PossessionTransitionElapsed + DeltaSeconds,
			PossessionTransitionDuration);
		const float Alpha = PossessionTransitionElapsed / PossessionTransitionDuration;
		const float EasedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
		const FVector TargetLocation = PossessionTransitionTarget->GetActorLocation() + FollowOffset;
		SetActorLocationAndRotation(
			FMath::Lerp(PossessionTransitionStartLocation, TargetLocation, EasedAlpha),
			TargetRotation);

		if (Alpha >= 1.0f)
		{
			APawn* CompletedTarget = PossessionTransitionTarget.Get();
			SetFollowTarget(CompletedTarget);
		}
		return;
	}

	const FVector TargetLocation = FollowTarget.IsValid()
		? FollowTarget->GetActorLocation() + FollowOffset
		: GetActorLocation();
	const FVector SmoothedLocation = FollowTarget.IsValid()
		? FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaSeconds, FollowLocationInterpSpeed)
		: TargetLocation;
	SetActorLocationAndRotation(SmoothedLocation, TargetRotation);
}
