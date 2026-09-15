#include "UTPFrogCharacter.h"

#include "../../Animation/UTPFrogAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"

AUTPFrogCharacter::AUTPFrogCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 52.0f);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -52.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = GroundMoveSpeed;
		Movement->MaxAcceleration = 1100.0f;
		Movement->BrakingDecelerationWalking = 1000.0f;
		Movement->AirControl = FrogAirControl;
	}

	// An unpossessed frog remains where it was left for later puzzle use.
	bDisableMovementWhenUnpossessed = true;
	Tags.AddUnique(FName(TEXT("Frog")));
	AnimalAnimClass = UTPFrogAnimInstance::StaticClass();

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FrogMeshAsset(
		TEXT("/Game/_Art/QuirkyMinimal/Frog/Models/Frog_LOD0.Frog_LOD0"));
	if (FrogMeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(FrogMeshAsset.Object);
	}
}

void AUTPFrogCharacter::BeginPlay()
{
	Super::BeginPlay();

	FrogState = bIsInShallowWater
		? EUTPFrogState::ShallowWater
		: EUTPFrogState::Grounded;
	ResetJumpCharge();
	ApplyMoveSpeed();
}

void AUTPFrogCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FrogState == EUTPFrogState::ChargingJump)
	{
		UpdateJumpCharge();
	}

	if (FrogState == EUTPFrogState::Airborne)
	{
		if (const UCharacterMovementComponent* Movement = GetCharacterMovement();
			Movement && Movement->IsMovingOnGround())
		{
			FrogState = bIsInShallowWater
				? EUTPFrogState::ShallowWater
				: EUTPFrogState::Grounded;
		}
	}
}

void AUTPFrogCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(
				JumpAction,
				ETriggerEvent::Triggered,
				this,
				&AUTPFrogCharacter::UpdateJumpCharge);
		}
	}
}

void AUTPFrogCharacter::Move(const FInputActionValue& Value)
{
	if (!IsSoulPossessed() || FrogState == EUTPFrogState::ChargingJump)
	{
		return;
	}

	Super::Move(Value);
}

void AUTPFrogCharacter::StartJump()
{
	if (!IsSoulPossessed() || IsPossessionTransitionInputLocked())
	{
		return;
	}

	BeginJumpCharge();
}

void AUTPFrogCharacter::StopJump()
{
	if (!IsSoulPossessed() || IsPossessionTransitionInputLocked())
	{
		return;
	}

	ReleaseJump();
}

void AUTPFrogCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	FrogState = bIsInShallowWater
		? EUTPFrogState::ShallowWater
		: EUTPFrogState::Grounded;
	ResetJumpCharge();
}

void AUTPFrogCharacter::OnPossessedBySoul_Implementation(APawn* SoulPawn)
{
	Super::OnPossessedBySoul_Implementation(SoulPawn);

	FrogState = bIsInShallowWater
		? EUTPFrogState::ShallowWater
		: EUTPFrogState::Grounded;
	ResetJumpCharge();
	ApplyMoveSpeed();
}

void AUTPFrogCharacter::OnReleasedFromSoul_Implementation(APawn* SoulPawn)
{
	ResetJumpCharge();
	Super::OnReleasedFromSoul_Implementation(SoulPawn);

	FrogState = bIsInShallowWater
		? EUTPFrogState::ShallowWater
		: EUTPFrogState::Grounded;
}

bool AUTPFrogCharacter::CanReleaseFromSoul_Implementation() const
{
	if (!IsSoulPossessed())
	{
		return false;
	}

	return FrogState == EUTPFrogState::Grounded ||
		FrogState == EUTPFrogState::ShallowWater ||
		FrogState == EUTPFrogState::Landing;
}

void AUTPFrogCharacter::BeginJumpCharge()
{
	if (FrogState != EUTPFrogState::Grounded &&
		FrogState != EUTPFrogState::ShallowWater)
	{
		return;
	}

	JumpChargeStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	JumpChargeRatio = 0.0f;
	FrogState = EUTPFrogState::ChargingJump;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
}

void AUTPFrogCharacter::UpdateJumpCharge()
{
	if (IsPossessionTransitionInputLocked())
	{
		return;
	}

	if (FrogState != EUTPFrogState::ChargingJump || !GetWorld())
	{
		return;
	}

	const float Elapsed = GetWorld()->GetTimeSeconds() - JumpChargeStartTime;
	JumpChargeRatio = FMath::Clamp(Elapsed / MaxJumpChargeTime, 0.0f, 1.0f);
}

