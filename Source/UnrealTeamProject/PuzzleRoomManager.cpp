#include "PuzzleRoomManager.h"
#include "PuzzleTriggerBase.h"
#include "Gimmick/PuzzleDoor.h"

APuzzleRoomManager::APuzzleRoomManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void APuzzleRoomManager::BeginPlay()
{
    Super::BeginPlay();

    // 게임 시작 시, 배열에 등록된 모든 액터의 '최초 위치(Transform)'를 저장합니다.
    for (AActor* Actor : ResettableActors)
    {
        if (Actor)
        {
            InitialTransforms.Add(Actor, Actor->GetActorTransform());
        }
    }
}

void APuzzleRoomManager::ResetRoom()
{
    for (AActor* Actor : ResettableActors)
    {
        if (!Actor) continue;

        // 1. 물리력이 적용 중인 상자(큐브)라면, 날아가던 가속도(Velocity)를 0으로 강제 정지시킵니다.
        if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
        {
            if (RootPrim->IsSimulatingPhysics())
            {
                RootPrim->SetPhysicsLinearVelocity(FVector::ZeroVector);
                RootPrim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
            }
        }

        // 2. 위치 초기화 (저장해둔 원래 자리로 순간이동)
        if (InitialTransforms.Contains(Actor))
        {
            Actor->SetActorTransform(InitialTransforms[Actor]);
        }

        // 3. 만약 이 액터가 발판이나 스위치라면? -> 강제로 끕니다.
        if (APuzzleTriggerBase* Trigger = Cast<APuzzleTriggerBase>(Actor))
        {
            Trigger->ForceReset();
        }

        // 4. 만약 이 액터가 문이라면? -> 강제로 닫습니다.
        if (APuzzleDoor* Door = Cast<APuzzleDoor>(Actor))
        {
            Door->CloseDoor();
        }
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Magenta, TEXT("방이 초기화되었습니다!"));
    }
}