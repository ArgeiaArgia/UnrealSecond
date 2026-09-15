// Fill out your copyright notice in the Description page of Project Settings.


#include "TempGrate.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ATempGrate::ATempGrate()
{
    PrimaryActorTick.bCanEverTick = false;

    BlockBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockBox"));
    RootComponent = BlockBox;

    GrateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrateMesh"));
    GrateMesh->SetupAttachment(RootComponent);
    GrateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 기본적으로 모든 것을 막음 (Pawn 포함)
    BlockBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BlockBox->SetCollisionObjectType(ECC_WorldStatic);
    BlockBox->SetCollisionResponseToAllChannels(ECR_Block);
    BlockBox->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // 영혼(Soul) 채널만 무시하여 통과하도록 설정
    BlockBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
}


