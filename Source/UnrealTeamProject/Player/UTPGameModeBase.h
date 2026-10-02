#pragma once

#include "GameFramework/GameModeBase.h"
#include "UTPGameModeBase.generated.h"

UCLASS()
class UNREALTEAMPROJECT_API AUTPGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUTPGameModeBase();

	virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
};
