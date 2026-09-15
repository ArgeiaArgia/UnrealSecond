#include "UTPPressurePlate.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "UTPWeightProviderInterface.h"

AUTPPressurePlate::AUTPPressurePlate()
{
	PrimaryActorTick.bCanEverTick = true;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(100.0f, 100.0f, 25.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
}

void AUTPPressurePlate::BeginPlay()
{
	Super::BeginPlay();

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AUTPPressurePlate::OnTriggerBeginOverlap);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AUTPPressurePlate::OnTriggerEndOverlap);
	RefreshPressedState();
}

void AUTPPressurePlate::Tick(float /*DeltaSeconds*/)
{
	RefreshPressedState();
}

void AUTPPressurePlate::RefreshPressedState()
{
	if (!TriggerVolume)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	TriggerVolume->GetOverlappingActors(OverlappingActors);

	float TotalWeight = 0.0f;
	for (AActor* Actor : OverlappingActors)
	{
		if (!IsValid(Actor) || !Actor->GetClass()->ImplementsInterface(UTPWeightProviderInterface::StaticClass()))
		{
			continue;
		}

		if (!ITPWeightProviderInterface::Execute_CanActivateWeightPlate(Actor))
		{
			continue;
		}

		TotalWeight += ITPWeightProviderInterface::Execute_GetWeight(Actor);
	}

	const bool bNewPressedState = TotalWeight >= RequiredWeight;
	if (bIsPressed == bNewPressedState)
	{
		return;
	}

	bIsPressed = bNewPressedState;
	OnPressedChanged.Broadcast(bIsPressed);
}

void AUTPPressurePlate::OnTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* /*OtherActor*/,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	RefreshPressedState();
}

void AUTPPressurePlate::OnTriggerEndOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* /*OtherActor*/,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	RefreshPressedState();
}
