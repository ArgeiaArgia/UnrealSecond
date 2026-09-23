#pragma once

#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "UTPPlayerController.generated.h"

class APawn;
class AUTPSharedCameraRig;
class UTPPossessionComponent;
class UInputAction;
class UInputMappingContext;

UCLASS()
class UNREALTEAMPROJECT_API AUTPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AUTPPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintCallable, Category="Possession")
	bool TryPossessTarget(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category="Possession")
	bool TogglePossession();

	void BeginPossessionCameraTransition(APawn* TargetPawn, float Duration);
	void CancelPossessionCameraTransition();
	bool IsPossessionCameraTransitionActive() const;
	void PreserveSharedCameraAngle();

	UFUNCTION(BlueprintPure, Category="Possession")
	UTPPossessionComponent* GetPossessionComponent() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTPPossessionComponent> PossessionComponent;

	UPROPERTY(Transient)
	TObjectPtr<AUTPSharedCameraRig> SharedCameraRig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> PossessTargetAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> LookAction;

private:
	void HandlePossessionPressed();
	void HandlePossessionReleased();
	void HandleLook(const FInputActionValue& Value);
	void UpdatePossessionAim(float DeltaTime);
	AActor* FindPossessionAimTarget() const;
	void CreateRuntimeInputMapping();
	void UpdateSharedCameraTarget(APawn* InPawn, bool bSnapToTarget = false);

	// Right mouse button is held while selecting a target. The target comes from
	// the SoulPawn's camera-facing possession sweep and is committed on release.
	TWeakObjectPtr<AActor> PossessionAimTarget;
	bool bPossessionAimActive = false;

	UPROPERTY(EditDefaultsOnly, Category="Possession|Aim", meta=(ClampMin="0.0"))
	float PossessionAimRotationInterpSpeed = 7.0f;

	UPROPERTY(EditDefaultsOnly, Category="Possession|Aim", meta=(ClampMin="0.0"))
	float PossessionAimTraceDistance = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category="Possession|Aim", meta=(ClampMin="0.0"))
	float PossessionAimTraceRadius = 150.0f;
};
