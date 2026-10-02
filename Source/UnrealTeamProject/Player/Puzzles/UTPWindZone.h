#pragma once

#include "GameFramework/Actor.h"
#include "../../TPToggleableInterface.h"
#include "UTPWindZone.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMeshComponent;

/** A volume that pushes flying animals along a controlled wind corridor. */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPWindZone : public AActor, public ITPToggleableInterface
{
	GENERATED_BODY()

public:
	AUTPWindZone();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="Wind")
	void SetWindEnabled(bool bInEnabled);

	UFUNCTION(BlueprintPure, Category="Wind")
	bool IsWindEnabled() const { return bEnabled; }

	virtual void SetToggleableEnabled_Implementation(bool bInEnabled) override;
	virtual bool IsToggleableEnabled_Implementation() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wind")
	TObjectPtr<UBoxComponent> WindVolume;

	/** Optional outlet model, such as a vent or blower. Assign its Static Mesh in the component details. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wind|Visual")
	TObjectPtr<UStaticMeshComponent> WindOutletMesh;

	/** Visualizes the active wind corridor using NS_Boundary_Cylinder. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wind|Visual")
	TObjectPtr<UNiagaraComponent> WindVisual;

	/** Per-zone runtime copy with camera-facing decal renderers removed. */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> DirectionalWindVisualSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind")
	FVector WindDirection = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind", meta=(ClampMin="0.0"))
	float WindSpeed = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind")
	float LiftStrength = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind")
	bool bRefreshOverlapsWhileEnabled = true;

	/** Enables the Niagara guide shown at the wind outlet and through the wind volume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind|Visual")
	bool bShowWindVisual = true;

	/** Additional local offset for aligning the guide with the physical wind outlet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind|Visual")
	FVector WindVisualOffset = FVector::ZeroVector;

	/** Multiplier applied to the visual cylinder radius derived from the wind volume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind|Visual", meta=(ClampMin="0.01"))
	float WindVisualRadiusScale = 1.0f;

	/** Multiplier applied to the visual cylinder height derived from the wind volume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind|Visual", meta=(ClampMin="0.01"))
	float WindVisualHeightScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind|Visual")
	FLinearColor WindVisualColor = FLinearColor(0.15f, 0.65f, 1.0f, 0.35f);

private:
	TSet<TWeakObjectPtr<AActor>> ActiveReceivers;

	UFUNCTION()
	void OnWindVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnWindVolumeEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void ApplyWindToActor(AActor* OtherActor);
	void RemoveWindFromActor(AActor* OtherActor);
	void CreateDirectionalWindVisualSystem();
	void RefreshWindVisual();
	void SetWindVisualEnabled(bool bInEnabled);
};
