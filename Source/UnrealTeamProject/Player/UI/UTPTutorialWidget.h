#pragma once

#include "Blueprint/UserWidget.h"
#include "UTPTutorialWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;

/**
 * A non-interactive, bottom-of-screen tutorial prompt. The widget is built in
 * C++ so a separate Widget Blueprint is not required for each tutorial step.
 */
UCLASS()
class UNREALTEAMPROJECT_API UUTPTutorialWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowMessage(const FText& InMessage);
	void HideMessage();

protected:
	virtual void NativeConstruct() override;

private:
	void BuildLayout();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> MessagePanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	FText CurrentMessage;
};
