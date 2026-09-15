// Fill out your copyright notice in the Description page of Project Settings.


#include "PuzzleDoor.h"


APuzzleDoor::APuzzleDoor()
{
    PrimaryActorTick.bCanEverTick = false;

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    RootComponent = DoorMesh;
}

void APuzzleDoor::OpenDoor()
{
    // 메쉬를 화면에서 숨기고, 충돌 판정을 완전히 끕니다.
    DoorMesh->SetVisibility(false);
    DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("DoorOpen"));
    }
}

void APuzzleDoor::CloseDoor()
{
    // 메쉬를 다시 화면에 보여주고, 충돌 판정을 켭니다.
    DoorMesh->SetVisibility(true);
    DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("문 닫힘 (Mesh 표시됨)"));
    }
}
