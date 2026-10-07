#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../TPToggleableInterface.h"
#include "ToggleablePillar.generated.h"

class UStaticMeshComponent;
class UStaticMesh;

/**
 * A pillar that travels between its placed (off) location and a designer-set
 * target location whenever a TPToggleableInterface signal changes state.
 */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AToggleablePillar : public AActor, public ITPToggleableInterface
{
    GENERATED_BODY()

public:
    AToggleablePillar();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    /** Moves the pillar to the position assigned to the on state. */
    UFUNCTION(BlueprintCallable, Category = "Puzzle|Pillar")
    void SetPillarOn(bool bNewIsOn);

    /** Reverses the current on/off state. */
    UFUNCTION(BlueprintCallable, Category = "Puzzle|Pillar")
    void TogglePillar();

    UFUNCTION(BlueprintPure, Category = "Puzzle|Pillar")
    bool IsPillarOn() const { return bIsOn; }

    virtual void SetToggleableEnabled_Implementation(bool bInEnabled) override;
    virtual bool IsToggleableEnabled_Implementation() const override;

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> PillarMesh;

    /** Static Mesh used for the pillar.  Assign any project or Engine mesh here. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Pillar")
    TObjectPtr<UStaticMesh> PillarAsset;

    /** Per-axis scale applied to Pillar Asset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Pillar", meta = (ClampMin = "0.01"))
    FVector PillarScale = FVector(1.2f, 1.2f, 4.0f);

    /**
     * Destination relative to the placed pillar location.  Use the viewport
     * widget to place it freely; a positive Z value raises the pillar.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Pillar", meta = (MakeEditWidget = true))
    FVector OnLocationOffset = FVector(0.0f, 0.0f, 400.0f);

    /** Travel speed in centimetres per second. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Pillar", meta = (ClampMin = "0.0", Units = "cm/s"))
    float MoveSpeed = 250.0f;

    /** Initial state applied when play begins. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Pillar")
    bool bStartsOn = false;

    /** If enabled, On moves to the placed/off position and Off moves to the target position. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Pillar")
    bool bInvertOnOff = false;

private:
    FVector OffWorldLocation = FVector::ZeroVector;
    FVector OnWorldLocation = FVector::ZeroVector;
    bool bIsOn = false;
};
