#pragma once

#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "UTPPlayerController.generated.h"

class APawn;
class AUTPSharedCameraRig;
class UTPPossessionComponent;
class UUTPPossessionProgressWidget;
class UUTPTutorialWidget;
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

	/** Replaces the currently shown tutorial prompt. Intended for TutorialTrigger actors and level Blueprints. */
	UFUNCTION(BlueprintCallable, Category="Tutorial")
	void ShowTutorialMessage(const FText& Message);

	UFUNCTION(BlueprintCallable, Category="Tutorial")
	void HideTutorialMessage();

	void BeginPossessionCameraTransition(APawn* TargetPawn, float Duration);
	void CancelPossessionCameraTransition();
	bool IsPossessionCameraTransitionActive() const;
	void PreserveSharedCameraAngle();

	UFUNCTION(BlueprintPure, Category="Possession")
	UTPPossessionComponent* GetPossessionComponent() const;

	// Finds the possessable Pawn closest to the center of the active camera view.
	// Candidates must be visible on screen, within MaxDistance of the controlled
	// Pawn, and have a clear path from that Pawn.
	AActor* FindCameraPossessionTarget(const APawn* SearchOriginPawn, float MaxDistance) const;

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
	void UpdatePossessionFocus();
	void UpdatePossessionAim(float DeltaTime);
	void CreatePossessionProgressWidget();
	void CreateTutorialWidget();
	void UpdatePossessionProgressWidget(AActor* TargetActor, float Progress);
	void HidePossessionProgressWidget();
	AActor* FindPossessionAimTarget() const;
	void SetPossessionFocusTarget(AActor* NewTarget);
	void SetPossessionAimZoomEnabled(bool bEnabled);
	void CreateRuntimeInputMapping();
	void UpdateSharedCameraTarget(APawn* InPawn, bool bSnapToTarget = false);

	// Right mouse button is held while selecting a target. The target is selected
	// from the active camera view and is committed after the hold duration.
	TWeakObjectPtr<AActor> PossessionAimTarget;

	// The Soul updates this through its own tick.  Controlled characters use
	// this cached target so their right-click hold commits the same focused
	// character instead of selecting a different candidate at the last frame.
	TWeakObjectPtr<AActor> PossessionFocusTarget;

	UPROPERTY(Transient)
	TObjectPtr<UUTPPossessionProgressWidget> PossessionProgressWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUTPTutorialWidget> TutorialWidget;

	/** Prompt shown automatically when the level starts. Clear this in the controller Blueprint to disable it. */
	UPROPERTY(EditDefaultsOnly, Category="Tutorial", meta=(MultiLine="true"))
	FText InitialTutorialMessage;

	bool bPossessionAimActive = false;
	float PossessionAimHoldElapsed = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="Possession|Aim", meta=(ClampMin="0.0"))
	float PossessionAimRotationInterpSpeed = 7.0f;

	UPROPERTY(EditDefaultsOnly, Category="Possession|Aim", meta=(ClampMin="0.0"))
	float PossessionAimTraceDistance = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category="Possession|Aim", meta=(ClampMin="0.0"))
	float PossessionAimHoldDuration = 1.0f;

	// Matches the SoulPawn debug trace while a character is controlled.
	UPROPERTY(EditDefaultsOnly, Category="Possession|Debug")
	bool bDrawCharacterPossessionTrace = true;

};
