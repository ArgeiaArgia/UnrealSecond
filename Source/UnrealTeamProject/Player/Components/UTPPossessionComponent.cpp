#include "UTPPossessionComponent.h"

#include "../UTPPlayerController.h"
#include "../UTPPossessableInterface.h"
#include "../UTPSoulPawn.h"

#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UTPPossessionComponent::UTPPossessionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTPPossessionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* CurrentPawn = GetCurrentControlledPawn())
	{
		SetSoulPawn(CurrentPawn);
	}
}

void UTPPossessionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bPossessionCinematicActive)
	{
		if (!IsValid(SoulPawn.Get()) || !PendingPossessionTarget.IsValid())
		{
			CancelPossessionTransition();
			return;
		}

		RemainingPossessionTransitionTime -= DeltaTime;
		// The camera owns the visual end of the transition. Waiting for it here
		// avoids a controller possession in the same frame that would snap the
		// shared camera to its new follow target.
		AUTPPlayerController* PlayerController = GetOwningPlayerController();
		if (RemainingPossessionTransitionTime <= 0.0f &&
			(!PlayerController || !PlayerController->IsPossessionCameraTransitionActive()))
		{
			if (bPossessionCinematicReturnsToSoul)
			{
				CompleteSoulReleaseTransition();
			}
			else
			{
				CompletePossessionTransition();
			}
		}
		return;
	}

	if (!bEnableSoulWindowTimeout || !bSoulWindowActive)
	{
		return;
	}

	if (!IsValid(SoulPawn.Get()) || !IsValid(StoredBodyPawn.Get()))
	{
		ClearSoulWindow();
		return;
	}

	RemainingSoulTime -= DeltaTime;
	if (RemainingSoulTime > 0.0f)
	{
		return;
	}

	ReturnToStoredBody();
}

void UTPPossessionComponent::SetSoulPawn(APawn* InSoulPawn)
{
	AUTPSoulPawn* InSoul = Cast<AUTPSoulPawn>(InSoulPawn);
	if (!InSoul)
	{
		return;
	}

	SoulPawn = InSoul;
	SoulPawnClass = InSoul->GetClass();
}

void UTPPossessionComponent::HandleControllerPossessed(APawn* InPawn)
{
	if (!IsValid(InPawn))
	{
		return;
	}

	// PossessionComponent owns the initial Soul reference. If the controller
	// receives its Soul pawn before BeginPlay, capture it here as well.
	if (InPawn->IsA(AUTPSoulPawn::StaticClass()))
	{
		if (!IsValid(SoulPawn.Get()))
		{
			SetSoulPawn(InPawn);
		}
		return;
	}

	// PossessPawn() handles its own success event after verifying the actual
	// controller pawn. Do not duplicate that event from OnPossess().
	if (bPossessionTransitionInProgress || !IsValid(SoulPawn.Get()))
	{
		return;
	}

	if (InPawn->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		ITPPossessableInterface::Execute_OnPossessedBySoul(InPawn, SoulPawn.Get());
	}
}

void UTPPossessionComponent::HandleControllerUnpossessed(APawn* PreviousPawn)
{
	if (bPossessionTransitionInProgress)
	{
		return;
	}

	if (IsValid(PreviousPawn) && PreviousPawn != SoulPawn.Get() &&
		PreviousPawn->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		ITPPossessableInterface::Execute_OnReleasedFromSoul(PreviousPawn, SoulPawn.Get());
	}

	// An external unpossess invalidates the current Soul Window and stored body.
	ClearSoulWindow();
}

bool UTPPossessionComponent::TryPossessTarget(AActor* TargetActor)
{
	AUTPPlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !TargetActor)
	{
		return false;
	}

	APawn* CurrentPawn = GetCurrentControlledPawn();
	if (!CurrentPawn)
	{
		return false;
	}

	APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (!TargetPawn)
	{
		return false;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		return false;
	}

	if (!ITPPossessableInterface::Execute_CanBePossessed(TargetActor, PlayerController))
	{
		return false;
	}

	if (CurrentPawn == SoulPawn.Get())
	{
		return BeginPossessionTransition(TargetPawn);
	}

	// The player may move directly between possessable bodies. There is no Soul
	// pawn to animate in this path, so transfer control after the same interface
	// validation used by a normal Soul-to-body possession.
	if (!CurrentPawn->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		return false;
	}

	return PossessFromCurrentBody(CurrentPawn, TargetPawn);
}

