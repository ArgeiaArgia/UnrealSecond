#include "PuzzleRotatingPlatform.h"
#include "Components/StaticMeshComponent.h"
#include "Math/UnrealMathUtility.h"

APuzzleRotatingPlatform::APuzzleRotatingPlatform()
{
    PrimaryActorTick.bCanEverTick = true;

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    PlatformMesh->SetMobility(EComponentMobility::Movable); // 회전을 위해 무버블 필수
    RootComponent = PlatformMesh;

    RotateSpeed = 90.0f; // 기본값: 1초에 90도 회전
    TargetRotationOffset = FRotator(0.0f, 90.0f, 0.0f); // 기본값: Z(Yaw)축으로 90도 회전
    bIsRotatingToTarget = false;
}

void APuzzleRotatingPlatform::BeginPlay()
{
    Super::BeginPlay();

    GlobalStartRotation = GetActorRotation();
    // 시작 회전값에 목표 오프셋을 더해 최종 도착 각도를 미리 계산해 둡니다.
    GlobalTargetRotation = GlobalStartRotation + TargetRotationOffset;
}

void APuzzleRotatingPlatform::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FRotator CurrentRotation = GetActorRotation();
    FRotator Destination = bIsRotatingToTarget ? GlobalTargetRotation : GlobalStartRotation;

    // 목표 각도에 도달하지 않았다면
    if (!CurrentRotation.Equals(Destination, 0.1f))
    {
        // RInterpConstantTo는 각도를 등속도로 부드럽게 돌려주는 함수입니다.
        FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, Destination, DeltaTime, RotateSpeed);
        SetActorRotation(NewRotation);
    }
}

void APuzzleRotatingPlatform::ActivatePlatform()
{
    bIsRotatingToTarget = true;
}

void APuzzleRotatingPlatform::DeactivatePlatform()
{
    bIsRotatingToTarget = false;
}

void APuzzleRotatingPlatform::ForceResetPlatform()
{
    bIsRotatingToTarget = false;
    SetActorRotation(GlobalStartRotation); // 제자리로 즉시 텔레포트
}