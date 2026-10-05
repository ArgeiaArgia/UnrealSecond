#include "UTPFlyingAnimalCharacter.h"
#include "UTPFlyingAnimalMovementComponent.h"

#include "../../Animation/UTPFlyingAnimalAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"

AUTPFlyingAnimalCharacter::AUTPFlyingAnimalCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTPFlyingAnimalMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 42.0f);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -42.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GravityScale = 1.0f;
		Movement->AirControl = 0.15f;
		Movement->MaxWalkSpeed = 250.0f;
		Movement->MaxFlySpeed = 250.0f;
		Movement->MaxAcceleration = 900.0f;
		Movement->BrakingDecelerationFlying = 900.0f;
	}

	// A released flying animal must remain where it landed.
	bDisableMovementWhenUnpossessed = true;
	AnimalAnimClass = UTPFlyingAnimalAnimInstance::StaticClass();

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FlyingFoxMeshAsset(
		TEXT("/Game/_Art/QuirkyMinimal/FlyingFox/Models/FlyingFox_LOD0.FlyingFox_LOD0"));
	if (FlyingFoxMeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(FlyingFoxMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> AnimalAbilityAsset(
		TEXT("/Game/Input/InputActions/IA_AnimalAbility.IA_AnimalAbility"));
	if (AnimalAbilityAsset.Succeeded())
	{
		AnimalAbilityAction = AnimalAbilityAsset.Object;
	}
}

void AUTPFlyingAnimalCharacter::BeginPlay()
{
	Super::BeginPlay();

	FlightState = EUTPFlightState::Perched;
	RemainingLandingTime = 0.0f;
	RemainingAbilityTime = 0.0f;
}

void AUTPFlyingAnimalCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsSoulPossessed())
	{
		return;
	}

	UpdateFlightState(DeltaSeconds);

	if (RemainingAbilityTime > 0.0f)
	{
		RemainingAbilityTime = FMath::Max(0.0f, RemainingAbilityTime - DeltaSeconds);
		bIsAbilityActive = RemainingAbilityTime > 0.0f;
	}
	else
	{
		bIsAbilityActive = false;
	}

	if (FlightState == EUTPFlightState::Landing)
	{
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->Velocity = FVector::ZeroVector;
		}
		return;
	}

	if (!IsFlying())
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		// The flight movement component applies descent during movement physics,
		// keeping it independent of horizontal steering and braking.
		Movement->SetMovementMode(MOVE_Flying);
	}
}

void AUTPFlyingAnimalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (AnimalAbilityAction)
		{
			EnhancedInputComponent->BindAction(
				AnimalAbilityAction,
				ETriggerEvent::Started,
				this,
				&AUTPFlyingAnimalCharacter::HandleAnimalAbilityStarted);
		}
	}
}

void AUTPFlyingAnimalCharacter::Move(const FInputActionValue& Value)
{
	if (!IsSoulPossessed() || IsPossessionTransitionInputLocked())
	{
		return;
	}

	// Keep standard camera-relative WASD input on the ground and in flight.
	// The bat is deliberately slow, but it must never discard forward/backward
	// movement merely because it has taken off.
	Super::Move(Value);
}

void AUTPFlyingAnimalCharacter::StartJump()
{
	StartTakeoff();
}

void AUTPFlyingAnimalCharacter::OnPossessedBySoul_Implementation(APawn* SoulPawn)
{
	StopSettlingAfterRelease();
	Super::OnPossessedBySoul_Implementation(SoulPawn);

	RemainingLandingTime = 0.0f;
	FlightState = EUTPFlightState::Perched;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->Velocity = FVector::ZeroVector;
		Movement->SetMovementMode(MOVE_Walking);
		Movement->GravityScale = 1.0f;
		Movement->AirControl = 0.15f;
	}
}

void AUTPFlyingAnimalCharacter::OnReleasedFromSoul_Implementation(APawn* SoulPawn)
{
	const bool bReleasedInAir = !IsOnWalkableGround();
	Super::OnReleasedFromSoul_Implementation(SoulPawn);

	FlightState = EUTPFlightState::Perched;
	RemainingLandingTime = 0.0f;
	RemainingAbilityTime = 0.0f;
	bIsAbilityActive = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->Velocity = FVector::ZeroVector;
		if (bReleasedInAir)
		{
			// Body transfers are allowed during flight. Let the abandoned body
			// fall to the floor instead of freezing it at its airborne position.
			if (!bSettlingAfterRelease)
			{
				bWasPhysicsWithNoControllerEnabled = Movement->bRunPhysicsWithNoController;
			}
			bSettlingAfterRelease = true;
			FlightState = EUTPFlightState::Glide;
			Movement->bRunPhysicsWithNoController = true;
			Movement->SetMovementMode(MOVE_Falling);
		}
	}
}

