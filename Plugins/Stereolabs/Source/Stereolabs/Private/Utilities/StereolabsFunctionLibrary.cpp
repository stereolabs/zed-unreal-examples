//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "StereolabsPrivatePCH.h"
#include "Stereolabs/Public/Utilities/StereolabsFunctionLibrary.h"
#include "IXRTrackingSystem.h"

#include <sl_mr_core/latency.hpp>
#include <sl_mr_core/Rendering.hpp>

DEFINE_LOG_CATEGORY(SLFunctionLibrary);

float USlFunctionLibrary::ConvertDepthToDistance(const FSlViewportHelper& ViewportHelper, const FVector2D& ScreenPosition, float Depth)
{
	static const FVector2D OutputRangeX(0, GSlCameraProxy->RetrieveMatSize.X);
	static const FVector2D OutputRangeY(0, GSlCameraProxy->RetrieveMatSize.Y);

	uint32 X = (uint32)FMath::GetMappedRangeValueUnclamped(ViewportHelper.RangeX, OutputRangeX, ScreenPosition.X);
	uint32 Y = (uint32)FMath::GetMappedRangeValueUnclamped(ViewportHelper.RangeY, OutputRangeY, ScreenPosition.Y);

	float InvHFocal = 1.0f / GSlCameraProxy->RetrieveLeftCameraParameters.HFocal;

	float DeprojX = ((float)X - GSlCameraProxy->RetrieveLeftCameraParameters.OpticalCenterX) * Depth * InvHFocal;
	float DeprojY = ((float)Y - GSlCameraProxy->RetrieveLeftCameraParameters.OpticalCenterY) * Depth * InvHFocal;

	return FMath::Sqrt(DeprojX*DeprojX + DeprojY*DeprojY + Depth*Depth);
}

float USlFunctionLibrary::ConvertCustomDepthToDistance(const FSlCameraParameters& CameraParameters, const FVector2D& Position, float Depth)
{
	float InvHFocal = 1.0f / CameraParameters.HFocal;

	float DeprojX = ((float)Position.X - CameraParameters.OpticalCenterX) * Depth * InvHFocal;
	float DeprojY = ((float)Position.Y - CameraParameters.OpticalCenterY) * Depth * InvHFocal;

	return FMath::Sqrt(DeprojX*DeprojX + DeprojY*DeprojY + Depth*Depth);
}

FVector2D USlFunctionLibrary::GetRenderPlaneSize(const FIntPoint& ImageResolution, float VerticalFOV, float PlaneDistance)
{
	return  sl::unreal::ToUnrealType(sl::mr::computeRenderPlaneSize(sl::unreal::ToSlType2(ImageResolution), VerticalFOV, PlaneDistance));
}

FVector2D USlFunctionLibrary::GetRenderPlaneSizeWithGamma(UObject* WorldContextObject, const FIntPoint& ImageResolution, float PerceptionDistance, float ZedFocal, float PlaneDistance)
{
	float EyeToZedDistance = 0.0f;

	if (GEngine->StereoRenderingDevice.IsValid() && GEngine->StereoRenderingDevice->IsStereoEnabled() && GEngine->XRSystem.IsValid() && GEngine->XRSystem->GetHMDDevice())
	{
		EyeToZedDistance = sl::mr::getEyeToZEDDistance(sl::unreal::ToSlType(GEngine->XRSystem->GetHMDDevice()->GetHMDDeviceType()));
	}
#if WITH_EDITOR
	else
	{
		SL_LOG_E(SLFunctionLibrary, "Eye to Zed distance not available for this HMD");

		return FVector2D();
	}
#endif

	float HMDFocal = 0.0f;

	if (GEngine->StereoRenderingDevice.IsValid() && GEngine->StereoRenderingDevice->IsStereoEnabled() && GEngine->XRSystem.IsValid() && GEngine->XRSystem->GetHMDDevice())
	{
		sl::Resolution HMDScreenResolution;

		switch (GEngine->XRSystem->GetHMDDevice()->GetHMDDeviceType())
		{
			case EHMDDeviceType::DT_SteamVR:
				{
					HMDScreenResolution.width = 3024;
					HMDScreenResolution.height = 1680;
				}
				break;
			case EHMDDeviceType::DT_OculusRift:
				{
					HMDScreenResolution.width = 2720;
					HMDScreenResolution.height = 1600;
				}
				break;
			default:
				{
					SL_LOG_E(SLFunctionLibrary, "HMD not supported");
				}
		}

		FMatrix ProjectionMatrix = GEngine->StereoRenderingDevice->GetStereoProjectionMatrix(EStereoscopicPass::eSSP_LEFT_EYE);

		HMDFocal = sl::mr::computeHMDFocal(HMDScreenResolution, ProjectionMatrix.M[0][0], ProjectionMatrix.M[1][1]);
	}
#if WITH_EDITOR
	else
	{
		SL_LOG_E(SLFunctionLibrary, "Focal not available for this HMD");

		return FVector2D();
	}
#endif

	return sl::unreal::ToUnrealType(sl::mr::computeRenderPlaneSizeWithGamma(sl::unreal::ToSlType2(ImageResolution), PerceptionDistance, EyeToZedDistance, PlaneDistance, HMDFocal, ZedFocal));
}

