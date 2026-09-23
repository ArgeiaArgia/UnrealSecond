#include "UTPPlayerController.h"

#include "Camera/UTPSharedCameraRig.h"
#include "Components/UTPPossessionComponent.h"
#include "UTPSoulPawn.h"

#include "Camera/PlayerCameraManager.h"
#include "CollisionShape.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputModifiers.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UTPPossessableInterface.h"
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

	// Start from an overhead angle and keep vertical look within this range.
	SetControlRotation(FRotator(-60.0f, GetControlRotation().Yaw, 0.0f));
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -80.0f;
		PlayerCameraManager->ViewPitchMax = -35.0f;
	}

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

void AUTPPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdatePossessionAim(DeltaTime);
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
		EnhancedInputComponent->BindAction(PossessTargetAction, ETriggerEvent::Completed,
			this, &AUTPPlayerController::HandlePossessionReleased);
		EnhancedInputComponent->BindAction(PossessTargetAction, ETriggerEvent::Canceled,
			this, &AUTPPlayerController::HandlePossessionReleased);
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

	APawn* CurrentPawn = GetPawn();
	if (CurrentPawn && (Cast<AUTPSoulPawn>(CurrentPawn) ||
		CurrentPawn->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass())))
	{
		// Hold right mouse to keep the camera trained on the currently valid,
		// camera-facing target. Possession itself only happens on release.
		bPossessionAimActive = true;
		PossessionAimTarget = FindPossessionAimTarget();
		UpdatePossessionAim(0.0f);
		return;
	}

	TogglePossession();
}

void AUTPPlayerController::HandlePossessionReleased()
{
	if (!bPossessionAimActive)
	{
		return;
	}

	bPossessionAimActive = false;
	PossessionAimTarget = nullptr;

	// Re-query at release so the target must still be visible and inside the
	// configured possession range. With no target, retain the prior behavior of
	// returning the currently controlled animal to Soul form.
	if (AActor* TargetActor = FindPossessionAimTarget())
	{
		TryPossessTarget(TargetActor);
		return;
	}

	if (!Cast<AUTPSoulPawn>(GetPawn()))
	{
		TogglePossession();
	}
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

void AUTPPlayerController::UpdatePossessionAim(float DeltaTime)
{
	if (!bPossessionAimActive)
	{
		return;
	}

	APawn* CurrentPawn = GetPawn();
	const bool bCanAimFromCurrentPawn = CurrentPawn &&
		(Cast<AUTPSoulPawn>(CurrentPawn) ||
			CurrentPawn->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()));
	if (!bCanAimFromCurrentPawn ||
		(PossessionComponent && PossessionComponent->IsPossessionTransitionInProgress()))
	{
		bPossessionAimActive = false;
		PossessionAimTarget = nullptr;
		return;
	}

	// Acquire one target per hold. Continuously replacing it while the control
	// rotation moves causes the camera to wobble between nearby characters.
	// A destroyed target can still be replaced on the next frame.
	if (!PossessionAimTarget.IsValid())
	{
		PossessionAimTarget = FindPossessionAimTarget();
	}
	AActor* TargetActor = PossessionAimTarget.Get();
	if (!IsValid(TargetActor))
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector TargetLocation = TargetActor->GetComponentsBoundingBox(true).GetCenter();
	if (TargetLocation.Equals(ViewLocation))
	{
		return;
	}

	FRotator DesiredRotation = GetControlRotation();
	// This is an overhead camera. Retaining its pitch prevents the view from
	// dipping or rising abruptly just because targets differ in height; only
	// rotate around the player so the locked character stays in front.
	DesiredRotation.Yaw = (TargetLocation - ViewLocation).Rotation().Yaw;
	DesiredRotation.Roll = 0.0f;

	const FRotator NewRotation = FMath::RInterpTo(
		GetControlRotation(),
		DesiredRotation,
		DeltaTime,
		PossessionAimRotationInterpSpeed);
	SetControlRotation(NewRotation);
}

AActor* AUTPPlayerController::FindPossessionAimTarget() const
{
	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn)
	{
		return nullptr;
	}

	if (const AUTPSoulPawn* SoulPawn = Cast<AUTPSoulPawn>(CurrentPawn))
	{
		return SoulPawn->GetFocusedPossessableTarget();
	}

	UWorld* World = GetWorld();
	if (!World || !CurrentPawn->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = CurrentPawn->GetComponentsBoundingBox(true).GetCenter();
	const FVector TraceEnd = TraceStart + ViewRotation.Vector() * PossessionAimTraceDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BodyPossessionTrace), false, CurrentPawn);
	TArray<FHitResult> HitResults;
	World->SweepMultiByObjectType(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(PossessionAimTraceRadius),
		QueryParams);

	for (const FHitResult& Hit : HitResults)
	{
		if (Hit.bStartPenetrating)
		{
			continue;
		}

		AActor* Candidate = Hit.GetActor();
		if (!Candidate || Candidate == CurrentPawn ||
			!Candidate->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()) ||
			!ITPPossessableInterface::Execute_CanBePossessed(Candidate, const_cast<AUTPPlayerController*>(this)))
		{
			continue;
		}

		// The object sweep ignores the floor; this thin Visibility ray retains
		// normal wall occlusion before allowing a direct body-to-body transfer.
		FHitResult SightHit;
		const bool bBlocked = World->LineTraceSingleByChannel(
			SightHit,
			TraceStart,
			Hit.ImpactPoint,
			ECC_Visibility,
			QueryParams);
		if (!bBlocked || SightHit.GetActor() == Candidate)
		{
			return Candidate;
		}
	}

	return nullptr;
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

	if (PossessTargetAction)
	{
		// Do not depend on the authored mapping asset: this runtime context makes
		// right mouse the sole possession control and removes the old keyboard key.
		RuntimeMappingContext->UnmapAllKeysFromAction(PossessTargetAction);
		RuntimeMappingContext->MapKey(PossessTargetAction, EKeys::RightMouseButton);
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
