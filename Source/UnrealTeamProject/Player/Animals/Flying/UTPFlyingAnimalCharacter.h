#pragma once

#include "../../Puzzles/UTPWindReceiverInterface.h"
#include "../../UTPPossessableCharacter.h"
#include "UTPFlyingAnimalCharacter.generated.h"

class UInputAction;

USTRUCT()
struct FUTPWindSourceState
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Direction = FVector::ZeroVector;

	UPROPERTY()
	float Speed = 0.0f;

	UPROPERTY()
	float Lift = 0.0f;
};

UENUM(BlueprintType)
enum class EUTPFlightState : uint8
{
	Perched UMETA(DisplayName="Perched"),
	Takeoff UMETA(DisplayName="Takeoff"),
	WindRide UMETA(DisplayName="Wind Ride"),
	Glide UMETA(DisplayName="Glide"),
	Landing UMETA(DisplayName="Landing")
};

/** Wind-dependent flying animal used by the FlyingFox asset. */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPFlyingAnimalCharacter
	: public AUTPPossessableCharacter
	, public ITPWindReceiverInterface
{
	GENERATED_BODY()

public:
	AUTPFlyingAnimalCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Move(const FInputActionValue& Value) override;
	virtual void StartJump() override;
	virtual void OnPossessedBySoul_Implementation(APawn* SoulPawn) override;
	virtual void OnReleasedFromSoul_Implementation(APawn* SoulPawn) override;
	virtual bool CanReleaseFromSoul_Implementation() const override;
	virtual void EnterWindZone_Implementation(
		AActor* WindSource,
		FVector WindDirection,
		float WindSpeed,
		float LiftStrength) override;
	virtual void ExitWindZone_Implementation(AActor* WindSource) override;

	UFUNCTION(BlueprintPure, Category="Animal|Flying")
	EUTPFlightState GetFlightState() const { return FlightState; }

	UFUNCTION(BlueprintPure, Category="Animal|Flying")
	bool IsFlying() const;

	UFUNCTION(BlueprintPure, Category="Animal|Flying")
	bool IsGliding() const;

	UFUNCTION(BlueprintPure, Category="Animal|Flying")
	bool IsInsideWindZone() const { return bIsInsideWindZone; }

	UFUNCTION(BlueprintCallable, Category="Animal|Flying")
	void StartTakeoff();

	UFUNCTION(BlueprintCallable, Category="Animal|Flying")
	void ActivateAnimalAbility();

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Flying")
	EUTPFlightState FlightState = EUTPFlightState::Perched;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Flying", meta=(ClampMin="0.0"))
	float WindRideSpeed = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Flying", meta=(ClampMin="0.0"))
	float MaxLateralSpeed = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Flying", meta=(ClampMin="0.0"))
	float GlideSinkSpeed = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Flying", meta=(ClampMin="0.0"))
	float TakeoffSpeed = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Flying", meta=(ClampMin="0.0"))
	float WindAcceleration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Flying", meta=(ClampMin="0.0"))
	float LandingStabilizationTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Flying")
	bool bRequireLandingTag = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Flying")
	FName LandingPlatformTag = TEXT("BatLanding");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Flying", meta=(ClampMin="0.0"))
	float AbilityGlideDuration = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Flying", meta=(ClampMin="0.0", ClampMax="1.0"))
	float AbilitySinkMultiplier = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Animal")
	TObjectPtr<UInputAction> AnimalAbilityAction;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Flying")
	FVector CurrentWindDirection = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Flying")
	float CurrentWindSpeed = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Flying")
	float CurrentLiftStrength = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Flying")
	bool bIsInsideWindZone = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Flying")
	bool bIsAbilityActive = false;

	FVector GetDesiredFlightVelocity() const;
	bool IsOnValidLandingSurface() const;
	void BeginLanding();

private:
	float LateralInput = 0.0f;
	float RemainingLandingTime = 0.0f;
	float RemainingAbilityTime = 0.0f;

	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<AActor>, FUTPWindSourceState> ActiveWindSources;

	void UpdateFlightState(float DeltaSeconds);
	void HandleAnimalAbilityStarted();
	void RebuildWindState();
};
