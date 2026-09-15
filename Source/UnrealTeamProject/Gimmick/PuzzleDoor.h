// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleDoor.generated.h"

UCLASS()
class UNREALTEAMPROJECT_API APuzzleDoor : public AActor
{
    GENERATED_BODY()

public:
    APuzzleDoor();

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void OpenDoor();

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void CloseDoor();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* DoorMesh;
};
