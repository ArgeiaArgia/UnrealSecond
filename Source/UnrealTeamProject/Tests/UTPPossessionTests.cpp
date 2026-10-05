#if WITH_DEV_AUTOMATION_TESTS

#include "../Player/Animals/Flying/UTPFlyingAnimalCharacter.h"
#include "../Player/Animals/Frog/UTPFrogCharacter.h"
#include "../Player/Animals/Turtle/UTPTurtleCharacter.h"
#include "../Player/Components/UTPPossessionComponent.h"
#include "../Player/UTPPlayerController.h"
#include "../Player/UTPPossessionTargeting.h"
#include "../Player/UTPSoulPawn.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Slate/SceneViewport.h"

namespace
{
	class FPossessionTestViewport : public FSceneViewport
	{
	public:
		explicit FPossessionTestViewport(FViewportClient* Client) : FSceneViewport(Client, nullptr) {}
		virtual FIntPoint GetSizeXY() const override { return FIntPoint(1280, 720); }
	};
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FUTPPossessionRoundTripTest,
	"UnrealTeamProject.Possession.FlyingFoxRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FUTPPossessionRoundTripTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	Names.Add(TEXT("Native"));
	Commands.Add(TEXT("Native"));
	Names.Add(TEXT("Blueprint"));
	Commands.Add(TEXT("Blueprint"));
}

