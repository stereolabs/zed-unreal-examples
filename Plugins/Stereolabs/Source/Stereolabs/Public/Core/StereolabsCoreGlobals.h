//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#pragma once

#include "Stereolabs/Public/Core/StereolabsBaseTypes.h"

#include "Math/Matrix.h"
#include "Kismet/KismetMathLibrary.h"
#include "HeadMountedDisplay.h"

#include <sl_mr_core/defines.hpp>

/** Id of the grab thread */
extern STEREOLABS_API uint32 GSlGrabThreadId;

/** True if the grab thread has been initialized */
extern STEREOLABS_API bool GSlIsGrabThreadIdInitialized;

/** Eye half baseline of the Zed */
extern STEREOLABS_API float GSlEyeHalfBaseline;

/** Camera proxy instance */
extern STEREOLABS_API class USlCameraProxy* GSlCameraProxy;

/*
 * @return True if called from the grab thread
 */
FORCEINLINE bool IsInSlGrabThread()
{
	if (GSlIsGrabThreadIdInitialized)
	{
		const uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		return CurrentThreadId == GSlGrabThreadId;
	}

	return false;
}

/*
 * @param TextureQualityPreset The preset
 * @return The size of the texture from the quality setting
 */
FORCEINLINE FIntPoint GetSlTextureSizeFromPreset(int32 TextureQualityPreset)
{
	TextureQualityPreset = FMath::Clamp(TextureQualityPreset, 0, 2);

	switch (static_cast<ESlTextureQuality>(TextureQualityPreset))
	{
		case ESlTextureQuality::TQ_High:
			return FIntPoint(1280, 720);
		case ESlTextureQuality::TQ_Medium:
			return FIntPoint(640, 360);
		case ESlTextureQuality::TQ_Low:
			return FIntPoint(384, 192);
		default:
			return FIntPoint(1280, 720);
	}
}

/*
 * Convert ESlTextureFormat to EPixelFormat
 * @param TextureFormat The texture format
 * @return				The EPixelFormat
 */
FORCEINLINE EPixelFormat GetPixelFormatFromSlTextureFormat(ESlTextureFormat TextureFormat)
{
	switch (TextureFormat)
	{
		case ESlTextureFormat::TF_R32_FLOAT:
			return EPixelFormat::PF_R32_FLOAT;
		case ESlTextureFormat::TF_B8G8R8A8_UNORM:
			/*
			 * Replace by PF_R8G8B8A8_UINT if using other engine source code.
			 * Convert texture sample from RGRA to BGRA in material.
			 */
			return EPixelFormat::PF_B8G8R8A8_UNORM; 
		case ESlTextureFormat::TF_R8G8B8A8_UINT:
			return EPixelFormat::PF_R8G8B8A8_UINT;
		case ESlTextureFormat::TF_A32B32G32R32F:
			return EPixelFormat::PF_A32B32G32R32F;
		case ESlTextureFormat::TF_R8_UNORM:
			return EPixelFormat::PF_G8;
	}

	ensureMsgf(false, TEXT("Unhandled ESlTextureFormat entry %u"), (uint32)TextureFormat);
	return PF_Unknown;
}

/*
 * Conversion functions
 */
namespace sl
{
	namespace unreal
	{
		namespace arrays
		{
			/*
			 * Convert from c++/sl array to TArray
			 */
			template<typename Unreal, typename Sl>
			FORCEINLINE TArray<Unreal> ToUnrealType(const std::vector<Sl>& SlArray)
			{
				size_t Size = SlArray.size();
				TArray<Unreal> Array;
				Array.Reserve(Size);

				for (size_t Index = 0; Index < Size; ++Index)
				{
					Array.Add(static_cast<Unreal>(SlArray[Index]));
				}

				return Array;
			}

			/*
			 * Convert from TArray array to c++/sl
			 */
			template<typename Sl, typename Unreal>
			FORCEINLINE std::vector<Sl> ToSlType(const TArray<Unreal>& UnrealArray)
			{
				int32 Size = UnrealArray.Num();
				std::vector<Sl> Array;
				Array.reserve(Size);

				for (int32 Index = 0; Index < Size; ++Index)
				{
					Array.push_back(static_cast<Sl>(UnrealArray[Index]));
				}

				return Array;
			}
		}

		/*
		 * Convert from sl::SELF_CALIBRATION_STATE to ESlSelfCalibrationState
		 */
		FORCEINLINE ESlSelfCalibrationState ToUnrealType(sl::SELF_CALIBRATION_STATE SlType)
		{
			switch (SlType)
			{
				case sl::SELF_CALIBRATION_STATE::SELF_CALIBRATION_STATE_NOT_STARTED:
					return ESlSelfCalibrationState::SCS_NotStarted;
				case sl::SELF_CALIBRATION_STATE::SELF_CALIBRATION_STATE_RUNNING:
					return ESlSelfCalibrationState::SCS_Running;
				case sl::SELF_CALIBRATION_STATE::SELF_CALIBRATION_STATE_FAILED:
					return ESlSelfCalibrationState::SCS_Failed;
				case sl::SELF_CALIBRATION_STATE::SELF_CALIBRATION_STATE_SUCCESS:
					return ESlSelfCalibrationState::SCS_Success;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::SELF_CALIBRATION_STATE entry %u"), (uint32)SlType);

					return (ESlSelfCalibrationState)0;
				}
			}
		};

