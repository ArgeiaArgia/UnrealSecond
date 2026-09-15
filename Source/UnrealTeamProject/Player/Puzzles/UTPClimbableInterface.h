#pragma once

#include "UObject/Interface.h"
#include "UTPClimbableInterface.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EUTPClimbSurfaceType : uint8
{
	Ledge UMETA(DisplayName="Ledge"),
	Pole UMETA(DisplayName="Pole"),
	Vine UMETA(DisplayName="Vine"),
	Rope UMETA(DisplayName="Rope")
};

UINTERFACE(BlueprintType)
class UNREALTEAMPROJECT_API UTPClimbableInterface : public UInterface
{
	GENERATED_BODY()
};

/** Common contract for ledges, poles, vines, and ropes that a monkey can follow. */
class UNREALTEAMPROJECT_API ITPClimbableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Climbing")
	bool CanClimb(AActor* Climber) const;

	virtual bool CanClimb_Implementation(AActor* Climber) const
	{
		return IsValid(Climber);
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Climbing")
	FVector GetClimbAttachLocation(FVector WorldLocation) const;

	virtual FVector GetClimbAttachLocation_Implementation(FVector WorldLocation) const
	{
		return WorldLocation;
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Climbing")
	FRotator GetClimbAttachRotation(FVector WorldLocation) const;

	virtual FRotator GetClimbAttachRotation_Implementation(FVector /*WorldLocation*/) const
	{
		return FRotator::ZeroRotator;
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Climbing")
	float GetClimbRouteLength() const;

	virtual float GetClimbRouteLength_Implementation() const
	{
		return 0.0f;
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Climbing")
	FVector GetClimbLocationAtDistance(float Distance) const;

	virtual FVector GetClimbLocationAtDistance_Implementation(float /*Distance*/) const
	{
		return FVector::ZeroVector;
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Climbing")
	FRotator GetClimbRotationAtDistance(float Distance) const;

	virtual FRotator GetClimbRotationAtDistance_Implementation(float /*Distance*/) const
	{
		return FRotator::ZeroRotator;
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Climbing")
	EUTPClimbSurfaceType GetClimbSurfaceType() const;

	virtual EUTPClimbSurfaceType GetClimbSurfaceType_Implementation() const
	{
		return EUTPClimbSurfaceType::Ledge;
	}
};
