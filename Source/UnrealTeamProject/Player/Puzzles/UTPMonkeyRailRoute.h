#pragma once

#include "Components/SplineMeshComponent.h"
#include "UTPClimbRoute.h"
#include "UTPMonkeyRailRoute.generated.h"

class UMaterialInterface;
class UStaticMesh;

/**
 * A monkey balance route. Its authored spline is rendered as a simple colored rope, not a rail.
 */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPMonkeyRailRoute : public AUTPClimbRoute
{
	GENERATED_BODY()

public:
	AUTPMonkeyRailRoute();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual FVector GetClimbLocationAtDistance_Implementation(float Distance) const override;
	virtual FRotator GetClimbRotationAtDistance_Implementation(float Distance) const override;

protected:
	/** Diameter of the visible rope in Unreal units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rope|Visual", meta=(ClampMin="0.1"))
	float RopeWidth = 8.0f;

	/** Color applied to the default rope material. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rope|Visual")
	FLinearColor RopeColor = FLinearColor(0.30f, 0.12f, 0.03f, 1.0f);

	/** Material used for the rope. Its Color parameter receives Rope Color. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rope|Visual")
	TObjectPtr<UMaterialInterface> RopeMaterial;

	/** Vertical distance from the rope center line to the monkey capsule center while balancing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rope|Traversal", meta=(ClampMin="0.0"))
	float MonkeyBalanceHeight = 88.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> RopeMeshSegments;

private:
	TObjectPtr<UStaticMesh> RopeMesh;

	void RebuildRopeMeshes();
	void ClearRopeMeshes();
};
