#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleLever.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeverStateChanged, bool, bIsOn);

/**
 * A two-state puzzle lever. Actors in OnStateTargets are enabled while the
 * lever is on, and actors in OffStateTargets are enabled while it is off.
 * Every target must implement TPToggleableInterface.
 */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API APuzzleLever : public AActor
{
    GENERATED_BODY()

public:
    APuzzleLever();

    virtual void BeginPlay() override;

    /** Reverses the lever state and applies it to all assigned targets. */
    UFUNCTION(BlueprintCallable, Category = "Puzzle|Lever")
    void ToggleLever();

    /** Sets the lever state explicitly. Useful for Blueprint events and resets. */
    UFUNCTION(BlueprintCallable, Category = "Puzzle|Lever")
    void SetLeverOn(bool bNewIsOn);

    UFUNCTION(BlueprintPure, Category = "Puzzle|Lever")
    bool IsLeverOn() const { return bIsOn; }

    /** Fired after the target objects have received their new enabled state. */
    UPROPERTY(BlueprintAssignable, Category = "Puzzle|Lever")
    FOnLeverStateChanged OnLeverStateChanged;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> LeverMesh;

    /** Objects enabled when this lever is on, and disabled when it is off. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Lever", meta = (MustImplement = "/Script/UnrealTeamProject.TPToggleableInterface"))
    TArray<TObjectPtr<AActor>> OnStateTargets;

    /** Objects disabled when this lever is on, and enabled when it is off. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Lever", meta = (MustImplement = "/Script/UnrealTeamProject.TPToggleableInterface"))
    TArray<TObjectPtr<AActor>> OffStateTargets;

    /** State applied when play begins. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Lever")
    bool bStartsOn = false;

    /** Allows this actor to work immediately without an additional interaction-input system. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Lever")
    bool bToggleOnOverlap = true;

    /** Leave empty to allow every actor; otherwise only actors with this tag can operate the lever. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Lever")
    FName RequiredTag;

    UFUNCTION()
    void OnTriggerBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnTriggerEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex);

private:
    void ApplyStateToTargets();
    bool CanOperate(AActor* OtherActor) const;

    bool bIsOn = false;
    TSet<TWeakObjectPtr<AActor>> ActorsCurrentlyOperating;
};
