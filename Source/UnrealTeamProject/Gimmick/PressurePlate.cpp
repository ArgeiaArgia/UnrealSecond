// Fill out your copyright notice in the Description page of Project Settings.


#include "PressurePlate.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PuzzleDoor.h"

APressurePlate::APressurePlate()
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    RootComponent = TriggerBox;

    PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
    PlateMesh->SetupAttachment(RootComponent);
}

void APressurePlate::BeginPlay()
{
    Super::BeginPlay();

    // 시작할 때 겹침 이벤트 함수 연결
    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APressurePlate::OnOverlapBegin);
    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APressurePlate::OnOverlapEnd);
}

void APressurePlate::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor) return;

    // 조건 없이 무조건 작동 신호 발송
    if (TargetDoor)
    {
        TargetDoor->OpenDoor();
    }
}

void APressurePlate::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor) return;

    // 에디터에서 스포이트로 찍어둔 문(TargetDoor)이 존재한다면 직접 닫아라!
    if (TargetDoor)
    {
        TargetDoor->CloseDoor();
    }
}
