#pragma once

#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "UTPPossessableInterface.h"
#include "UTPPossessableCharacter.generated.h"

class UAnimInstance;
class UInputAction;

UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPPossessableCharacter : public ACharacter, public ITPPossessableInterface
{
	GENERATED_BODY()

public:
	AUTPPossessableCharacter();

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual bool CanBePossessed_Implementation(AController* InstigatingController) const override;
	virtual void OnPossessionFocusChanged_Implementation(bool bIsFocused) override;
	virtual void OnPossessedBySoul_Implementation(APawn* SoulPawn) override;
	virtual void OnReleasedFromSoul_Implementation(APawn* SoulPawn) override;
	virtual bool CanReleaseFromSoul_Implementation() const override;

	UFUNCTION(BlueprintPure, Category="Possession")
	bool IsSoulPossessed() const;

	UFUNCTION(BlueprintPure, Category="Possession")
	bool IsCurrentlyPossessed() const;

	UFUNCTION(BlueprintPure, Category="Possession")
	APawn* GetCurrentSoulPawn() const;

	UFUNCTION(BlueprintPure, Category="Possession")
	bool IsPossessionTransitionInputLocked() const;

	UFUNCTION(BlueprintCallable, Category="Animal|Movement")
	virtual void Move(const FInputActionValue& Value);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Possession")
	bool bAllowSoulPossession = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Possession")
	bool bUseCustomDepthFocus = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Possession", meta=(ClampMin="0"))
	int32 FocusCustomDepthStencilValue = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Possession")
	bool bDisableMovementWhenUnpossessed = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TSubclassOf<UAnimInstance> AnimalAnimClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Animal")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Animal")
	TObjectPtr<UInputAction> JumpAction;

	virtual void StartJump();
	virtual void StopJump();

private:
	UPROPERTY(Transient)
	TObjectPtr<APawn> CurrentSoulPawn;

	UPROPERTY(Transient)
	bool bIsSoulPossessed = false;

	UPROPERTY(Transient)
	TEnumAsByte<EMovementMode> CachedMovementMode = MOVE_Walking;

	UPROPERTY(Transient)
	uint8 CachedCustomMovementMode = 0;

	UPROPERTY(Transient)
	bool bHasCachedMovementMode = false;

	void UpdateFocusVisuals(bool bIsFocused);
	void ApplyPossessedMovementState();
	void ApplyReleasedMovementState();
	void ApplyAnimInstanceClass();
};
