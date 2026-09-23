#include "AreaForceVolume.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AAreaForceVolume::AAreaForceVolume()
{
    PrimaryActorTick.bCanEverTick = true; // �� ������ ���� ���ؾ� �ϹǷ� ƽ Ȱ��ȭ

    VolumeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("VolumeBox"));
    RootComponent = VolumeBox;
    VolumeBox->SetCollisionProfileName(TEXT("Trigger"));

    PushDirection = FVector(1.0f, 0.0f, 0.0f);
    PushStrength = 2000.0f;
    TargetTag = NAME_None;

    VolumeBox->OnComponentBeginOverlap.AddDynamic(this, &AAreaForceVolume::OnOverlapBegin);
    VolumeBox->OnComponentEndOverlap.AddDynamic(this, &AAreaForceVolume::OnOverlapEnd);
}

void AAreaForceVolume::BeginPlay()
{
    Super::BeginPlay();
    PushDirection.Normalize(); // ���� ���� ����ȭ (���̸� 1�� ����)
}

void AAreaForceVolume::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bEnabled)
    {
        return;
    }

    for (AActor* Actor : AffectedActors)
    {
        if (IsValid(Actor))
        {
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
                    // �� ��° ���� true�� ������ ������ �����ϴ� ���ӵ�(Acceleration) ��尡 �Ǿ� ������ �и��� �˴ϴ�.
                    PrimitiveComp->AddForce(PushDirection * PushStrength, NAME_None, true);
                }
            }
        }
    }
}

void AAreaForceVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this) return;

    if (!TargetTag.IsNone() && !OtherActor->ActorHasTag(TargetTag)) return;

    AffectedActors.Add(OtherActor);

    // ����� ����� �νĵǾ����� ȭ�� ���� ��ܿ� �Ķ��� �α׸� ���ϴ�.
    if (GEngine)
    {
        FString Msg = FString::Printf(TEXT("�ٶ�/�޷� ���� �νĵ�: %s"), *OtherActor->GetName());
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, Msg);
    }
}

void AAreaForceVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor || OtherActor == this) return;

    if (AffectedActors.Contains(OtherActor))
    {
        AffectedActors.Remove(OtherActor);
    }
}

void AAreaForceVolume::SetToggleableEnabled_Implementation(bool bInEnabled)
{
    bEnabled = bInEnabled;
}

bool AAreaForceVolume::IsToggleableEnabled_Implementation() const
{
    return bEnabled;
}
