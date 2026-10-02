#include "UTPPlayerController.h"

#include "Camera/UTPSharedCameraRig.h"
#include "Components/UTPPossessionComponent.h"
#include "UI/UTPPossessionProgressWidget.h"
#include "UI/UTPTutorialWidget.h"
#include "UTPSoulPawn.h"

#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputModifiers.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "UTPPossessableInterface.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/ConstructorHelpers.h"

AUTPPlayerController::AUTPPlayerController()
{
	PossessionComponent = CreateDefaultSubobject<UTPPossessionComponent>(TEXT("PossessionComponent"));
	bAutoManageActiveCameraTarget = false;
	InitialTutorialMessage = NSLOCTEXT("Tutorial", "InitialMovementPrompt", "WASD로 움직여 땅으로 올라가세요.");

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
	// A completed or interrupted possession cinematic must never leave a
	// residual look-input lock on the normal exploration camera.
	ResetIgnoreLookInput();

	// Start from an overhead angle and keep vertical look within this range.
	SetControlRotation(FRotator(-60.0f, GetControlRotation().Yaw, 0.0f));
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -85.0f;
		PlayerCameraManager->ViewPitchMax = 20.0f;
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
	CreatePossessionProgressWidget();
	CreateTutorialWidget();
	if (!InitialTutorialMessage.IsEmpty())
	{
		ShowTutorialMessage(InitialTutorialMessage);
	}

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
	UpdatePossessionFocus();
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
	SetPossessionFocusTarget(nullptr);
	SetPossessionAimZoomEnabled(false);
	HidePossessionProgressWidget();
	UpdateSharedCameraTarget(InPawn);

	if (PossessionComponent)
	{
		PossessionComponent->HandleControllerPossessed(InPawn);
	}
}

void AUTPPlayerController::OnUnPossess()
{
	APawn* PreviousPawn = GetPawn();
	SetPossessionFocusTarget(nullptr);
	SetPossessionAimZoomEnabled(false);
	HidePossessionProgressWidget();
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

void AUTPPlayerController::ShowTutorialMessage(const FText& Message)
{
	CreateTutorialWidget();
	if (TutorialWidget)
	{
		TutorialWidget->ShowMessage(Message);
	}
}

void AUTPPlayerController::HideTutorialMessage()
{
	if (TutorialWidget)
	{
		TutorialWidget->HideMessage();
	}
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
		// on-screen target. Possession completes after the configured duration.
		bPossessionAimActive = true;
		PossessionAimHoldElapsed = 0.0f;
		PossessionAimTarget = PossessionFocusTarget.IsValid()
			? PossessionFocusTarget.Get()
			: FindPossessionAimTarget();
		if (PossessionAimTarget.IsValid())
		{
			SetPossessionFocusTarget(PossessionAimTarget.Get());
		}
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
	PossessionAimHoldElapsed = 0.0f;
	SetPossessionAimZoomEnabled(false);
	HidePossessionProgressWidget();
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

	// During a possession focus, UpdatePossessionAim owns the control rotation
	// so the target remains fixed. Outside focus, set it directly rather than
	// going through AddYawInput/AddPitchInput: those inputs can remain ignored
	// by another controller input lock even after normal play has resumed.
	if (bPossessionAimActive)
	{
		return;
	}

	FRotator NewControlRotation = GetControlRotation().GetNormalized();
	NewControlRotation.Yaw = FRotator::NormalizeAxis(NewControlRotation.Yaw + LookValue.X);
	const float MinimumPitch = PlayerCameraManager ? PlayerCameraManager->ViewPitchMin : -85.0f;
	const float MaximumPitch = PlayerCameraManager ? PlayerCameraManager->ViewPitchMax : 20.0f;
	NewControlRotation.Pitch = FMath::Clamp(
		NewControlRotation.Pitch + LookValue.Y,
		MinimumPitch,
		MaximumPitch);
	NewControlRotation.Roll = 0.0f;
	SetControlRotation(NewControlRotation);
}

void AUTPPlayerController::UpdatePossessionFocus()
{
	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn || Cast<AUTPSoulPawn>(CurrentPawn))
	{
		// The Soul owns its focus state and its focus visuals.
		SetPossessionFocusTarget(nullptr);
		return;
	}

	if (!CurrentPawn->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()) ||
		(PossessionComponent && PossessionComponent->IsPossessionTransitionInProgress()) ||
		bPossessionAimActive)
	{
		if (!bPossessionAimActive)
		{
			SetPossessionFocusTarget(nullptr);
		}
		return;
	}

	AActor* NewTarget = FindCameraPossessionTarget(CurrentPawn, PossessionAimTraceDistance);
	if (bDrawCharacterPossessionTrace)
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		GetPlayerViewPoint(ViewLocation, ViewRotation);

		const FVector DebugEnd = NewTarget
			? NewTarget->GetComponentsBoundingBox(true).GetCenter()
			: ViewLocation + ViewRotation.Vector() * PossessionAimTraceDistance;
		const FColor DebugColor = NewTarget ? FColor::Green : FColor::Cyan;
		DrawDebugLine(GetWorld(), ViewLocation, DebugEnd, DebugColor, false, 0.0f, 0, 1.5f);
		if (NewTarget)
		{
			DrawDebugSphere(GetWorld(), DebugEnd, 18.0f, 12, DebugColor, false, 0.0f, 0, 1.5f);
		}
	}

	SetPossessionFocusTarget(NewTarget);
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
		PossessionAimHoldElapsed = 0.0f;
		SetPossessionAimZoomEnabled(false);
		HidePossessionProgressWidget();
		return;
	}

	// Lock the initial target for the duration of the hold. Re-selecting every
	// frame while the camera rotates can alternate between nearby candidates and
	// make the camera visibly shake.
	if (!PossessionAimTarget.IsValid())
	{
		PossessionAimTarget = PossessionFocusTarget.IsValid()
			? PossessionFocusTarget.Get()
			: FindPossessionAimTarget();
		PossessionAimHoldElapsed = 0.0f;
	}

	AActor* TargetActor = PossessionAimTarget.Get();
	if (!IsValid(TargetActor))
	{
		PossessionAimHoldElapsed = 0.0f;
		SetPossessionAimZoomEnabled(false);
		HidePossessionProgressWidget();
		return;
	}
	SetPossessionAimZoomEnabled(true);

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector TargetLocation = TargetActor->GetComponentsBoundingBox(true).GetCenter();
	if (TargetLocation.Equals(ViewLocation))
	{
		return;
	}

	FRotator DesiredRotation = (TargetLocation - ViewLocation).Rotation();
	DesiredRotation.Roll = 0.0f;
	SetControlRotation(FMath::RInterpTo(
		GetControlRotation(),
		DesiredRotation,
		DeltaTime,
		PossessionAimRotationInterpSpeed));

	PossessionAimHoldElapsed += DeltaTime;
	const float HoldProgress = PossessionAimHoldDuration > KINDA_SMALL_NUMBER
		? PossessionAimHoldElapsed / PossessionAimHoldDuration
		: 1.0f;
	UpdatePossessionProgressWidget(TargetActor, HoldProgress);
	if (PossessionAimHoldElapsed >= PossessionAimHoldDuration)
	{
		bPossessionAimActive = false;
		PossessionAimTarget = nullptr;
		PossessionAimHoldElapsed = 0.0f;
		HidePossessionProgressWidget();

		SetPossessionAimZoomEnabled(false);
		TryPossessTarget(TargetActor);
	}
}

