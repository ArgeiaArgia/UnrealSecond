#pragma once

#include "GameFramework/Actor.h"
#include "../../TPToggleableInterface.h"
#include "UTPShallowWaterInterface.h"
#include "UTPWaterZone.generated.h"

class UBoxComponent;

/** Shallow water volume. It adjusts tagged frog actors without owning frog gameplay logic. */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPWaterZone : public AActor, public ITPShallowWaterInterface, public ITPToggleableInterface
{
	GENERATED_BODY()

public:
	AUTPWaterZone();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual bool CanTraverseWater_Implementation(AActor* Actor) const override;
	virtual void EnterWater_Implementation(AActor* Actor) override;
	virtual void ExitWater_Implementation(AActor* Actor) override;

	UFUNCTION(BlueprintCallable, Category="Water")
	void SetWaterEnabled(bool bInEnabled);

	UFUNCTION(BlueprintPure, Category="Water")
	bool IsWaterEnabled() const { return bIsEnabled; }

	virtual void SetToggleableEnabled_Implementation(bool bInEnabled) override;
	virtual bool IsToggleableEnabled_Implementation() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Water")
	TObjectPtr<UBoxComponent> WaterVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Water", meta=(ClampMin="0.0"))
	float WaterMoveSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Water")
	bool bAllowFrogTraversal = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Water")
	bool bIsEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Water")
	bool bRefreshOverlapsWhileEnabled = true;

private:
	TSet<TWeakObjectPtr<AActor>> ActiveActors;

	UFUNCTION()
	void OnWaterVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnWaterVolumeEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void ApplyWaterToActor(AActor* OtherActor);
	void RemoveWaterFromActor(AActor* OtherActor);
};
