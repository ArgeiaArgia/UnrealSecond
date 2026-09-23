#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleMovingPlatform.generated.h"

UCLASS()
class UNREALTEAMPROJECT_API APuzzleMovingPlatform : public AActor
{
    GENERATED_BODY()

public:
    APuzzleMovingPlatform();

    virtual void Tick(float DeltaTime) override;

    // 스위치나 발판에서 호출하여 이동을 시작/복귀시키는 함수
    UFUNCTION(BlueprintCallable, Category = "Puzzle Platform")
    void ActivatePlatform();

    UFUNCTION(BlueprintCallable, Category = "Puzzle Platform")
    void DeactivatePlatform();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* PlatformMesh;

    // 에디터 뷰포트에서 마우스로 직접 끌어서 목표 위치를 정할 수 있게 해주는 마법의 태그
    UPROPERTY(EditAnywhere, meta = (MakeEditWidget = true), Category = "Puzzle Platform")
    FVector TargetLocation;

    // 플랫폼 이동 속도
    UPROPERTY(EditAnywhere, Category = "Puzzle Platform")
    float MoveSpeed;

private:
    FVector GlobalStartLocation;
    FVector GlobalTargetLocation;
    bool bIsMovingToTarget;
};