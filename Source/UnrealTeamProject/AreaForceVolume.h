// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AreaForceVolume.generated.h"

class UBoxComponent;

UCLASS()
class UNREALTEAMPROJECT_API AAreaForceVolume : public AActor
{
    GENERATED_BODY()

public:
    AAreaForceVolume();
    virtual void Tick(float DeltaTime) override;

    // 밀어낼 방향 (월드 좌표 기준)
    UPROPERTY(EditAnywhere, Category = "Force Settings")
    FVector PushDirection;

    // 밀어내는 힘의 세기
    UPROPERTY(EditAnywhere, Category = "Force Settings")
    float PushStrength;

    // 영향을 받을 액터의 태그 (비워두면 모두 밀어냄)
    UPROPERTY(EditAnywhere, Category = "Force Settings")
    FName TargetTag;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBoxComponent* VolumeBox;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    // 영역 안에 들어와 있는 유효한 액터들을 기억하는 배열
    UPROPERTY()
    TSet<AActor*> AffectedActors;
};