bool UTPPossessionComponent::TogglePossession()
{
	APawn* CurrentPawn = GetCurrentControlledPawn();
	if (!CurrentPawn || bPossessionTransitionInProgress)
	{
		return false;
	}

	if (CurrentPawn == SoulPawn.Get())
	{
		return ReturnToStoredBody();
	}

	return ReturnToSoul();
}

bool UTPPossessionComponent::ReturnToSoul()
{
	AUTPPlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !SoulPawnClass || bPossessionTransitionInProgress)
	{
		return false;
	}

	APawn* CurrentPawn = GetCurrentControlledPawn();
	if (!CurrentPawn || CurrentPawn->IsA(AUTPSoulPawn::StaticClass()))
	{
		return false;
	}

	APawn* PreviousBody = CurrentPawn;

	if (PreviousBody->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()) &&
		!ITPPossessableInterface::Execute_CanReleaseFromSoul(PreviousBody))
	{
		return false;
	}

	FTransform SoulSpawnTransform = PreviousBody->GetActorTransform();
	SoulSpawnTransform.AddToTranslation(SoulReleaseSpawnOffset);
	AUTPSoulPawn* NewSoulPawn = SpawnSoulPawn(SoulSpawnTransform);
	if (!NewSoulPawn)
	{
		return false;
	}

	NewSoulPawn->PlayPossessionReleaseEffect(PreviousBody->GetActorLocation());
	return BeginSoulReleaseTransition(PreviousBody, NewSoulPawn);
}

bool UTPPossessionComponent::ReturnToStoredBody()
{
	AUTPPlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !IsValid(SoulPawn.Get()) || !IsValid(StoredBodyPawn.Get()) || bPossessionTransitionInProgress)
	{
		return false;
	}

	APawn* CurrentPawn = GetCurrentControlledPawn();
	if (CurrentPawn != SoulPawn.Get())
	{
		return false;
	}

	APawn* BodyPawn = StoredBodyPawn.Get();
	if (!IsValid(BodyPawn) || BodyPawn == SoulPawn.Get())
	{
		return false;
	}

	const bool bWasSoulWindowActive = bSoulWindowActive;
	const float PreviousSoulTime = RemainingSoulTime;

	if (!PossessPawn(BodyPawn))
	{
		// Possess() may trigger the controller's unpossess callback. Restore the
		// window state so a failed return does not destroy the retry information.
		bSoulWindowActive = bWasSoulWindowActive;
		RemainingSoulTime = PreviousSoulTime;
		return false;
	}

	ClearSoulWindow();

	if (BodyPawn->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		ITPPossessableInterface::Execute_OnPossessedBySoul(BodyPawn, SoulPawn.Get());
	}

	DestroySoulPawn();

	return true;
}

void UTPPossessionComponent::BeginSoulWindow(APawn* InSoulPawn, APawn* InStoredBody)
{
	SoulPawn = InSoulPawn;
	StoredBodyPawn = InStoredBody;
	RemainingSoulTime = bEnableSoulWindowTimeout ? SoulWindowSeconds : 0.0f;
	bSoulWindowActive = bEnableSoulWindowTimeout && SoulWindowSeconds > 0.0f && IsValid(SoulPawn.Get()) && IsValid(StoredBodyPawn.Get());
}

void UTPPossessionComponent::ClearSoulWindow()
{
	bSoulWindowActive = false;
	RemainingSoulTime = 0.0f;
	StoredBodyPawn = nullptr;
}

APawn* UTPPossessionComponent::GetSoulPawn() const
{
	return SoulPawn.Get();
}

APawn* UTPPossessionComponent::GetStoredBodyPawn() const
{
	return StoredBodyPawn.Get();
}

bool UTPPossessionComponent::IsSoulWindowActive() const
{
	return bSoulWindowActive;
}

float UTPPossessionComponent::GetRemainingSoulTime() const
{
	return RemainingSoulTime;
}

bool UTPPossessionComponent::IsPossessionTransitionInProgress() const
{
	return bPossessionTransitionInProgress;
}

