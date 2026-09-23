#include "PuzzleTriggerBase.h"

#include "AreaForceVolume.h"
#include "Gimmick/PuzzleDoor.h"
#include "Gimmick/PuzzleMovingPlatform.h"
#include "Gimmick/PuzzleRotatingPlatform.h"
#include "Components/BoxComponent.h"
#include "TPToggleableInterface.h"

APuzzleTriggerBase::APuzzleTriggerBase()
{
    PrimaryActorTick.bCanEverTick = true;
    ValidOverlappingCount = 0;

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    RootComponent = TriggerBox;
    TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APuzzleTriggerBase::OnOverlapBegin);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APuzzleTriggerBase::OnOverlapEnd);
}

void APuzzleTriggerBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshTriggerState();
}

void APuzzleTriggerBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    RefreshTriggerState();
}

void APuzzleTriggerBase::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    RefreshTriggerState();
}

void APuzzleTriggerBase::RefreshTriggerState()
{
    if (!TriggerBox)
    {
        return;
    }

    TArray<AActor*> OverlappingActors;
    TriggerBox->GetOverlappingActors(OverlappingActors);

    int32 NewValidOverlappingCount = 0;
    for (AActor* OtherActor : OverlappingActors)
    {
        if (!IsValid(OtherActor) || OtherActor == this)
        {
            continue;
        }

        if (!RequiredTag.IsNone() && !OtherActor->ActorHasTag(RequiredTag))
        {
            continue;
        }

        ++NewValidOverlappingCount;
    }

    const bool bWasActivated = ValidOverlappingCount > 0;
    const bool bShouldActivate = NewValidOverlappingCount > 0;
    ValidOverlappingCount = NewValidOverlappingCount;

    if (!bWasActivated && bShouldActivate)
    {
        ActivateTrigger();
    }
    else if (bWasActivated && !bShouldActivate)
    {
        DeactivateTrigger();
    }
}

void APuzzleTriggerBase::ActivateTrigger()
{
    TSet<AActor*> HandledTargets;

    for (APuzzleDoor* Door : TargetDoors)
    {
        if (IsValid(Door))
        {
            Door->OpenDoor();
            HandledTargets.Add(Door);
        }
    }

    for (APuzzleMovingPlatform* Platform : TargetPlatforms)
    {
        if (IsValid(Platform))
        {
            Platform->ActivatePlatform();
            HandledTargets.Add(Platform);
        }
    }

    for (APuzzleRotatingPlatform* Rotator : TargetRotators)
    {
        if (IsValid(Rotator))
        {
            Rotator->ActivatePlatform();
            HandledTargets.Add(Rotator);
        }
    }

    for (AAreaForceVolume* Wind : TargetWindAreas)
    {
        if (IsValid(Wind))
        {
            Wind->ActivateWind();
            HandledTargets.Add(Wind);
        }
    }

    AActor* GenericTarget = TargetDoor.Get();
    if (IsValid(GenericTarget) && !HandledTargets.Contains(GenericTarget) && GenericTarget->GetClass()->ImplementsInterface(UTPToggleableInterface::StaticClass()))
    {
        ITPToggleableInterface::Execute_SetToggleableEnabled(GenericTarget, true);
    }
}

void APuzzleTriggerBase::DeactivateTrigger()
{
    TSet<AActor*> HandledTargets;

    for (APuzzleDoor* Door : TargetDoors)
    {
        if (IsValid(Door))
        {
            Door->CloseDoor();
            HandledTargets.Add(Door);
        }
    }

    for (APuzzleMovingPlatform* Platform : TargetPlatforms)
    {
        if (IsValid(Platform))
        {
            Platform->DeactivatePlatform();
            HandledTargets.Add(Platform);
        }
    }

    for (APuzzleRotatingPlatform* Rotator : TargetRotators)
    {
        if (IsValid(Rotator))
        {
            Rotator->DeactivatePlatform();
            HandledTargets.Add(Rotator);
        }
    }

    for (AAreaForceVolume* Wind : TargetWindAreas)
    {
        if (IsValid(Wind))
        {
            Wind->DeactivateWind();
            HandledTargets.Add(Wind);
        }
    }

    AActor* GenericTarget = TargetDoor.Get();
    if (IsValid(GenericTarget) && !HandledTargets.Contains(GenericTarget) && GenericTarget->GetClass()->ImplementsInterface(UTPToggleableInterface::StaticClass()))
    {
        ITPToggleableInterface::Execute_SetToggleableEnabled(GenericTarget, false);
    }
}

void APuzzleTriggerBase::ForceReset()
{
    ValidOverlappingCount = 0;
    DeactivateTrigger();
}
