#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialTrigger.generated.h"

class UBoxComponent;

/**
 * One-shot level trigger that replaces the currently shown tutorial prompt.
 * Place it in a level and set TutorialMessage on the actor instance.
 */
UCLASS(Blueprintable)
class UNREALTEAMPROJECT_API ATutorialTrigger : public AActor
{
	GENERATED_BODY()

public:
	ATutorialTrigger();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** Text shown after the controlled pawn enters this trigger. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial", meta = (MultiLine = "true"))
	FText TutorialMessage;

	/** Hide the current prompt instead of replacing it with TutorialMessage. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tutorial")
	bool bHideTutorial = false;

	/** Prevents this trigger from showing the same prompt again after it has fired. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bTriggerOnce = true;

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

private:
	bool bHasTriggered = false;
};
