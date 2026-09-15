// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TempGrate.generated.h"

UCLASS()
class UNREALTEAMPROJECT_API ATempGrate : public AActor
{
    GENERATED_BODY()

public:
    ATempGrate();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* BlockBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* GrateMesh;

};
