#include "ChameleonSwitch.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

AChameleonSwitch::AChameleonSwitch()
{
    SwitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwitchMesh"));
    SwitchMesh->SetupAttachment(TriggerBox);
}

void AChameleonSwitch::ActivateTrigger()
{
    Super::ActivateTrigger();

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("SwtichOn"));
    }
}

void AChameleonSwitch::DeactivateTrigger()
{
    // 중요: 카멜레온의 혀(투사체)는 스위치를 통과한 뒤 파괴됩니다.
    // 기본 발판처럼 물체가 나갔다고 해서 스위치가 바로 꺼지면 안 되므로, 
    // Super::DeactivateTrigger(); 를 호출하지 않고 비워둡니다.
    // 이렇게 하면 R키(방 리셋 매니저)를 누르기 전까지 문이 열린 상태로 영구 유지됩니다.
}