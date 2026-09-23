#include "PuzzleTriggerBase.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
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
    if (IsValid(TargetDoor) && TargetDoor->GetClass()->ImplementsInterface(UTPToggleableInterface::StaticClass()))
    {
        ITPToggleableInterface::Execute_SetToggleableEnabled(TargetDoor, true);
    }
}

void APuzzleTriggerBase::DeactivateTrigger()
{
    if (IsValid(TargetDoor) && TargetDoor->GetClass()->ImplementsInterface(UTPToggleableInterface::StaticClass()))
    {
        ITPToggleableInterface::Execute_SetToggleableEnabled(TargetDoor, false);
    }
}

void APuzzleTriggerBase::ForceReset()
{
    ValidOverlappingCount = 0;
    DeactivateTrigger();
}
