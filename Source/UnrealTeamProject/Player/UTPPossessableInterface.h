#pragma once

#include "UObject/Interface.h"
#include "UTPPossessableInterface.generated.h"

class AController;
class APawn;

UINTERFACE(BlueprintType)
class UNREALTEAMPROJECT_API UTPPossessableInterface : public UInterface
{
	GENERATED_BODY()
};

class UNREALTEAMPROJECT_API ITPPossessableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Possession")
	bool CanBePossessed(AController* InstigatingController) const;

	virtual bool CanBePossessed_Implementation(AController* InstigatingController) const
	{
		return true;
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Possession")
	void OnPossessionFocusChanged(bool bIsFocused);

	virtual void OnPossessionFocusChanged_Implementation(bool bIsFocused)
	{
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Possession")
	void OnPossessedBySoul(APawn* SoulPawn);

	virtual void OnPossessedBySoul_Implementation(APawn* SoulPawn)
	{
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Possession")
	void OnReleasedFromSoul(APawn* SoulPawn);

	virtual void OnReleasedFromSoul_Implementation(APawn* SoulPawn)
	{
	}

	/**
	 * Returns whether the current body can be left behind at this moment.
	 * Flying animals use this to prevent leaving a body suspended in mid-air.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Possession")
	bool CanReleaseFromSoul() const;

	virtual bool CanReleaseFromSoul_Implementation() const
	{
		return true;
	}
};
