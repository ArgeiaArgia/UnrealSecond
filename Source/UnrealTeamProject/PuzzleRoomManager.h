// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleRoomManager.generated.h"

UCLASS()
class UNREALTEAMPROJECT_API APuzzleRoomManager : public AActor
{
	GENERATED_BODY()
	
public:
    APuzzleRoomManager();

    // 에디터에서 스포이트로 지정할 '이 방에서 초기화할 대상들' (상자, 문, 발판 등 모두 가능)
    UPROPERTY(EditAnywhere, Category = "Puzzle Reset")
    TArray<AActor*> ResettableActors;

    // 언제든 호출할 수 있는 방 초기화 함수
    UFUNCTION(BlueprintCallable, Category = "Puzzle Reset")
    void ResetRoom();

protected:
    virtual void BeginPlay() override;

private:
    // 시작할 때의 위치, 회전, 크기(Transform)를 기억해둘 Map 자료구조
    TMap<AActor*, FTransform> InitialTransforms;
};
