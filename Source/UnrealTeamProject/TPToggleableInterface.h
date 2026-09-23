#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TPToggleableInterface.generated.h"

/**
 * Common on/off contract for puzzle actors such as doors and environmental
 * volumes. Controllers should use this interface instead of concrete classes.
 */
UINTERFACE(BlueprintType)
class UNREALTEAMPROJECT_API UTPToggleableInterface : public UInterface
{
	GENERATED_BODY()
};

class UNREALTEAMPROJECT_API ITPToggleableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Toggleable")
	void SetToggleableEnabled(bool bInEnabled);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Toggleable")
	bool IsToggleableEnabled() const;
};
