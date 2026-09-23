#include "PuzzleRoomManager.h"
#include "PuzzleTriggerBase.h"
#include "TPToggleableInterface.h"

APuzzleRoomManager::APuzzleRoomManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void APuzzleRoomManager::BeginPlay()
{
    Super::BeginPlay();

    // ���� ���� ��, �迭�� ��ϵ� ��� ������ '���� ��ġ(Transform)'�� �����մϴ�.
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

        // 1. �������� ���� ���� ����(ť��)���, ���ư��� ���ӵ�(Velocity)�� 0���� ���� ������ŵ�ϴ�.
        if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
        {
            if (RootPrim->IsSimulatingPhysics())
            {
                RootPrim->SetPhysicsLinearVelocity(FVector::ZeroVector);
                RootPrim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
            }
        }

        // 2. ��ġ �ʱ�ȭ (�����ص� ���� �ڸ��� �����̵�)
        if (InitialTransforms.Contains(Actor))
        {
            Actor->SetActorTransform(InitialTransforms[Actor]);
        }

        // 3. ���� �� ���Ͱ� �����̳� ����ġ���? -> ������ ���ϴ�.
        if (APuzzleTriggerBase* Trigger = Cast<APuzzleTriggerBase>(Actor))
        {
            Trigger->ForceReset();
        }

        // 4. Any on/off puzzle actor returns to its off state.
        if (Actor->GetClass()->ImplementsInterface(UTPToggleableInterface::StaticClass()))
        {
            ITPToggleableInterface::Execute_SetToggleableEnabled(Actor, false);
        }
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Magenta, TEXT("���� �ʱ�ȭ�Ǿ����ϴ�!"));
    }
}
