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

    // Generic puzzle-controller API retained for existing actors and Blueprints.
    virtual void SetToggleableEnabled_Implementation(bool bInEnabled) override;
    virtual bool IsToggleableEnabled_Implementation() const override;

    // Wind-specific API retained for existing trigger and Blueprint references.
    UFUNCTION(BlueprintCallable, Category = "Puzzle Wind")
    void ActivateWind();

    UFUNCTION(BlueprintCallable, Category = "Puzzle Wind")
    void DeactivateWind();

    UFUNCTION(BlueprintCallable, Category = "Puzzle Wind")
    void ForceResetWind();

    UPROPERTY(EditAnywhere, Category = "Force Settings")
    FVector PushDirection;

    UPROPERTY(EditAnywhere, Category = "Force Settings")
    float PushStrength;

    UPROPERTY(EditAnywhere, Category = "Force Settings")
    FName TargetTag;

    UPROPERTY(EditAnywhere, Category = "Force Settings")
    FName IgnoreTag;

    // Preserves the generic toggle state used by the GH-side puzzle system.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Force Settings")
    bool bEnabled = true;

    // Preserves the wind area's initial state used by the main-side puzzle system.
    UPROPERTY(EditAnywhere, Category = "Force Settings")
    bool bStartActive = true;

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

    // Kept in sync with bEnabled by both public activation APIs.
    bool bIsActive = true;
};
