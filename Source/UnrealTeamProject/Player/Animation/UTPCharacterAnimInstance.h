#pragma once

#include "Animation/AnimInstance.h"
#include "UTPCharacterAnimInstance.generated.h"

class APawn;

UCLASS(Abstract, Blueprintable)
class UNREALTEAMPROJECT_API UTPCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UTPCharacterAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category="Animation|State")
	float GroundSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Animation|State")
	float Direction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Animation|State")
	float VerticalSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Animation|State")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category="Animation|State")
	bool bIsPossessed = false;

	UPROPERTY(BlueprintReadOnly, Category="Animation|State")
	bool bIsAbilityActive = false;

	virtual void RefreshAnimationState(float DeltaSeconds);
	virtual bool IsPossessedByPlayer(const APawn* PawnOwner) const;
	virtual bool IsAbilityActive(const APawn* PawnOwner) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<APawn> CachedPawnOwner;

	void ResetAnimationState();
	static float CalculateDirection(const APawn* PawnOwner, const FVector& Velocity);
};
