#include "PressurePlate.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h" // 부모의 TriggerBox를 사용하기 위해 포함

APressurePlate::APressurePlate()
{
    PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
    // 부모 클래스가 가지고 있는 TriggerBox를 찾아 그 아래에 메쉬를 붙여줍니다.
    PlateMesh->SetupAttachment(TriggerBox);
}

void APressurePlate::ActivateTrigger()
{
    // 1. 부모의 기본 로직 (문 열기) 실행
    Super::ActivateTrigger();

    // 2. 블루프린트에 만들어둔 이벤트 발생
    OnPlateActivated.Broadcast();

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Plate Push"));
    }
}

void APressurePlate::DeactivateTrigger()
{
    // 1. 부모의 기본 로직 (문 닫기) 실행
    Super::DeactivateTrigger();

    // 2. 블루프린트에 만들어둔 이벤트 발생
    OnPlateDeactivated.Broadcast();

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Plate UP"));
    }
}