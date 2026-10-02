#include "UTPPossessionProgressWidget.h"

#include "Rendering/DrawElements.h"

namespace
{
	constexpr int32 CircleSegments = 64;
	constexpr float StartAngleRadians = -PI * 0.5f;
}

void UUTPPossessionProgressWidget::ShowProgress(
	const FVector2D& InScreenPosition,
	float InRadius,
	float InProgress)
{
	ScreenPosition = InScreenPosition;
	Radius = FMath::Max(InRadius, RingThickness);
	Progress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	bShowProgress = true;
}

void UUTPPossessionProgressWidget::HideProgress()
{
	bShowProgress = false;
	Progress = 0.0f;
}

int32 UUTPPossessionProgressWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 NextLayer = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);

	if (!bShowProgress)
	{
		return NextLayer;
	}

	auto MakeArcPoints = [this](float EndAngleRadians, int32 SegmentCount)
	{
		TArray<FVector2D> Points;
		Points.Reserve(SegmentCount + 1);
		for (int32 Index = 0; Index <= SegmentCount; ++Index)
		{
			const float Alpha = static_cast<float>(Index) / SegmentCount;
			const float Angle = FMath::Lerp(StartAngleRadians, EndAngleRadians, Alpha);
			Points.Add(ScreenPosition + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Points;
	};

	const TArray<FVector2D> BackgroundPoints = MakeArcPoints(StartAngleRadians + 2.0f * PI, CircleSegments);
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		NextLayer,
		AllottedGeometry.ToPaintGeometry(),
		BackgroundPoints,
		ESlateDrawEffect::None,
		BackgroundColor,
		true,
		RingThickness);

	if (Progress > KINDA_SMALL_NUMBER)
	{
		const int32 ProgressSegments = FMath::Max(1, FMath::CeilToInt(CircleSegments * Progress));
		const TArray<FVector2D> ProgressPoints = MakeArcPoints(
			StartAngleRadians + 2.0f * PI * Progress,
			ProgressSegments);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			NextLayer + 1,
			AllottedGeometry.ToPaintGeometry(),
			ProgressPoints,
			ESlateDrawEffect::None,
			ProgressColor,
			true,
			RingThickness);
	}

	return NextLayer + 1;
}
