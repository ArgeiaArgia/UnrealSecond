// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../TPToggleableInterface.h"
#include "PuzzleDoor.generated.h"

UCLASS()
class UNREALTEAMPROJECT_API APuzzleDoor : public AActor, public ITPToggleableInterface
{
    GENERATED_BODY()

public:
    APuzzleDoor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void OpenDoor();

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void CloseDoor();

    virtual void SetToggleableEnabled_Implementation(bool bInEnabled) override;
    virtual bool IsToggleableEnabled_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* DoorMesh;

    // 닫힌 상태를 기준으로 문이 열릴 때 회전할 Yaw 각도입니다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Door", meta = (ClampMin = "-180.0", ClampMax = "180.0", Units = "deg"))
    float OpenYawAngle = 90.0f;

    // 문이 목표 각도에 도달하는 속도입니다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Door", meta = (ClampMin = "0.0"))
    float RotationSpeed = 2.5f;

private:
    FRotator ClosedRotation = FRotator::ZeroRotator;
    bool bIsOpen = false;
};
