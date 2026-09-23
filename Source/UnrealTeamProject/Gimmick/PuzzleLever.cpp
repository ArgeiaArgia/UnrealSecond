#include "PuzzleLever.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../TPToggleableInterface.h"

APuzzleLever::APuzzleLever()
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    SetRootComponent(TriggerBox);
    TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APuzzleLever::OnTriggerBeginOverlap);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APuzzleLever::OnTriggerEndOverlap);

    LeverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeverMesh"));
    LeverMesh->SetupAttachment(TriggerBox);
    LeverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APuzzleLever::BeginPlay()
{
    Super::BeginPlay();

    bIsOn = bStartsOn;
    ApplyStateToTargets();
}

void APuzzleLever::ToggleLever()
{
    SetLeverOn(!bIsOn);
}

void APuzzleLever::SetLeverOn(bool bNewIsOn)
{
    if (bIsOn == bNewIsOn)
    {
        return;
    }

    bIsOn = bNewIsOn;
    ApplyStateToTargets();
    OnLeverStateChanged.Broadcast(bIsOn);
}

void APuzzleLever::OnTriggerBeginOverlap(
    UPrimitiveComponent* /*OverlappedComponent*/,
    AActor* OtherActor,
    UPrimitiveComponent* /*OtherComponent*/,
    int32 /*OtherBodyIndex*/,
    bool /*bFromSweep*/,
    const FHitResult& /*SweepResult*/)
{
    if (!bToggleOnOverlap || !CanOperate(OtherActor))
    {
        return;
    }

    if (!ActorsCurrentlyOperating.Contains(OtherActor))
    {
        ActorsCurrentlyOperating.Add(OtherActor);
        ToggleLever();
    }
}

void APuzzleLever::OnTriggerEndOverlap(
    UPrimitiveComponent* /*OverlappedComponent*/,
    AActor* OtherActor,
    UPrimitiveComponent* /*OtherComponent*/,
    int32 /*OtherBodyIndex*/)
{
    ActorsCurrentlyOperating.Remove(OtherActor);
}

void APuzzleLever::ApplyStateToTargets()
{
    const auto ApplyTargets = [this](const TArray<TObjectPtr<AActor>>& Targets, bool bEnabled)
    {
        for (AActor* Target : Targets)
        {
            if (IsValid(Target) && Target->GetClass()->ImplementsInterface(UTPToggleableInterface::StaticClass()))
            {
                ITPToggleableInterface::Execute_SetToggleableEnabled(Target, bEnabled);
            }
        }
    };

    ApplyTargets(OnStateTargets, bIsOn);
    ApplyTargets(OffStateTargets, !bIsOn);
}

bool APuzzleLever::CanOperate(AActor* OtherActor) const
{
    return IsValid(OtherActor)
        && OtherActor != this
        && (RequiredTag.IsNone() || OtherActor->ActorHasTag(RequiredTag));
}
