#include "UTPWindZone.h"

#include "Components/BoxComponent.h"
#include "UTPWindReceiverInterface.h"

AUTPWindZone::AUTPWindZone()
{
	PrimaryActorTick.bCanEverTick = true;

	WindVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WindVolume"));
	SetRootComponent(WindVolume);
	WindVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WindVolume->SetCollisionObjectType(ECC_WorldDynamic);
	WindVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	WindVolume->SetGenerateOverlapEvents(true);
}

void AUTPWindZone::BeginPlay()
{
	Super::BeginPlay();

	WindVolume->OnComponentBeginOverlap.AddDynamic(this, &AUTPWindZone::OnWindVolumeBeginOverlap);
	WindVolume->OnComponentEndOverlap.AddDynamic(this, &AUTPWindZone::OnWindVolumeEndOverlap);
}

void AUTPWindZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bEnabled)
	{
		const TArray<TWeakObjectPtr<AActor>> ReceiversToRemove = ActiveReceivers.Array();
		for (const TWeakObjectPtr<AActor>& Receiver : ReceiversToRemove)
		{
			RemoveWindFromActor(Receiver.Get());
		}

		ActiveReceivers.Empty();
		return;
	}

	if (!bRefreshOverlapsWhileEnabled)
	{
		return;
	}

	// This also handles a zone being enabled while a receiver is already inside it.
	TArray<AActor*> OverlappingActors;
	WindVolume->GetOverlappingActors(OverlappingActors);
	for (AActor* OtherActor : OverlappingActors)
	{
		ApplyWindToActor(OtherActor);
	}
}

void AUTPWindZone::SetWindEnabled(bool bInEnabled)
{
	if (bEnabled == bInEnabled)
	{
		return;
	}

	bEnabled = bInEnabled;
	if (!bEnabled)
	{
		const TArray<TWeakObjectPtr<AActor>> ReceiversToRemove = ActiveReceivers.Array();
		for (const TWeakObjectPtr<AActor>& Receiver : ReceiversToRemove)
		{
			RemoveWindFromActor(Receiver.Get());
		}

		ActiveReceivers.Empty();
	}
}

void AUTPWindZone::SetToggleableEnabled_Implementation(bool bInEnabled)
{
	SetWindEnabled(bInEnabled);
}

bool AUTPWindZone::IsToggleableEnabled_Implementation() const
{
	return IsWindEnabled();
}

void AUTPWindZone::OnWindVolumeBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ApplyWindToActor(OtherActor);
}

void AUTPWindZone::OnWindVolumeEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	RemoveWindFromActor(OtherActor);
}

void AUTPWindZone::ApplyWindToActor(AActor* OtherActor)
{
	if (!bEnabled || !IsValid(OtherActor) ||
		!OtherActor->GetClass()->ImplementsInterface(UTPWindReceiverInterface::StaticClass()))
	{
		return;
	}

	const FVector Direction = WindDirection.GetSafeNormal();
	ITPWindReceiverInterface::Execute_EnterWindZone(
		OtherActor,
		this,
		Direction,
		WindSpeed,
		LiftStrength);
	ActiveReceivers.Add(OtherActor);
}

void AUTPWindZone::RemoveWindFromActor(AActor* OtherActor)
{
	if (!IsValid(OtherActor) ||
		!OtherActor->GetClass()->ImplementsInterface(UTPWindReceiverInterface::StaticClass()))
	{
		return;
	}

	ITPWindReceiverInterface::Execute_ExitWindZone(OtherActor, this);
	ActiveReceivers.Remove(OtherActor);
}
