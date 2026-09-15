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
	void HandleLook(const FInputActionValue& Value);
	void CreateRuntimeInputMapping();
	void UpdateSharedCameraTarget(APawn* InPawn, bool bSnapToTarget = false);
};
