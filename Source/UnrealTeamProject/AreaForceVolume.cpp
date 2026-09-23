#include "AreaForceVolume.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AAreaForceVolume::AAreaForceVolume()
{
    PrimaryActorTick.bCanEverTick = true;

    VolumeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("VolumeBox"));
    RootComponent = VolumeBox;
    VolumeBox->SetCollisionProfileName(TEXT("Trigger"));

    PushDirection = FVector(0.0f, 0.0f, 1.0f);
    PushStrength = 2000.0f;
    TargetTag = NAME_None;
    IgnoreTag = TEXT("Turtle");

    VolumeBox->OnComponentBeginOverlap.AddDynamic(this, &AAreaForceVolume::OnOverlapBegin);
    VolumeBox->OnComponentEndOverlap.AddDynamic(this, &AAreaForceVolume::OnOverlapEnd);
}

void AAreaForceVolume::BeginPlay()
{
    Super::BeginPlay();

    PushDirection.Normalize();
    // Both persisted settings may exist in old levels; either one can start the volume disabled.
    bIsActive = bStartActive;
}

void AAreaForceVolume::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bEnabled || !bIsActive)
    {
        return;
    }

    for (AActor* Actor : AffectedActors)
    {
        if (!IsValid(Actor))
        {
            continue;
        }

        if (ACharacter* Character = Cast<ACharacter>(Actor))
        {
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            {
                Movement->AddForce(PushDirection * PushStrength);
            }
        }
        else if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
        {
            if (PrimitiveComp->IsSimulatingPhysics())
            {
                PrimitiveComp->AddForce(PushDirection * PushStrength, NAME_None, true);
            }
        }
    }
}

void AAreaForceVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    if (!TargetTag.IsNone() && !OtherActor->ActorHasTag(TargetTag))
    {
        return;
    }

    if (!IgnoreTag.IsNone() && OtherActor->ActorHasTag(IgnoreTag))
    {
        return;
    }

    AffectedActors.Add(OtherActor);

    if (GEngine)
    {
        const FString Msg = FString::Printf(TEXT("바람/물살 영역 인식됨: %s"), *OtherActor->GetName());
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, Msg);
    }
}

void AAreaForceVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor && OtherActor != this)
    {
        AffectedActors.Remove(OtherActor);
    }
}

void AAreaForceVolume::SetToggleableEnabled_Implementation(bool bInEnabled)
{
    bEnabled = bInEnabled;
    bIsActive = bInEnabled;
}

bool AAreaForceVolume::IsToggleableEnabled_Implementation() const
{
    return bEnabled && bIsActive;
}

void AAreaForceVolume::ActivateWind()
{
    SetToggleableEnabled_Implementation(true);
}

void AAreaForceVolume::DeactivateWind()
{
    SetToggleableEnabled_Implementation(false);
}

void AAreaForceVolume::ForceResetWind()
{
    SetToggleableEnabled_Implementation(bStartActive);
}
