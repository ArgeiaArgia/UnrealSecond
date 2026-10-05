#if WITH_DEV_AUTOMATION_TESTS

#include "../Player/UI/UTPTutorialWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUTPTutorialWidgetLayoutTest,
	"UnrealTeamProject.Tutorial.VisibleLayoutOnFirstBuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUTPTutorialWidgetLayoutTest::RunTest(const FString& Parameters)
{
	UUTPTutorialWidget* Widget = NewObject<UUTPTutorialWidget>();
	Widget->Initialize();
	const FText FirstMessage = FText::FromString(TEXT("WASD movement tutorial"));
	Widget->ShowMessage(FirstMessage);
	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();

	UCanvasPanel* Root = Cast<UCanvasPanel>(Widget->GetRootWidget());
	if (!TestNotNull(TEXT("Tutorial has a canvas root"), Root))
	{
		return false;
	}
	// A layout built in NativeConstruct has UMG objects but no rendered Slate
	// children on its first build. This catches the original invisible prompt.
	TestTrue(TEXT("Canvas is connected to Slate on the first build"), Root->GetCachedWidget().IsValid());
	UTextBlock* MessageText = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("TutorialMessageText")));
	if (!TestNotNull(TEXT("Message text exists"), MessageText))
	{
		return false;
	}
	TestTrue(TEXT("Message set before construction is preserved"), MessageText->GetText().EqualTo(FirstMessage));
	TestTrue(TEXT("Shown tutorial is visible without consuming input"),
		Widget->GetVisibility() == ESlateVisibility::HitTestInvisible);

	const FText NextMessage = FText::FromString(TEXT("Possession tutorial"));
	Widget->ShowMessage(NextMessage);
	TestTrue(TEXT("Next trigger replaces the displayed text"), MessageText->GetText().EqualTo(NextMessage));
	Widget->HideMessage();
	TestTrue(TEXT("Hide trigger collapses the tutorial"), Widget->GetVisibility() == ESlateVisibility::Collapsed);
	Widget->ShowMessage(FText::GetEmpty());
	TestTrue(TEXT("An empty message stays hidden"), Widget->GetVisibility() == ESlateVisibility::Collapsed);
	return true;
}

#endif
