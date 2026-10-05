#include "UTPTutorialTrigger.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Player/UTPPlayerController.h"
#include "TimerManager.h"

AUTPTutorialTrigger::AUTPTutorialTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 150.0f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AUTPTutorialTrigger::OnTriggerBeginOverlap);

	TutorialMessage = NSLOCTEXT("Tutorial", "PossessionPrompt", "오른쪽 마우스 버튼을 길게 눌러 동물에게 빙의하세요.");
}

void AUTPTutorialTrigger::BeginPlay()
{
	Super::BeginPlay();
	// Initial overlap events can precede possession. Check again after startup
	// so a player spawned inside the volume does not have to leave and re-enter.
	GetWorldTimerManager().SetTimerForNextTick(this, &AUTPTutorialTrigger::CheckInitialOverlaps);
}

void AUTPTutorialTrigger::CheckInitialOverlaps()
{
	TArray<AActor*> OverlappingActors;
	TriggerBox->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		TryActivate(Actor);
	}
}

void AUTPTutorialTrigger::OnTriggerBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComponent*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	TryActivate(OtherActor);
}

void AUTPTutorialTrigger::TryActivate(AActor* OtherActor)
{
	if ((bTriggerOnce && bHasTriggered) || !IsValid(OtherActor))
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	AUTPPlayerController* PlayerController = Pawn ? Cast<AUTPPlayerController>(Pawn->GetController()) : nullptr;
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	bHasTriggered = true;
	if (bHideTutorial)
	{
		PlayerController->HideTutorialMessage();
	}
	else
	{
		PlayerController->ShowTutorialMessage(TutorialMessage);
	}

	if (bTriggerOnce)
	{
		TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
