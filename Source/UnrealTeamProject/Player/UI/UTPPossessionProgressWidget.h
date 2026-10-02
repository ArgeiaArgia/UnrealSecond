#pragma once

#include "Blueprint/UserWidget.h"
#include "UTPPossessionProgressWidget.generated.h"

/**
 * Full-screen, non-interactive overlay used while a possession target is held.
 * The controller supplies viewport-space values every frame; NativePaint draws
 * the ring around the current target without requiring a Blueprint widget.
 */
UCLASS()
class UNREALTEAMPROJECT_API UUTPPossessionProgressWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowProgress(const FVector2D& InScreenPosition, float InRadius, float InProgress);
	void HideProgress();

protected:
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	float Radius = 72.0f;
	float Progress = 0.0f;
	bool bShowProgress = false;

	UPROPERTY(EditDefaultsOnly, Category="Possession|UI")
	FLinearColor BackgroundColor = FLinearColor(0.03f, 0.05f, 0.12f, 0.88f);

	UPROPERTY(EditDefaultsOnly, Category="Possession|UI")
	FLinearColor ProgressColor = FLinearColor(0.25f, 0.9f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category="Possession|UI", meta=(ClampMin="1.0"))
	float RingThickness = 6.0f;
};
