#pragma once

#include "../../UTPPossessableCharacter.h"
#include "UTPFrogCharacter.generated.h"

UENUM(BlueprintType)
enum class EUTPFrogState : uint8
{
	Grounded UMETA(DisplayName="Grounded"),
	ChargingJump UMETA(DisplayName="Charging Jump"),
	Airborne UMETA(DisplayName="Airborne"),
	ShallowWater UMETA(DisplayName="Shallow Water"),
	Landing UMETA(DisplayName="Landing")
};

/** Possessable frog focused on charge jumps and shallow-water traversal. */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPFrogCharacter : public AUTPPossessableCharacter
{
	GENERATED_BODY()

public:
	AUTPFrogCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Move(const FInputActionValue& Value) override;
	virtual void StartJump() override;
	virtual void StopJump() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnPossessedBySoul_Implementation(APawn* SoulPawn) override;
	virtual void OnReleasedFromSoul_Implementation(APawn* SoulPawn) override;
	virtual bool CanReleaseFromSoul_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category="Animal|Frog|Jump")
	void BeginJumpCharge();

	UFUNCTION(BlueprintCallable, Category="Animal|Frog|Jump")
	void UpdateJumpCharge();

	UFUNCTION(BlueprintCallable, Category="Animal|Frog|Jump")
	void ReleaseJump();

	UFUNCTION(BlueprintCallable, Category="Animal|Frog|Water")
	void EnterShallowWater();

	UFUNCTION(BlueprintCallable, Category="Animal|Frog|Water")
	void ExitShallowWater();

	UFUNCTION(BlueprintCallable, Category="Animal|Frog|Water")
	void SetShallowWaterMoveSpeed(float NewMoveSpeed);

	UFUNCTION(BlueprintCallable, Category="Animal|Frog|Water")
	void ResetShallowWaterMoveSpeed();

	void EnterShallowWaterFromZone(AActor* WaterSource, float NewMoveSpeed);
	void ExitShallowWaterFromZone(AActor* WaterSource);

	UFUNCTION(BlueprintPure, Category="Animal|Frog")
	EUTPFrogState GetFrogState() const { return FrogState; }

	UFUNCTION(BlueprintPure, Category="Animal|Frog")
	bool IsChargingJump() const { return FrogState == EUTPFrogState::ChargingJump; }

	UFUNCTION(BlueprintPure, Category="Animal|Frog")
	bool IsInShallowWater() const { return bIsInShallowWater; }

	UFUNCTION(BlueprintPure, Category="Animal|Frog")
	float GetJumpChargeRatio() const { return JumpChargeRatio; }

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Frog")
	EUTPFrogState FrogState = EUTPFrogState::Grounded;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Frog", meta=(ClampMin="0.0"))
	float MinJumpVelocity = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Frog", meta=(ClampMin="0.0"))
	float MaxJumpVelocity = 1100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Frog", meta=(ClampMin="0.01"))
	float MaxJumpChargeTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Frog", meta=(ClampMin="0.0"))
	float WaterMoveSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Frog", meta=(ClampMin="0.0"))
	float GroundMoveSpeed = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Frog", meta=(ClampMin="0.0"))
	float JumpHorizontalSpeed = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Frog", meta=(ClampMin="0.0"))
	float FrogAirControl = 0.15f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Frog")
	float JumpChargeRatio = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Frog")
	bool bIsInShallowWater = false;

	UPROPERTY(Transient)
	float ShallowWaterMoveSpeedOverride = -1.0f;

	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<AActor>, float> ActiveWaterSources;

private:
	float JumpChargeStartTime = 0.0f;

	void ResetJumpCharge();
	void ApplyMoveSpeed();
	void RebuildWaterState();
};
