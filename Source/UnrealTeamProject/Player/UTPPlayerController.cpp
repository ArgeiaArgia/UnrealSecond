#include "UTPPlayerController.h"

#include "Camera/UTPSharedCameraRig.h"
#include "Components/UTPPossessionComponent.h"
#include "UTPSoulPawn.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputModifiers.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/ConstructorHelpers.h"

AUTPPlayerController::AUTPPlayerController()
{
	PossessionComponent = CreateDefaultSubobject<UTPPossessionComponent>(TEXT("PossessionComponent"));
	bAutoManageActiveCameraTarget = false;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextAsset(
		TEXT("/Game/Input/IMC_Player.IMC_Player"));
	if (MappingContextAsset.Succeeded())
	{
		DefaultMappingContext = MappingContextAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> PossessTargetActionAsset(
		TEXT("/Game/Input/InputActions/IA_PossessTarget.IA_PossessTarget"));
	if (PossessTargetActionAsset.Succeeded())
	{
		PossessTargetAction = PossessTargetActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionAsset(
		TEXT("/Game/Input/InputActions/IA_Look.IA_Look"));
	if (LookActionAsset.Succeeded())
	{
		LookAction = LookActionAsset.Object;
	}
}

void AUTPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		SharedCameraRig = World->SpawnActor<AUTPSharedCameraRig>(AUTPSharedCameraRig::StaticClass());
		if (SharedCameraRig)
		{
			SharedCameraRig->SetOwner(this);
			SharedCameraRig->Initialize(this);
			UpdateSharedCameraTarget(GetPawn(), true);
		}
	}

	if (PossessionComponent)
	{
		PossessionComponent->SetSoulPawn(Cast<APawn>(GetPawn()));
	}

	CreateRuntimeInputMapping();

	if (RuntimeMappingContext)
	{
		if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->AddMappingContext(RuntimeMappingContext, 0);
			}
		}
	}
}

void AUTPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (PossessTargetAction)
	{
		EnhancedInputComponent->BindAction(PossessTargetAction, ETriggerEvent::Started,
			this, &AUTPPlayerController::HandlePossessionPressed);
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered,
			this, &AUTPPlayerController::HandleLook);
	}
}

void AUTPPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	UpdateSharedCameraTarget(InPawn);

	if (PossessionComponent)
	{
		PossessionComponent->HandleControllerPossessed(InPawn);
	}
}

void AUTPPlayerController::OnUnPossess()
{
	APawn* PreviousPawn = GetPawn();
	Super::OnUnPossess();

	if (PossessionComponent)
	{
		PossessionComponent->HandleControllerUnpossessed(PreviousPawn);
	}
}

bool AUTPPlayerController::TryPossessTarget(AActor* TargetActor)
{
	return PossessionComponent ? PossessionComponent->TryPossessTarget(TargetActor) : false;
}

bool AUTPPlayerController::TogglePossession()
{
	return PossessionComponent ? PossessionComponent->TogglePossession() : false;
}

void AUTPPlayerController::BeginPossessionCameraTransition(APawn* TargetPawn, float Duration)
{
	if (SharedCameraRig)
	{
		SharedCameraRig->BeginPossessionTransition(TargetPawn, Duration);
	}
}

void AUTPPlayerController::CancelPossessionCameraTransition()
{
	if (SharedCameraRig)
	{
		SharedCameraRig->CancelPossessionTransition();
	}
}

bool AUTPPlayerController::IsPossessionCameraTransitionActive() const
{
	return SharedCameraRig && SharedCameraRig->IsPossessionTransitionActive();
}

void AUTPPlayerController::PreserveSharedCameraAngle()
{
	if (SharedCameraRig)
	{
		SetControlRotation(SharedCameraRig->GetCameraRigRotation());
	}
}

UTPPossessionComponent* AUTPPlayerController::GetPossessionComponent() const
{
	return PossessionComponent;
}

void AUTPPlayerController::HandlePossessionPressed()
{
	if (PossessionComponent && PossessionComponent->IsPossessionTransitionInProgress())
	{
		return;
	}

	if (AUTPSoulPawn* SoulPawn = Cast<AUTPSoulPawn>(GetPawn()))
	{
		SoulPawn->RequestPossessFocusedTarget();
		return;
	}

	TogglePossession();
}

void AUTPPlayerController::HandleLook(const FInputActionValue& Value)
{
	if (PossessionComponent && PossessionComponent->IsPossessionTransitionInProgress())
	{
		return;
	}

	const FVector2D LookValue = Value.Get<FVector2D>();
	if (LookValue.IsNearlyZero())
	{
		return;
	}

	AddYawInput(LookValue.X);
	AddPitchInput(LookValue.Y);
}

void AUTPPlayerController::CreateRuntimeInputMapping()
{
	if (!DefaultMappingContext)
	{
		return;
	}

	RuntimeMappingContext = DuplicateObject<UInputMappingContext>(DefaultMappingContext, this);
	if (!RuntimeMappingContext || !LookAction)
	{
		return;
	}

	// A 1D MouseY value must be swizzled into the Y component of IA_Look (Axis2D).
	// Otherwise both mouse directions feed yaw and pitch never receives an input.
	RuntimeMappingContext->UnmapKey(LookAction, EKeys::MouseX);
	RuntimeMappingContext->UnmapKey(LookAction, EKeys::MouseY);
	RuntimeMappingContext->MapKey(LookAction, EKeys::MouseX);

	FEnhancedActionKeyMapping& MouseYMapping = RuntimeMappingContext->MapKey(LookAction, EKeys::MouseY);
	MouseYMapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(RuntimeMappingContext));
}

void AUTPPlayerController::UpdateSharedCameraTarget(APawn* InPawn, bool bSnapToTarget)
{
	if (!SharedCameraRig)
	{
		return;
	}

	SharedCameraRig->SetFollowTarget(InPawn, bSnapToTarget);

	// Possessing a Character can make the engine fall back to that Pawn's
	// default camera viewpoint. Always restore the shared rig immediately so
	// possessed animals use the same third-person, control-rotation camera as
	// the soul.
	SetViewTargetWithBlend(SharedCameraRig, 0.0f);
}
