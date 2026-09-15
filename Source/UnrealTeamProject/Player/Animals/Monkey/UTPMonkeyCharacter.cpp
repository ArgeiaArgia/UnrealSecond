#include "UTPMonkeyCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr uint8 MonkeyClimbingMovementMode = 1;
}

AUTPMonkeyCharacter::AUTPMonkeyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 320.0f;
		Movement->MaxAcceleration = 1100.0f;
		Movement->BrakingDecelerationWalking = 1000.0f;
		Movement->JumpZVelocity = 520.0f;
		Movement->AirControl = 0.35f;
	}

	// An unpossessed monkey is intentionally left in the puzzle at its current location.
	bDisableMovementWhenUnpossessed = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MonkeyMeshAsset(
		TEXT("/Game/_Art/QuirkyMinimal/Monkey/Models/Monkey_LOD0.Monkey_LOD0"));
	if (MonkeyMeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MonkeyMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InteractActionAsset(
		TEXT("/Game/Input/InputActions/IA_Interact.IA_Interact"));
	if (InteractActionAsset.Succeeded())
	{
		InteractAction = InteractActionAsset.Object;
	}
}

void AUTPMonkeyCharacter::BeginPlay()
{
	Super::BeginPlay();

	MonkeyState = EUTPMonkeyState::Grounded;
	bIsAttachedToClimbRoute = false;
	ClimbInput = 0.0f;
}

void AUTPMonkeyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsSoulPossessed())
	{
		return;
	}

	if (bIsAttachedToClimbRoute)
	{
		TickClimbing(DeltaSeconds);
	}

	if (MonkeyState == EUTPMonkeyState::Jumping)
	{
		if (const UCharacterMovementComponent* Movement = GetCharacterMovement();
			Movement && Movement->IsMovingOnGround())
		{
			MonkeyState = EUTPMonkeyState::Grounded;
		}
	}
}

void AUTPMonkeyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(
				InteractAction,
				ETriggerEvent::Started,
				this,
				&AUTPMonkeyCharacter::HandleInteractStarted);
		}

		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(
				MoveAction,
				ETriggerEvent::Completed,
				this,
				&AUTPMonkeyCharacter::ClearMoveInput);
			EnhancedInputComponent->BindAction(
				MoveAction,
				ETriggerEvent::Canceled,
				this,
				&AUTPMonkeyCharacter::ClearMoveInput);
		}
	}
}

void AUTPMonkeyCharacter::Move(const FInputActionValue& Value)
{
	if (!IsSoulPossessed() || IsPossessionTransitionInputLocked())
	{
		return;
	}

	const FVector2D Input = Value.Get<FVector2D>();
	if (bIsAttachedToClimbRoute)
	{
		ClimbInput = FMath::Clamp(Input.Y, -1.0f, 1.0f);
		if (!FMath::IsNearlyZero(ClimbInput))
		{
			MonkeyState = EUTPMonkeyState::Climbing;
			if (UCharacterMovementComponent* Movement = GetCharacterMovement())
			{
				Movement->SetMovementMode(MOVE_Custom, MonkeyClimbingMovementMode);
			}
		}
		return;
	}

	Super::Move(Value);
}

void AUTPMonkeyCharacter::StartJump()
{
	if (!IsSoulPossessed() || IsPossessionTransitionInputLocked())
	{
		return;
	}

	if (bIsAttachedToClimbRoute)
	{
		DetachFromClimbRoute(true);
		return;
	}

	Super::StartJump();
	MonkeyState = EUTPMonkeyState::Jumping;
}

