#include "UTPGameModeBase.h"

#include "UTPPlayerController.h"
#include "UTPSoulPawn.h"

AUTPGameModeBase::AUTPGameModeBase()
{
	DefaultPawnClass = AUTPSoulPawn::StaticClass();
	PlayerControllerClass = AUTPPlayerController::StaticClass();
}
