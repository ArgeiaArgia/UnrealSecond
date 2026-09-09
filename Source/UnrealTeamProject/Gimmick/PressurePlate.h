// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PressurePlate.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlateStateChanged);

class APuzzleDoor;

UCLASS()
class UNREALTEAMPROJECT_API APressurePlate : public AActor
{
	GENERATED_BODY()
	
public:
    APressurePlate();

    UPROPERTY(EditAnywhere, Category = "Puzzle")
    APuzzleDoor* TargetDoor;

    UPROPERTY(BlueprintAssignable, Category = "Puzzle Events")
    FOnPlateStateChanged OnPlateActivated;

    UPROPERTY(BlueprintAssignable, Category = "Puzzle Events")
    FOnPlateStateChanged OnPlateDeactivated;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* TriggerBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* PlateMesh;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