void AUTPMonkeyCharacter::OnPossessedBySoul_Implementation(APawn* SoulPawn)
{
	const bool bCanResumeRoute = IsValid(ActiveClimbRoute) && bIsAttachedToClimbRoute;

	Super::OnPossessedBySoul_Implementation(SoulPawn);

	ClimbInput = 0.0f;
	if (bCanResumeRoute && ActiveClimbRoute->GetClass()->ImplementsInterface(UTPClimbableInterface::StaticClass()) &&
		ITPClimbableInterface::Execute_CanClimb(ActiveClimbRoute, this))
	{
		MonkeyState = EUTPMonkeyState::Climbing;
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Custom, MonkeyClimbingMovementMode);
			Movement->Velocity = FVector::ZeroVector;
		}
		SnapToClimbRoute();
	}
	else
	{
		ResetClimbState(true);
		MonkeyState = EUTPMonkeyState::Grounded;
	}
}

void AUTPMonkeyCharacter::OnReleasedFromSoul_Implementation(APawn* SoulPawn)
{
	const bool bWasOnRoute = bIsAttachedToClimbRoute;

	Super::OnReleasedFromSoul_Implementation(SoulPawn);

	ClimbInput = 0.0f;
	if (bWasOnRoute && IsValid(ActiveClimbRoute))
	{
		MonkeyState = EUTPMonkeyState::Climbing;
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->Velocity = FVector::ZeroVector;
		}
	}
	else
	{
		ResetClimbState(true);
		MonkeyState = EUTPMonkeyState::Grounded;
	}
}

bool AUTPMonkeyCharacter::CanReleaseFromSoul_Implementation() const
{
	if (!IsSoulPossessed())
	{
		return false;
	}

	if (bIsAttachedToClimbRoute || MonkeyState == EUTPMonkeyState::Grounded)
	{
		const UCharacterMovementComponent* Movement = GetCharacterMovement();
		return !Movement || !Movement->IsFalling();
	}

	return false;
}

bool AUTPMonkeyCharacter::TryAttachToClimbRoute()
{
	if (!IsSoulPossessed())
	{
		return false;
	}

	AActor* TargetRoute = FindNearbyClimbRoute();
	if (!IsValid(TargetRoute))
	{
		return false;
	}

	const float RouteLength = ITPClimbableInterface::Execute_GetClimbRouteLength(TargetRoute);
	if (RouteLength <= 0.0f)
	{
		return false;
	}

	ActiveClimbRoute = TargetRoute;
	ClimbDistance = FindNearestDistanceOnRoute(TargetRoute);
	CurrentClimbSurface = ITPClimbableInterface::Execute_GetClimbSurfaceType(TargetRoute);
	ClimbInput = 0.0f;
	bIsAttachedToClimbRoute = true;
	MonkeyState = EUTPMonkeyState::Climbing;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Custom, MonkeyClimbingMovementMode);
	}

	SnapToClimbRoute();
	return true;
}

void AUTPMonkeyCharacter::DetachFromClimbRoute(bool bLaunchFromRoute)
{
	if (!ActiveClimbRoute || !bIsAttachedToClimbRoute)
	{
		return;
	}

	const FVector LaunchDirection = GetActorForwardVector() * RouteJumpForwardSpeed +
		FVector::UpVector * RouteJumpUpwardSpeed;
	ResetClimbState(true);
	MonkeyState = EUTPMonkeyState::Jumping;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Falling);
	}

	if (bLaunchFromRoute)
	{
		LaunchCharacter(LaunchDirection, true, true);
	}
}

