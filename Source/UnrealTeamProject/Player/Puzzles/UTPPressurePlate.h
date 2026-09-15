#pragma once

#include "GameFramework/Actor.h"
#include "UTPPressurePlate.generated.h"

class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUTPPressurePlateStateChanged, bool, bPressed);

UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPPressurePlate : public AActor
{
	GENERATED_BODY()

public:
	AUTPPressurePlate();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category="Puzzle|Pressure Plate")
	bool IsPressed() const { return bIsPressed; }

	UFUNCTION(BlueprintCallable, Category="Puzzle|Pressure Plate")
	void RefreshPressedState();

	UPROPERTY(BlueprintAssignable, Category="Puzzle|Pressure Plate")
	FUTPPressurePlateStateChanged OnPressedChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle|Pressure Plate", meta=(ClampMin="0.0"))
	float RequiredWeight = 80.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Puzzle|Pressure Plate")
	bool bIsPressed = false;

private:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
