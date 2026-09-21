// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleTriggerBase.generated.h"

class UBoxComponent;
class APuzzleDoor;
class APuzzleMovingPlatform; // 이동 다리 클래스 전방 선언 추가
class APuzzleRotatingPlatform; // 이동 다리 클래스 전방 선언 추가
class AAreaForceVolume; // 이동 다리 클래스 전방 선언 추가

UCLASS()
class UNREALTEAMPROJECT_API APuzzleTriggerBase : public AActor
{
    GENERATED_BODY()

public:
    APuzzleTriggerBase();

    // 트리거를 작동시킬 수 있는 특정 액터 태그 (비워두면 모두 허용)
    UPROPERTY(EditAnywhere, Category = "Puzzle")
    FName RequiredTag;

    // 방 초기화 매니저(PuzzleRoomManager)에서 호출할 강제 초기화 함수
    void ForceReset();

protected:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBoxComponent* TriggerBox;

    UPROPERTY(EditAnywhere, Category = "Puzzle Link")
    TArray<APuzzleDoor*> TargetDoors;

    UPROPERTY(EditAnywhere, Category = "Puzzle Link")
    TArray<APuzzleMovingPlatform*> TargetPlatforms;

    UPROPERTY(EditAnywhere, Category = "Puzzle Link")
    TArray<APuzzleRotatingPlatform*> TargetRotators;

    UPROPERTY(EditAnywhere, Category = "Puzzle Link")
    TArray<AAreaForceVolume*> TargetWindAreas;

    // 트리거 영역 안에 있는 유효한 액터 수
    int32 ValidOverlappingCount;

    UFUNCTION()
    virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // 자식 클래스(발판, 스위치 등)에서 애니메이션 연출을 추가할 수 있도록 virtual 선언
    virtual void ActivateTrigger();
    virtual void DeactivateTrigger();

};