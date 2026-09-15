#pragma once

#include "../../Puzzles/UTPClimbableInterface.h"
#include "../../UTPPossessableCharacter.h"
#include "UTPMonkeyCharacter.generated.h"

class UInputAction;

UENUM(BlueprintType)
enum class EUTPMonkeyState : uint8
{
	Grounded UMETA(DisplayName="Grounded"),
	Jumping UMETA(DisplayName="Jumping"),
	Climbing UMETA(DisplayName="Climbing"),
	Landing UMETA(DisplayName="Landing")
};

/** Vertical exploration animal. Climbing is restricted to authored spline routes. */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPMonkeyCharacter : public AUTPPossessableCharacter
{
	GENERATED_BODY()

public:
	AUTPMonkeyCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Move(const FInputActionValue& Value) override;
	virtual void StartJump() override;
	virtual void OnPossessedBySoul_Implementation(APawn* SoulPawn) override;
	virtual void OnReleasedFromSoul_Implementation(APawn* SoulPawn) override;
	virtual bool CanReleaseFromSoul_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category="Animal|Monkey|Climbing")
	bool TryAttachToClimbRoute();

	UFUNCTION(BlueprintCallable, Category="Animal|Monkey|Climbing")
	void DetachFromClimbRoute(bool bLaunchFromRoute = true);

	UFUNCTION(BlueprintPure, Category="Animal|Monkey")
	EUTPMonkeyState GetMonkeyState() const { return MonkeyState; }

	UFUNCTION(BlueprintPure, Category="Animal|Monkey")
	bool IsAttachedToClimbRoute() const { return bIsAttachedToClimbRoute; }

	UFUNCTION(BlueprintPure, Category="Animal|Monkey")
	EUTPClimbSurfaceType GetCurrentClimbSurface() const { return CurrentClimbSurface; }

	UFUNCTION(BlueprintPure, Category="Animal|Monkey|Climbing")
	AActor* GetActiveClimbRoute() const { return ActiveClimbRoute.Get(); }

	UFUNCTION(BlueprintPure, Category="Animal|Monkey|Climbing")
	float GetClimbInput() const { return ClimbInput; }

	UFUNCTION(BlueprintPure, Category="Animal|Monkey|Climbing")
	float GetClimbSpeed() const { return ClimbSpeed; }

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Monkey")
	EUTPMonkeyState MonkeyState = EUTPMonkeyState::Grounded;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Monkey|Climbing", meta=(ClampMin="1.0"))
	float ClimbSearchRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Monkey|Climbing", meta=(ClampMin="0.0"))
	float ClimbSpeed = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Monkey|Climbing", meta=(ClampMin="0.0"))
	float RouteEndTolerance = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Monkey|Climbing", meta=(ClampMin="0.0"))
	float RouteJumpForwardSpeed = 160.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animal|Monkey|Climbing", meta=(ClampMin="0.0"))
	float RouteJumpUpwardSpeed = 420.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Monkey|Climbing")
	bool bIsAttachedToClimbRoute = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Monkey|Climbing")
	EUTPClimbSurfaceType CurrentClimbSurface = EUTPClimbSurfaceType::Ledge;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Animal|Monkey|Climbing")
	float ClimbDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Animal")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveClimbRoute;

private:
	float ClimbInput = 0.0f;

	void TickClimbing(float DeltaSeconds);
	void SnapToClimbRoute();
	AActor* FindNearbyClimbRoute() const;
	float FindNearestDistanceOnRoute(AActor* Route) const;
	void HandleInteractStarted();
	void ClearMoveInput();
	void ResetClimbState(bool bClearRoute);
};
