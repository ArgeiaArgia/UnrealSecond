#include "TutorialTrigger.h"

#include "Components/BoxComponent.h"
#include "Player/UTPPlayerController.h"

ATutorialTrigger::ATutorialTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 150.0f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ATutorialTrigger::OnTriggerBeginOverlap);

	TutorialMessage = NSLOCTEXT("Tutorial", "PossessionPrompt", "오른쪽 마우스 버튼을 길게 눌러 동물에게 빙의하세요.");
}

void ATutorialTrigger::OnTriggerBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComponent*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if ((bTriggerOnce && bHasTriggered) || !IsValid(OtherActor))
	{
		return;
	}

	AUTPPlayerController* PlayerController = Cast<AUTPPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PlayerController || PlayerController->GetPawn() != OtherActor)
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