		/*
		 * Convert from sl::TIME_REFERENCE to ESlTimeReference
		 */
		FORCEINLINE ESlTimeReference ToUnrealType(sl::TIME_REFERENCE SlType)
		{
			switch (SlType)
			{
				case sl::TIME_REFERENCE::TIME_REFERENCE_IMAGE:
					return ESlTimeReference::TR_Image;
				case sl::TIME_REFERENCE::TIME_REFERENCE_CURRENT:
					return ESlTimeReference::TR_Current;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::TIME_REFERENCE entry %u"), (uint32)SlType);

					return (ESlTimeReference)0;
				}
			}
		};

		/*
		 * Convert from sl::REFERENCE_FRAME to ESlReferenceFrame
		 */
		FORCEINLINE ESlReferenceFrame ToUnrealType(sl::REFERENCE_FRAME SlType)
		{
			switch (SlType)
			{
				case sl::REFERENCE_FRAME::REFERENCE_FRAME_CAMERA:
					return ESlReferenceFrame::RF_Camera;
				case sl::REFERENCE_FRAME::REFERENCE_FRAME_WORLD:
					return ESlReferenceFrame::RF_World;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::REFERENCE_FRAME entry %u"), (uint32)SlType);

					return (ESlReferenceFrame)0;
				}
			}
		};

		/*
		 * Convert from sl::TRACKING_STATE to EZEDTrackingState
		 */
		FORCEINLINE ESlTrackingState ToUnrealType(sl::TRACKING_STATE SlType)
		{
			switch (SlType)
			{
				case sl::TRACKING_STATE::TRACKING_STATE_SEARCHING:
					return ESlTrackingState::TS_TrackingSearch;
				case sl::TRACKING_STATE::TRACKING_STATE_OK:
					return ESlTrackingState::TS_TrackingOk;
				case sl::TRACKING_STATE::TRACKING_STATE_OFF:
					return ESlTrackingState::TS_TrackingOff;
				case sl::TRACKING_STATE::TRACKING_STATE_FPS_TOO_LOW:
					return ESlTrackingState::TS_FpsTooLow;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::TRACKING_STATE entry %u"), (uint32)SlType);

					return (ESlTrackingState)0;
				}
			}
		};

		/*
		 * Convert from ESlResolution to sl::RESOLUTION
		 */
		FORCEINLINE ESlResolution ToUnrealType(sl::RESOLUTION SlType)
		{
			switch (SlType)
			{
				case sl::RESOLUTION::RESOLUTION_HD2K:
					return ESlResolution::R_HD2K;
				case sl::RESOLUTION::RESOLUTION_HD1080:
					return ESlResolution::R_HD1080;
				case sl::RESOLUTION::RESOLUTION_HD720:
					return ESlResolution::R_HD720;
				case sl::RESOLUTION::RESOLUTION_VGA:
					return ESlResolution::R_VGA;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::RESOLUTION entry %u"), (uint32)SlType);

					return (ESlResolution)0;
				}		
			}
		}

		/*
		 * Convert from sl::ERROR_CODE to ESlErrorCode
		 */
		FORCEINLINE ESlErrorCode ToUnrealType(sl::ERROR_CODE SlType)
		{
			switch (SlType)
			{
				case sl::ERROR_CODE::SUCCESS:
					return ESlErrorCode::EC_Success;
				case sl::ERROR_CODE::ERROR_CODE_FAILURE:
					return ESlErrorCode::EC_Failure;
				case sl::ERROR_CODE::ERROR_CODE_NO_GPU_COMPATIBLE:
					return ESlErrorCode::EC_NoGpuCompatible;
				case sl::ERROR_CODE::ERROR_CODE_NOT_ENOUGH_GPUMEM:
					return ESlErrorCode::EC_NotEnoughGPUMemory;
				case sl::ERROR_CODE::ERROR_CODE_SENSOR_NOT_DETECTED:
					return ESlErrorCode::EC_SensorNotDetected;
				case sl::ERROR_CODE::ERROR_CODE_CAMERA_NOT_DETECTED:
					return ESlErrorCode::EC_CameraNotDetected;
				case sl::ERROR_CODE::ERROR_CODE_INVALID_RESOLUTION:
					return ESlErrorCode::EC_InvalidResolution;
				case sl::ERROR_CODE::ERROR_CODE_LOW_USB_BANDWIDTH:
					return ESlErrorCode::EC_LowUSBBandwidth;
				case sl::ERROR_CODE::ERROR_CODE_CALIBRATION_FILE_NOT_AVAILABLE:
					return ESlErrorCode::EC_CalibrationFileNotAvailable;
				case sl::ERROR_CODE::ERROR_CODE_INVALID_CALIBRATION_FILE:
					return ESlErrorCode::EC_InvalidCalibrationFile;
				case sl::ERROR_CODE::ERROR_CODE_INVALID_SVO_FILE:
					return ESlErrorCode::EC_InvalidSVOFile;
				case sl::ERROR_CODE::ERROR_CODE_SVO_RECORDING_ERROR:
					return ESlErrorCode::EC_SVORecordingError;
				case sl::ERROR_CODE::ERROR_CODE_INVALID_COORDINATE_SYSTEM:
					return ESlErrorCode::EC_InvalidCoordinateSystem;
				case sl::ERROR_CODE::ERROR_CODE_INVALID_FIRMWARE:
					return ESlErrorCode::EC_InvalidFirmware;
				case sl::ERROR_CODE::ERROR_CODE_INVALID_FUNCTION_PARAMETERS:
					return ESlErrorCode::EC_InvalidFunctionParameters;
				case sl::ERROR_CODE::ERROR_CODE_NOT_A_NEW_FRAME:
					return ESlErrorCode::EC_NotANewFrame;
				case sl::ERROR_CODE::ERROR_CODE_CUDA_ERROR:
					return ESlErrorCode::EC_CUDAError;
				case sl::ERROR_CODE::ERROR_CODE_CAMERA_NOT_INITIALIZED:
					return ESlErrorCode::EC_CameraNotInitialized;
				case sl::ERROR_CODE::ERROR_CODE_NVIDIA_DRIVER_OUT_OF_DATE:
					return ESlErrorCode::EC_NVIDIADriverOutOfDate;
				case sl::ERROR_CODE::ERROR_CODE_INVALID_FUNCTION_CALL:
					return ESlErrorCode::EC_InvalidFunctionCall;
				case sl::ERROR_CODE::ERROR_CODE_CORRUPTED_SDK_INSTALLATION:
					return ESlErrorCode::EC_CorruptedSDKInstallation;
				case sl::ERROR_CODE::ERROR_CODE_INCOMPATIBLE_SDK_VERSION:
					return ESlErrorCode::EC_IncompatibleSDKVersion;
				case sl::ERROR_CODE::ERROR_CODE_INVALID_AREA_FILE:
					return ESlErrorCode::EC_InvalidAreaFile;
				case sl::ERROR_CODE::ERROR_CODE_INCOMPATIBLE_AREA_FILE:
					return ESlErrorCode::EC_IncompatibleAreaFile;
				case sl::ERROR_CODE::ERROR_CODE_CAMERA_DETECTION_ISSUE:
					return ESlErrorCode::EC_CameraDetectionIssue;
				case sl::ERROR_CODE::ERROR_CODE_CAMERA_ALREADY_IN_USE:
					return ESlErrorCode::EC_CameraAlreadyInUse;
				case sl::ERROR_CODE::ERROR_CODE_LAST:
					return ESlErrorCode::EC_None;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::ERROR_CODE entry %u"), (uint32)SlType);

					return (ESlErrorCode)0;
				}
			}
		}

		/*
		 * Convert from sl::AREA_EXPORT_STATE to ESlSpatialMemoryExportState
		 */
		FORCEINLINE ESlSpatialMemoryExportState ToUnrealType(sl::AREA_EXPORT_STATE SlType)
		{
			switch (SlType)
			{
				case sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_SUCCESS:
					return ESlSpatialMemoryExportState::SMES_Success;
				case sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_RUNNING:
					return ESlSpatialMemoryExportState::SMES_Running;
				case sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_NOT_STARTED:
					return ESlSpatialMemoryExportState::SMES_NotStarted;
				case sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_FILE_EMPTY:
					return ESlSpatialMemoryExportState::SMES_FileEmpty;
				case sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_FILE_ERROR:
					return ESlSpatialMemoryExportState::SMES_FileError;
				case sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_SPATIAL_MEMORY_DISABLED:
					return ESlSpatialMemoryExportState::SMES_SpatialMemoryDisabled;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::AREA_EXPORT_STATE entry %u"), (uint32)SlType);

					return (ESlSpatialMemoryExportState)0;
				}
			}
		}

		/*
		 * Convert from sl::MAT_TYPE to ESlMatType
		 */
		FORCEINLINE ESlMatType ToUnrealType(sl::MAT_TYPE SlType)
		{
			switch (SlType)
			{
				case sl::MAT_TYPE::MAT_TYPE_32F_C1:
					return ESlMatType::MT_32F_C1;
				case sl::MAT_TYPE::MAT_TYPE_32F_C2:
					return ESlMatType::MT_32F_C2;
				case sl::MAT_TYPE::MAT_TYPE_32F_C3:
					return ESlMatType::MT_32F_C3;
				case sl::MAT_TYPE::MAT_TYPE_32F_C4:
					return ESlMatType::MT_32F_C4;
				case sl::MAT_TYPE::MAT_TYPE_8U_C1:
					return ESlMatType::MT_8U_C1;
				case sl::MAT_TYPE::MAT_TYPE_8U_C2:
					return ESlMatType::MT_8U_C2;
				case sl::MAT_TYPE::MAT_TYPE_8U_C3:
					return ESlMatType::MT_8U_C3;
				case sl::MAT_TYPE::MAT_TYPE_8U_C4:
					return ESlMatType::MT_8U_C4;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::MAT_TYPE entry %u"), (uint32)SlType);

					return (ESlMatType)0;
				}
						
			}
		}

		/*
		 * Convert from sl::MEM to ESlMemoryType
		 */
		FORCEINLINE ESlMemoryType ToUnrealType(sl::MEM SlType)
		{
			if (SlType == sl::MEM::MEM_CPU)
			{
				return ESlMemoryType::MT_CPU;
			}
			else if (SlType == sl::MEM::MEM_GPU)
			{
				return ESlMemoryType::MT_GPU;
			}
			else if (SlType == (sl::MEM::MEM_CPU | sl::MEM::MEM_GPU))
			{
				return (ESlMemoryType::MT_CPU | ESlMemoryType::MT_GPU);
			}

			ensureMsgf(false, TEXT("Unhandled sl::MEM entry %u"), (uint32)SlType);

			return (ESlMemoryType)0;
		}

		/*
		 * Convert from sl::MODEL to ESlModel
		 */
		FORCEINLINE ESlModel ToUnrealType(sl::MODEL SlType)
		{
			switch (SlType)
			{
				case sl::MODEL::MODEL_ZED:
					return ESlModel::M_Zed;
				case sl::MODEL::MODEL_ZED_M:
					return ESlModel::M_ZedM;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::MODEL entry %u"), (uint32)SlType);

					return (ESlModel)0;
				}
			}
		}

		/*
		 * Convert from sl::CAMERA_STATE to ESlCameraState
		 */
		FORCEINLINE ESlCameraState ToUnrealType(sl::CAMERA_STATE SlType)
		{
			switch (SlType)
			{
				case sl::CAMERA_STATE::CAMERA_STATE_AVAILABLE:
					return ESlCameraState::CS_Available;
				case sl::CAMERA_STATE::CAMERA_STATE_NOT_AVAILABLE:
					return ESlCameraState::CS_NotAvailable;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::CAMERA_STATE entry %u"), (uint32)SlType);

					return (ESlCameraState)0;
				}
			}
		}

		/*
	 	 * Convert from sl::SENSING_MODE to ESlSensingMode
	 	 */
		FORCEINLINE ESlSensingMode ToUnrealType(sl::SENSING_MODE SlType)
		{
			switch (SlType)
			{
				case sl::SENSING_MODE::SENSING_MODE_FILL:
					return ESlSensingMode::SM_Fill;
				case sl::SENSING_MODE::SENSING_MODE_STANDARD:
					return ESlSensingMode::SM_Standard;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled sl::SENSING_MODE entry %u"), (uint32)SlType);

					return (ESlSensingMode)0;
				}
			}
		}

		/*
		 * Convert from ESlSelfCalibrationState to sl::SELF_CALIBRATION_STATE
		 */
		FORCEINLINE sl::SELF_CALIBRATION_STATE ToUnrealType(ESlSelfCalibrationState UnrealType)
		{
			switch (UnrealType)
			{
				case ESlSelfCalibrationState::SCS_NotStarted:
					return sl::SELF_CALIBRATION_STATE::SELF_CALIBRATION_STATE_NOT_STARTED;
				case ESlSelfCalibrationState::SCS_Running:
					return sl::SELF_CALIBRATION_STATE::SELF_CALIBRATION_STATE_RUNNING;
				case ESlSelfCalibrationState::SCS_Failed:
					return sl::SELF_CALIBRATION_STATE::SELF_CALIBRATION_STATE_FAILED;
				case ESlSelfCalibrationState::SCS_Success:
					return sl::SELF_CALIBRATION_STATE::SELF_CALIBRATION_STATE_SUCCESS;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlSelfCalibrationState entry %u"), (uint32)UnrealType);

					return (sl::SELF_CALIBRATION_STATE)0;
				}
			}
		};

		/*
		 * Convert from ESlTimeReference to sl::TIME_REFERENCE
		 */
		FORCEINLINE sl::TIME_REFERENCE ToSlType(ESlTimeReference UnrealType)
		{
			switch (UnrealType)
			{
				case ESlTimeReference::TR_Image:
					return sl::TIME_REFERENCE::TIME_REFERENCE_IMAGE;
				case ESlTimeReference::TR_Current:
					return sl::TIME_REFERENCE::TIME_REFERENCE_CURRENT;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlTimeReference entry %u"), (uint32)UnrealType);

					return (sl::TIME_REFERENCE)0;
				}
			}
		};

		/*
		 * Convert from ESlReferenceFrame to sl::REFERENCE_FRAME
		 */
		FORCEINLINE sl::REFERENCE_FRAME ToSlType(ESlReferenceFrame UnrealType)
		{
			switch (UnrealType)
			{
				case ESlReferenceFrame::RF_Camera:
					return sl::REFERENCE_FRAME::REFERENCE_FRAME_CAMERA;
				case ESlReferenceFrame::RF_World:
					return sl::REFERENCE_FRAME::REFERENCE_FRAME_WORLD;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlReferenceFrame entry %u"), (uint32)UnrealType);

					return (sl::REFERENCE_FRAME)0;
				}
			}
		};

		/*
		 * Convert from ESlTrackingState to sl::TRACKING_STATE
		 */
		FORCEINLINE sl::TRACKING_STATE ToSlType(ESlTrackingState UnrealType)
		{
			switch (UnrealType)
			{
				case ESlTrackingState::TS_TrackingSearch:
					return sl::TRACKING_STATE::TRACKING_STATE_SEARCHING;
				case ESlTrackingState::TS_TrackingOk:
					return sl::TRACKING_STATE::TRACKING_STATE_OK;
				case ESlTrackingState::TS_TrackingOff:
					return sl::TRACKING_STATE::TRACKING_STATE_OFF;
				case ESlTrackingState::TS_FpsTooLow:
					return sl::TRACKING_STATE::TRACKING_STATE_FPS_TOO_LOW;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlTrackingState entry %u"), (uint32)UnrealType);

					return (sl::TRACKING_STATE)0;
				}
			}
		}

		/*
		 * Convert from ESlResolution to sl::RESOLUTION
		 */
		FORCEINLINE sl::RESOLUTION ToSlType(ESlResolution UnrealType)
		{
			switch (UnrealType)
			{
				case ESlResolution::R_HD2K:
					return sl::RESOLUTION::RESOLUTION_HD2K;
				case ESlResolution::R_HD1080:
					return sl::RESOLUTION::RESOLUTION_HD1080;
				case ESlResolution::R_HD720:
					return sl::RESOLUTION::RESOLUTION_HD720;
				case ESlResolution::R_VGA:
					return sl::RESOLUTION::RESOLUTION_VGA;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlResolution entry %u"), (uint32)UnrealType);

					return (sl::RESOLUTION)0;
				}		
			}
		}

		/*
		 * Convert from ESlDepthMode to sl::DEPTH_MODE
		 */
		FORCEINLINE sl::DEPTH_MODE ToSlType(ESlDepthMode UnrealType)
		{
			switch (UnrealType)
			{
				case ESlDepthMode::DM_None:
					return sl::DEPTH_MODE::DEPTH_MODE_NONE;
				case ESlDepthMode::DM_Performance:
					return sl::DEPTH_MODE::DEPTH_MODE_PERFORMANCE;
				case ESlDepthMode::DM_Medium:
					return sl::DEPTH_MODE::DEPTH_MODE_MEDIUM;
				case ESlDepthMode::DM_Quality:
					return sl::DEPTH_MODE::DEPTH_MODE_QUALITY;
				case ESlDepthMode::DM_Ultra:
					return sl::DEPTH_MODE::DEPTH_MODE_ULTRA;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlDepthMode entry %u"), (uint32)UnrealType);

					return (sl::DEPTH_MODE)0;
				}		
			}
		}

		/*
		 * Convert from ESlMeasure to sl::MEASURE
		 */
		FORCEINLINE sl::MEASURE ToSlType(ESlMeasure UnrealType)
		{
			switch (UnrealType)
			{
				case ESlMeasure::M_Depth:
					return sl::MEASURE::MEASURE_DEPTH;
				case ESlMeasure::M_DepthRight:
					return sl::MEASURE::MEASURE_DEPTH_RIGHT;
				case ESlMeasure::M_Disparity:
					return sl::MEASURE::MEASURE_DISPARITY;
				case ESlMeasure::M_DisparityRight:
					return sl::MEASURE::MEASURE_DISPARITY_RIGHT;
				case ESlMeasure::M_Normals:
					return sl::MEASURE::MEASURE_NORMALS;
				case ESlMeasure::M_NormalsRight:
					return sl::MEASURE::MEASURE_NORMALS_RIGHT;
				case ESlMeasure::M_XYZ:
					return sl::MEASURE::MEASURE_XYZ;
				case ESlMeasure::M_XYZ_Right:
					return sl::MEASURE::MEASURE_XYZ_RIGHT;
				case ESlMeasure::M_XYZ_RGBA:
					return sl::MEASURE::MEASURE_XYZRGBA;
				case ESlMeasure::M_XYZ_RGBA_Right:
					return sl::MEASURE::MEASURE_XYZRGBA_RIGHT;
				case ESlMeasure::M_XYZ_BGRA:
					return sl::MEASURE::MEASURE_XYZBGRA;
				case ESlMeasure::M_XYZ_BGRA_Right:
					return sl::MEASURE::MEASURE_XYZBGRA_RIGHT;
				case ESlMeasure::M_XYZ_ARGB:
					return sl::MEASURE::MEASURE_XYZARGB;
				case ESlMeasure::M_XYZ_ARGB_Right:
					return sl::MEASURE::MEASURE_XYZARGB_RIGHT;
				case ESlMeasure::M_XYZ_ABGR:
					return sl::MEASURE::MEASURE_XYZABGR;
				case ESlMeasure::M_XYZ_ABGR_Right:
					return sl::MEASURE::MEASURE_XYZABGR_RIGHT;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlMeasure entry %u"), (uint32)UnrealType);

					return (sl::MEASURE)0;
				}
			}
		}

		/*
		 * Convert from ESlView to sl::VIEW
		 */
		FORCEINLINE sl::VIEW ToSlType(ESlView UnrealType)
		{
			switch (UnrealType)
			{
				case ESlView::V_Left:
					return  sl::VIEW::VIEW_LEFT;
				case ESlView::V_Right:
					return sl::VIEW::VIEW_RIGHT;
				case ESlView::V_LeftUnrectified:
					return sl::VIEW::VIEW_LEFT_UNRECTIFIED;
				case ESlView::V_RightUnrectified:
					return sl::VIEW::VIEW_RIGHT_UNRECTIFIED;
				case ESlView::V_LeftUnrectifiedGray:
					return sl::VIEW::VIEW_LEFT_UNRECTIFIED_GRAY;
				case ESlView::V_RightUnrectifiedGray:
					return sl::VIEW::VIEW_RIGHT_UNRECTIFIED_GRAY;
				case ESlView::V_LeftGray:
					return sl::VIEW::VIEW_LEFT_GRAY;
				case ESlView::V_RightGray:
					return sl::VIEW::VIEW_RIGHT_GRAY;
				case ESlView::V_SideBySide:
					return sl::VIEW::VIEW_SIDE_BY_SIDE;
				case ESlView::V_Depth:
					return sl::VIEW::VIEW_DEPTH;
				case ESlView::V_DepthRight:
					return sl::VIEW::VIEW_DEPTH_RIGHT;
				case ESlView::V_Confidence:
					return sl::VIEW::VIEW_CONFIDENCE;
				case ESlView::V_Normals:
					return sl::VIEW::VIEW_NORMALS;
				case ESlView::V_NormalsRight:
					return sl::VIEW::VIEW_NORMALS_RIGHT;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlView entry %u"), (uint32)UnrealType);

					return (sl::VIEW)0;
				}
			}
		}

		/*
		 * Convert from EStereoRenderingDeviceType to sl::mr::HMD_DEVICE_TYPE
		 */
		FORCEINLINE sl::mr::HMD_DEVICE_TYPE ToSlType(EHMDDeviceType::Type UnrealType)
		{
			switch (UnrealType)
			{
				case EHMDDeviceType::DT_OculusRift:
					return sl::mr::HMD_DEVICE_TYPE::HMD_DEVICE_TYPE_OCULUS;
				case EHMDDeviceType::DT_SteamVR:
					return sl::mr::HMD_DEVICE_TYPE::HMD_DEVICE_TYPE_HTC;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled EHMDDeviceType entry %u"), (uint32)UnrealType);

					return (sl::mr::HMD_DEVICE_TYPE)0;
				}
			}
		}

		/*
		 * Convert from ESlErrorCode to sl::ERROR_CODE
		 */
		FORCEINLINE sl::ERROR_CODE ToSlType(ESlErrorCode UnrealType)
		{
			switch (UnrealType)
			{
				case ESlErrorCode::EC_Success:
					return sl::ERROR_CODE::SUCCESS;
				case ESlErrorCode::EC_Failure:
					return sl::ERROR_CODE::ERROR_CODE_FAILURE;
				case ESlErrorCode::EC_NoGpuCompatible:
					return sl::ERROR_CODE::ERROR_CODE_NO_GPU_COMPATIBLE;
				case ESlErrorCode::EC_NotEnoughGPUMemory:
					return sl::ERROR_CODE::ERROR_CODE_NOT_ENOUGH_GPUMEM;
				case ESlErrorCode::EC_CameraNotDetected:
					return sl::ERROR_CODE::ERROR_CODE_CAMERA_NOT_DETECTED;
				case ESlErrorCode::EC_SensorNotDetected:
					return sl::ERROR_CODE::ERROR_CODE_SENSOR_NOT_DETECTED;
				case ESlErrorCode::EC_InvalidResolution:
					return sl::ERROR_CODE::ERROR_CODE_INVALID_RESOLUTION;
				case ESlErrorCode::EC_LowUSBBandwidth:
					return sl::ERROR_CODE::ERROR_CODE_LOW_USB_BANDWIDTH;
				case ESlErrorCode::EC_CalibrationFileNotAvailable:
					return sl::ERROR_CODE::ERROR_CODE_CALIBRATION_FILE_NOT_AVAILABLE;
				case ESlErrorCode::EC_InvalidCalibrationFile:
					return sl::ERROR_CODE::ERROR_CODE_INVALID_CALIBRATION_FILE;
				case ESlErrorCode::EC_InvalidSVOFile:
					return sl::ERROR_CODE::ERROR_CODE_INVALID_SVO_FILE;
				case ESlErrorCode::EC_SVORecordingError:
					return sl::ERROR_CODE::ERROR_CODE_SVO_RECORDING_ERROR;
				case ESlErrorCode::EC_InvalidCoordinateSystem:
					return sl::ERROR_CODE::ERROR_CODE_INVALID_COORDINATE_SYSTEM;
				case ESlErrorCode::EC_InvalidFirmware:
					return sl::ERROR_CODE::ERROR_CODE_INVALID_FIRMWARE;
				case ESlErrorCode::EC_InvalidFunctionParameters:
					return sl::ERROR_CODE::ERROR_CODE_INVALID_FUNCTION_PARAMETERS;
				case ESlErrorCode::EC_NotANewFrame:
					return sl::ERROR_CODE::ERROR_CODE_NOT_A_NEW_FRAME;
				case ESlErrorCode::EC_CUDAError:
					return sl::ERROR_CODE::ERROR_CODE_CUDA_ERROR;
				case ESlErrorCode::EC_CameraNotInitialized:
					return sl::ERROR_CODE::ERROR_CODE_CAMERA_NOT_INITIALIZED;
				case ESlErrorCode::EC_NVIDIADriverOutOfDate:
					return sl::ERROR_CODE::ERROR_CODE_NVIDIA_DRIVER_OUT_OF_DATE;
				case ESlErrorCode::EC_InvalidFunctionCall:
					return sl::ERROR_CODE::ERROR_CODE_INVALID_FUNCTION_CALL;
				case ESlErrorCode::EC_CorruptedSDKInstallation:
					return sl::ERROR_CODE::ERROR_CODE_CORRUPTED_SDK_INSTALLATION;
				case ESlErrorCode::EC_IncompatibleSDKVersion:
					return sl::ERROR_CODE::ERROR_CODE_INCOMPATIBLE_SDK_VERSION;
				case ESlErrorCode::EC_InvalidAreaFile:
					return sl::ERROR_CODE::ERROR_CODE_INVALID_AREA_FILE;
				case ESlErrorCode::EC_IncompatibleAreaFile:
					return sl::ERROR_CODE::ERROR_CODE_INCOMPATIBLE_AREA_FILE;
				case ESlErrorCode::EC_CameraDetectionIssue:
					return sl::ERROR_CODE::ERROR_CODE_CAMERA_DETECTION_ISSUE;
				case ESlErrorCode::EC_CameraAlreadyInUse:
					return sl::ERROR_CODE::ERROR_CODE_CAMERA_ALREADY_IN_USE;
				case ESlErrorCode::EC_None:
					return sl::ERROR_CODE::ERROR_CODE_LAST;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlErrorCode entry %u"), (uint32)UnrealType);

					return (sl::ERROR_CODE)0;
				}
			}
		}

		/*
		 * Convert from ESlSpatialMemoryExportState to sl::AREA_EXPORT_STATE
		 */
		FORCEINLINE sl::AREA_EXPORT_STATE ToSlType(ESlSpatialMemoryExportState UnrealType)
		{
			switch (UnrealType)
			{
				case ESlSpatialMemoryExportState::SMES_Success:
					return sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_SUCCESS;
				case ESlSpatialMemoryExportState::SMES_Running:
					return sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_RUNNING;
				case ESlSpatialMemoryExportState::SMES_NotStarted:
					return sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_NOT_STARTED;
				case ESlSpatialMemoryExportState::SMES_FileEmpty:
					return sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_FILE_EMPTY;
				case ESlSpatialMemoryExportState::SMES_FileError:
					return sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_FILE_ERROR;
				case ESlSpatialMemoryExportState::SMES_SpatialMemoryDisabled:
					return sl::AREA_EXPORT_STATE::AREA_EXPORT_STATE_SPATIAL_MEMORY_DISABLED;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlSpatialMemoryExportState entry %u"), (uint32)UnrealType);

					return (sl::AREA_EXPORT_STATE)0;
				}
			}
		}

		/*
		 * Convert from ESlSVOCompressionMode to sl::SVO_COMPRESSION_MODE
		 */
		FORCEINLINE sl::SVO_COMPRESSION_MODE ToSlType(ESlSVOCompressionMode UnrealType)
		{
			switch (UnrealType)
			{
				case ESlSVOCompressionMode::SCM_Lossless:
					return sl::SVO_COMPRESSION_MODE::SVO_COMPRESSION_MODE_LOSSLESS;
				case ESlSVOCompressionMode::SCM_Lossy:
					return sl::SVO_COMPRESSION_MODE::SVO_COMPRESSION_MODE_LOSSY;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlSVOCompressionMode entry %u"), (uint32)UnrealType);

					return (sl::SVO_COMPRESSION_MODE)0;
				}
			}
		}

		/*
		 * Convert from ESlMeshFileFormat to sl::MESH_FILE_FORMAT
		 */
		FORCEINLINE sl::MESH_FILE_FORMAT ToSlType(ESlMeshFileFormat UnrealType)
		{
			switch (UnrealType)
			{
				case ESlMeshFileFormat::MFF_OBJ:
					return sl::MESH_FILE_FORMAT::MESH_FILE_OBJ;
				case ESlMeshFileFormat::MFF_PLY:
					return sl::MESH_FILE_FORMAT::MESH_FILE_PLY;
				case ESlMeshFileFormat::MFF_PLY_BIN:
					return sl::MESH_FILE_FORMAT::MESH_FILE_PLY_BIN;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlMeshFileFormat entry %u"), (uint32)UnrealType);

					return (sl::MESH_FILE_FORMAT)0;
				}
			}
		}

		/*
		 * Convert from ESlMeshTextureFormat to sl::MESH_TEXTURE_FORMAT
	 	 */
		FORCEINLINE sl::MESH_TEXTURE_FORMAT ToSlType(ESlMeshTextureFormat UnrealType)
		{
			switch (UnrealType)
			{
				case ESlMeshTextureFormat::MTF_RGB:
					return sl::MESH_TEXTURE_FORMAT::MESH_TEXTURE_RGB;
				case ESlMeshTextureFormat::MTF_RGBA:
					return sl::MESH_TEXTURE_FORMAT::MESH_TEXTURE_RGBA;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlMeshTextureFormat entry %u"), (uint32)UnrealType);

					return (sl::MESH_TEXTURE_FORMAT)0;
				}
			}
		}

		/*
		 * Convert from ESlFilterIntensity to sl::MeshFilterParameters::FILTER
		 */
		FORCEINLINE sl::MeshFilterParameters::MESH_FILTER ToSlType(ESlFilterIntensity UnrealType)
		{
			switch (UnrealType)
			{
				case ESlFilterIntensity::FI_Low:
					return  sl::MeshFilterParameters::MESH_FILTER::MESH_FILTER_LOW;
				case ESlFilterIntensity::FI_Medium:
					return  sl::MeshFilterParameters::MESH_FILTER::MESH_FILTER_MEDIUM;
				case ESlFilterIntensity::FI_High:
					return  sl::MeshFilterParameters::MESH_FILTER::MESH_FILTER_HIGH;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlFilterIntensity entry %u"), (uint32)UnrealType);

					return (sl::MeshFilterParameters::MESH_FILTER)0;
				}
			}
		}
		
		/*
		 * Convert from ESlUnit to sl::UNIT
		 */
		FORCEINLINE sl::UNIT ToSlType(ESlUnit UnrealType)
		{
			switch (UnrealType)
			{
				case ESlUnit::DU_Centimeter:
					return sl::UNIT::UNIT_CENTIMETER;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlUnit entry %u"), (uint32)UnrealType);

					return (sl::UNIT)0;
				}
			}
		}

		/*
		 * Convert from ESlCoordinateSystem to sl::COORDINATE_SYSTEM
		 */
		FORCEINLINE sl::COORDINATE_SYSTEM ToSlType(ESlCoordinateSystem UnrealType)
		{
			switch (UnrealType)
			{
				case ESlCoordinateSystem::CS_Cartesian:
					return sl::COORDINATE_SYSTEM::COORDINATE_SYSTEM_LEFT_HANDED_Z_UP;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlCoordinateSystem entry %u"), (uint32)UnrealType);

					return (sl::COORDINATE_SYSTEM)0;
				}
			}
		}

		/*
		 * Convert from ESlCopyType to sl::COPY_TYPE
		 */
		FORCEINLINE sl::COPY_TYPE ToSlType(ESlCopyType UnrealType)
		{
			switch (UnrealType)
			{
				case ESlCopyType::CT_CPUToCPU:
					return sl::COPY_TYPE::COPY_TYPE_CPU_CPU;
				case ESlCopyType::CT_CPUToGPU:
					return sl::COPY_TYPE::COPY_TYPE_CPU_GPU;
				case ESlCopyType::CT_GPUToCPU:
					return sl::COPY_TYPE::COPY_TYPE_GPU_CPU;
				case ESlCopyType::CT_GPUToGPU:
					return sl::COPY_TYPE::COPY_TYPE_GPU_GPU;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlCopyType entry %u"), (uint32)UnrealType);

					return (sl::COPY_TYPE)0;
				}
			}
		}

		/*
		 * Convert from ESlMatType to sl::MAT_TYPE
		 */
		FORCEINLINE sl::MAT_TYPE ToSlType(ESlMatType UnrealType)
		{
			switch (UnrealType)
			{
				case ESlMatType::MT_32F_C1:
					return sl::MAT_TYPE::MAT_TYPE_32F_C1;
				case ESlMatType::MT_32F_C2:
					return sl::MAT_TYPE::MAT_TYPE_32F_C2;
				case ESlMatType::MT_32F_C3:
					return sl::MAT_TYPE::MAT_TYPE_32F_C3;
				case  ESlMatType::MT_32F_C4:
					return sl::MAT_TYPE::MAT_TYPE_32F_C4;
				case ESlMatType::MT_8U_C1:
					return sl::MAT_TYPE::MAT_TYPE_8U_C1;
				case ESlMatType::MT_8U_C2:
					return sl::MAT_TYPE::MAT_TYPE_8U_C2;
				case ESlMatType::MT_8U_C3:
					return sl::MAT_TYPE::MAT_TYPE_8U_C3;
				case ESlMatType::MT_8U_C4:
					return sl::MAT_TYPE::MAT_TYPE_8U_C4;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlMatType entry %u"), (uint32)UnrealType);

					return (sl::MAT_TYPE)0;
				}
			}
		}

		/*
		 * Convert from ESlModel to sl::MODEL
		 */
		FORCEINLINE sl::MODEL ToSlType(ESlModel UnrealType)
		{
			switch (UnrealType)
			{
				case ESlModel::M_Zed:
					return  sl::MODEL::MODEL_ZED;
				case  ESlModel::M_ZedM:
					return sl::MODEL::MODEL_ZED_M;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlModel entry %u"), (uint32)UnrealType);

					return (sl::MODEL)0;
				}
			}
		}

		/*
		 * Convert from ESlCameraState to sl::CAMERA_STATE
		 */
		FORCEINLINE sl::CAMERA_STATE ToSlType(ESlCameraState UnrealType)
		{
			switch (UnrealType)
			{
				case ESlCameraState::CS_Available:
					return sl::CAMERA_STATE::CAMERA_STATE_AVAILABLE;
				case ESlCameraState::CS_NotAvailable:
					return sl::CAMERA_STATE::CAMERA_STATE_NOT_AVAILABLE;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlCameraState entry %u"), (uint32)UnrealType);

					return (sl::CAMERA_STATE)0;
				}
			}
		}

		/*
		 * Convert from ESlMemoryType to sl::MEM
		 */
		FORCEINLINE sl::MEM ToSlType(ESlMemoryType UnrealType)
		{
			if (UnrealType == ESlMemoryType::MT_CPU)
			{
				return sl::MEM::MEM_CPU;
			}
			else if(UnrealType == ESlMemoryType::MT_GPU)
			{
				return sl::MEM::MEM_GPU;
			}
			else if (UnrealType == (ESlMemoryType::MT_CPU | ESlMemoryType::MT_GPU))
			{
				return (sl::MEM::MEM_CPU | sl::MEM::MEM_GPU);
			}

			ensureMsgf(false, TEXT("Unhandled ESlMemoryType entry %u"), (uint32)UnrealType);

			return (sl::MEM)0;
		}

		/*
		 * Convert from ESlSpatialMappingRange to sl::SpatialMappingParameters::RANGE
		 */
		FORCEINLINE sl::SpatialMappingParameters::MAPPING_RANGE ToSlType(ESlSpatialMappingRange UnrealType)
		{
			switch (UnrealType)
			{
				case ESlSpatialMappingRange::SMR_Near:
					return sl::SpatialMappingParameters::MAPPING_RANGE::MAPPING_RANGE_NEAR;
				case ESlSpatialMappingRange::SMR_Medium:
					return sl::SpatialMappingParameters::MAPPING_RANGE::MAPPING_RANGE_MEDIUM;
				case ESlSpatialMappingRange::SMR_Far:
					return sl::SpatialMappingParameters::MAPPING_RANGE::MAPPING_RANGE_FAR;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlSpatialMappingRange entry %u"), (uint32)UnrealType);

					return (sl::SpatialMappingParameters::MAPPING_RANGE)0;
				}
			}
		}

		/*
		 * Convert from ESlSpatialMappingResolution to sl::SpatialMappingParameters::RESOLUTION
		 */
		FORCEINLINE sl::SpatialMappingParameters::MAPPING_RESOLUTION ToSlType(ESlSpatialMappingResolution UnrealType)
		{
			switch (UnrealType)
			{
				case ESlSpatialMappingResolution::SMR_Low:
					return sl::SpatialMappingParameters::MAPPING_RESOLUTION::MAPPING_RESOLUTION_LOW;
				case ESlSpatialMappingResolution::SMR_Medium:
					return sl::SpatialMappingParameters::MAPPING_RESOLUTION::MAPPING_RESOLUTION_MEDIUM;
				case ESlSpatialMappingResolution::SMR_High:
					return sl::SpatialMappingParameters::MAPPING_RESOLUTION::MAPPING_RESOLUTION_HIGH;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlSpatialMappingResolution entry %u"), (uint32)UnrealType);

					return (sl::SpatialMappingParameters::MAPPING_RESOLUTION)0;
				}
			}
		}

		/*
	 	 * Convert from ESlSensingMode to sl::SENSING_MODE
	 	 */
		FORCEINLINE sl::SENSING_MODE ToSlType(ESlSensingMode UnrealType)
		{
			switch (UnrealType)
			{
				case ESlSensingMode::SM_Fill:
					return sl::SENSING_MODE::SENSING_MODE_FILL;
				case ESlSensingMode::SM_Standard:
					return sl::SENSING_MODE::SENSING_MODE_STANDARD;
				default:
				{
					ensureMsgf(false, TEXT("Unhandled ESlSensingMode entry %u"), (uint32)UnrealType);

					return (sl::SENSING_MODE)0;
				}
			}
		}

		/*
		 * Convert from sl::Resolution to FIntPoint
		 */
		FORCEINLINE FIntPoint ToUnrealType(const sl::Resolution& SlVector)
		{
			return FIntPoint(SlVector.width, SlVector.height);

		}

		/*
		 * Convert from sl::uchar2 to FIntPoint
		 */
		FORCEINLINE FIntPoint ToUnrealType(const sl::uchar2& SlVector)
		{
			return FIntPoint(SlVector.x, SlVector.y);
		}

		/*
		 * Convert from sl::uchar3 to FIntVector
		 */
		FORCEINLINE FIntVector ToUnrealType(const sl::uchar3& SlVector)
		{
			return FIntVector(SlVector.x, SlVector.y, SlVector.z);
		}

		/*
		 * Convert from sl::uchar4 to FColor
		 */
		FORCEINLINE FColor ToUnrealType(const sl::uchar4& SlColor)
		{
			return FColor(SlColor.x, SlColor.y, SlColor.z, SlColor.w);
		}

		/*
	 	 * Convert from sl::float2 to FVector2D
		 */
		FORCEINLINE FVector2D ToUnrealType(const sl::float2& SlVector)
		{
			return FVector2D(SlVector.x, SlVector.y);
		}

		/*
		 * Convert from sl::float3 to FVector
		*/
		 FORCEINLINE FVector ToUnrealType(const sl::float3& SlVector)
		{
			return FVector(SlVector.x, SlVector.y, SlVector.z);
		}

		/*
		 * Convert from sl::float4 to FVector4
		 */
		FORCEINLINE FVector4 ToUnrealType(const sl::float4& SlVector)
		{
			return FVector4(SlVector.x, SlVector.y, SlVector.z, SlVector.w);
		}

		/*
		 * Convert from sl::Orientation to FQuat
		 */
		FORCEINLINE FQuat ToUnrealType(const sl::Orientation& SlQuat)
		{
			return FQuat(SlQuat.x, SlQuat.y, SlQuat.z, SlQuat.w);
		}

		/*
		 * Convert from sl::Matrix4f to FMatrix (row to column)
		 */
		FORCEINLINE FMatrix ToUnrealType(const sl::Matrix4f& SlMatrix)
		{
			FMatrix Matrix;

			// X plane
			Matrix.M[0][0] = SlMatrix.r00;
			Matrix.M[0][1] = SlMatrix.r10;
			Matrix.M[0][2] = SlMatrix.r20;
			Matrix.M[0][3] = SlMatrix.m30;

			// Y plane
			Matrix.M[1][0] = SlMatrix.r01;
			Matrix.M[1][1] = SlMatrix.r11;
			Matrix.M[1][2] = SlMatrix.r21;
			Matrix.M[1][3] = SlMatrix.m31;

			// Z plane
			Matrix.M[2][0] = SlMatrix.r02;
			Matrix.M[2][1] = SlMatrix.r12;
			Matrix.M[2][2] = SlMatrix.r22;
			Matrix.M[2][3] = SlMatrix.m32;

			// Origin
			Matrix.M[3][0] = SlMatrix.tx;
			Matrix.M[3][1] = SlMatrix.ty;
			Matrix.M[3][2] = SlMatrix.tz;
			Matrix.M[3][3] = SlMatrix.m33;

			return Matrix;
		}

		/*
		 * Convert from sl::Matrix3f to FRotationMatrix (row to column)
		 */
		FORCEINLINE FMatrix ToUnrealType(const sl::Matrix3f& SlMatrix)
		{
			FMatrix Matrix;

			// X plane
			Matrix.M[0][0] = SlMatrix.r00;
			Matrix.M[0][1] = SlMatrix.r10;
			Matrix.M[0][2] = SlMatrix.r20;

			// Y plane
			Matrix.M[1][0] = SlMatrix.r01;
			Matrix.M[1][1] = SlMatrix.r11;
			Matrix.M[1][2] = SlMatrix.r21;

			// Z plane
			Matrix.M[2][0] = SlMatrix.r02;
			Matrix.M[2][1] = SlMatrix.r12;
			Matrix.M[2][2] = SlMatrix.r22;

			return Matrix;
		}

		/*
		 * Convert from sl::Transform to FTransform
		 */
		FORCEINLINE FTransform ToUnrealType(const sl::Transform& SlTransform)
		{
			return FTransform(sl::unreal::ToUnrealType(static_cast<sl::Matrix4f>(SlTransform)));
		}

		/*
		 * Convert from sl::RecordingState to FSlRecordingState
		 */
		FORCEINLINE FSlRecordingState ToUnrealType(const sl::RecordingState& SlData)
		{
			FSlRecordingState RecordingState;

			RecordingState.CurrentCompressionTime = SlData.current_compression_time;
			RecordingState.CurrentCompressionRatio = SlData.current_compression_ratio;
			RecordingState.AverageCompressionTime = SlData.average_compression_time;
			RecordingState.AverageCompressionRatio = SlData.average_compression_ratio;
			RecordingState.Status = SlData.status;

			return RecordingState;
		}

		/*
		 * Convert from sl::DeviceProperties to FSlDeviceProperties
		 */
		FORCEINLINE FSlDeviceProperties ToUnrealType(const sl::DeviceProperties& SlData)
		{
			FSlDeviceProperties DeviceProperties;

			DeviceProperties.CameraModel = sl::unreal::ToUnrealType(SlData.camera_model);
			DeviceProperties.CameraState = sl::unreal::ToUnrealType(SlData.camera_state);
			DeviceProperties.ID = SlData.id;
			//DeviceProperties.Path = FString(SlData.path.c_str());
			DeviceProperties.SerialNumber = static_cast<int32>(SlData.serial_number);

			return DeviceProperties;
		}

		/*
		 * Convert from sl::CameraParameters to FSlCameraParameters
		 */
		FORCEINLINE FSlCameraParameters ToUnrealType(const sl::CameraParameters& SlData)
		{
			FSlCameraParameters CameraParameters;

			CameraParameters.HFOV = SlData.h_fov;
			CameraParameters.VFOV = SlData.v_fov;
			CameraParameters.HFocal = SlData.fx;
			CameraParameters.VFocal = SlData.fy;
			CameraParameters.OpticalCenterX = SlData.cx;
			CameraParameters.OpticalCenterY = SlData.cy;

			std::vector<double> Tmp;
			Tmp.assign(SlData.disto, SlData.disto + 5);
			CameraParameters.Disto = sl::unreal::arrays::ToUnrealType<float, double>(Tmp);

			CameraParameters.Resolution = sl::unreal::ToUnrealType(SlData.image_size);

			return CameraParameters;
		}

		/*
		 * Convert from sl::CalibrationParameters to FSlCalibrationParameters
		 */
		FORCEINLINE FSlCalibrationParameters ToUnrealType(const sl::CalibrationParameters& SlData)
		{
			FSlCalibrationParameters CalibrationParameters;

			CalibrationParameters.LeftCameraParameters = sl::unreal::ToUnrealType(SlData.left_cam);
			CalibrationParameters.RightCameraParameters = sl::unreal::ToUnrealType(SlData.right_cam);
			CalibrationParameters.Rotation = sl::unreal::ToUnrealType(SlData.R);
			CalibrationParameters.Translation = sl::unreal::ToUnrealType(SlData.T);

			return CalibrationParameters;
		}

		 
		/*
		 * Convert from sl::CameraInformation to FSlCameraInformation
		 */
		FORCEINLINE FSlCameraInformation ToUnrealType(const sl::CameraInformation& SlData)
		{
			FSlCameraInformation CameraInformation;

			CameraInformation.CalibrationParameters = sl::unreal::ToUnrealType(SlData.calibration_parameters);
			CameraInformation.CalibrationParametersRaw = sl::unreal::ToUnrealType(SlData.calibration_parameters_raw);
			CameraInformation.CameraModel = sl::unreal::ToUnrealType(SlData.camera_model);
			CameraInformation.FirmwareVersion = static_cast<int32>(SlData.firmware_version);
			CameraInformation.SerialNumber = static_cast<int32>(SlData.serial_number);
			CameraInformation.HalfBaseline = SlData.calibration_parameters.T.x / 2.0f;

			return CameraInformation;
		}

		/*
		 * Convert from sl::Pose to FSlPose
		 */
		FORCEINLINE FSlPose ToUnrealType(const sl::Pose& SlData)
		{
			FSlPose Pose;

			Pose.Transform = sl::unreal::ToUnrealType(SlData.pose_data);
			Pose.Timestamp = SlData.timestamp;
			Pose.Confidence = SlData.pose_confidence;
			Pose.bValid = SlData.valid;

			return Pose;
		}

		/*
		 * Convert from sl::IMUData to FSlIMUData
		 */
		FORCEINLINE FSlIMUData ToUnrealType(const sl::IMUData& SlData)
		{
			FSlIMUData IMUData;

			IMUData.Transform = sl::unreal::ToUnrealType(SlData.pose_data);
			IMUData.Timestamp = SlData.timestamp;
			IMUData.Confidence = SlData.pose_confidence;
			IMUData.bValid = SlData.valid;
			IMUData.OrientationCovariance = sl::unreal::ToUnrealType(SlData.orientation_covariance);
			IMUData.AngularVelocity = sl::unreal::ToUnrealType(SlData.angular_velocity);
			IMUData.LinearAcceleration = sl::unreal::ToUnrealType(SlData.linear_acceleration);
			IMUData.AngularVelocityConvariance = sl::unreal::ToUnrealType(SlData.angular_velocity_convariance);
			IMUData.LinearAccelerationConvariance = sl::unreal::ToUnrealType(SlData.linear_acceleration_convariance);

			return IMUData;
		}

		/*
		 * Convert from FMatrix to sl::Matrix4f (column to row)
		 */
		FORCEINLINE sl::Matrix4f ToSlType(const FMatrix& UnrealMatrix)
		{
			sl::Matrix4f Matrix;

			// X plane
			Matrix.r00 = UnrealMatrix.M[0][0];
			Matrix.r10 = UnrealMatrix.M[0][1];
			Matrix.r20 = UnrealMatrix.M[0][2];
			Matrix.m30 = UnrealMatrix.M[0][3];

			// Y plane
			Matrix.r01 = UnrealMatrix.M[1][0];
			Matrix.r11 = UnrealMatrix.M[1][1];
			Matrix.r21 = UnrealMatrix.M[1][2];
			Matrix.m31 = UnrealMatrix.M[1][3];

			// Z plane
			Matrix.r02 = UnrealMatrix.M[2][0];
			Matrix.r12 = UnrealMatrix.M[2][1];
			Matrix.r22 = UnrealMatrix.M[2][2];
			Matrix.m32 = UnrealMatrix.M[2][3];
			 
			// Origin
			Matrix.tx  = UnrealMatrix.M[3][0];
			Matrix.ty  = UnrealMatrix.M[3][1];
			Matrix.tz  = UnrealMatrix.M[3][2];
		    Matrix.m33 = UnrealMatrix.M[3][3];

			return Matrix;
		}

		/*
		 * Convert from FTransform to sl::Transform 
		 */
		FORCEINLINE sl::Transform ToSlType(const FTransform& UnrealTransform)
		{
			return static_cast<sl::Transform>(sl::unreal::ToSlType(UnrealTransform.ToMatrixWithScale()));
		}

		/*
		 * Convert from FIntPoint to sl::Resolution
		 */
		FORCEINLINE sl::Resolution ToSlType2(const FIntPoint& UnrealType)
		{
			return sl::Resolution(UnrealType.X, UnrealType.Y);
		}

		/*
		 * Convert from FIntPoint to sl::uchar2
		 */
		FORCEINLINE sl::uchar2 ToSlType(const FIntPoint& UnrealVector)
		{
			return sl::uchar2(FMath::Clamp(UnrealVector.X, 0, 255), FMath::Clamp(UnrealVector.Y, 0, 255));
		}

		/*
		 * Convert from FVector to sl::uchar3
		 */
		FORCEINLINE sl::uchar3 ToSlType(const FIntVector& UnrealVector)
		{
			return sl::uchar3(FMath::Clamp(UnrealVector.X, 0, 255), FMath::Clamp(UnrealVector.Y, 0, 255), FMath::Clamp(UnrealVector.Z, 0, 255));
		}

		/*
		 * Convert from FColor to sl::uchar4
		 */
		FORCEINLINE sl::uchar4 ToSlType(const FColor& UnrealColor)
		{
			return sl::uchar4(UnrealColor.R, UnrealColor.G, UnrealColor.B, UnrealColor.A);
		}

		/*
		 * Convert from FVector2D to sl::float2
		 */
		FORCEINLINE sl::float2 ToSlType(const FVector2D& UnrealVector)
		{
			return sl::float2(UnrealVector.X, UnrealVector.Y);
		}

		/*
		 * Convert from FVector to sl::float3
		 */
		FORCEINLINE sl::float3 ToSlType(const FVector& UnrealVector)
		{
			return sl::float3(UnrealVector.X, UnrealVector.Y, UnrealVector.Z);
		}

		/*
		 * Convert from FVector4 to sl::float4
		 */
		FORCEINLINE sl::float4 ToSlType(const FVector4& UnrealVector)
		{
			return sl::float4(UnrealVector.X, UnrealVector.Y, UnrealVector.Z, UnrealVector.W);
		}

		/*
		 * Convert from FSlTrackingParameters to sl::TrackingParameters
		 */
		FORCEINLINE sl::TrackingParameters ToSlType(const FSlTrackingParameters& UnrealData)
		{
			sl::TrackingParameters TrackingParameters;

			if (UnrealData.bLoadSpatialMemoryFile)
			{
				TrackingParameters.area_file_path = TCHAR_TO_UTF8(*UnrealData.SpatialMemoryFileLoadingPath);
			}
			TrackingParameters.enable_spatial_memory = UnrealData.bEnableSpatialMemory;
			TrackingParameters.enable_pose_smoothing = UnrealData.bEnablePoseSmoothing;
			TrackingParameters.initial_world_transform = sl::unreal::ToSlType(FTransform(UnrealData.Rotation, UnrealData.Location));

			return TrackingParameters;
		}

		/*
		 * Covnert from FSlInitParameters to sl::InitParameters
		 */
		FORCEINLINE sl::InitParameters ToSlType(const FSlInitParameters& UnrealData)
		{
			sl::InitParameters InitParameters;

			InitParameters.camera_disable_self_calib = UnrealData.bDisableSelfCalibration;
			InitParameters.camera_fps = UnrealData.FPS;
			InitParameters.camera_image_flip = UnrealData.bVerticalFlipImage;
			InitParameters.camera_resolution = sl::unreal::ToSlType(UnrealData.Resolution);
			InitParameters.coordinate_system = sl::unreal::ToSlType(UnrealData.CoordinateSystem);
			InitParameters.coordinate_units = sl::unreal::ToSlType(UnrealData.Unit);
			InitParameters.depth_minimum_distance = UnrealData.DepthMinimumDistance;
			InitParameters.depth_mode = sl::unreal::ToSlType(UnrealData.DepthMode);
			InitParameters.enable_right_side_measure = UnrealData.bEnableRightSideMeasure;
			InitParameters.sdk_gpu_id = FMath::FloorToInt(UnrealData.GPUID);
			InitParameters.sdk_verbose = UnrealData.bVerbose;
			InitParameters.sdk_verbose_log_file = TCHAR_TO_UTF8(*UnrealData.VerboseFilePath);
			if (UnrealData.bUseSVO)
			{
				InitParameters.svo_input_filename = TCHAR_TO_UTF8(*UnrealData.SVOFilePath);
			}
			InitParameters.svo_real_time_mode = UnrealData.bRealTime;
			InitParameters.depth_stabilization = UnrealData.bEnableDepthStabilization;

			return InitParameters;
		}

		/*
		 * Convert from FSlRuntimeParameters to sl::RuntimeParameters
		 */
		FORCEINLINE sl::RuntimeParameters ToSlType(const FSlRuntimeParameters& UnrealData)
		{
			sl::RuntimeParameters RuntimeParameters;
			
			RuntimeParameters.enable_depth = UnrealData.bEnableDepth;
			RuntimeParameters.enable_point_cloud = UnrealData.bEnablePointCloud;
			RuntimeParameters.measure3D_reference_frame = sl::unreal::ToSlType(UnrealData.ReferenceFrame);
			RuntimeParameters.sensing_mode = sl::unreal::ToSlType(UnrealData.SensingMode);
			
			return RuntimeParameters;
		}

		/*
		 * Convert from FSlSpatialMappingParameters to sl::SpatialMappingParameters
		 */
		FORCEINLINE sl::SpatialMappingParameters ToSlType(const FSlSpatialMappingParameters& UnrealData)
		{
			sl::SpatialMappingParameters SpatialMappingParameters;

			SpatialMappingParameters.max_memory_usage = UnrealData.MaxMemoryUsage;
			SpatialMappingParameters.save_texture = UnrealData.bSaveTexture;
			SpatialMappingParameters.range_meter = (UnrealData.PresetRange == ESlSpatialMappingRange::SMR_Custom ? UnrealData.MaxRange : sl::SpatialMappingParameters::get(sl::unreal::ToSlType(UnrealData.PresetRange)));
			SpatialMappingParameters.resolution_meter = (UnrealData.PresetResolution == ESlSpatialMappingResolution::SMR_Custom || UnrealData.PresetResolution == ESlSpatialMappingResolution::SMR_VeryLow ? UnrealData.Resolution : sl::SpatialMappingParameters::get(sl::unreal::ToSlType(UnrealData.PresetResolution)));;
			SpatialMappingParameters.use_chunk_only = UnrealData.GetUseChunksOnly();
			SpatialMappingParameters.reverse_vertex_order = true;

			return SpatialMappingParameters;
		}

		/*
		 * Convert from FSlMeshFilterParameters to sl::MeshFilterParameters
		 */
		FORCEINLINE sl::MeshFilterParameters ToSlType(const FSlMeshFilterParameters& UnrealData)
		{
			return sl::MeshFilterParameters(sl::unreal::ToSlType(UnrealData.FilterIntensity));
		}

		/*
		* Convert from sl::CameraParameters to FSlCameraParameters
		*/
		FORCEINLINE sl::CameraParameters ToSlType(const FSlCameraParameters& UnrealData)
		{
			sl::CameraParameters CameraParameters;

			CameraParameters.h_fov = UnrealData.HFOV;
			CameraParameters.v_fov = UnrealData.VFOV;
			CameraParameters.fx = UnrealData.HFocal;
			CameraParameters.fy = UnrealData.VFocal;
			CameraParameters.cx = UnrealData.OpticalCenterX;
			CameraParameters.cy = UnrealData.OpticalCenterY;

			CameraParameters.disto[0] = UnrealData.Disto[0];
			CameraParameters.disto[1] = UnrealData.Disto[1];
			CameraParameters.disto[2] = UnrealData.Disto[2];
			CameraParameters.disto[3] = UnrealData.Disto[3];
			CameraParameters.disto[4] = UnrealData.Disto[4];

			CameraParameters.image_size = sl::unreal::ToSlType2(UnrealData.Resolution);

			return CameraParameters;
		}

		/*
		* Convert from sl::CalibrationParameters to FSlCalibrationParameters
		*/
		FORCEINLINE sl::CalibrationParameters ToSlType(const FSlCalibrationParameters& UnrealData)
		{
			sl::CalibrationParameters CalibrationParameters;

			CalibrationParameters.left_cam = sl::unreal::ToSlType(UnrealData.LeftCameraParameters);
			CalibrationParameters.left_cam = sl::unreal::ToSlType(UnrealData.RightCameraParameters);
			CalibrationParameters.R = sl::unreal::ToSlType(UnrealData.Rotation);
			CalibrationParameters.T = sl::unreal::ToSlType(UnrealData.Translation);

			return CalibrationParameters;
		}


		/*
		* Convert from sl::CameraInformation to FSlCameraInformation
		*/
		FORCEINLINE sl::CameraInformation ToSlType(const FSlCameraInformation& UnrealData)
		{
			sl::CameraInformation CameraInformation;

			CameraInformation.calibration_parameters = sl::unreal::ToSlType(UnrealData.CalibrationParameters);
			CameraInformation.calibration_parameters_raw = sl::unreal::ToSlType(UnrealData.CalibrationParametersRaw);
			CameraInformation.camera_model = sl::unreal::ToSlType(UnrealData.CameraModel);
			CameraInformation.firmware_version = UnrealData.FirmwareVersion;
			CameraInformation.serial_number = UnrealData.SerialNumber;

			return CameraInformation;
		}
	}
}