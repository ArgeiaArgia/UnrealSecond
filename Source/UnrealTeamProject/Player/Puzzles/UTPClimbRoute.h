#pragma once

#include "GameFramework/Actor.h"
#include "UTPClimbableInterface.h"
#include "UTPClimbRoute.generated.h"

class USphereComponent;
class USplineComponent;

/** Spline-authored ledge or pole route used by the monkey's climbing prototype. */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPClimbRoute
	: public AActor
	, public ITPClimbableInterface
{
	GENERATED_BODY()

public:
	AUTPClimbRoute();

	virtual bool CanClimb_Implementation(AActor* Climber) const override;
	virtual FVector GetClimbAttachLocation_Implementation(FVector WorldLocation) const override;
	virtual FRotator GetClimbAttachRotation_Implementation(FVector WorldLocation) const override;
	virtual float GetClimbRouteLength_Implementation() const override;
	virtual FVector GetClimbLocationAtDistance_Implementation(float Distance) const override;
	virtual FRotator GetClimbRotationAtDistance_Implementation(float Distance) const override;
	virtual EUTPClimbSurfaceType GetClimbSurfaceType_Implementation() const override;

	UFUNCTION(BlueprintPure, Category="Climbing")
	USplineComponent* GetClimbSpline() const { return ClimbSpline; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USplineComponent> ClimbSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> DetectionVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climbing")
	EUTPClimbSurfaceType SurfaceType = EUTPClimbSurfaceType::Ledge;

	/** Offset from the authored spline center line to the monkey's capsule center. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climbing", meta=(ClampMin="0.0"))
	float ClimbOffset = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climbing", meta=(ClampMin="1.0"))
	float MaxAttachDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climbing")
	bool bIsEnabled = true;
};