void AUTPPlayerController::CreatePossessionProgressWidget()
{
	if (!IsLocalController() || PossessionProgressWidget)
	{
		return;
	}

	PossessionProgressWidget = CreateWidget<UUTPPossessionProgressWidget>(
		this,
		UUTPPossessionProgressWidget::StaticClass());
	if (PossessionProgressWidget)
	{
		// The overlay only renders feedback; it must never consume the right-click
		// that drives the possession hold.
		PossessionProgressWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		PossessionProgressWidget->AddToViewport();
	}
}

void AUTPPlayerController::CreateTutorialWidget()
{
	if (!IsLocalController() || TutorialWidget)
	{
		return;
	}

	TutorialWidget = CreateWidget<UUTPTutorialWidget>(this, UUTPTutorialWidget::StaticClass());
	if (TutorialWidget)
	{
		TutorialWidget->SetVisibility(ESlateVisibility::Collapsed);
		TutorialWidget->AddToViewport();
	}
}

void AUTPPlayerController::UpdatePossessionProgressWidget(AActor* TargetActor, float Progress)
{
	if (!PossessionProgressWidget || !IsValid(TargetActor))
	{
		return;
	}

	const FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
	const FVector TargetCenter = TargetBounds.GetCenter();
	FVector2D ScreenCenter;
	if (!ProjectWorldLocationToScreen(TargetCenter, ScreenCenter, false))
	{
		HidePossessionProgressWidget();
		return;
	}

	const float WorldRadius = FMath::Max(TargetBounds.GetExtent().Z, 50.0f);
	FVector2D ScreenEdge;
	float RingRadius = 72.0f;
	if (ProjectWorldLocationToScreen(TargetCenter + FVector(0.0f, 0.0f, WorldRadius), ScreenEdge, false))
	{
		RingRadius = FMath::Clamp(FVector2D::Distance(ScreenCenter, ScreenEdge) + 24.0f, 56.0f, 160.0f);
	}

	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	PossessionProgressWidget->ShowProgress(ScreenCenter / ViewportScale, RingRadius / ViewportScale, Progress);
}

void AUTPPlayerController::HidePossessionProgressWidget()
{
	if (PossessionProgressWidget)
	{
		PossessionProgressWidget->HideProgress();
	}
}

