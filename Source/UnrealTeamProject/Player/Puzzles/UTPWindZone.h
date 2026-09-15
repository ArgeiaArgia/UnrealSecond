#pragma once

#include "GameFramework/Actor.h"
#include "UTPWindZone.generated.h"

class UBoxComponent;

/** A volume that pushes flying animals along a controlled wind corridor. */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPWindZone : public AActor
{
	GENERATED_BODY()

public:
	AUTPWindZone();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="Wind")
	void SetWindEnabled(bool bInEnabled);

	UFUNCTION(BlueprintPure, Category="Wind")
	bool IsWindEnabled() const { return bEnabled; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wind")
	TObjectPtr<UBoxComponent> WindVolume;

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
};
