#include "UTPSoulPawn.h"

#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"
#include "Animation/UTPSoulAnimInstance.h"
#include "Components/UTPPossessionComponent.h"
#include "UTPPossessableInterface.h"
#include "UTPPlayerController.h"

#include "Engine/World.h"
#include "CollisionShape.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "InputAction.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AUTPSoulPawn::AUTPSoulPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(34.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("Pawn"));

	VisualMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VisualMeshComponent"));
	VisualMeshComponent->SetupAttachment(CollisionComponent);
	VisualMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMeshComponent->SetGenerateOverlapEvents(false);
	VisualMeshComponent->SetRelativeLocation(FVector::ZeroVector);
	// The Ghost asset is authored facing negative X. Align its visual forward
	// direction with this Pawn's positive X forward axis.
	VisualMeshComponent->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = CollisionComponent;
	MovementComponent->MaxSpeed = 700.0f;
	MovementComponent->Acceleration = 4096.0f;
	MovementComponent->Deceleration = 4096.0f;

	AnimInstanceClass = UTPSoulAnimInstance::StaticClass();

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionAsset(
		TEXT("/Game/Input/InputActions/IA_Move.IA_Move"));
	if (MoveActionAsset.Succeeded())
	{
		MoveAction = MoveActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> SoulAscendActionAsset(
		TEXT("/Game/Input/InputActions/IA_SoulAscend.IA_SoulAscend"));
	if (SoulAscendActionAsset.Succeeded())
	{
		SoulAscendAction = SoulAscendActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> SoulDescendActionAsset(
		TEXT("/Game/Input/InputActions/IA_SoulDescend.IA_SoulDescend"));
	if (SoulDescendActionAsset.Succeeded())
	{
		SoulDescendAction = SoulDescendActionAsset.Object;
	}

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void AUTPSoulPawn::BeginPlay()
{
	Super::BeginPlay();
	ApplyAnimInstanceClass();
}

void AUTPSoulPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyAnimInstanceClass();
}

void AUTPSoulPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPossessionVanishActive)
	{
		PossessionVanishElapsed = FMath::Min(PossessionVanishElapsed + DeltaSeconds, PossessionVanishDuration);
		const float Alpha = PossessionVanishDuration > KINDA_SMALL_NUMBER
			? PossessionVanishElapsed / PossessionVanishDuration
			: 1.0f;
		// Ease-in keeps the soul readable at the start, then pulls it away quickly.
		SetActorScale3D(PossessionVanishInitialScale * (1.0f - FMath::InterpEaseIn(0.0f, 1.0f, Alpha, 2.0f)));
		return;
	}

	if (bPossessionMaterializeActive)
	{
		PossessionMaterializeElapsed = FMath::Min(
			PossessionMaterializeElapsed + DeltaSeconds,
			PossessionMaterializeDuration);
		const float Alpha = PossessionMaterializeDuration > KINDA_SMALL_NUMBER
			? PossessionMaterializeElapsed / PossessionMaterializeDuration
			: 1.0f;
		SetActorScale3D(PossessionMaterializeFinalScale * FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f));
		if (Alpha >= 1.0f)
		{
			FinishPossessionMaterialize();
		}
		return;
	}

	const FVector HorizontalVelocity(GetVelocity().X, GetVelocity().Y, 0.0f);
	if (!HorizontalVelocity.IsNearlyZero())
	{
		const FRotator DesiredRotation(0.0f, HorizontalVelocity.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, RotationInterpSpeed));
	}

	UpdateFocusedTarget();
}

void AUTPSoulPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUTPSoulPawn::MoveForward);
	}

	if (SoulAscendAction)
	{
		EnhancedInputComponent->BindAction(SoulAscendAction, ETriggerEvent::Triggered, this, &AUTPSoulPawn::MoveUp);
	}

	if (SoulDescendAction)
	{
		EnhancedInputComponent->BindAction(SoulDescendAction, ETriggerEvent::Triggered, this, &AUTPSoulPawn::MoveDown);
	}

}

void AUTPSoulPawn::ApplyAnimInstanceClass()
{
	if (VisualMeshComponent && AnimInstanceClass)
	{
		VisualMeshComponent->SetAnimInstanceClass(AnimInstanceClass);
	}
}

void AUTPSoulPawn::MoveForward(const FInputActionValue& Value)
{
	const AUTPPlayerController* PlayerController = Cast<AUTPPlayerController>(Controller);
	if (PlayerController && PlayerController->GetPossessionComponent() &&
		PlayerController->GetPossessionComponent()->IsPossessionTransitionInProgress())
	{
		return;
	}

	const FVector2D MovementValue = Value.Get<FVector2D>();
	if (Controller && !MovementValue.IsNearlyZero())
	{
		const FRotator ControlRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector ForwardDirection = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(ForwardDirection, MovementValue.Y * MoveSpeedScale);
		AddMovementInput(RightDirection, MovementValue.X * MoveSpeedScale);
	}
}

