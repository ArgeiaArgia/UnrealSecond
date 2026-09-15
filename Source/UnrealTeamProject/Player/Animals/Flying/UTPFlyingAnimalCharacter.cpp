#include "UTPFlyingAnimalCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"

AUTPFlyingAnimalCharacter::AUTPFlyingAnimalCharacter()
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
		Movement->MaxAcceleration = 900.0f;
	}

	// A released flying animal must remain where it landed.
	bDisableMovementWhenUnpossessed = true;

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
	LateralInput = 0.0f;
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

	if (FlightState == EUTPFlightState::Perched || FlightState == EUTPFlightState::Landing)
	{
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->Velocity = FVector::ZeroVector;
		}
		return;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Falling);
		Movement->Velocity = FMath::VInterpTo(
			Movement->Velocity,
			GetDesiredFlightVelocity(),
			DeltaSeconds,
			WindAcceleration);
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
	if (!IsSoulPossessed() || IsPossessionTransitionInputLocked() ||
		FlightState == EUTPFlightState::Perched || FlightState == EUTPFlightState::Landing)
	{
		LateralInput = 0.0f;
		return;
	}

	const FVector2D Input = Value.Get<FVector2D>();
	// Only the X axis is used. Forward/backward input cannot fight the wind.
	LateralInput = FMath::Clamp(Input.X, -1.0f, 1.0f);
}

void AUTPFlyingAnimalCharacter::StartJump()
{
	StartTakeoff();
}

void AUTPFlyingAnimalCharacter::OnPossessedBySoul_Implementation(APawn* SoulPawn)
{
	Super::OnPossessedBySoul_Implementation(SoulPawn);

	LateralInput = 0.0f;
	RemainingLandingTime = 0.0f;
	if (FlightState == EUTPFlightState::Landing)
	{
		FlightState = EUTPFlightState::Perched;
	}

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
	Super::OnReleasedFromSoul_Implementation(SoulPawn);

	FlightState = EUTPFlightState::Perched;
	LateralInput = 0.0f;
	RemainingLandingTime = 0.0f;
	RemainingAbilityTime = 0.0f;
	bIsAbilityActive = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->Velocity = FVector::ZeroVector;
	}
}

bool AUTPFlyingAnimalCharacter::CanReleaseFromSoul_Implementation() const
{
	return FlightState == EUTPFlightState::Perched || FlightState == EUTPFlightState::Landing;
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
			FlightState = EUTPFlightState::Glide;
		}
		return;
	}

	CurrentWindDirection = SelectedSource->Direction;
	CurrentWindSpeed = SelectedSource->Speed;
	CurrentLiftStrength = SelectedSource->Lift;
	bIsInsideWindZone = true;

	if (FlightState == EUTPFlightState::Glide)
	{
		FlightState = EUTPFlightState::WindRide;
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
	if (!IsSoulPossessed() || IsPossessionTransitionInputLocked() || FlightState != EUTPFlightState::Perched)
	{
		return;
	}

	FlightState = EUTPFlightState::Takeoff;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Falling);
	}

	LaunchCharacter(
		GetActorForwardVector() * TakeoffSpeed + FVector::UpVector * TakeoffSpeed,
		true,
		true);

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

FVector AUTPFlyingAnimalCharacter::GetDesiredFlightVelocity() const
{
	if (bIsInsideWindZone && !CurrentWindDirection.IsNearlyZero())
	{
		const FVector WindDirection = CurrentWindDirection.GetSafeNormal();
		FVector LateralDirection = FVector::CrossProduct(FVector::UpVector, WindDirection).GetSafeNormal();
		if (LateralDirection.IsNearlyZero())
		{
			LateralDirection = GetActorRightVector().GetSafeNormal();
		}

		const float SinkScale = bIsAbilityActive ? AbilitySinkMultiplier : 1.0f;
		return WindDirection * (CurrentWindSpeed > 0.0f ? CurrentWindSpeed : WindRideSpeed) +
			LateralDirection * LateralInput * MaxLateralSpeed +
			FVector::UpVector * CurrentLiftStrength -
			FVector::UpVector * GlideSinkSpeed * SinkScale;
	}

	const FVector CurrentHorizontalVelocity = FVector(GetVelocity().X, GetVelocity().Y, 0.0f);
	FVector LateralDirection = FVector::CrossProduct(FVector::UpVector, GetActorForwardVector()).GetSafeNormal();
	if (LateralDirection.IsNearlyZero())
	{
		LateralDirection = GetActorRightVector().GetSafeNormal();
	}

	const float SinkScale = bIsAbilityActive ? AbilitySinkMultiplier : 1.0f;
	return CurrentHorizontalVelocity +
		LateralDirection * LateralInput * MaxLateralSpeed -
		FVector::UpVector * GlideSinkSpeed * SinkScale;
}

bool AUTPFlyingAnimalCharacter::IsOnValidLandingSurface() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || !Movement->IsMovingOnGround())
	{
		return false;
	}

	if (!bRequireLandingTag)
	{
		return true;
	}

	const AActor* FloorActor = Movement->CurrentFloor.HitResult.GetActor();
	return IsValid(FloorActor) && FloorActor->ActorHasTag(LandingPlatformTag);
}

void AUTPFlyingAnimalCharacter::BeginLanding()
{
	if (FlightState == EUTPFlightState::Perched || FlightState == EUTPFlightState::Landing)
	{
		return;
	}

	FlightState = EUTPFlightState::Landing;
	RemainingLandingTime = LandingStabilizationTime;
	LateralInput = 0.0f;

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
		if (RemainingLandingTime <= 0.0f && IsOnValidLandingSurface())
		{
			FlightState = EUTPFlightState::Perched;
		}
		return;
	}

	if ((FlightState == EUTPFlightState::WindRide || FlightState == EUTPFlightState::Glide) &&
		IsOnValidLandingSurface())
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
		FlightState = EUTPFlightState::Glide;
	}
}

void AUTPFlyingAnimalCharacter::HandleAnimalAbilityStarted()
{
	ActivateAnimalAbility();
}