bool AUTPFlyingAnimalCharacter::CanReleaseFromSoul_Implementation() const
{
	// Taking off, gliding and riding wind must never lock possession.
	return true;
}

void AUTPFlyingAnimalCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (FlightState == EUTPFlightState::Falling && IsSoulPossessed())
	{
		BeginLanding();
	}
	if (bSettlingAfterRelease && !IsSoulPossessed())
	{
		StopSettlingAfterRelease();
		FlightState = EUTPFlightState::Perched;
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}
}

void AUTPFlyingAnimalCharacter::StopSettlingAfterRelease()
{
	if (!bSettlingAfterRelease)
	{
		return;
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bRunPhysicsWithNoController = bWasPhysicsWithNoControllerEnabled;
	}
	bSettlingAfterRelease = false;
}

void AUTPFlyingAnimalCharacter::EnterWindZone_Implementation(
	AActor* WindSource,
	FVector WindDirection,
	float WindSpeed,
	float LiftStrength)
{
	if (!IsValid(WindSource))
	{
		return;
	}

	FUTPWindSourceState& SourceState = ActiveWindSources.FindOrAdd(WindSource);
	SourceState.Direction = WindDirection.GetSafeNormal();
	SourceState.Speed = FMath::Max(0.0f, WindSpeed);
	SourceState.Lift = LiftStrength;
	RebuildWindState();
}

void AUTPFlyingAnimalCharacter::ExitWindZone_Implementation(AActor* WindSource)
{
	ActiveWindSources.Remove(WindSource);
	RebuildWindState();
}

void AUTPFlyingAnimalCharacter::RebuildWindState()
{
	for (auto It = ActiveWindSources.CreateIterator(); It; ++It)
	{
		if (!IsValid(It.Key().Get()))
		{
			It.RemoveCurrent();
		}
	}

	const FUTPWindSourceState* SelectedSource = nullptr;
	for (const TPair<TWeakObjectPtr<AActor>, FUTPWindSourceState>& Entry : ActiveWindSources)
	{
		if (IsValid(Entry.Key.Get()) && Entry.Value.Speed > 0.0f &&
			!Entry.Value.Direction.IsNearlyZero())
		{
			SelectedSource = &Entry.Value;
			break;
		}
	}

	if (!SelectedSource)
	{
		bIsInsideWindZone = false;
		CurrentWindDirection = FVector::ZeroVector;
		CurrentWindSpeed = 0.0f;
		CurrentLiftStrength = 0.0f;

		if (FlightState == EUTPFlightState::WindRide)
		{
			BeginWindExitFall();
		}
		return;
	}

	CurrentWindDirection = SelectedSource->Direction;
	CurrentWindSpeed = SelectedSource->Speed;
	CurrentLiftStrength = SelectedSource->Lift;
	bIsInsideWindZone = true;

	if (FlightState == EUTPFlightState::Glide || FlightState == EUTPFlightState::Falling)
	{
		FlightState = EUTPFlightState::WindRide;
		if (IsSoulPossessed())
		{
			if (UCharacterMovementComponent* Movement = GetCharacterMovement())
			{
				Movement->SetMovementMode(MOVE_Flying);
			}
		}
	}
}

void AUTPFlyingAnimalCharacter::BeginWindExitFall()
{
	FlightState = EUTPFlightState::Falling;
	RemainingAbilityTime = 0.0f;
	bIsAbilityActive = false;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		// Stop the updraft's residual rise, then use ordinary CharacterMovement
		// gravity and falling physics without glide-speed or ability overrides.
		Movement->Velocity.Z = FMath::Min(Movement->Velocity.Z, 0.0);
		Movement->GravityScale = 1.0f;
		Movement->SetMovementMode(MOVE_Falling);
	}
}

bool AUTPFlyingAnimalCharacter::IsFlying() const
{
	return FlightState == EUTPFlightState::Takeoff ||
		FlightState == EUTPFlightState::WindRide ||
		FlightState == EUTPFlightState::Glide;
}

