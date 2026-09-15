#pragma once

#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "UTPSoulPawn.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UFloatingPawnMovement;
class UAnimInstance;
class UInputAction;
class UNiagaraSystem;

UCLASS()
class UNREALTEAMPROJECT_API AUTPSoulPawn : public APawn
{
	GENERATED_BODY()

public:
	AUTPSoulPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintCallable, Category="Soul|Movement")
	void MoveForward(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category="Soul|Movement")
	void MoveRight(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category="Soul|Movement")
	void MoveUp(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category="Soul|Movement")
	void MoveDown(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category="Soul|Possession")
	void RequestPossessFocusedTarget();

	UFUNCTION(BlueprintPure, Category="Soul|Possession")
	AActor* GetFocusedPossessableTarget() const;

	void BeginPossessionVanish(float InDuration);
	void CancelPossessionVanish();
	void BeginPossessionMaterialize(float InDuration);
	void FinishPossessionMaterialize();
	void CancelPossessionMaterialize();
	void PlayPossessionArrivalEffect(const FVector& Location) const;
	void PlayPossessionReleaseEffect(const FVector& Location) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> VisualMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TSubclassOf<UAnimInstance> AnimInstanceClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Soul|Movement")
	float MoveSpeedScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Soul|Movement", meta=(ClampMin="0.0"))
	float RotationInterpSpeed = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Soul")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Soul")
	TObjectPtr<UInputAction> SoulAscendAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Soul")
	TObjectPtr<UInputAction> SoulDescendAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Soul|Possession")
	float PossessionTraceDistance = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Soul|Possession", meta=(ClampMin="0"))
	float PossessionTraceRadius = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Soul|Possession")
	TEnumAsByte<ECollisionChannel> PossessionTraceChannel = ECC_Visibility;

	// Optional Niagara assets. Assign these in BP_SoulPawn; no effect is spawned
	// when an asset slot is left empty.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Soul|Possession|Effects")
	TObjectPtr<UNiagaraSystem> PossessionStartEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Soul|Possession|Effects")
	TObjectPtr<UNiagaraSystem> PossessionArrivalEffect;

	// Played when Q releases the current body and the SoulPawn reappears.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Soul|Possession|Effects")
	TObjectPtr<UNiagaraSystem> PossessionReleaseEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Soul|Possession|Effects")
	FVector PossessionEffectScale = FVector::OneVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Debug")
	bool bDrawPossessionTrace = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> FocusedPossessableTarget;

	FVector PossessionVanishInitialScale = FVector::OneVector;
	float PossessionVanishElapsed = 0.0f;
	float PossessionVanishDuration = 0.0f;
	FVector PossessionMaterializeFinalScale = FVector::OneVector;
	float PossessionMaterializeElapsed = 0.0f;
	float PossessionMaterializeDuration = 0.0f;
	ECollisionEnabled::Type CachedCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	bool bPossessionVanishActive = false;
	bool bPossessionMaterializeActive = false;
	bool bHasCachedCollisionEnabled = false;

	void ApplyAnimInstanceClass();
	void RestorePossessionCollision();
	void SpawnPossessionEffect(UNiagaraSystem* Effect, const FVector& Location) const;
	void UpdateFocusedTarget();
	void SetFocusedPossessableTarget(AActor* NewTarget);
	bool CanPossessActor(AActor* TargetActor) const;
};
