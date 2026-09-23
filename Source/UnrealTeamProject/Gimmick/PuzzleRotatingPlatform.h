#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleRotatingPlatform.generated.h"

UCLASS()
class UNREALTEAMPROJECT_API APuzzleRotatingPlatform : public AActor
{
    GENERATED_BODY()

public:
    APuzzleRotatingPlatform();
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Puzzle Platform")
    void ActivatePlatform();

    UFUNCTION(BlueprintCallable, Category = "Puzzle Platform")
    void DeactivatePlatform();

    // 방 초기화 매니저용 강제 리셋
    UFUNCTION(BlueprintCallable, Category = "Puzzle Platform")
    void ForceResetPlatform();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* PlatformMesh;

    // 현재 각도에서 얼마나 더 회전할 것인지 (예: Z축 90도 회전 -> Yaw 90.0)
    UPROPERTY(EditAnywhere, Category = "Puzzle Platform")
    FRotator TargetRotationOffset;

    // 회전 속도
    UPROPERTY(EditAnywhere, Category = "Puzzle Platform")
    float RotateSpeed;

private:
    FRotator GlobalStartRotation;
    FRotator GlobalTargetRotation;
    bool bIsRotatingToTarget;
};