void AUTPMonkeyCharacter::TickClimbing(float DeltaSeconds)
{
	if (!IsValid(ActiveClimbRoute) ||
		!ActiveClimbRoute->GetClass()->ImplementsInterface(UTPClimbableInterface::StaticClass()))
	{
		ResetClimbState(true);
		MonkeyState = EUTPMonkeyState::Grounded;
		return;
	}

	const float RouteLength = ITPClimbableInterface::Execute_GetClimbRouteLength(ActiveClimbRoute);
	if (RouteLength <= 0.0f)
	{
		return;
	}

	const float PreviousDistance = ClimbDistance;
	ClimbDistance = FMath::Clamp(ClimbDistance + ClimbInput * ClimbSpeed * DeltaSeconds, 0.0f, RouteLength);
	SnapToClimbRoute();

	const bool bReachedLowerEnd = ClimbDistance <= RouteEndTolerance && ClimbInput < 0.0f;
	const bool bReachedUpperEnd = (RouteLength - ClimbDistance) <= RouteEndTolerance && ClimbInput > 0.0f;
	if (bReachedLowerEnd || bReachedUpperEnd)
	{
		ClimbDistance = bReachedLowerEnd ? 0.0f : RouteLength;
		ClimbInput = 0.0f;
		MonkeyState = EUTPMonkeyState::Climbing;
		SnapToClimbRoute();
	}
	else if (FMath::IsNearlyEqual(PreviousDistance, ClimbDistance))
	{
		// Keep the actor snapped even while the input is idle.
		SnapToClimbRoute();
	}
}

void AUTPMonkeyCharacter::SnapToClimbRoute()
{
	if (!IsValid(ActiveClimbRoute) ||
		!ActiveClimbRoute->GetClass()->ImplementsInterface(UTPClimbableInterface::StaticClass()))
	{
		return;
	}

	const FVector NewLocation = ITPClimbableInterface::Execute_GetClimbLocationAtDistance(
		ActiveClimbRoute, ClimbDistance);
	const FRotator NewRotation = ITPClimbableInterface::Execute_GetClimbRotationAtDistance(
		ActiveClimbRoute, ClimbDistance);
	SetActorLocationAndRotation(NewLocation, NewRotation, true);
}

AActor* AUTPMonkeyCharacter::FindNearbyClimbRoute() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AActor* BestRoute = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate) ||
			!Candidate->GetClass()->ImplementsInterface(UTPClimbableInterface::StaticClass()))
		{
			continue;
		}

		if (!ITPClimbableInterface::Execute_CanClimb(Candidate, const_cast<AUTPMonkeyCharacter*>(this)))
		{
			continue;
		}

		const FVector AttachLocation = ITPClimbableInterface::Execute_GetClimbAttachLocation(
			Candidate, GetActorLocation());
		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), AttachLocation);
		if (DistanceSquared <= FMath::Square(ClimbSearchRadius) && DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestRoute = Candidate;
		}
	}

	return BestRoute;
}

float AUTPMonkeyCharacter::FindNearestDistanceOnRoute(AActor* Route) const
{
	if (!IsValid(Route) || !Route->GetClass()->ImplementsInterface(UTPClimbableInterface::StaticClass()))
	{
		return 0.0f;
	}

	const float RouteLength = ITPClimbableInterface::Execute_GetClimbRouteLength(Route);
	if (RouteLength <= 0.0f)
	{
		return 0.0f;
	}

	constexpr int32 SampleCount = 64;
	float BestDistance = 0.0f;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index <= SampleCount; ++Index)
	{
		const float Distance = RouteLength * static_cast<float>(Index) / static_cast<float>(SampleCount);
		const FVector Location = ITPClimbableInterface::Execute_GetClimbLocationAtDistance(Route, Distance);
		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Location);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestDistance = Distance;
		}
	}

	return BestDistance;
}

void AUTPMonkeyCharacter::HandleInteractStarted()
{
	if (IsPossessionTransitionInputLocked())
	{
		return;
	}

	if (bIsAttachedToClimbRoute)
	{
		return;
	}

	TryAttachToClimbRoute();
}

void AUTPMonkeyCharacter::ClearMoveInput()
{
	ClimbInput = 0.0f;
}

void AUTPMonkeyCharacter::ResetClimbState(bool bClearRoute)
{
	ClimbInput = 0.0f;
	bIsAttachedToClimbRoute = false;
	CurrentClimbSurface = EUTPClimbSurfaceType::Ledge;
	ClimbDistance = 0.0f;
	if (bClearRoute)
	{
		ActiveClimbRoute = nullptr;
	}
}
