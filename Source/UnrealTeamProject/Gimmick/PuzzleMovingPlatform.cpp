#include "PuzzleMovingPlatform.h"
#include "Components/StaticMeshComponent.h"

APuzzleMovingPlatform::APuzzleMovingPlatform()
{
    PrimaryActorTick.bCanEverTick = true;

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    PlatformMesh->SetMobility(EComponentMobility::Movable);
    RootComponent = PlatformMesh;

    MoveSpeed = 200.0f;
    TargetLocation = FVector(0.0f, 0.0f, 500.0f); // 기본값: 위로 5m 이동
    bIsMovingToTarget = false;
}

void APuzzleMovingPlatform::BeginPlay()
{
    Super::BeginPlay();

    // 시작할 때의 원래 위치를 기억해 둡니다.
    GlobalStartLocation = GetActorLocation();

    // TargetLocation은 로컬 좌표(자기 자신 기준)이므로, 월드 좌표로 변환해 줍니다.
    GlobalTargetLocation = GetTransform().TransformPosition(TargetLocation);
}

void APuzzleMovingPlatform::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector CurrentLocation = GetActorLocation();

    // 스위치가 켜졌으면 타겟 위치로, 꺼졌으면 시작 위치로 방향을 정합니다.
    FVector Destination = bIsMovingToTarget ? GlobalTargetLocation : GlobalStartLocation;

    // 현재 위치가 목적지에 도달하지 않았다면 계속 이동시킵니다.
    if (!CurrentLocation.Equals(Destination, 1.0f))
    {
        // VInterpConstantTo는 등속도로 지정된 위치까지 부드럽게 이동시키는 함수입니다.
        FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, Destination, DeltaTime, MoveSpeed);
        SetActorLocation(NewLocation);
    }
}

void APuzzleMovingPlatform::ActivatePlatform()
{
    bIsMovingToTarget = true;
}

void APuzzleMovingPlatform::DeactivatePlatform()
{
    bIsMovingToTarget = false;
}