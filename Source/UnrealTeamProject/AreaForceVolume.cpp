#include "AreaForceVolume.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AAreaForceVolume::AAreaForceVolume()
{
    PrimaryActorTick.bCanEverTick = true; // 매 프레임 힘을 가해야 하므로 틱 활성화

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
    PushDirection.Normalize(); // 방향 벡터 정규화 (길이를 1로 맞춤)
}

void AAreaForceVolume::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

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
                    // 세 번째 인자 true를 넣으면 질량을 무시하는 가속도(Acceleration) 모드가 되어 무조건 밀리게 됩니다.
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

    // 대상이 제대로 인식되었는지 화면 좌측 상단에 파란색 로그를 띄웁니다.
    if (GEngine)
    {
        FString Msg = FString::Printf(TEXT("바람/급류 구역 인식됨: %s"), *OtherActor->GetName());
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