void AUTPSoulPawn::MoveRight(const FInputActionValue& Value)
{
	MoveForward(Value);
}

void AUTPSoulPawn::MoveUp(const FInputActionValue& Value)
{
	const AUTPPlayerController* PlayerController = Cast<AUTPPlayerController>(Controller);
	if (PlayerController && PlayerController->GetPossessionComponent() &&
		PlayerController->GetPossessionComponent()->IsPossessionTransitionInProgress())
	{
		return;
	}

	const float MovementValue = Value.Get<float>();
	if (Controller && !FMath::IsNearlyZero(MovementValue))
	{
		AddMovementInput(FVector::UpVector, MovementValue * MoveSpeedScale);
	}
}

void AUTPSoulPawn::MoveDown(const FInputActionValue& Value)
{
	const AUTPPlayerController* PlayerController = Cast<AUTPPlayerController>(Controller);
	if (PlayerController && PlayerController->GetPossessionComponent() &&
		PlayerController->GetPossessionComponent()->IsPossessionTransitionInProgress())
	{
		return;
	}

	const float MovementValue = Value.Get<float>();
	if (Controller && !FMath::IsNearlyZero(MovementValue))
	{
		AddMovementInput(FVector::DownVector, MovementValue * MoveSpeedScale);
	}
}

void AUTPSoulPawn::RequestPossessFocusedTarget()
{
	AUTPPlayerController* PlayerController = Cast<AUTPPlayerController>(GetController());
	if (!PlayerController || (PlayerController->GetPossessionComponent() &&
		PlayerController->GetPossessionComponent()->IsPossessionTransitionInProgress()))
	{
		return;
	}

	PlayerController->TryPossessTarget(FocusedPossessableTarget.Get());
}

AActor* AUTPSoulPawn::GetFocusedPossessableTarget() const
{
	return FocusedPossessableTarget.Get();
}

