#include "UTPPossessableCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/UTPPossessionComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "InputAction.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"
#include "UTPPlayerController.h"

AUTPPossessableCharacter::AUTPPossessableCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	// Possession targeting uses the Visibility channel. Make every derived animal
	// reliably discoverable regardless of its Blueprint collision preset.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	// The shared third-person camera is owned by a separate camera actor, so its
	// spring arm cannot automatically ignore the possessed Character. Never let
	// an animal's capsule retract that arm into a first-person viewpoint.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetGenerateOverlapEvents(false);

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		Movement->MaxWalkSpeed = 350.0f;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionAsset(
		TEXT("/Game/Input/InputActions/IA_Move.IA_Move"));
	if (MoveActionAsset.Succeeded())
	{
		MoveAction = MoveActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionAsset(
		TEXT("/Game/Input/InputActions/IA_Jump.IA_Jump"));
	if (JumpActionAsset.Succeeded())
	{
		JumpAction = JumpActionAsset.Object;
	}
}

void AUTPPossessableCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Apply this at runtime as well: child Blueprints may have saved an older
	// capsule collision preset that overrides the native constructor default.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	ApplyAnimInstanceClass();
	UpdateFocusVisuals(false);
	ApplyReleasedMovementState();
}

void AUTPPossessableCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ApplyAnimInstanceClass();
}

void AUTPPossessableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered,
			this, &AUTPPossessableCharacter::Move);
	}

	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started,
			this, &AUTPPossessableCharacter::StartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed,
			this, &AUTPPossessableCharacter::StopJump);
	}
}

void AUTPPossessableCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementValue = Value.Get<FVector2D>();
	if (!Controller || MovementValue.IsNearlyZero() || IsPossessionTransitionInputLocked())
	{
		return;
	}

	const FRotator ControlRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementValue.Y);
	AddMovementInput(RightDirection, MovementValue.X);
}

void AUTPPossessableCharacter::StartJump()
{
	if (IsPossessionTransitionInputLocked())
	{
		return;
	}

	Jump();
}

void AUTPPossessableCharacter::StopJump()
{
	if (IsPossessionTransitionInputLocked())
	{
		return;
	}

	StopJumping();
}

bool AUTPPossessableCharacter::CanBePossessed_Implementation(AController* InstigatingController) const
{
	const AController* CurrentController = GetController();
	if (CurrentController && CurrentController == InstigatingController)
	{
		return false;
	}

	return bAllowSoulPossession && !bIsSoulPossessed && !IsActorBeingDestroyed() && !IsPendingKillPending();
}

void AUTPPossessableCharacter::OnPossessionFocusChanged_Implementation(bool bIsFocused)
{
	UpdateFocusVisuals(bIsFocused);
}

void AUTPPossessableCharacter::OnPossessedBySoul_Implementation(APawn* SoulPawn)
{
	CurrentSoulPawn = SoulPawn;
	bIsSoulPossessed = true;

	ApplyPossessedMovementState();
}

void AUTPPossessableCharacter::OnReleasedFromSoul_Implementation(APawn* SoulPawn)
{
	if (CurrentSoulPawn == SoulPawn)
	{
		CurrentSoulPawn = nullptr;
	}

	bIsSoulPossessed = false;
	ApplyReleasedMovementState();
}

bool AUTPPossessableCharacter::CanReleaseFromSoul_Implementation() const
{
	return true;
}

bool AUTPPossessableCharacter::IsSoulPossessed() const
{
	return bIsSoulPossessed;
}

bool AUTPPossessableCharacter::IsCurrentlyPossessed() const
{
	return IsSoulPossessed();
}

APawn* AUTPPossessableCharacter::GetCurrentSoulPawn() const
{
	return CurrentSoulPawn.Get();
}

bool AUTPPossessableCharacter::IsPossessionTransitionInputLocked() const
{
	const AUTPPlayerController* PlayerController = Cast<AUTPPlayerController>(GetController());
	const UTPPossessionComponent* PossessionComponent = PlayerController
		? PlayerController->GetPossessionComponent()
		: nullptr;
	return PossessionComponent && PossessionComponent->IsPossessionTransitionInProgress();
}

void AUTPPossessableCharacter::UpdateFocusVisuals(bool bIsFocused)
{
	if (!bUseCustomDepthFocus || !GetMesh())
	{
		return;
	}

	GetMesh()->SetRenderCustomDepth(bIsFocused);
	GetMesh()->SetCustomDepthStencilValue(bIsFocused ? FocusCustomDepthStencilValue : 0);
}

void AUTPPossessableCharacter::ApplyPossessedMovementState()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (!bHasCachedMovementMode)
		{
			CachedMovementMode = Movement->MovementMode;
			CachedCustomMovementMode = Movement->CustomMovementMode;
			bHasCachedMovementMode = true;
		}

		Movement->SetMovementMode(MOVE_Walking);
		// Match the soul: movement input is projected onto the camera/control
		// direction, then CharacterMovement turns the animal toward its actual
		// travel direction.
		Movement->bOrientRotationToMovement = true;
	}

	bUseControllerRotationYaw = false;
}

void AUTPPossessableCharacter::ApplyReleasedMovementState()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (bDisableMovementWhenUnpossessed)
		{
			Movement->DisableMovement();
		}
		else if (bHasCachedMovementMode)
		{
			Movement->SetMovementMode(CachedMovementMode, CachedCustomMovementMode);
		}
		else
		{
			Movement->SetMovementMode(MOVE_Walking);
		}

		Movement->bOrientRotationToMovement = true;
	}

	bUseControllerRotationYaw = false;
	bHasCachedMovementMode = false;
}

void AUTPPossessableCharacter::ApplyAnimInstanceClass()
{
	if (GetMesh() && AnimalAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(AnimalAnimClass);
	}
}