bool AUTPFlyingAnimalCharacter::IsGliding() const
{
	return FlightState == EUTPFlightState::Glide || bIsAbilityActive;
}

void AUTPFlyingAnimalCharacter::StartTakeoff()
{
	if (!IsSoulPossessed() || IsPossessionTransitionInputLocked())
	{
		return;
	}

	// Input can arrive before the actor tick has noticed this frame's contact.
	if ((IsFlying() || FlightState == EUTPFlightState::Falling) && IsOnWalkableGround())
	{
		BeginLanding();
	}

	// Ground contact permits another jump immediately, including during the
	// landing animation's stabilization time. Airborne flight still cannot jump.
	if (FlightState != EUTPFlightState::Perched &&
		!(FlightState == EUTPFlightState::Landing && IsOnWalkableGround()))
	{
		return;
	}

	RemainingLandingTime = 0.0f;
	FlightState = EUTPFlightState::Takeoff;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Flying);
		Movement->Velocity.Z = TakeoffSpeed;
	}

	FlightState = bIsInsideWindZone ? EUTPFlightState::WindRide : EUTPFlightState::Glide;
}

void AUTPFlyingAnimalCharacter::ActivateAnimalAbility()
{
	if (!IsSoulPossessed() || IsPossessionTransitionInputLocked() || !IsFlying())
	{
		return;
	}

	RemainingAbilityTime = AbilityGlideDuration;
	bIsAbilityActive = RemainingAbilityTime > 0.0f;
}

float AUTPFlyingAnimalCharacter::GetDesiredFlightVerticalVelocity() const
{
	const float SinkScale = bIsAbilityActive ? AbilitySinkMultiplier : 1.0f;
	// Include the current's world-space vertical velocity. Rotating a wind zone
	// upward must provide an updraft rather than only its additional lift.
	const float WindVerticalVelocity = CurrentWindDirection.Z * CurrentWindSpeed;
	return WindVerticalVelocity + CurrentLiftStrength
		- GlideSinkSpeed * SinkScale;
}

bool AUTPFlyingAnimalCharacter::IsOnWalkableGround() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || Movement->Velocity.Z > 0.0f)
	{
		return false;
	}

	// MOVE_Flying does not update CurrentFloor or automatically enter walking.
	// Any walkable floor must end the flight, including ordinary untagged ground.
	FFindFloorResult Floor;
	Movement->FindFloor(GetActorLocation(), Floor, false);
	return Floor.IsWalkableFloor() &&
		Floor.GetDistanceToFloor() <= UCharacterMovementComponent::MAX_FLOOR_DIST;
}

void AUTPFlyingAnimalCharacter::BeginLanding()
{
	if (FlightState == EUTPFlightState::Perched || FlightState == EUTPFlightState::Landing)
	{
		return;
	}

	FlightState = EUTPFlightState::Landing;
	RemainingLandingTime = LandingStabilizationTime;
	RemainingAbilityTime = 0.0f;
	bIsAbilityActive = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
		Movement->Velocity = FVector::ZeroVector;
	}
}

void AUTPFlyingAnimalCharacter::UpdateFlightState(float DeltaSeconds)
{
	if (FlightState == EUTPFlightState::Landing)
	{
		RemainingLandingTime = FMath::Max(0.0f, RemainingLandingTime - DeltaSeconds);
		if (RemainingLandingTime <= 0.0f && IsOnWalkableGround())
		{
			FlightState = EUTPFlightState::Perched;
		}
		return;
	}

	if ((FlightState == EUTPFlightState::WindRide || FlightState == EUTPFlightState::Glide ||
		FlightState == EUTPFlightState::Falling) &&
		IsOnWalkableGround())
	{
		BeginLanding();
		return;
	}

	if (FlightState == EUTPFlightState::Takeoff && bIsInsideWindZone)
	{
		FlightState = EUTPFlightState::WindRide;
	}
	else if (FlightState == EUTPFlightState::Takeoff && !bIsInsideWindZone)
	{
		FlightState = EUTPFlightState::Glide;
	}
	else if (FlightState == EUTPFlightState::WindRide && !bIsInsideWindZone)
	{
		BeginWindExitFall();
	}
}

void AUTPFlyingAnimalCharacter::HandleAnimalAbilityStarted()
{
	ActivateAnimalAbility();
}
