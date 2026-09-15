// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../PuzzleTriggerBase.h" // 변경: AActor 대신 새로 만든 부모 클래스 포함
#include "PressurePlate.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlateStateChanged);

class APuzzleDoor;

UCLASS()
class UNREALTEAMPROJECT_API APressurePlate : public APuzzleTriggerBase
{
	GENERATED_BODY()
	
public:
    APressurePlate();

    // TargetDoor는 부모 클래스에 이미 있으므로 삭제했습니다.

    UPROPERTY(BlueprintAssignable, Category = "Puzzle Events")
    FOnPlateStateChanged OnPlateActivated;

    UPROPERTY(BlueprintAssignable, Category = "Puzzle Events")
    FOnPlateStateChanged OnPlateDeactivated;

protected:
    // TriggerBox와 OnOverlap 함수들은 부모가 대신 처리하므로 삭제했습니다.

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* PlateMesh;

    // 부모의 함수를 덮어씌워서 발판만의 기능(이벤트 발생, 연출)을 추가합니다.
    virtual void ActivateTrigger() override;
    virtual void DeactivateTrigger() override;
};