void AUTPFrogCharacter::ReleaseJump()
{
	if (IsPossessionTransitionInputLocked())
	{
		return;
	}

	if (FrogState != EUTPFrogState::ChargingJump)
	{
		return;
	}

	UpdateJumpCharge();

	const float JumpVelocity = FMath::Lerp(
		MinJumpVelocity,
		MaxJumpVelocity,
		JumpChargeRatio);

	FVector LaunchDirection = GetLastMovementInputVector();
	if (LaunchDirection.IsNearlyZero())
	{
		LaunchDirection = GetActorForwardVector();
	}

	LaunchDirection.Z = 0.0f;
	LaunchDirection = LaunchDirection.GetSafeNormal();

	LaunchCharacter(
		LaunchDirection * JumpHorizontalSpeed + FVector::UpVector * JumpVelocity,
		true,
		true);

	ResetJumpCharge();
	FrogState = EUTPFrogState::Airborne;
}

void AUTPFrogCharacter::EnterShallowWater()
{
	bIsInShallowWater = true;
	if (FrogState == EUTPFrogState::Grounded)
	{
		FrogState = EUTPFrogState::ShallowWater;
	}

	ApplyMoveSpeed();
}

void AUTPFrogCharacter::ExitShallowWater()
{
	bIsInShallowWater = false;
	if (FrogState == EUTPFrogState::ShallowWater)
	{
		FrogState = EUTPFrogState::Grounded;
	}

	ApplyMoveSpeed();
}

void AUTPFrogCharacter::SetShallowWaterMoveSpeed(float NewMoveSpeed)
{
	ShallowWaterMoveSpeedOverride = FMath::Max(0.0f, NewMoveSpeed);
	ApplyMoveSpeed();
}

void AUTPFrogCharacter::ResetShallowWaterMoveSpeed()
{
	ShallowWaterMoveSpeedOverride = -1.0f;
	ApplyMoveSpeed();
}

void AUTPFrogCharacter::EnterShallowWaterFromZone(AActor* WaterSource, float NewMoveSpeed)
{
	if (!IsValid(WaterSource))
	{
		return;
	}

	ActiveWaterSources.FindOrAdd(WaterSource) = FMath::Max(0.0f, NewMoveSpeed);
	RebuildWaterState();
}

void AUTPFrogCharacter::ExitShallowWaterFromZone(AActor* WaterSource)
{
	ActiveWaterSources.Remove(WaterSource);
	RebuildWaterState();
}

void AUTPFrogCharacter::ResetJumpCharge()
{
	JumpChargeRatio = 0.0f;
	JumpChargeStartTime = 0.0f;
}

void AUTPFrogCharacter::ApplyMoveSpeed()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		const float ActiveWaterSpeed = ShallowWaterMoveSpeedOverride >= 0.0f
			? ShallowWaterMoveSpeedOverride
			: WaterMoveSpeed;
		Movement->MaxWalkSpeed = bIsInShallowWater ? ActiveWaterSpeed : GroundMoveSpeed;
		Movement->AirControl = FrogAirControl;
	}
}

void AUTPFrogCharacter::RebuildWaterState()
{
	for (auto It = ActiveWaterSources.CreateIterator(); It; ++It)
	{
		if (!IsValid(It.Key().Get()))
		{
			It.RemoveCurrent();
		}
	}

	if (ActiveWaterSources.IsEmpty())
	{
		bIsInShallowWater = false;
		ShallowWaterMoveSpeedOverride = -1.0f;
		if (FrogState == EUTPFrogState::ShallowWater)
		{
			FrogState = EUTPFrogState::Grounded;
		}
		ApplyMoveSpeed();
		return;
	}

	bIsInShallowWater = true;
	float SlowestWaterSpeed = TNumericLimits<float>::Max();
	for (const TPair<TWeakObjectPtr<AActor>, float>& Entry : ActiveWaterSources)
	{
		SlowestWaterSpeed = FMath::Min(SlowestWaterSpeed, Entry.Value);
	}

	ShallowWaterMoveSpeedOverride = SlowestWaterSpeed;
	if (FrogState == EUTPFrogState::Grounded)
	{
		FrogState = EUTPFrogState::ShallowWater;
	}
	ApplyMoveSpeed();
}
