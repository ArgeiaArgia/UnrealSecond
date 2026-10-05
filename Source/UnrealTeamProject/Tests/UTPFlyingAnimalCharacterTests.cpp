#if WITH_DEV_AUTOMATION_TESTS

#include "../Player/Animals/Flying/UTPFlyingAnimalCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FUTPBatRepeatJumpTest,
	"UnrealTeamProject.Flying.RepeatJumpOnUntaggedGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FUTPBatRepeatJumpTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	Names.Add(TEXT("Native"));
	Commands.Add(TEXT("Native"));
	Names.Add(TEXT("FlyingFoxBlueprint"));
	Commands.Add(TEXT("Blueprint"));
}

bool FUTPBatRepeatJumpTest::RunTest(const FString& Parameters)
{
	UClass* BatClass = Parameters == TEXT("Blueprint")
		? LoadClass<AUTPFlyingAnimalCharacter>(nullptr,
			TEXT("/Game/Blueprints/Characters/BP_FlyingFox.BP_FlyingFox_C"))
		: AUTPFlyingAnimalCharacter::StaticClass();
	if (!TestNotNull(TEXT("Bat class loads"), BatClass))
	{
		return false;
	}

	const UWorld::InitializationValues Settings = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false)
		.RequiresHitProxies(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
		nullptr, true, ERHIFeatureLevel::Num, &Settings);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };

	AActor* Ground = World->SpawnActor<AActor>();
	UBoxComponent* GroundCollision = NewObject<UBoxComponent>(Ground);
	Ground->SetRootComponent(GroundCollision);
	GroundCollision->SetBoxExtent(FVector(500.0f, 500.0f, 20.0f));
	GroundCollision->SetCollisionProfileName(TEXT("BlockAll"));
	GroundCollision->RegisterComponent();
	Ground->SetActorLocation(FVector(0.0f, 0.0f, -20.0f));
	TestTrue(TEXT("Ground has no special landing tag"), Ground->Tags.IsEmpty());

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AUTPFlyingAnimalCharacter* Bat = World->SpawnActor<AUTPFlyingAnimalCharacter>(
		BatClass, FVector(0.0f, 0.0f, 45.0f), FRotator::ZeroRotator, SpawnParameters);
	if (!TestNotNull(TEXT("Bat spawns"), Bat))
	{
		return false;
	}
	Bat->OnPossessedBySoul_Implementation(nullptr);
	UCharacterMovementComponent* Movement = Bat->GetCharacterMovement();
	Movement->bRunPhysicsWithNoController = true;
	constexpr float Step = 1.0f / 60.0f;
	const auto TickFlight = [&]()
	{
		Bat->Tick(Step);
		Movement->TickComponent(Step, LEVELTICK_All, nullptr);
	};
	const auto WaitForLanding = [&]()
	{
		for (int32 Frame = 0; Frame < 600 && (Bat->IsFlying() || Movement->IsFalling()); ++Frame)
		{
			TickFlight();
		}
		return Bat->GetFlightState() == EUTPFlightState::Landing && Movement->IsMovingOnGround();
	};

	Bat->StartTakeoff();
	TestTrue(TEXT("First jump takes off"), Bat->IsFlying() && Movement->Velocity.Z > 0.0f);
	for (int32 Frame = 0; Frame < 10; ++Frame)
	{
		TickFlight();
	}
	const double AirborneVelocity = Movement->Velocity.Z;
	Bat->StartTakeoff();
	TestEqual(TEXT("Jump while airborne does not restart takeoff"), Movement->Velocity.Z, AirborneVelocity);
	if (!TestTrue(TEXT("First jump lands on ordinary ground"), WaitForLanding()))
	{
		return false;
	}
	TestTrue(TEXT("Landing does not penetrate the floor"),
		Bat->GetActorLocation().Z >= Bat->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	Bat->StartTakeoff();
	TestTrue(TEXT("Can jump immediately during landing stabilization"),
		Bat->IsFlying() && Movement->Velocity.Z > 0.0f);
	if (!TestTrue(TEXT("Second jump also lands"), WaitForLanding()))
	{
		return false;
	}
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		TickFlight();
	}
	TestTrue(TEXT("Settled landing returns to perched"), Bat->GetFlightState() == EUTPFlightState::Perched);
	Bat->StartTakeoff();
	TestTrue(TEXT("Can jump again after settling"), Bat->IsFlying() && Movement->Velocity.Z > 0.0f);

	// Leaving the final current must use the same gravity as an ordinary
	// character, even when the glide ability was active before wind exit.
	Bat->SetActorLocation(FVector(0.0f, 0.0f, 3000.0f));
	AActor* FirstWind = World->SpawnActor<AActor>();
	AActor* SecondWind = World->SpawnActor<AActor>();
	Bat->EnterWindZone_Implementation(FirstWind, FVector::UpVector, 1000.0f, 0.0f);
	Bat->EnterWindZone_Implementation(SecondWind, FVector::UpVector, 1000.0f, 0.0f);
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		TickFlight();
	}
	TestTrue(TEXT("Wind lifts the bat"), Movement->Velocity.Z > 0.0f);
	Bat->ExitWindZone_Implementation(FirstWind);
	TestTrue(TEXT("Remaining wind continues flight"),
		Bat->IsInsideWindZone() && Movement->Velocity.Z > 0.0f);
	Bat->ActivateAnimalAbility();
	TestTrue(TEXT("Glide ability is active before wind exit"), Bat->IsFlightAbilityActive());
	Bat->ExitWindZone_Implementation(SecondWind);
	TestTrue(TEXT("Final wind exit switches to gravity-driven falling"),
		Movement->IsFalling() && Bat->GetFlightState() == EUTPFlightState::Falling);
	TestTrue(TEXT("Wind exit cancels residual upward lift"), Movement->Velocity.Z <= 0.0f);
	TestFalse(TEXT("Wind exit cancels the glide ability"), Bat->IsFlightAbilityActive());
	Bat->ActivateAnimalAbility();
	TestFalse(TEXT("Glide ability cannot slow the fall"), Bat->IsFlightAbilityActive());

	ACharacter* Reference = World->SpawnActor<ACharacter>(
		FVector(200.0f, 0.0f, Bat->GetActorLocation().Z), FRotator::ZeroRotator, SpawnParameters);
	UCharacterMovementComponent* ReferenceMovement = Reference->GetCharacterMovement();
	ReferenceMovement->bRunPhysicsWithNoController = true;
	ReferenceMovement->GravityScale = 1.0f;
	ReferenceMovement->SetMovementMode(MOVE_Falling);
	const double WindExitHeight = Bat->GetActorLocation().Z;
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		TickFlight();
		ReferenceMovement->TickComponent(Step, LEVELTICK_All, nullptr);
	}
	TestTrue(TEXT("Actor ticks preserve falling movement mode"), Movement->IsFalling());
	TestTrue(TEXT("Fall accelerates under gravity"), Movement->Velocity.Z < -400.0f);
	TestTrue(TEXT("Bat falls at the same speed as an ordinary character"),
		FMath::IsNearlyEqual(Movement->Velocity.Z, ReferenceMovement->Velocity.Z, 0.5));
	TestTrue(TEXT("Bat drops the same distance as an ordinary character"),
		FMath::IsNearlyEqual(Bat->GetActorLocation().Z, Reference->GetActorLocation().Z, 0.5));
	TestTrue(TEXT("Gravity moves the bat downward"), Bat->GetActorLocation().Z < WindExitHeight);
	Reference->Destroy();
	Bat->EnterWindZone_Implementation(FirstWind, FVector::UpVector, 1000.0f, 0.0f);
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		TickFlight();
	}
	TestTrue(TEXT("Reentering wind restores ascent"), Movement->Velocity.Z > 0.0f);
	Bat->ExitWindZone_Implementation(FirstWind);
	if (!TestTrue(TEXT("Gravity-driven fall lands"), WaitForLanding()))
	{
		return false;
	}
	Bat->StartTakeoff();
	TestTrue(TEXT("Can jump after gravity-driven fall"), Bat->IsFlying() && Movement->Velocity.Z > 0.0f);
	Bat->SetActorLocation(FVector(0.0f, 0.0f, 3000.0f));
	Movement->Velocity.Z = 0.0f;
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		TickFlight();
	}
	TestTrue(TEXT("Next ordinary jump uses slow descent again"),
		Movement->Velocity.Z < -50.0f && Movement->Velocity.Z > -150.0f);
	return true;
}

#endif
