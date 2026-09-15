#include "UTPWaterZone.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "../Animals/Frog/UTPFrogCharacter.h"

AUTPWaterZone::AUTPWaterZone()
{
	PrimaryActorTick.bCanEverTick = true;

	WaterVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("WaterVolume"));
	SetRootComponent(WaterVolume);
	WaterVolume->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));
	WaterVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WaterVolume->SetCollisionObjectType(ECC_WorldDynamic);
	WaterVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	WaterVolume->SetGenerateOverlapEvents(true);
}

void AUTPWaterZone::BeginPlay()
{
	Super::BeginPlay();

	WaterVolume->OnComponentBeginOverlap.AddDynamic(this, &AUTPWaterZone::OnWaterVolumeBeginOverlap);
	WaterVolume->OnComponentEndOverlap.AddDynamic(this, &AUTPWaterZone::OnWaterVolumeEndOverlap);
}

void AUTPWaterZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsEnabled)
	{
		const TArray<TWeakObjectPtr<AActor>> ActorsToRemove = ActiveActors.Array();
		for (const TWeakObjectPtr<AActor>& Actor : ActorsToRemove)
		{
			RemoveWaterFromActor(Actor.Get());
		}

		ActiveActors.Empty();
		return;
	}

	if (!bRefreshOverlapsWhileEnabled)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	WaterVolume->GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		ApplyWaterToActor(Actor);
	}
}

bool AUTPWaterZone::CanTraverseWater_Implementation(AActor* Actor) const
{
	return bIsEnabled && bAllowFrogTraversal && IsValid(Actor) && Actor->ActorHasTag(FName(TEXT("Frog")));
}

void AUTPWaterZone::EnterWater_Implementation(AActor* Actor)
{
	if (!CanTraverseWater_Implementation(Actor))
	{
		return;
	}

	if (AUTPFrogCharacter* Frog = Cast<AUTPFrogCharacter>(Actor))
	{
		Frog->EnterShallowWaterFromZone(this, WaterMoveSpeed);
		ActiveActors.Add(Actor);
	}
}

void AUTPWaterZone::ExitWater_Implementation(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		ActiveActors.Remove(Actor);
		return;
	}

	if (AUTPFrogCharacter* Frog = Cast<AUTPFrogCharacter>(Actor))
	{
		Frog->ExitShallowWaterFromZone(this);
	}

	ActiveActors.Remove(Actor);
}

void AUTPWaterZone::SetWaterEnabled(bool bInEnabled)
{
	if (bIsEnabled == bInEnabled)
	{
		return;
	}

	bIsEnabled = bInEnabled;
	if (!bIsEnabled)
	{
		const TArray<TWeakObjectPtr<AActor>> ActorsToRemove = ActiveActors.Array();
		for (const TWeakObjectPtr<AActor>& Actor : ActorsToRemove)
		{
			RemoveWaterFromActor(Actor.Get());
		}

		ActiveActors.Empty();
	}
}

void AUTPWaterZone::OnWaterVolumeBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	ApplyWaterToActor(OtherActor);
}

void AUTPWaterZone::OnWaterVolumeEndOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	RemoveWaterFromActor(OtherActor);
}

void AUTPWaterZone::ApplyWaterToActor(AActor* OtherActor)
{
	if (!CanTraverseWater_Implementation(OtherActor))
	{
		return;
	}

	EnterWater_Implementation(OtherActor);
}

void AUTPWaterZone::RemoveWaterFromActor(AActor* OtherActor)
{
	if (!OtherActor || !ActiveActors.Contains(OtherActor))
	{
		return;
	}

	ExitWater_Implementation(OtherActor);
}
