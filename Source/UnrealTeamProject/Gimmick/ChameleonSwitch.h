// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../PuzzleTriggerBase.h"
#include "ChameleonSwitch.generated.h"

UCLASS()
class UNREALTEAMPROJECT_API AChameleonSwitch : public APuzzleTriggerBase
{
	GENERATED_BODY()
	
public:
    AChameleonSwitch();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* SwitchMesh;

    // ī�᷹�� ����ġ ������ ����/���� ������ ���� �Լ� �����
    virtual void ActivateTrigger() override;
    virtual void DeactivateTrigger() override;

};
