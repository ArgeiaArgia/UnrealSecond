#pragma once

#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "UTPPressurePlate.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUTPPressurePlateStateChanged, bool, bPressed);

UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API AUTPPressurePlate : public AActor
{
	GENERATED_BODY()

public:
	AUTPPressurePlate();

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
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

	// 메시 자체에 단순 충돌이 없어도 캐릭터가 판을 통과하지 않도록 보장합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> BlockingVolume;

	// 외형 전용 메시입니다. 실제 눌림 판정은 TriggerVolume이 처리합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PlateMesh;

	// 눌림 상태에 따라 켜고 끌 대상입니다. 기존 문 참조도 그대로 사용할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle|Pressure Plate", meta=(MustImplement="/Script/UnrealTeamProject.TPToggleableInterface"))
	TObjectPtr<AActor> TargetDoor;

	// 눌린 동안 켜고 해제 시 끌 추가 대상입니다. 기존 바람 영역 참조도 그대로 사용할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle|Pressure Plate|Targets", meta=(MustImplement="/Script/UnrealTeamProject.TPToggleableInterface"))
	TArray<TObjectPtr<AActor>> TargetWindZones;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle|Pressure Plate", meta=(ClampMin="0.0"))
	float RequiredWeight = 80.0f;

	// 눌렸을 때 판과 밟는 충돌면이 함께 내려가는 거리입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle|Pressure Plate|Feedback", meta=(ClampMin="0.0", Units="cm"))
	float PressDepth = 20.0f;

	// 눌릴 때의 반응 속도입니다. 값이 클수록 더 빠르게 내려갑니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle|Pressure Plate|Feedback", meta=(ClampMin="0.0"))
	float PressSpeed = 16.0f;

	// 무게가 사라진 뒤 원위치로 돌아오는 속도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Puzzle|Pressure Plate|Feedback", meta=(ClampMin="0.0"))
	float ReleaseSpeed = 8.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Puzzle|Pressure Plate")
	bool bIsPressed = false;

private:
	FTimerHandle RefreshTimerHandle;
	float NextDebugLogTime = 0.0f;
	FVector PlateMeshRestingLocation = FVector::ZeroVector;
	FVector BlockingVolumeRestingLocation = FVector::ZeroVector;

	void UpdateTriggerVolumeFromPlateMesh();
	void UpdatePlateDepression(float DeltaSeconds);
	void ApplyTargetState(bool bPressed);

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
