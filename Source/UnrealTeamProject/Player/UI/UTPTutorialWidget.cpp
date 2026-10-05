#include "UTPTutorialWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> UUTPTutorialWidget::RebuildWidget()
{
	// The root must exist before UUserWidget builds its Slate content; creating
	// it in NativeConstruct leaves the displayed content as an empty spacer.
	BuildLayout();
	return Super::RebuildWidget();
}

void UUTPTutorialWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CurrentMessage.IsEmpty())
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		ShowMessage(CurrentMessage);
	}
}

void UUTPTutorialWidget::ShowMessage(const FText& InMessage)
{
	CurrentMessage = InMessage;
	if (MessageText)
	{
		MessageText->SetText(CurrentMessage);
	}

	SetVisibility(CurrentMessage.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UUTPTutorialWidget::HideMessage()
{
	CurrentMessage = FText::GetEmpty();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UUTPTutorialWidget::BuildLayout()
{
	if (RootCanvas)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TutorialCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	MessagePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TutorialMessagePanel"));
	MessagePanel->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.06f, 0.88f));
	MessagePanel->SetPadding(FMargin(28.0f, 18.0f));

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TutorialMessageText"));
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetAutoWrapText(true);
	MessageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	MessagePanel->SetContent(MessageText);

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(MessagePanel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 1.0f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	PanelSlot->SetPosition(FVector2D(0.0f, -64.0f));
	PanelSlot->SetSize(FVector2D(760.0f, 82.0f));
}
