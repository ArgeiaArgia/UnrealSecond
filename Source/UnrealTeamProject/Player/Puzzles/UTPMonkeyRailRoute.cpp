#include "UTPMonkeyRailRoute.h"

#include "Components/SplineComponent.h"
#include "Math/RotationMatrix.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AUTPMonkeyRailRoute::AUTPMonkeyRailRoute()
{
	// Preserve per-instance spline point edits. The spline itself is the rope path.
	ClimbSpline->bInputSplinePointsToConstructionScript = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultRopeMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (DefaultRopeMesh.Succeeded())
	{
		RopeMesh = DefaultRopeMesh.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultRopeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (DefaultRopeMaterial.Succeeded())
	{
		RopeMaterial = DefaultRopeMaterial.Object;
	}
}

void AUTPMonkeyRailRoute::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildRopeMeshes();
}

FVector AUTPMonkeyRailRoute::GetClimbLocationAtDistance_Implementation(float Distance) const
{
	if (!ClimbSpline)
	{
		return GetActorLocation();
	}

	const float ClampedDistance = FMath::Clamp(Distance, 0.0f, ClimbSpline->GetSplineLength());
	const FVector RailCenter = ClimbSpline->GetLocationAtDistanceAlongSpline(
		ClampedDistance, ESplineCoordinateSpace::World);

	// The generic climb route offsets a monkey to the side. This route uses only the spline,
	// so place the capsule directly over that line as if balancing on a rope.
	return RailCenter + FVector::UpVector * MonkeyBalanceHeight;
}

FRotator AUTPMonkeyRailRoute::GetClimbRotationAtDistance_Implementation(float Distance) const
{
	if (!ClimbSpline)
	{
		return GetActorRotation();
	}

	const float ClampedDistance = FMath::Clamp(Distance, 0.0f, ClimbSpline->GetSplineLength());
	const FVector RailDirection = ClimbSpline->GetDirectionAtDistanceAlongSpline(
		ClampedDistance, ESplineCoordinateSpace::World);

	// Follow the rope direction without inheriting spline roll, keeping the monkey upright
	// like a tightrope walker.
	return FRotationMatrix::MakeFromXZ(RailDirection, FVector::UpVector).Rotator();
}

void AUTPMonkeyRailRoute::RebuildRopeMeshes()
{
	ClearRopeMeshes();

	if (!ClimbSpline || !RopeMesh)
	{
		return;
	}

	const float SplineLength = ClimbSpline->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// The engine cylinder is 100 units wide. One segment per spline section keeps the rope smooth.
	const int32 SegmentCount = FMath::Max(ClimbSpline->GetNumberOfSplinePoints() - 1, 1);
	RopeMeshSegments.Reserve(SegmentCount);
	const FVector2D RopeScale(RopeWidth / 100.0f, RopeWidth / 100.0f);

	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const float StartDistance = SplineLength * static_cast<float>(SegmentIndex) / SegmentCount;
		const float EndDistance = SplineLength * static_cast<float>(SegmentIndex + 1) / SegmentCount;

		USplineMeshComponent* RopeSegment = NewObject<USplineMeshComponent>(this);
		RopeSegment->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		RopeSegment->SetMobility(EComponentMobility::Movable);
		RopeSegment->SetupAttachment(ClimbSpline);
		RopeSegment->SetStaticMesh(RopeMesh);
		RopeSegment->SetForwardAxis(ESplineMeshAxis::Z, false);
		RopeSegment->SetStartScale(RopeScale, false);
		RopeSegment->SetEndScale(RopeScale, false);
		RopeSegment->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (RopeMaterial)
		{
			UMaterialInstanceDynamic* DynamicRopeMaterial = UMaterialInstanceDynamic::Create(RopeMaterial, RopeSegment);
			DynamicRopeMaterial->SetVectorParameterValue(TEXT("Color"), RopeColor);
			RopeSegment->SetMaterial(0, DynamicRopeMaterial);
		}

		const FVector StartLocation = ClimbSpline->GetLocationAtDistanceAlongSpline(
			StartDistance, ESplineCoordinateSpace::Local);
		const FVector StartTangent = ClimbSpline->GetTangentAtDistanceAlongSpline(
			StartDistance, ESplineCoordinateSpace::Local);
		const FVector EndLocation = ClimbSpline->GetLocationAtDistanceAlongSpline(
			EndDistance, ESplineCoordinateSpace::Local);
		const FVector EndTangent = ClimbSpline->GetTangentAtDistanceAlongSpline(
			EndDistance, ESplineCoordinateSpace::Local);
		RopeSegment->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, true);
		RopeSegment->RegisterComponent();

		RopeMeshSegments.Add(RopeSegment);
	}
}

void AUTPMonkeyRailRoute::ClearRopeMeshes()
{
	// Construction can be rerun after duplicating an actor, compiling a Blueprint, or editing
	// a spline point. Scan the actor rather than relying only on the transient array so no
	// previous construction-script rope segment remains in the scene.
	TInlineComponentArray<USplineMeshComponent*> ExistingRopeSegments(this);
	GetComponents(ExistingRopeSegments);
	for (USplineMeshComponent* RopeSegment : ExistingRopeSegments)
	{
		if (IsValid(RopeSegment) && RopeSegment->GetAttachParent() == ClimbSpline)
		{
			// Older versions registered these as instance components. Remove that persistent
			// registration before destroying them so they cannot reappear on the next edit.
			RemoveInstanceComponent(RopeSegment);
			RopeSegment->DestroyComponent();
		}
	}

	RopeMeshSegments.Reset();
}
