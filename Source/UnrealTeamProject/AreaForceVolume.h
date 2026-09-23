// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPToggleableInterface.h"
#include "AreaForceVolume.generated.h"

class UBoxComponent;

UCLASS()
class UNREALTEAMPROJECT_API AAreaForceVolume : public AActor, public ITPToggleableInterface
{
    GENERATED_BODY()

public:
    AAreaForceVolume();
    virtual void Tick(float DeltaTime) override;

    virtual void SetToggleableEnabled_Implementation(bool bInEnabled) override;
    virtual bool IsToggleableEnabled_Implementation() const override;

    // �о ���� (���� ��ǥ ����)
    UPROPERTY(EditAnywhere, Category = "Force Settings")
    FVector PushDirection;

    // �о�� ���� ����
    UPROPERTY(EditAnywhere, Category = "Force Settings")
    float PushStrength;

    // ������ ���� ������ �±� (����θ� ��� �о)
    UPROPERTY(EditAnywhere, Category = "Force Settings")
    FName TargetTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Force Settings")
    bool bEnabled = true;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBoxComponent* VolumeBox;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    // ���� �ȿ� ���� �ִ� ��ȿ�� ���͵��� ����ϴ� �迭
    UPROPERTY()
    TSet<AActor*> AffectedActors;
};
