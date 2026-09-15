#pragma once

#include "Components/ActorComponent.h"
#include "UTPPossessionComponent.generated.h"

class AActor;
class AController;
class APawn;
class AUTPSoulPawn;
class AUTPPlayerController;

UCLASS(ClassGroup=(Player), meta=(BlueprintSpawnableComponent))
class UNREALTEAMPROJECT_API UTPPossessionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTPPossessionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="Possession")
	void SetSoulPawn(APawn* InSoulPawn);

	void HandleControllerPossessed(APawn* InPawn);
	void HandleControllerUnpossessed(APawn* PreviousPawn);

	UFUNCTION(BlueprintCallable, Category="Possession")
	bool TryPossessTarget(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category="Possession")
	bool TogglePossession();

	UFUNCTION(BlueprintCallable, Category="Possession")
	bool ReturnToSoul();

	UFUNCTION(BlueprintCallable, Category="Possession")
	bool ReturnToStoredBody();

	UFUNCTION(BlueprintCallable, Category="Possession")
	void BeginSoulWindow(APawn* InSoulPawn, APawn* InStoredBody);

	UFUNCTION(BlueprintCallable, Category="Possession")
	void ClearSoulWindow();

	UFUNCTION(BlueprintPure, Category="Possession")
	APawn* GetSoulPawn() const;

	UFUNCTION(BlueprintPure, Category="Possession")
	APawn* GetStoredBodyPawn() const;

	UFUNCTION(BlueprintPure, Category="Possession")
	bool IsSoulWindowActive() const;

	UFUNCTION(BlueprintPure, Category="Possession")
	float GetRemainingSoulTime() const;

	UFUNCTION(BlueprintPure, Category="Possession")
	bool IsPossessionTransitionInProgress() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Possession", meta=(ClampMin="0.0"))
	float SoulWindowSeconds = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Possession")
	bool bEnableSoulWindowTimeout = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Possession|Transition", meta=(ClampMin="0.0"))
	float PossessionTransitionSeconds = 0.45f;

	// Keeps the Soul visible above the released body while the camera eases to it.
	// Set this to zero to spawn exactly at the body's root location.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Possession|Transition")
	FVector SoulReleaseSpawnOffset = FVector(0.0f, 0.0f, 120.0f);

private:
	UPROPERTY(Transient)
	TObjectPtr<APawn> SoulPawn;

	// Keep the original Blueprint class so releasing possession recreates the
	// same SoulPawn visual and settings at the released body's location.
	UPROPERTY(Transient)
	TSubclassOf<AUTPSoulPawn> SoulPawnClass;

	UPROPERTY(Transient)
	TObjectPtr<APawn> StoredBodyPawn;

	TWeakObjectPtr<APawn> PendingPossessionTarget;

	TWeakObjectPtr<APawn> PendingReleasedBody;

	float RemainingSoulTime = 0.0f;
	float RemainingPossessionTransitionTime = 0.0f;
	bool bSoulWindowActive = false;
	bool bPossessionTransitionInProgress = false;
	bool bPossessionCinematicActive = false;
	bool bPossessionCinematicReturnsToSoul = false;

	AUTPPlayerController* GetOwningPlayerController() const;
	APawn* GetCurrentControlledPawn() const;
	bool PossessPawn(APawn* NewPawn);
	AUTPSoulPawn* SpawnSoulPawn(const FTransform& SpawnTransform);
	void DestroySoulPawn();
	bool BeginPossessionTransition(APawn* TargetPawn);
	void CompletePossessionTransition();
	bool BeginSoulReleaseTransition(APawn* PreviousBody, AUTPSoulPawn* NewSoulPawn);
	void CompleteSoulReleaseTransition();
	void CancelPossessionTransition();
};
