//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "StereolabsPrivatePCH.h"
#include "Stereolabs/Public/Utilities/StereolabsViewportHelper.h"
#include "Stereolabs/Public/Core/StereolabsCameraProxy.h"
#include "HeadMountedDisplayFunctionLibrary.h"

FSlViewportHelper::FSlViewportHelper()
	:
	ScreenRatio(0.0f),
	GameViewportClient(nullptr)
{
}

void FSlViewportHelper::AddToViewportResizeEvent(UGameViewportClient* NewGameViewportClient)
{
	// Already init
	if (GameViewportClient)
	{
		return;
	}

	GameViewportClient = NewGameViewportClient;

	ViewportResizedEventHandle = FViewport::ViewportResizedEvent.AddLambda([this](FViewport* Viewport, uint32)
	{
		if (Viewport->GetClient() == GameViewportClient)
		{
			Update(Viewport->GetSizeXY());
		}
	});
}

void FSlViewportHelper::Update(const FIntPoint& ViewportSize)
{
	FIntPoint Resolution = GSlCameraProxy->CameraInformation.CalibrationParameters.LeftCameraParameters.Resolution;

	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		if (Size == Resolution)
		{
			return;
		}

		Size = Resolution;

		Offset.X = 0.0f;
		Offset.Y = 0.0f;
	}
	else
	{
		// No update needed
		if (Size == ViewportSize)
		{
			return;
		}

		//Update
		Size = ViewportSize;

		ScreenRatio = (float)Size.X / (float)Size.Y;
		float ImageRatio = (float)Resolution.X / (float)Resolution.Y;

		// Not 16/9
		if (ScreenRatio < ImageRatio)
		{
			Offset.X = 0.0f;
			Offset.Y = (Size.Y - (Size.X / ImageRatio)) / 2;
		}
		else if (ScreenRatio > ImageRatio)
		{
			Offset.X = (Size.X - (Size.Y * ImageRatio)) / 2;
			Offset.Y = 0.0f;
		}
		else
		{
			Offset.X = 0.0f;
			Offset.Y = 0.0f;
		}
	}

	RangeX.X = Offset.X;
	RangeX.Y = Size.X - Offset.X;

	RangeY.X = Offset.Y;
	RangeY.Y = Size.Y - Offset.Y;
}