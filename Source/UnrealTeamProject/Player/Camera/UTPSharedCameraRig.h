#pragma once

#include "GameFramework/Actor.h"
#include "UTPSharedCameraRig.generated.h"

class APlayerController;
class APawn;
class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

UCLASS()
class UNREALTEAMPROJECT_API AUTPSharedCameraRig : public AActor
{
	GENERATED_BODY()

public:
	AUTPSharedCameraRig();

	virtual void Tick(float DeltaSeconds) override;

	void Initialize(APlayerController* InOwningPlayerController);
	void SetFollowTarget(APawn* InFollowTarget, bool bSnapToTarget = false);
	void BeginPossessionTransition(APawn* InTransitionTarget, float InDuration);
	void CancelPossessionTransition();
	bool IsPossessionTransitionActive() const;
	FRotator GetCameraRigRotation() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
	FVector FollowOffset = FVector(0.0f, 0.0f, 60.0f);

	// Keep this disabled by default so the viewed pawn stays centered while moving.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
	bool bEnableFollowLag = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(EditCondition="bEnableFollowLag", ClampMin="0.0"))
	float FollowLagSpeed = 20.0f;

	// The persistent shared rig eases to ordinary follow targets as well, so
	// changing bodies never looks like swapping to another camera.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(ClampMin="0.0"))
	float FollowLocationInterpSpeed = 18.0f;

private:
	TWeakObjectPtr<APlayerController> OwningPlayerController;
	TWeakObjectPtr<APawn> FollowTarget;
	TWeakObjectPtr<APawn> PossessionTransitionTarget;
	FVector PossessionTransitionStartLocation = FVector::ZeroVector;
	float PossessionTransitionElapsed = 0.0f;
	float PossessionTransitionDuration = 0.0f;
	bool bPossessionTransitionActive = false;
};
