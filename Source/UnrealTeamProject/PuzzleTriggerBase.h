#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleTriggerBase.generated.h"

class UBoxComponent;

UCLASS()
class UNREALTEAMPROJECT_API APuzzleTriggerBase : public AActor
{
    GENERATED_BODY()

public:
    APuzzleTriggerBase();

    virtual void Tick(float DeltaSeconds) override;

    // Existing door references remain valid, while any Toggleable actor can now be assigned.
    UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (MustImplement = "/Script/UnrealTeamProject.TPToggleableInterface"))
    TObjectPtr<AActor> TargetDoor;

    UPROPERTY(EditAnywhere, Category = "Puzzle")
    FName RequiredTag;

    void ForceReset();

protected:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBoxComponent* TriggerBox;

    int32 ValidOverlappingCount;

    // Rebuilds activation from the actual overlap set when an overlap event is missed.
    void RefreshTriggerState();

    UFUNCTION()
    virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    virtual void ActivateTrigger();
    virtual void DeactivateTrigger();
};
