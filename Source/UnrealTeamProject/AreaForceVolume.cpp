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

    PushDirection = FVector(0.0f, 0.0f, 1.0f); // 바람이므로 기본값을 위쪽(Z축)으로 설정
    PushStrength = 2000.0f;
    TargetTag = NAME_None;
    IgnoreTag = TEXT("Turtle"); // 기본값으로 거북이를 무시하도록 설정
    bStartActive = true;

    VolumeBox->OnComponentBeginOverlap.AddDynamic(this, &AAreaForceVolume::OnOverlapBegin);
    VolumeBox->OnComponentEndOverlap.AddDynamic(this, &AAreaForceVolume::OnOverlapEnd);
}

void AAreaForceVolume::BeginPlay()
{
    Super::BeginPlay();
    PushDirection.Normalize();
    bIsActive = bStartActive; // 시작 상태 적용
}

void AAreaForceVolume::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 스위치가 꺼져있으면 아무 힘도 주지 않고 종료
    if (!bIsActive) return;

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
                    PrimitiveComp->AddForce(PushDirection * PushStrength, NAME_None, true);
                }
            }
        }
    }
}

void AAreaForceVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this) return;

    // 타겟 태그가 설정되어 있는데 안 맞으면 무시
    if (!TargetTag.IsNone() && !OtherActor->ActorHasTag(TargetTag)) return;

    // 무시할 태그(거북이)를 가지고 있으면 배열에 추가하지 않고 무시
    if (!IgnoreTag.IsNone() && OtherActor->ActorHasTag(IgnoreTag)) return;

    AffectedActors.Add(OtherActor);

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

void AAreaForceVolume::ActivateWind()
{
    bIsActive = true;
}

void AAreaForceVolume::DeactivateWind()
{
    bIsActive = false;
}

void AAreaForceVolume::ForceResetWind()
{
    // 방 초기화 시 원래의 시작 상태로 되돌림
    bIsActive = bStartActive;
}