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

    // 트리거(발판/스위치)에서 호출할 On/Off 함수
    UFUNCTION(BlueprintCallable, Category = "Puzzle Wind")
    void ActivateWind();

    UFUNCTION(BlueprintCallable, Category = "Puzzle Wind")
    void DeactivateWind();

    // 방 초기화 매니저용 강제 리셋
    UFUNCTION(BlueprintCallable, Category = "Puzzle Wind")
    void ForceResetWind();

    UPROPERTY(EditAnywhere, Category = "Force Settings")
    FVector PushDirection;

    UPROPERTY(EditAnywhere, Category = "Force Settings")
    float PushStrength;

    UPROPERTY(EditAnywhere, Category = "Force Settings")
    FName TargetTag;

    // 새롭게 추가: 영향을 받지 않고 무시할 액터 태그 (예: Turtle)
    UPROPERTY(EditAnywhere, Category = "Force Settings")
    FName IgnoreTag;

    // 게임 시작 시 바람이 켜져 있을지 여부
    UPROPERTY(EditAnywhere, Category = "Force Settings")
    bool bStartActive;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBoxComponent* VolumeBox;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    UPROPERTY()
    TSet<AActor*> AffectedActors;

    bool bIsActive;
};