void AUTPPlayerController::SetPossessionAimZoomEnabled(bool bEnabled)
{
	if (SharedCameraRig)
	{
		SharedCameraRig->SetPossessionAimZoomEnabled(bEnabled);
	}
}

AActor* AUTPPlayerController::FindPossessionAimTarget() const
{
	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn)
	{
		return nullptr;
	}

	// Soul focus is refreshed every frame using the same camera test, and it
	// retains the Soul-specific maximum distance configured by the designer.
	if (const AUTPSoulPawn* SoulPawn = Cast<AUTPSoulPawn>(CurrentPawn))
	{
		return SoulPawn->GetFocusedPossessableTarget();
	}

	return FindCameraPossessionTarget(CurrentPawn, PossessionAimTraceDistance);
}

void AUTPPlayerController::SetPossessionFocusTarget(AActor* NewTarget)
{
	if (PossessionFocusTarget.Get() == NewTarget)
	{
		return;
	}

	if (AActor* PreviousTarget = PossessionFocusTarget.Get())
	{
		if (PreviousTarget->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
		{
			ITPPossessableInterface::Execute_OnPossessionFocusChanged(PreviousTarget, false);
		}
	}

	PossessionFocusTarget = nullptr;
	if (IsValid(NewTarget) && NewTarget->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()) &&
		ITPPossessableInterface::Execute_CanBePossessed(NewTarget, this))
	{
		PossessionFocusTarget = NewTarget;
		ITPPossessableInterface::Execute_OnPossessionFocusChanged(NewTarget, true);
	}
}

AActor* AUTPPlayerController::FindCameraPossessionTarget(const APawn* SearchOriginPawn, float MaxDistance) const
{
	if (!SearchOriginPawn || MaxDistance <= 0.0f)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return nullptr;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CameraPossessionTrace), false, SearchOriginPawn);
	AActor* BestTarget = nullptr;
	float BestViewAlignment = -1.0f;
	float BestDistance = MaxDistance;
	const FVector CameraForward = ViewRotation.Vector();
	const FVector OriginLocation = SearchOriginPawn->GetComponentsBoundingBox(true).GetCenter();

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Candidate = *It;
		if (!Candidate || Candidate == SearchOriginPawn ||
			!Candidate->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()) ||
			!ITPPossessableInterface::Execute_CanBePossessed(Candidate, const_cast<AUTPPlayerController*>(this)))
		{
			continue;
		}

		const FVector TargetLocation = Candidate->GetComponentsBoundingBox(true).GetCenter();
		const FVector CameraToTarget = TargetLocation - ViewLocation;
		const float CameraDistance = CameraToTarget.Length();
		const float TargetDistance = FVector::Distance(OriginLocation, TargetLocation);
		if (CameraDistance <= KINDA_SMALL_NUMBER || TargetDistance > MaxDistance)
		{
			continue;
		}

		const float ViewAlignment = FVector::DotProduct(CameraForward, CameraToTarget / CameraDistance);
		if (ViewAlignment <= 0.0f)
		{
			continue;
		}

		FVector2D ScreenPosition;
		if (!ProjectWorldLocationToScreen(TargetLocation, ScreenPosition, false) ||
			ScreenPosition.X < 0.0f || ScreenPosition.X > ViewportWidth ||
			ScreenPosition.Y < 0.0f || ScreenPosition.Y > ViewportHeight)
		{
			continue;
		}

		// The target must be visible from the active camera and reachable from
		// the currently controlled Pawn without an intervening blocking object.
		FHitResult CameraSightHit;
		const bool bCameraBlocked = World->LineTraceSingleByChannel(
			CameraSightHit,
			ViewLocation,
			TargetLocation,
			ECC_Visibility,
			QueryParams);
		if (bCameraBlocked && CameraSightHit.GetActor() != Candidate)
		{
			continue;
		}

		FHitResult PathHit;
		const bool bPathBlocked = World->LineTraceSingleByChannel(
			PathHit,
			OriginLocation,
			TargetLocation,
			ECC_Visibility,
			QueryParams);
		if (!bPathBlocked || PathHit.GetActor() == Candidate)
		{
			// Prefer the target nearest the center of the camera; use distance as
			// a deterministic tie-breaker for targets at the same view angle.
			if (ViewAlignment > BestViewAlignment ||
				(FMath::IsNearlyEqual(ViewAlignment, BestViewAlignment) && TargetDistance < BestDistance))
			{
				BestTarget = Candidate;
				BestViewAlignment = ViewAlignment;
				BestDistance = TargetDistance;
			}
		}
	}

	return BestTarget;
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
	UInputModifierSwizzleAxis* MouseYSwizzle = NewObject<UInputModifierSwizzleAxis>(RuntimeMappingContext);
	MouseYSwizzle->Order = EInputAxisSwizzle::YXZ;
	MouseYMapping.Modifiers.Add(MouseYSwizzle);
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
