#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleTriggerBase.generated.h"

class UBoxComponent;
class APuzzleDoor;
class APuzzleMovingPlatform;
class APuzzleRotatingPlatform;
class AAreaForceVolume;

UCLASS()
class UNREALTEAMPROJECT_API APuzzleTriggerBase : public AActor
{
    GENERATED_BODY()

public:
    APuzzleTriggerBase();
    virtual void Tick(float DeltaSeconds) override;

    // Retained for already-authored levels and for any actor implementing the toggle interface.
    UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (MustImplement = "/Script/UnrealTeamProject.TPToggleableInterface"))
    TObjectPtr<AActor> TargetDoor;

    UPROPERTY(EditAnywhere, Category = "Puzzle")
    FName RequiredTag;

    void ForceReset();

protected:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBoxComponent* TriggerBox;

    // Typed arrays are retained for the main-side multi-target puzzle workflow.
    UPROPERTY(EditAnywhere, Category = "Puzzle Link")
    TArray<APuzzleDoor*> TargetDoors;

    UPROPERTY(EditAnywhere, Category = "Puzzle Link")
    TArray<APuzzleMovingPlatform*> TargetPlatforms;

    UPROPERTY(EditAnywhere, Category = "Puzzle Link")
    TArray<APuzzleRotatingPlatform*> TargetRotators;

    UPROPERTY(EditAnywhere, Category = "Puzzle Link")
    TArray<AAreaForceVolume*> TargetWindAreas;

    int32 ValidOverlappingCount;

    void RefreshTriggerState();

    UFUNCTION()
    virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    virtual void ActivateTrigger();
    virtual void DeactivateTrigger();
};
