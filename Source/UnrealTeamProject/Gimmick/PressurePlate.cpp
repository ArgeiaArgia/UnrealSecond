#include "PressurePlate.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h" // 부모의 TriggerBox를 사용하기 위해 포함
#include "PuzzleMovingPlatform.h"

APressurePlate::APressurePlate()
{
    PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
    // 부모 클래스가 가지고 있는 TriggerBox를 찾아 그 아래에 메쉬를 붙여줍니다.
    PlateMesh->SetupAttachment(TriggerBox);
}

void APressurePlate::ActivateTrigger()
{
    // 부모 클래스(PuzzleTriggerBase)의 문 열기 & 다리 이동 배열 작동 로직을 호출
    Super::ActivateTrigger();

    // 블루프린트용 이벤트 (필요하다면 유지)
    OnPlateActivated.Broadcast();
}

void APressurePlate::DeactivateTrigger()
{
    // 부모 클래스의 문 닫기 & 다리 복귀 로직 호출
    Super::DeactivateTrigger();

    OnPlateDeactivated.Broadcast();
}