#include "UTPGameModeBase.h"

#include "UTPPlayerController.h"
#include "UTPSoulPawn.h"

AUTPGameModeBase::AUTPGameModeBase()
{
	DefaultPawnClass = AUTPSoulPawn::StaticClass();
	PlayerControllerClass = AUTPPlayerController::StaticClass();
}

void AUTPGameModeBase::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	Super::FinishRestartPlayer(NewPlayer, StartRotation);

	// BP_SoulPawn is the pawn class used by the maps. Apply the PlayerStart yaw
	// after Blueprint defaults and possession have completed so its visible body
	// cannot retain a fixed authored direction.
	if (AUTPSoulPawn* SoulPawn = NewPlayer ? Cast<AUTPSoulPawn>(NewPlayer->GetPawn()) : nullptr)
	{
		SoulPawn->SetActorRotation(FRotator(0.0f, StartRotation.Yaw, 0.0f));
	}
}