FVector4 USlFunctionLibrary::GetOpticalCentersOffsets(const FIntPoint& ImageResolution, float Distance)
{
	return sl::unreal::ToUnrealType(sl::mr::computeOpticalCentersOffsets(GSlCameraProxy->SlCameraInformation.calibration_parameters, sl::unreal::ToSlType2(ImageResolution), Distance));
}

FVector2D USlFunctionLibrary::GetOffCenterProjectionOffset(ESlEye Eye)
{
	FVector2D OffCenterProjectionOffset;
	FSlCameraInformation CameraInformation = GSlCameraProxy->CameraInformation;

	float Width = (float)CameraInformation.CalibrationParameters.LeftCameraParameters.Resolution.X;
	float Height = (float)CameraInformation.CalibrationParameters.LeftCameraParameters.Resolution.Y;
	float OpticalCenterX = (Eye == ESlEye::E_Left ? CameraInformation.CalibrationParameters.LeftCameraParameters.OpticalCenterX : CameraInformation.CalibrationParameters.RightCameraParameters.OpticalCenterX);
	float OpticalCenterY = (Eye == ESlEye::E_Left ? CameraInformation.CalibrationParameters.LeftCameraParameters.OpticalCenterY : CameraInformation.CalibrationParameters.RightCameraParameters.OpticalCenterY);

	OffCenterProjectionOffset.X = 2.0f * ((Width - 1.0f * OpticalCenterX) / Width) - 1.0f;
	OffCenterProjectionOffset.Y = -2.0f * ((Height - 1.0f * OpticalCenterY) / Height) + 1.0f;

	return OffCenterProjectionOffset;
}

void USlFunctionLibrary::GetSceneCaptureProjectionMatrix(FMatrix& ProjectionMatrix, ESlEye Eye)
{
	FSlCameraInformation CameraInformation = GSlCameraProxy->CameraInformation;

	float XAxisMultiplier = 1.0f;
	float YAxisMultiplier = (float)CameraInformation.CalibrationParameters.LeftCameraParameters.Resolution.X / (float)CameraInformation.CalibrationParameters.LeftCameraParameters.Resolution.Y;
	float FOV = CameraInformation.CalibrationParameters.LeftCameraParameters.HFOV * (float)PI / 360.0f;

	if ((int32)ERHIZBuffer::IsInverted)
	{
		ProjectionMatrix = FReversedZPerspectiveMatrix(
			FOV,
			FOV,
			XAxisMultiplier,
			YAxisMultiplier,
			GNearClippingPlane,
			GNearClippingPlane
			);
	}
	else
	{
		ProjectionMatrix = FPerspectiveMatrix(
			FOV,
			FOV,
			XAxisMultiplier,
			YAxisMultiplier,
			GNearClippingPlane,
			GNearClippingPlane
			);
	}

	FVector2D OffCenterProjectionOffset = GetOffCenterProjectionOffset(Eye);

	const float Left = -1.0f + OffCenterProjectionOffset.X;
	const float Right = Left + 2.0f;
	const float Bottom = -1.0f + OffCenterProjectionOffset.Y;
	const float Top = Bottom + 2.0f;
	ProjectionMatrix.M[2][0] = (Left + Right) / (Left - Right);
	ProjectionMatrix.M[2][1] = (Bottom + Top) / (Bottom - Top);
}

FString USlFunctionLibrary::ErrorCodeToString(ESlErrorCode ErrorCode)
{
	FString ErrorCodeString;

	if (ErrorCode > ESlErrorCode::EC_None)
	{
		ErrorCodeString = FString("Error code not in range");
	}
	else
	{
		if (ErrorCode == ESlErrorCode::EC_None)
		{
			ErrorCodeString = "No Error";
		}
		else
		{
			FString Tmp = FString(sl::toString(sl::unreal::ToSlType(ErrorCode)).get());
			ErrorCodeString = Tmp.Left(1) + Tmp.ToLower().RightChop(1);
		}
	}

	return ErrorCodeString;
}