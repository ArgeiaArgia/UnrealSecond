#include "PuzzleTriggerBase.h"
#include "Gimmick/PuzzleDoor.h" // 실제 문 헤더 파일명에 맞게 수정 필요
#include "Gimmick/PuzzleMovingPlatform.h" // 실제 문 헤더 파일명에 맞게 수정 필요
#include "Gimmick/PuzzleRotatingPlatform.h"
#include "AreaForceVolume.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"

APuzzleTriggerBase::APuzzleTriggerBase()
{
    PrimaryActorTick.bCanEverTick = false;
    ValidOverlappingCount = 0;

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    RootComponent = TriggerBox;

    // 겹침 판정을 위해 동적 오브젝트와 모두 겹치도록 설정
    TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APuzzleTriggerBase::OnOverlapBegin);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APuzzleTriggerBase::OnOverlapEnd);
}

void APuzzleTriggerBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this) return;

    // RequiredTag가 설정되어 있는데 대상이 해당 태그를 갖지 않으면 무시
    if (!RequiredTag.IsNone() && !OtherActor->ActorHasTag(RequiredTag)) return;

    ValidOverlappingCount++;

    // 유효한 첫 번째 액터가 들어올 때 문 개방
    if (ValidOverlappingCount == 1)
    {
        ActivateTrigger();
    }
}

void APuzzleTriggerBase::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor || OtherActor == this) return;

    if (!RequiredTag.IsNone() && !OtherActor->ActorHasTag(RequiredTag)) return;

    ValidOverlappingCount--;

    // 유효한 액터가 모두 나갔을 때 문 폐쇄
    if (ValidOverlappingCount <= 0)
    {
        ValidOverlappingCount = 0; // 음수 방지 안전장치
        DeactivateTrigger();
    }
}

void APuzzleTriggerBase::ActivateTrigger()
{
    // 배열에 등록된 모든 문 열기
    for (APuzzleDoor* Door : TargetDoors)
    {
        if (Door)
        {
            Door->OpenDoor();
        }
    }

    // 배열에 등록된 모든 이동 다리 작동
    for (APuzzleMovingPlatform* Platform : TargetPlatforms)
    {
        if (Platform)
        {
            Platform->ActivatePlatform();
        }
    }

    for (APuzzleRotatingPlatform* Rotator : TargetRotators)
    {
        if (Rotator) Rotator->ActivatePlatform();
    }

    for (AAreaForceVolume* Wind : TargetWindAreas)
    {
        if (Wind) Wind->ActivateWind();
    }
}

void APuzzleTriggerBase::DeactivateTrigger()
{
    // 배열에 등록된 모든 문 닫기
    for (APuzzleDoor* Door : TargetDoors)
    {
        if (Door)
        {
            Door->CloseDoor();
        }
    }

    // 배열에 등록된 모든 이동 다리 복귀
    for (APuzzleMovingPlatform* Platform : TargetPlatforms)
    {
        if (Platform)
        {
            Platform->DeactivatePlatform();
        }
    }

    for (APuzzleRotatingPlatform* Rotator : TargetRotators)
    {
        if (Rotator) Rotator->DeactivatePlatform();
    }

    for (APuzzleRotatingPlatform* Rotator : TargetRotators)
    {
        if (Rotator) Rotator->DeactivatePlatform();
    }
}

void APuzzleTriggerBase::ForceReset()
{
    // 방 초기화 시 카운트를 0으로 되돌리고 문을 닫음
    ValidOverlappingCount = 0;
    DeactivateTrigger();
}