AUTPPlayerController* UTPPossessionComponent::GetOwningPlayerController() const
{
	return Cast<AUTPPlayerController>(GetOwner());
}

APawn* UTPPossessionComponent::GetCurrentControlledPawn() const
{
	const AUTPPlayerController* PlayerController = GetOwningPlayerController();
	return PlayerController ? PlayerController->GetPawn() : nullptr;
}

bool UTPPossessionComponent::PossessPawn(APawn* NewPawn)
{
	AUTPPlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !NewPawn)
	{
		return false;
	}

	const bool bWasTransitionInProgress = bPossessionTransitionInProgress;
	bPossessionTransitionInProgress = true;

	PlayerController->Possess(NewPawn);

	const bool bPossessed = PlayerController->GetPawn() == NewPawn;
	bPossessionTransitionInProgress = bWasTransitionInProgress;
	return bPossessed;
}

bool UTPPossessionComponent::PossessFromCurrentBody(APawn* PreviousBody, APawn* TargetPawn)
{
	if (!IsValid(PreviousBody) || !IsValid(TargetPawn) || PreviousBody == TargetPawn ||
		bPossessionTransitionInProgress)
	{
		return false;
	}

	if (!PossessPawn(TargetPawn))
	{
		return false;
	}

	// PossessPawn temporarily suppresses controller callbacks while it changes
	// the pawn. Notify both bodies explicitly once the transfer has succeeded.
	if (PreviousBody->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		ITPPossessableInterface::Execute_OnReleasedFromSoul(PreviousBody, SoulPawn.Get());
	}

	if (TargetPawn->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		ITPPossessableInterface::Execute_OnPossessedBySoul(TargetPawn, SoulPawn.Get());
	}

	return true;
}

AUTPSoulPawn* UTPPossessionComponent::SpawnSoulPawn(const FTransform& SpawnTransform)
{
	UWorld* World = GetWorld();
	if (!World || !SoulPawnClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwningPlayerController();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AUTPSoulPawn>(SoulPawnClass, SpawnTransform, SpawnParameters);
}

void UTPPossessionComponent::DestroySoulPawn()
{
	if (IsValid(SoulPawn.Get()))
	{
		SoulPawn->Destroy();
	}

	SoulPawn = nullptr;
}

bool UTPPossessionComponent::BeginPossessionTransition(APawn* TargetPawn)
{
	AUTPPlayerController* PlayerController = GetOwningPlayerController();
	AUTPSoulPawn* CurrentSoulPawn = Cast<AUTPSoulPawn>(SoulPawn.Get());
	if (!PlayerController || !CurrentSoulPawn || !IsValid(TargetPawn) || bPossessionTransitionInProgress)
	{
		return false;
	}

	PendingPossessionTarget = TargetPawn;
	RemainingPossessionTransitionTime = FMath::Max(0.0f, PossessionTransitionSeconds);
	bPossessionTransitionInProgress = true;
	bPossessionCinematicActive = true;

	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
	// The overlap begins on the outside of a body. Aim at its bounds center so
	// the fade visually reads as the soul entering that body instead of simply
	// disappearing at the contact point.
	const FVector PossessionDestination = TargetPawn->GetComponentsBoundingBox(true).GetCenter();
	CurrentSoulPawn->BeginPossessionVanish(PossessionDestination, PossessionTransitionSeconds);
	PlayerController->BeginPossessionCameraTransition(TargetPawn, PossessionTransitionSeconds);

	if (RemainingPossessionTransitionTime <= 0.0f)
	{
		CompletePossessionTransition();
	}

	return true;
}

void UTPPossessionComponent::CompletePossessionTransition()
{
	AUTPPlayerController* PlayerController = GetOwningPlayerController();
	AUTPSoulPawn* CurrentSoulPawn = Cast<AUTPSoulPawn>(SoulPawn.Get());
	APawn* TargetPawn = PendingPossessionTarget.Get();
	if (!PlayerController || !CurrentSoulPawn || !IsValid(TargetPawn))
	{
		CancelPossessionTransition();
		return;
	}

	if (!PossessPawn(TargetPawn))
	{
		CancelPossessionTransition();
		return;
	}

	// Possessing a Character can update control rotation from that Character.
	// Restore the transition camera's final angle so the one shared rig keeps
	// its view instead of jumping behind the new body.
	PlayerController->PreserveSharedCameraAngle();

	if (TargetPawn->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		ITPPossessableInterface::Execute_OnPossessedBySoul(TargetPawn, CurrentSoulPawn);
	}

	CurrentSoulPawn->PlayPossessionArrivalEffect(TargetPawn->GetActorLocation());
	DestroySoulPawn();
	PendingPossessionTarget = nullptr;
	RemainingPossessionTransitionTime = 0.0f;
	bPossessionCinematicActive = false;
	bPossessionTransitionInProgress = false;
	bPossessionCinematicReturnsToSoul = false;
	PlayerController->SetIgnoreMoveInput(false);
	PlayerController->SetIgnoreLookInput(false);
}

bool UTPPossessionComponent::BeginSoulReleaseTransition(APawn* PreviousBody, AUTPSoulPawn* NewSoulPawn)
{
	AUTPPlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !IsValid(PreviousBody) || !NewSoulPawn || bPossessionTransitionInProgress)
	{
		if (NewSoulPawn)
		{
			NewSoulPawn->Destroy();
		}
		return false;
	}

	SoulPawn = NewSoulPawn;
	PendingPossessionTarget = NewSoulPawn;
	PendingReleasedBody = PreviousBody;
	RemainingPossessionTransitionTime = FMath::Max(0.0f, PossessionTransitionSeconds);
	bPossessionTransitionInProgress = true;
	bPossessionCinematicActive = true;
	bPossessionCinematicReturnsToSoul = true;

	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
	NewSoulPawn->BeginPossessionMaterialize(PossessionTransitionSeconds);
	PlayerController->BeginPossessionCameraTransition(NewSoulPawn, PossessionTransitionSeconds);

	if (RemainingPossessionTransitionTime <= 0.0f)
	{
		CompleteSoulReleaseTransition();
	}

	return true;
}