void AUTPSoulPawn::BeginPossessionVanish(float InDuration)
{
	if (bPossessionVanishActive)
	{
		return;
	}

	bPossessionVanishActive = true;
	PossessionVanishElapsed = 0.0f;
	PossessionVanishDuration = FMath::Max(0.0f, InDuration);
	PossessionVanishInitialScale = GetActorScale3D();
	SpawnPossessionEffect(PossessionStartEffect, GetActorLocation());
	if (MovementComponent)
	{
		MovementComponent->StopMovementImmediately();
	}

	if (CollisionComponent)
	{
		CachedCollisionEnabled = CollisionComponent->GetCollisionEnabled();
		bHasCachedCollisionEnabled = true;
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AUTPSoulPawn::CancelPossessionVanish()
{
	if (!bPossessionVanishActive)
	{
		return;
	}

	bPossessionVanishActive = false;
	PossessionVanishElapsed = 0.0f;
	PossessionVanishDuration = 0.0f;
	SetActorScale3D(PossessionVanishInitialScale);

	RestorePossessionCollision();
}

void AUTPSoulPawn::BeginPossessionMaterialize(float InDuration)
{
	if (bPossessionMaterializeActive)
	{
		return;
	}

	bPossessionMaterializeActive = true;
	PossessionMaterializeElapsed = 0.0f;
	PossessionMaterializeDuration = FMath::Max(0.0f, InDuration);
	PossessionMaterializeFinalScale = GetActorScale3D();
	SetActorScale3D(FVector::ZeroVector);

	if (CollisionComponent)
	{
		CachedCollisionEnabled = CollisionComponent->GetCollisionEnabled();
		bHasCachedCollisionEnabled = true;
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AUTPSoulPawn::FinishPossessionMaterialize()
{
	if (!bPossessionMaterializeActive)
	{
		return;
	}

	bPossessionMaterializeActive = false;
	PossessionMaterializeElapsed = PossessionMaterializeDuration;
	SetActorScale3D(PossessionMaterializeFinalScale);
	RestorePossessionCollision();
}

void AUTPSoulPawn::CancelPossessionMaterialize()
{
	if (!bPossessionMaterializeActive)
	{
		return;
	}

	bPossessionMaterializeActive = false;
	PossessionMaterializeElapsed = 0.0f;
	PossessionMaterializeDuration = 0.0f;
	SetActorScale3D(PossessionMaterializeFinalScale);
	RestorePossessionCollision();
}

void AUTPSoulPawn::PlayPossessionArrivalEffect(const FVector& Location) const
{
	SpawnPossessionEffect(PossessionArrivalEffect, Location);
}

void AUTPSoulPawn::PlayPossessionReleaseEffect(const FVector& Location) const
{
	SpawnPossessionEffect(PossessionReleaseEffect, Location);
}

void AUTPSoulPawn::SpawnPossessionEffect(UNiagaraSystem* Effect, const FVector& Location) const
{
	if (Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			Effect,
			Location,
			GetActorRotation(),
			PossessionEffectScale);
	}
}

void AUTPSoulPawn::RestorePossessionCollision()
{
	if (CollisionComponent && bHasCachedCollisionEnabled)
	{
		CollisionComponent->SetCollisionEnabled(CachedCollisionEnabled);
	}

	bHasCachedCollisionEnabled = false;
}

void AUTPSoulPawn::UpdateFocusedTarget()
{
	if (!IsLocallyControlled())
	{
		SetFocusedPossessableTarget(nullptr);
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	UWorld* World = GetWorld();
	if (!PlayerController || !World)
	{
		SetFocusedPossessableTarget(nullptr);
		return;
	}

	FRotator TraceRotation;
	FVector ViewLocation;
	PlayerController->GetPlayerViewPoint(ViewLocation, TraceRotation);

	// The Pawn root may be positioned below the visible soul mesh. Start from the
	// rendered mesh bounds so the sweep visibly originates from the soul center.
	const FVector TraceStart = VisualMeshComponent && VisualMeshComponent->IsRegistered()
		? VisualMeshComponent->Bounds.Origin
		: CollisionComponent->GetComponentLocation();
	const FVector TraceEnd = TraceStart + TraceRotation.Vector() * PossessionTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SoulPossessionTrace), false, this);

	const FCollisionShape TraceShape = FCollisionShape::MakeSphere(PossessionTraceRadius);
	TArray<FHitResult> HitResults;
	// Search only Pawn objects here. A wide sweep against Visibility also catches
	// the floor under a target before it reaches that target.
	const FCollisionObjectQueryParams TargetObjectQuery(ECC_Pawn);
	World->SweepMultiByObjectType(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		TargetObjectQuery,
		TraceShape,
		QueryParams);

	AActor* NewTarget = nullptr;
	FHitResult RelevantHit;
	bool bHasRelevantHit = false;
	for (const FHitResult& Hit : HitResults)
	{
		// The widened sweep may begin in floor collision around the soul. That
		// initial penetration should not hide an intended target farther ahead.
		if (Hit.bStartPenetrating)
		{
			continue;
		}

		AActor* HitActor = Hit.GetActor();
		if (HitActor && CanPossessActor(HitActor))
		{
			// The broad Pawn sweep deliberately ignores the floor. Retain normal
			// wall occlusion by checking a thin Visibility ray to this candidate.
			FHitResult SightHit;
			const bool bSightBlocked = World->LineTraceSingleByChannel(
				SightHit,
				TraceStart,
				Hit.ImpactPoint,
				PossessionTraceChannel,
				QueryParams);

			if (!bSightBlocked || SightHit.GetActor() == HitActor)
			{
				NewTarget = HitActor;
				RelevantHit = Hit;
				bHasRelevantHit = true;
				break;
			}

			RelevantHit = SightHit;
			bHasRelevantHit = true;
			break;
		}
	}

	if (bDrawPossessionTrace)
	{
		const FVector DebugEnd = bHasRelevantHit ? RelevantHit.Location : TraceEnd;
		const FColor DebugColor = NewTarget ? FColor::Green : (bHasRelevantHit ? FColor::Red : FColor::Cyan);
		const FVector SweepVector = DebugEnd - TraceStart;
		const float SweepLength = SweepVector.Length();
		if (SweepLength > KINDA_SMALL_NUMBER)
		{
			const FVector SweepDirection = SweepVector / SweepLength;
			const FVector SweepCenter = (TraceStart + DebugEnd) * 0.5f;
			DrawDebugCapsule(
				World,
				SweepCenter,
				SweepLength * 0.5f + PossessionTraceRadius,
				PossessionTraceRadius,
				FRotationMatrix::MakeFromZ(SweepDirection).ToQuat(),
				DebugColor,
				false,
				0.0f,
				0,
				1.5f);
		}

		if (bHasRelevantHit)
		{
			DrawDebugSphere(World, RelevantHit.ImpactPoint, 12.0f, 12, DebugColor, false, 0.0f, 0, 1.5f);
		}
	}

	SetFocusedPossessableTarget(NewTarget);
}

void AUTPSoulPawn::SetFocusedPossessableTarget(AActor* NewTarget)
{
	if (FocusedPossessableTarget.Get() == NewTarget)
	{
		return;
	}

	AActor* PreviousTarget = FocusedPossessableTarget.Get();
	if (PreviousTarget && PreviousTarget->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		ITPPossessableInterface::Execute_OnPossessionFocusChanged(PreviousTarget, false);
	}

	FocusedPossessableTarget = nullptr;

	if (NewTarget && NewTarget->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		if (CanPossessActor(NewTarget))
		{
			FocusedPossessableTarget = NewTarget;
			ITPPossessableInterface::Execute_OnPossessionFocusChanged(NewTarget, true);
		}
	}
}

bool AUTPSoulPawn::CanPossessActor(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UTPPossessableInterface::StaticClass()))
	{
		return false;
	}

	const AController* OwningController = GetController();
	return ITPPossessableInterface::Execute_CanBePossessed(TargetActor, const_cast<AController*>(OwningController));
}
