#include "UTPClimbRoute.h"

#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"

AUTPClimbRoute::AUTPClimbRoute()
{
	PrimaryActorTick.bCanEverTick = false;

	ClimbSpline = CreateDefaultSubobject<USplineComponent>(TEXT("ClimbSpline"));
	SetRootComponent(ClimbSpline);
	ClimbSpline->SetClosedLoop(false);
	ClimbSpline->SetMobility(EComponentMobility::Movable);

	DetectionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionVolume"));
	DetectionVolume->SetupAttachment(ClimbSpline);
	DetectionVolume->InitSphereRadius(150.0f);
	DetectionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionVolume->SetCollisionObjectType(ECC_WorldDynamic);
	DetectionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DetectionVolume->SetGenerateOverlapEvents(true);
}

bool AUTPClimbRoute::CanClimb_Implementation(AActor* Climber) const
{
	if (!bIsEnabled || !IsValid(Climber) || !ClimbSpline)
	{
		return false;
	}

	const FVector AttachLocation = GetClimbAttachLocation_Implementation(Climber->GetActorLocation());
	return FVector::DistSquared(Climber->GetActorLocation(), AttachLocation) <=
		FMath::Square(MaxAttachDistance);
}

FVector AUTPClimbRoute::GetClimbAttachLocation_Implementation(FVector WorldLocation) const
{
	if (!ClimbSpline)
	{
		return GetActorLocation();
	}

	const float InputKey = ClimbSpline->FindInputKeyClosestToWorldLocation(WorldLocation);
	const float Distance = ClimbSpline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
	return GetClimbLocationAtDistance_Implementation(Distance);
}

FRotator AUTPClimbRoute::GetClimbAttachRotation_Implementation(FVector WorldLocation) const
{
	if (!ClimbSpline)
	{
		return GetActorRotation();
	}

	const float InputKey = ClimbSpline->FindInputKeyClosestToWorldLocation(WorldLocation);
	const float Distance = ClimbSpline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
	return GetClimbRotationAtDistance_Implementation(Distance);
}

float AUTPClimbRoute::GetClimbRouteLength_Implementation() const
{
	return ClimbSpline ? ClimbSpline->GetSplineLength() : 0.0f;
}

FVector AUTPClimbRoute::GetClimbLocationAtDistance_Implementation(float Distance) const
{
	if (!ClimbSpline)
	{
		return GetActorLocation();
	}

	const float ClampedDistance = FMath::Clamp(Distance, 0.0f, ClimbSpline->GetSplineLength());
	const FVector CenterLocation = ClimbSpline->GetLocationAtDistanceAlongSpline(
		ClampedDistance, ESplineCoordinateSpace::World);
	const FRotator SplineRotation = ClimbSpline->GetRotationAtDistanceAlongSpline(
		ClampedDistance, ESplineCoordinateSpace::World);

	return CenterLocation + SplineRotation.RotateVector(FVector::RightVector * ClimbOffset);
}

FRotator AUTPClimbRoute::GetClimbRotationAtDistance_Implementation(float Distance) const
{
	if (!ClimbSpline)
	{
		return GetActorRotation();
	}

	return ClimbSpline->GetRotationAtDistanceAlongSpline(
		FMath::Clamp(Distance, 0.0f, ClimbSpline->GetSplineLength()),
		ESplineCoordinateSpace::World);
}

EUTPClimbSurfaceType AUTPClimbRoute::GetClimbSurfaceType_Implementation() const
{
	return SurfaceType;
}