void UTPPossessionComponent::CompleteSoulReleaseTransition()
{
	AUTPPlayerController* PlayerController = GetOwningPlayerController();
	AUTPSoulPawn* NewSoulPawn = Cast<AUTPSoulPawn>(SoulPawn.Get());
	APawn* PreviousBody = PendingReleasedBody.Get();
	if (!PlayerController || !NewSoulPawn || !IsValid(PreviousBody))
	{
		CancelPossessionTransition();
		return;
	}

	if (!PossessPawn(NewSoulPawn))
	{
		CancelPossessionTransition();
		return;
	}

	PlayerController->PreserveSharedCameraAngle();

	NewSoulPawn->FinishPossessionMaterialize();
	BeginSoulWindow(NewSoulPawn, PreviousBody);

	if (PreviousBody->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		ITPPossessableInterface::Execute_OnReleasedFromSoul(PreviousBody, NewSoulPawn);
	}

	PendingPossessionTarget = nullptr;
	PendingReleasedBody = nullptr;
	RemainingPossessionTransitionTime = 0.0f;
	bPossessionCinematicActive = false;
	bPossessionTransitionInProgress = false;
	bPossessionCinematicReturnsToSoul = false;
	PlayerController->SetIgnoreMoveInput(false);
	PlayerController->SetIgnoreLookInput(false);
}

void UTPPossessionComponent::CancelPossessionTransition()
{
	if (bPossessionCinematicReturnsToSoul)
	{
		DestroySoulPawn();
	}
	else if (AUTPSoulPawn* CurrentSoulPawn = Cast<AUTPSoulPawn>(SoulPawn.Get()))
	{
		CurrentSoulPawn->CancelPossessionVanish();
	}

	if (AUTPPlayerController* PlayerController = GetOwningPlayerController())
	{
		PlayerController->SetIgnoreMoveInput(false);
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->CancelPossessionCameraTransition();
	}

	PendingPossessionTarget = nullptr;
	PendingReleasedBody = nullptr;
	RemainingPossessionTransitionTime = 0.0f;
	bPossessionCinematicActive = false;
	bPossessionTransitionInProgress = false;
	bPossessionCinematicReturnsToSoul = false;
}