bool FUTPPossessionRoundTripTest::RunTest(const FString& Parameters)
{
	const bool bBlueprint = Parameters == TEXT("Blueprint");
	UClass* BatClass = bBlueprint ? LoadClass<AUTPFlyingAnimalCharacter>(nullptr,
		TEXT("/Game/Blueprints/Characters/BP_FlyingFox.BP_FlyingFox_C")) : AUTPFlyingAnimalCharacter::StaticClass();
	UClass* TurtleClass = bBlueprint ? LoadClass<AUTPTurtleCharacter>(nullptr,
		TEXT("/Game/Blueprints/Characters/BP_SnappingTurtle.BP_SnappingTurtle_C")) : AUTPTurtleCharacter::StaticClass();
	UClass* FrogClass = bBlueprint ? LoadClass<AUTPFrogCharacter>(nullptr,
		TEXT("/Game/Blueprints/Characters/BP_Frog.BP_Frog_C")) : AUTPFrogCharacter::StaticClass();
	if (!TestNotNull(TEXT("Flying fox class loads"), BatClass) ||
		!TestNotNull(TEXT("Turtle class loads"), TurtleClass) ||
		!TestNotNull(TEXT("Frog class loads"), FrogClass))
	{
		return false;
	}

	const UWorld::InitializationValues Settings = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false)
		.RequiresHitProxies(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
		nullptr, true, ERHIFeatureLevel::Num, &Settings);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AUTPPlayerController* Controller = World->SpawnActor<AUTPPlayerController>();
	AUTPSoulPawn* Soul = World->SpawnActor<AUTPSoulPawn>();
	AUTPFlyingAnimalCharacter* Bat = World->SpawnActor<AUTPFlyingAnimalCharacter>(
		BatClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AUTPTurtleCharacter* Turtle = World->SpawnActor<AUTPTurtleCharacter>(
		TurtleClass, FVector(400.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
	AUTPFrogCharacter* Frog = World->SpawnActor<AUTPFrogCharacter>(
		FrogClass, FVector(0.0f, 400.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
	if (!TestNotNull(TEXT("Controller spawns"), Controller) || !TestNotNull(TEXT("Soul spawns"), Soul) ||
		!TestNotNull(TEXT("Bat spawns"), Bat) || !TestNotNull(TEXT("Turtle spawns"), Turtle) ||
		!TestNotNull(TEXT("Frog spawns"), Frog))
	{
		return false;
	}
	for (ACharacter* Animal : {static_cast<ACharacter*>(Bat), static_cast<ACharacter*>(Turtle), static_cast<ACharacter*>(Frog)})
	{
		Animal->SetActorLocation(Animal->GetActorLocation() +
			FVector(0.0f, 0.0f, Animal->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.0f));
	}
	AActor* Ground = World->SpawnActor<AActor>();
	UBoxComponent* GroundCollision = NewObject<UBoxComponent>(Ground);
	Ground->SetRootComponent(GroundCollision);
	GroundCollision->SetBoxExtent(FVector(1000.0f, 1000.0f, 20.0f));
	GroundCollision->SetCollisionProfileName(TEXT("BlockAll"));
	GroundCollision->RegisterComponent();
	Ground->SetActorLocation(FVector(0.0f, 0.0f, -20.0f));

	Controller->Possess(Soul);
	Controller->Player = NewObject<ULocalPlayer>(GEngine);
	UTPPossessionComponent* Possession = Controller->GetPossessionComponent();
	Possession->SetSoulPawn(Soul);
	TestTrue(TEXT("Controller starts on soul"), Controller->GetPawn() == Soul);
	const auto Transfer = [&](AUTPPossessableCharacter* Target, const TCHAR* Description)
	{
		AUTPPossessableCharacter* Previous = Cast<AUTPPossessableCharacter>(Controller->GetPawn());
		if (!TestTrue(Description, Controller->TryPossessTarget(Target)))
		{
			return false;
		}
		Possession->TickComponent(1.0f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Transfer reaches target"), Controller->GetPawn() == Target);
		TestFalse(TEXT("Transfer unlocks input"), Possession->IsPossessionTransitionInProgress());
		TestFalse(TEXT("Transfer unlocks movement"), Controller->IsMoveInputIgnored());
		TestFalse(TEXT("Transfer unlocks look"), Controller->IsLookInputIgnored());
		TestTrue(TEXT("Target is soul possessed"), Target->IsSoulPossessed());
		if (Previous)
		{
			TestFalse(TEXT("Previous body is released"), Previous->IsSoulPossessed());
			TestTrue(TEXT("Previous body can be possessed again"), Previous->CanBePossessed_Implementation(Controller));
		}
		return Controller->GetPawn() == Target;
	};
	if (!Transfer(Turtle, TEXT("Soul to turtle")) || !Transfer(Bat, TEXT("Turtle to bat")))
	{
		return false;
	}
	TestTrue(TEXT("Grounded bat can release"), ITPPossessableInterface::Execute_CanReleaseFromSoul(Bat));
	Bat->StartTakeoff();
	UCharacterMovementComponent* Movement = Bat->GetCharacterMovement();
	constexpr float Step = 1.0f / 60.0f;
	for (int32 Frame = 0; Frame < 600 && Bat->IsFlying(); ++Frame)
	{
		Bat->Tick(Step);
		Movement->TickComponent(Step, LEVELTICK_All, nullptr);
	}
	TestFalse(TEXT("Jump finishes flight on ordinary ground"), Bat->IsFlying());
	TestTrue(TEXT("Bat can release after landing"), ITPPossessableInterface::Execute_CanReleaseFromSoul(Bat));
	for (APawn* Target : {static_cast<APawn*>(Turtle), static_cast<APawn*>(Frog)})
	{
		FHitResult Hit;
		const bool bBlocked = World->LineTraceSingleByChannel(Hit,
			UTPPossessionTargeting::GetFocusLocation(*Bat), UTPPossessionTargeting::GetFocusLocation(*Target),
			ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(PossessionRoundTrip), false, Bat));
		TestTrue(TEXT("Ground does not block possession from bat"), !bBlocked || Hit.GetActor() == Target);
	}
	if (!Transfer(Turtle, TEXT("Bat back to turtle")) || !Transfer(Bat, TEXT("Turtle back to bat")) ||
		!Transfer(Frog, TEXT("Bat to frog")) || !Transfer(Bat, TEXT("Frog back to bat")))
	{
		return false;
	}
	Bat->SetActorLocation(FVector(0.0f, 0.0f, 400.0f));
	Bat->StartTakeoff();
	Bat->ActivateAnimalAbility();
	TestTrue(TEXT("Bat takes off before transferring"), Bat->IsFlying());
	if (!Transfer(Turtle, TEXT("Airborne bat to turtle")))
	{
		return false;
	}
	TestTrue(TEXT("Released airborne bat falls"), Movement->IsFalling());
	TestFalse(TEXT("Release clears flight ability"), Bat->IsFlightAbilityActive());
	for (int32 Frame = 0; Frame < 600 && Movement->IsFalling(); ++Frame)
	{
		Bat->Tick(Step);
		Movement->TickComponent(Step, LEVELTICK_All, nullptr);
	}
	TestTrue(TEXT("Released bat settles on ground"), Movement->MovementMode == MOVE_None);
	TestTrue(TEXT("Released bat rests in perched state"), Bat->GetFlightState() == EUTPFlightState::Perched);
	TestFalse(TEXT("Landing restores unattended physics setting"), Movement->bRunPhysicsWithNoController);
	if (!Transfer(Bat, TEXT("Turtle to landed bat")))
	{
		return false;
	}
	Bat->SetActorLocation(FVector(0.0f, 0.0f, 400.0f));
	Bat->StartTakeoff();
	if (!Transfer(Frog, TEXT("Airborne bat to frog")) || !Transfer(Bat, TEXT("Repossess bat before landing")))
	{
		return false;
	}
	TestFalse(TEXT("Repossessing cancels unattended fall"), Movement->bRunPhysicsWithNoController);
	TestTrue(TEXT("Repossessing resets old flight state"), Bat->GetFlightState() == EUTPFlightState::Perched);

	// A platform edge can block the body's ray while the overhead camera has a
	// clear view. Exercise target selection and the actual possession hold binding.
	Bat->SetActorLocation(FVector(0.0f, 0.0f, 200.0f));
	Bat->StartTakeoff();
	AActor* Ledge = World->SpawnActor<AActor>();
	UBoxComponent* LedgeCollision = NewObject<UBoxComponent>(Ledge);
	Ledge->SetRootComponent(LedgeCollision);
	LedgeCollision->SetBoxExtent(FVector(20.0f, 120.0f, 125.0f));
	LedgeCollision->SetCollisionProfileName(TEXT("BlockAll"));
	LedgeCollision->RegisterComponent();
	Ledge->SetActorLocation(FVector(200.0f, 0.0f, 125.0f));
	ACameraActor* Camera = World->SpawnActor<ACameraActor>();
	Camera->SetActorLocation(FVector(-200.0f, 0.0f, 900.0f));
	Camera->SetActorRotation((UTPPossessionTargeting::GetFocusLocation(*Turtle) - Camera->GetActorLocation()).Rotation());
	Controller->SetViewTarget(Camera);
	Controller->PlayerCameraManager->UpdateCamera(Step);
	ULocalPlayer* LocalPlayer = CastChecked<ULocalPlayer>(Controller->Player);
	UGameViewportClient* ViewportClient = NewObject<UGameViewportClient>(GEngine);
	FPossessionTestViewport Viewport(ViewportClient);
	ViewportClient->Viewport = &Viewport;
	LocalPlayer->ViewportClient = ViewportClient;
	LocalPlayer->PlayerController = Controller;
	LocalPlayer->Origin = FVector2D::ZeroVector;
	LocalPlayer->Size = FVector2D(1.0f, 1.0f);
	ON_SCOPE_EXIT { ViewportClient->Viewport = nullptr; LocalPlayer->ViewportClient = nullptr; };
	FVector2D TargetScreenPosition;
	TestTrue(TEXT("Turtle projects into the test viewport"), Controller->ProjectWorldLocationToScreen(
		UTPPossessionTargeting::GetFocusLocation(*Turtle), TargetScreenPosition, false));
	FHitResult LedgeHit;
	TestFalse(TEXT("Camera has a clear view above the ledge"), World->LineTraceSingleByChannel(LedgeHit,
		Camera->GetActorLocation(), UTPPossessionTargeting::GetFocusLocation(*Turtle),
		ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(PossessionCameraSight), false, Bat)) && LedgeHit.GetActor() != Turtle);
	TestTrue(TEXT("Ledge blocks the body's direct ray"), World->LineTraceSingleByChannel(LedgeHit,
		UTPPossessionTargeting::GetFocusLocation(*Bat), UTPPossessionTargeting::GetFocusLocation(*Turtle),
		ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(PossessionLedge), false, Bat)) && LedgeHit.GetActor() == Ledge);
	TestTrue(TEXT("Airborne bat can select the visible turtle above a ledge"),
		Controller->FindCameraPossessionTarget(Bat, 1800.0f) == Turtle);
	Camera->SetActorLocation(FVector(-200.0f, 0.0f, 150.0f));
	Camera->SetActorRotation((UTPPossessionTargeting::GetFocusLocation(*Turtle) - Camera->GetActorLocation()).Rotation());
	Controller->PlayerCameraManager->UpdateCamera(Step);
	TestTrue(TEXT("Flying possession still respects camera occlusion"),
		Controller->FindCameraPossessionTarget(Bat, 1800.0f) != Turtle);
	Camera->SetActorLocation(FVector(-200.0f, 0.0f, 900.0f));
	Camera->SetActorRotation((UTPPossessionTargeting::GetFocusLocation(*Turtle) - Camera->GetActorLocation()).Rotation());
	Controller->PlayerCameraManager->UpdateCamera(Step);
	Controller->InputComponent = NewObject<UEnhancedInputComponent>(Controller);
	Controller->PlayerInput = NewObject<UEnhancedPlayerInput>(Controller);
	Controller->SetupInputComponent();
	const UInputAction* PossessAction = LoadObject<UInputAction>(nullptr,
		TEXT("/Game/Input/InputActions/IA_PossessTarget.IA_PossessTarget"));
	bool bStartedHold = false;
	for (const auto& Binding : CastChecked<UEnhancedInputComponent>(Controller->InputComponent)->GetActionEventBindings())
	{
		if (Binding->GetAction() == PossessAction && Binding->GetTriggerEvent() == ETriggerEvent::Started)
		{
			Binding->Execute(FInputActionInstance(PossessAction));
			bStartedHold = true;
			break;
		}
	}
	TestTrue(TEXT("Possession input binding starts the hold"), bStartedHold);
	Controller->PlayerTick(1.1f);
	Possession->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Holding possession while flying reaches the selected turtle"), Controller->GetPawn() == Turtle);
	if (!Transfer(Bat, TEXT("Return to bat for wind ride")))
	{
		return false;
	}
	AActor* WindSource = World->SpawnActor<AActor>();
	Bat->EnterWindZone_Implementation(WindSource, FVector::ForwardVector, 900.0f, 0.0f);
	Bat->StartTakeoff();
	TestTrue(TEXT("Bat enters wind ride before the next hold"), Bat->GetFlightState() == EUTPFlightState::WindRide);
	Camera->SetActorRotation((UTPPossessionTargeting::GetFocusLocation(*Frog) - Camera->GetActorLocation()).Rotation());
	Controller->PlayerCameraManager->UpdateCamera(Step);
	Controller->PlayerTick(Step);
	for (const auto& Binding : CastChecked<UEnhancedInputComponent>(Controller->InputComponent)->GetActionEventBindings())
	{
		if (Binding->GetAction() == PossessAction && Binding->GetTriggerEvent() == ETriggerEvent::Started)
		{
			Binding->Execute(FInputActionInstance(PossessAction));
			break;
		}
	}
	Controller->PlayerTick(1.1f);
	Possession->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Holding possession during wind ride reaches the frog"), Controller->GetPawn() == Frog);
	return true;
}

#endif
