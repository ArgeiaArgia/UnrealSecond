// Fill out your copyright notice in the Description page of Project Settings.

#include "PuzzleDoor.h"

#include "Components/StaticMeshComponent.h"

APuzzleDoor::APuzzleDoor()
{
    PrimaryActorTick.bCanEverTick = true;

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    RootComponent = DoorMesh;
}

void APuzzleDoor::BeginPlay()
{
    Super::BeginPlay();
    ClosedRotation = GetActorRotation();
}

void APuzzleDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const FRotator TargetRotation = bIsOpen
        ? ClosedRotation + FRotator(0.0f, OpenYawAngle, 0.0f)
        : ClosedRotation;
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, RotationSpeed));
}

void APuzzleDoor::OpenDoor()
{
	if (bIsOpen)
	{
		return;
	}

    bIsOpen = true;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Door Open"));
    }
}

void APuzzleDoor::CloseDoor()
{
	if (!bIsOpen)
	{
		return;
	}

    bIsOpen = false;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Door Close"));
    }
}

void APuzzleDoor::SetToggleableEnabled_Implementation(bool bInEnabled)
{
	if (bInEnabled)
	{
		OpenDoor();
	}
	else
	{
		CloseDoor();
	}
}

bool APuzzleDoor::IsToggleableEnabled_Implementation() const
{
	return bIsOpen;
}
