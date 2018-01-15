//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "ZEDPrivatePCH.h"
#include "ZED/Public/Core/ZEDCamera.h"
#include "ZED/Public/Core/ZEDCoreGlobals.h"
#include "ZED/Public/Core/ZEDPlayerController.h"
#include "ZED/Public/Utilities/ZEDFunctionLibrary.h"
#include "Stereolabs/Public/Core/StereolabsCoreUtilities.h"
#include "Stereolabs/Public/Utilities/StereolabsFunctionLibrary.h"
#include "Stereolabs/Public/Core/StereolabsCameraProxy.h"
#include "HeadMountedDisplayFunctionLibrary.h"

#include <sl_mr_core/Rendering.hpp>
#include <sl_mr_core/latency.hpp>
#include <sl_mr_core/antidrift.hpp>

DEFINE_LOG_CATEGORY(ZEDCamera);

#define ZED_CAMERA_LOG(Format, ...) SL_LOG(ZEDCamera, Format, ##__VA_ARGS__)
#define ZED_CAMERA_LOG_W(Format, ...) SL_LOG_W(ZEDCamera, Format, ##__VA_ARGS__)
#define ZED_CAMERA_LOG_E(Format, ...) SL_LOG_E(ZEDCamera, Format, ##__VA_ARGS__)
#define ZED_CAMERA_LOG_F(Format, ...) SL_LOG_F(ZEDCamera, Format, ##__VA_ARGS__)

/** Preset for depth texture quality */
static TAutoConsoleVariable<int32> CVarZEDDepthTextureQualityPreset(
	TEXT("r.ZED.DepthTextureQualityPreset"),
	0,
	TEXT("Set the quality of the ZED depth texture.")
	TEXT("	0: low (default)")
	TEXT("	1: medium")
	TEXT("	2: high"),
	ECVF_RenderThreadSafe
	);

AZEDCamera::AZEDCamera()
	:
	bPositionalTrackingInitialized(false),
	bHMDHasTrackers(false),
	bCurrentDepthEnabled(false),
	bUseHMDTrackingAsOrigin(false),
	bInit(false),
	HMDRenderPlaneDistance(1000.0f),
	HMDCameraOffset(-20000.0f),
	RenderingMode(ESlRenderingMode::RM_None),
	Batch(nullptr),
	LeftEyeColor(nullptr),
	LeftEyeDepth(nullptr),
	RightEyeColor(nullptr),
	RightEyeDepth(nullptr),
	LeftEyeNormals(nullptr),
	RightEyeNormals(nullptr),
	LeftEyeRenderTarget(nullptr),
	RightEyeRenderTarget(nullptr),
	CurrentDepthTextureQualityPreset(0)
{
	// Controller tick the camera to make it the first actor to tick
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UMaterial> ZedMaterial(TEXT("Material'/Stereolabs/ZED/Materials/Mono/M_ZED_Mono.M_ZED_Mono'"));
	ZedSourceMaterial = ZedMaterial.Object;

	static ConstructorHelpers::FObjectFinder<UMaterial> HMDLeftEyeMaterial(TEXT("Material'/Stereolabs/ZED/Materials/Stereo/M_ZED_HMDLeftEye.M_ZED_HMDLeftEye'"));
	HMDLeftEyeSourceMaterial = HMDLeftEyeMaterial.Object;

	static ConstructorHelpers::FObjectFinder<UMaterial> HMDRightEyeMaterial(TEXT("Material'/Stereolabs/ZED/Materials/Stereo/M_ZED_HMDRightEye.M_ZED_HMDRightEye'"));
	HMDRightEyeSourceMaterial = HMDRightEyeMaterial.Object;
}

void AZEDCamera::BeginPlay()
{	
	Super::BeginPlay();

	GSlCameraProxy->OnCameraClosed.AddDynamic(this, &AZEDCamera::CameraClosed);

	CameraRenderPlaneDistance = GNearClippingPlane;
}

void AZEDCamera::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (GSlCameraProxy)
	{
		GSlCameraProxy->OnCameraClosed.RemoveDynamic(this, &AZEDCamera::CameraClosed);

		GSlCameraProxy->RemoveFromGrabDelegate(GrabDelegateHandle);
	}

	UHeadMountedDisplayFunctionLibrary::SetSpectatorScreenMode(ESpectatorScreenMode::SingleEyeCroppedToFill);
}

#if WITH_EDITOR
void AZEDCamera::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!GSlCameraProxy)
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);
		return;
	}

	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyChangedEvent.Property->GetOwnerStruct())
	{
		FString StructName = PropertyChangedEvent.Property->GetOwnerStruct()->GetName();

		if (StructName == FString("SlCameraSettings"))
		{
			SetCameraSettings(CameraSettings);
		}
		
		if (StructName == FString("SlRuntimeParameters"))
		{
			SetRuntimeParameters(RuntimeParameters);
		}
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlRenderingParameters, ThreadingMode))
	{
		SetThreadingMode(RenderingParameters.ThreadingMode);
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlSVOParameters, bLoop))
	{
		GSlCameraProxy->SetSVOPlaybackLooping(SVOParameters.bLoop);
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlRenderingParameters, PerceptionDistance))
	{
		UpdatePlanesSize();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool AZEDCamera::CanEditChange(const UProperty* InProperty) const
{
	FName PropertyName = InProperty->GetFName();

	if (!GSlCameraProxy)
	{
		return false;
	}

	if (InProperty->GetOwnerStruct())
	{
		if (InProperty->GetOwnerStruct()->GetName() == FString("SlCameraSettings"))
		{
			return !GSlCameraProxy->bSVOPlaybackEnabled;
		}

		if (InProperty->GetOwnerStruct()->GetName() == FString("SlRuntimeParameters"))
		{
			if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlRuntimeParameters, ReferenceFrame))
			{
				return false;
			}

			return true;
		}
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlCameraSettings, WhiteBalance))
	{
		return !CameraSettings.bAutoWhiteBalance;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlCameraSettings, Gain) || PropertyName == GET_MEMBER_NAME_CHECKED(FSlCameraSettings, Exposure))
	{
		return !CameraSettings.bAutoGainAndExposure;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlRenderingParameters, PerceptionDistance))
	{
		return UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlTrackingParameters, bEnableTracking))
	{
		return false;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlSVOParameters, RecordingFilePath) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FSlSVOParameters, CompressionMode))
	{
		return !GSlCameraProxy->bSVORecordingEnabled && GSlCameraProxy->GetSVONumberOfFrames() == -1;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlSVOParameters, bLoop))
	{
		return InitParameters.bUseSVO;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlRenderingParameters, ThreadingMode))
	{
		return !InitParameters.bUseSVO;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FSlTrackingParameters, bEnablePoseSmoothing))
	{
		return TrackingParameters.bEnableSpatialMemory;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AZEDCamera, bUseHMDTrackingAsOrigin))
	{
		return UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
	}

	return Super::CanEditChange(InProperty);
}
#endif

void AZEDCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RenderingParameters.ThreadingMode == ESlThreadingMode::TM_SingleThreaded)
	{
		GSlCameraProxy->Grab();
	}

	bool bUpdateTracking = false;
	SL_SCOPE_LOCK(Lock, TrackingUpdateSection)
		if (GSlCameraProxy->bTrackingEnabled)
		{
			bUpdateTracking = TrackingData.Timestamp.Timestamp != CurrentFrameTrackingData.Timestamp.Timestamp;
			if (bUpdateTracking)
			{
				TrackingData = CurrentFrameTrackingData;
			}
		}
		else
		{
			TrackingData = CurrentFrameTrackingData;
		}
	SL_SCOPE_UNLOCK

	// Always tick to retrieve last images
	bool bNewImage = Batch->Tick();

	// Stereo update
	if (RenderingMode == ESlRenderingMode::RM_Stereo)
	{
		FVector HMDLocation;
		FRotator HMDRotation;
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HMDRotation, HMDLocation);

		// HMD tracking transform
		sl::Transform SlHMDTransform = sl::unreal::ToSlType(FTransform(HMDRotation, HMDLocation));

		// Current timestamp
		sl::timeStamp CurrentTimestamp = GSlCameraProxy->GetCamera().getTimestamp(sl::TIME_REFERENCE::TIME_REFERENCE_CURRENT);
	
		// Set IMU prior
		if (GSlCameraProxy->bTrackingEnabled && GSlCameraProxy->GetCameraModel() == ESlModel::M_ZedM)
		{
			sl::Transform PastTransform;
			bool bTransformRetrieved = sl::mr::latencyCorrectorGetTransform(CurrentTimestamp - sl::timeStamp(44000000), PastTransform, false);
			if (bTransformRetrieved)
			{
				GSlCameraProxy->SetIMUPrior(sl::unreal::ToUnrealType(PastTransform));
			}
		}

		// latency transform
		sl::Transform SlLatencyTransform;

		// Latency corrector if new image
		if(bNewImage || bUpdateTracking)
		{
			// Add key pose
			sl::mr::latencyCorrectorAddKeyPose(sl::mr::keyPose(SlHMDTransform, CurrentTimestamp));

			// Latency corrector
			sl::mr::latencyCorrectorGetTransform(TrackingData.Timestamp.Timestamp, SlLatencyTransform);

			if (GSlCameraProxy->GetCameraModel() == ESlModel::M_ZedM)
			{
				// Update HMD planes rotation
				//SetHMDPlanesRotation(TrackingData.IMURotator);
				SetHMDPlanesRotation(sl::unreal::ToUnrealType(SlLatencyTransform).Rotator());
			}
			else
			{
				// Update HMD planes rotation
				SetHMDPlanesRotation(sl::unreal::ToUnrealType(SlLatencyTransform).Rotator());
			}
		}

		// Update HMD planes location
		SetHMDPlanesLocation(HMDLocation);

		if (bUpdateTracking)
		{
			// Initialize drift corrector if failed because out of tracking area
			InitializeDriftCorrectorConstOffset(HMDLocation, HMDRotation);
			
			// Remove HMD origin from tracking
			FZEDTrackingData TmpTrackingData = TrackingData;
			TmpTrackingData.ZedPathTransform = TmpTrackingData.ZedPathTransform * TrackingOriginFromHMD.Inverse();
				
			sl::mr::trackingData SlTrackingData = sl::unreal::ToSlType(TmpTrackingData);
			sl::mr::driftCorrectorGetTrackingData(SlTrackingData, SlHMDTransform, SlLatencyTransform, bHMDHasTrackers && UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition(), true);

			TrackingData.ZedWorldTransform = sl::unreal::ToUnrealType(SlTrackingData.zedWorldTransform);
			TrackingData.OffsetZedWorldTransform = sl::unreal::ToUnrealType(SlTrackingData.offsetZedWorldTransform);
		}
	}
	// Mono update
	else
	{
		TrackingData.ZedWorldTransform		 = TrackingData.ZedPathTransform;
		TrackingData.OffsetZedWorldTransform = TrackingData.ZedWorldTransform;
	}

	// Update tracking data
	if (bUpdateTracking)
	{
		GZedTrackingData = TrackingData;

		GZedRawLocation = TrackingData.ZedWorldTransform.GetLocation();
		GZedRawRotation = TrackingData.ZedWorldTransform.Rotator();

		GZedViewPointLocation = TrackingData.OffsetZedWorldTransform.GetLocation();
		GZedViewPointRotation = TrackingData.OffsetZedWorldTransform.Rotator();

		OnTrackingDataUpdated.Broadcast(TrackingData);
	}


	// Depth texture quality, normals will have the same size for performance purpose
	int32 GDepthTextureSizePreset = CVarZEDDepthTextureQualityPreset.GetValueOnGameThread();
	if (CurrentDepthTextureQualityPreset != GDepthTextureSizePreset)
	{
		CurrentDepthTextureQualityPreset = GDepthTextureSizePreset;

		if (bCurrentDepthEnabled)
		{
			FVector2D DepthSize = GetSlTextureSizeFromPreset(CurrentDepthTextureQualityPreset);

			// Left depth
			Batch->RemoveTexture(LeftEyeDepth);
			LeftEyeDepth->Resize(DepthSize.X, DepthSize.Y);
			Batch->AddTexture(LeftEyeDepth);

			// Left normals
			Batch->RemoveTexture(LeftEyeNormals);
			LeftEyeNormals->Resize(DepthSize.X, DepthSize.Y);
			Batch->AddTexture(LeftEyeNormals);

			ZedLeftEyeMaterialInstanceDynamic->SetTextureParameterValue("Depth", LeftEyeDepth->Texture);
			ZedLeftEyeMaterialInstanceDynamic->SetTextureParameterValue("Normals", LeftEyeNormals->Texture);

			if (RenderingMode == ESlRenderingMode::RM_Stereo)
			{
				// Right depth
				Batch->RemoveTexture(RightEyeDepth);
				RightEyeDepth->Resize(DepthSize.X, DepthSize.Y);
				Batch->AddTexture(RightEyeDepth);

				// Right normals
				Batch->RemoveTexture(RightEyeNormals);
				RightEyeNormals->Resize(DepthSize.X, DepthSize.Y);
				Batch->AddTexture(RightEyeNormals);

				ZedRightEyeMaterialInstanceDynamic->SetTextureParameterValue("Depth", RightEyeDepth->Texture);
				ZedRightEyeMaterialInstanceDynamic->SetTextureParameterValue("Normals", RightEyeNormals->Texture);
			}
		}
#if WITH_EDITOR
		else
		{
			ZED_CAMERA_LOG_E("Resizing depth and normal without depth enabled in runtime parameters");
		}
#endif

		// Depth retrieve toggle
		if (bCurrentDepthEnabled != RuntimeParameters.bEnableDepth)
		{
			bCurrentDepthEnabled = RuntimeParameters.bEnableDepth;

			if (!bCurrentDepthEnabled)
			{
				ZedLeftEyeMaterialInstanceDynamic->SetTextureParameterValue("Depth", nullptr);
				ZedLeftEyeMaterialInstanceDynamic->SetTextureParameterValue("Normals", nullptr);

				Batch->RemoveTexture(LeftEyeDepth);
				delete LeftEyeDepth;
				LeftEyeDepth = nullptr;

				Batch->RemoveTexture(LeftEyeNormals);
				delete LeftEyeNormals;
				LeftEyeNormals = nullptr;

				if (RenderingMode == ESlRenderingMode::RM_Stereo)
				{
					ZedRightEyeMaterialInstanceDynamic->SetTextureParameterValue("Depth", nullptr);
					ZedRightEyeMaterialInstanceDynamic->SetTextureParameterValue("Normals", nullptr);

					Batch->RemoveTexture(RightEyeDepth);
					delete RightEyeDepth;
					RightEyeDepth = nullptr;

					Batch->RemoveTexture(RightEyeNormals);
					delete RightEyeNormals;
					RightEyeNormals = nullptr;
				}
			}
			else
			{
				CreateLeftTextures(false);

				Batch->AddTexture(LeftEyeDepth);
				Batch->AddTexture(LeftEyeNormals);

				ZedLeftEyeMaterialInstanceDynamic->SetTextureParameterValue("Depth", LeftEyeDepth->Texture);
				ZedLeftEyeMaterialInstanceDynamic->SetTextureParameterValue("Normals", LeftEyeNormals->Texture);

				if (RenderingMode == ESlRenderingMode::RM_Stereo)
				{
					CreateRightTextures(false);

					Batch->AddTexture(RightEyeDepth);
					Batch->AddTexture(RightEyeNormals);

					ZedRightEyeMaterialInstanceDynamic->SetTextureParameterValue("Depth", RightEyeDepth->Texture);
					ZedRightEyeMaterialInstanceDynamic->SetTextureParameterValue("Normals", RightEyeNormals->Texture);
				}
			}
		}
	}
}

void AZEDCamera::GrabCallback(ESlErrorCode ErrorCode, const FSlTimestamp& Timestamp)
{
	if (ErrorCode != ESlErrorCode::EC_Success)
	{
		return;
	}

	Batch->RetrieveCurrentFrame(Timestamp);

	SL_SCOPE_LOCK(Lock, TrackingUpdateSection)
		sl::Camera& Zed = GSlCameraProxy->GetCamera();

		sl::Pose Pose;
		sl::TRACKING_STATE TrackingState = Zed.getPosition(Pose, sl::REFERENCE_FRAME::REFERENCE_FRAME_WORLD);

		CurrentFrameTrackingData.TrackingState = sl::unreal::ToUnrealType(TrackingState);
		CurrentFrameTrackingData.Timestamp = Timestamp;

#if WITH_EDITOR
		if (TrackingState == sl::TRACKING_STATE::TRACKING_STATE_FPS_TOO_LOW)
		{
			ZED_CAMERA_LOG_W("FPS too low for good tracking.");
		}
		else if (TrackingState == sl::TRACKING_STATE::TRACKING_STATE_SEARCHING)
		{
			ZED_CAMERA_LOG_W("Tracking trying to relocate.");
		}
#endif

		// Get the IMU rotation
		if (TrackingState == sl::TRACKING_STATE::TRACKING_STATE_OK ||
			TrackingState == sl::TRACKING_STATE::TRACKING_STATE_FPS_TOO_LOW ||
			TrackingState == sl::TRACKING_STATE::TRACKING_STATE_SEARCHING)
		{
			CurrentFrameTrackingData.ZedPathTransform = sl::unreal::ToUnrealType(Pose.pose_data);
		}

		if (GSlCameraProxy->GetCameraModel() == ESlModel::M_ZedM)
		{
			sl::IMUData IMUData;
			sl::ERROR_CODE IMUErrorCode = Zed.getIMUData(IMUData, sl::TIME_REFERENCE::TIME_REFERENCE_IMAGE);

			if (IMUErrorCode == sl::ERROR_CODE::SUCCESS)
			{
				CurrentFrameTrackingData.IMURotator = sl::unreal::ToUnrealType(IMUData.pose_data.getRotation()).Rotator();
			}
#if WITH_EDITOR
			else
			{
				FString ErrorString(sl::toString(IMUErrorCode).c_str());
				ZED_CAMERA_LOG_E("Error while getting IMU data : \"%s\"", *ErrorString);
			}
#endif
		}
	SL_SCOPE_UNLOCK
}

void AZEDCamera::CreateLeftTextures(bool bCreateColorTexture/* = true*/)
{
	if (bCreateColorTexture)
	{
		FIntPoint Resolution = GSlCameraProxy->CameraInformation.CalibrationParameters.LeftCameraParameters.Resolution;

		LeftEyeColor = USlViewTexture::CreateGPUViewTexture("LeftEyeColor", Resolution.X, Resolution.Y, ESlView::V_Left, true, ESlTextureFormat::TF_B8G8R8A8_UNORM);
	}

	if (RuntimeParameters.bEnableDepth)
	{
		FIntPoint TextureSize = GetSlTextureSizeFromPreset(CurrentDepthTextureQualityPreset);

		LeftEyeDepth = USlMeasureTexture::CreateGPUMeasureTexture("LeftEyeDepth", TextureSize.X, TextureSize.Y, ESlMeasure::M_Depth, true, ESlTextureFormat::TF_R32_FLOAT);
		LeftEyeNormals = USlMeasureTexture::CreateGPUMeasureTexture("LeftEyeNormals", TextureSize.X, TextureSize.Y, ESlMeasure::M_Normals, true, ESlTextureFormat::TF_A32B32G32R32F);
	}
}

void AZEDCamera::CreateRightTextures(bool bCreateColorTexture/* = true*/)
{
	if (bCreateColorTexture)
	{
		FIntPoint Resolution = GSlCameraProxy->CameraInformation.CalibrationParameters.LeftCameraParameters.Resolution;

		RightEyeColor = USlViewTexture::CreateGPUViewTexture("RightEyeColor", Resolution.X, Resolution.Y, ESlView::V_Right, true, ESlTextureFormat::TF_B8G8R8A8_UNORM);
	}

	if (RuntimeParameters.bEnableDepth)
	{
		FIntPoint TextureSize = GetSlTextureSizeFromPreset(CurrentDepthTextureQualityPreset);

		RightEyeDepth = USlMeasureTexture::CreateGPUMeasureTexture("RightEyeDepth", TextureSize.X, TextureSize.Y, ESlMeasure::M_DepthRight, true, ESlTextureFormat::TF_R32_FLOAT);
		RightEyeNormals = USlMeasureTexture::CreateGPUMeasureTexture("RightEyeNormals", TextureSize.X, TextureSize.Y, ESlMeasure::M_NormalsRight, true, ESlTextureFormat::TF_A32B32G32R32F);
	}
}

void AZEDCamera::SetThreadingMode(ESlThreadingMode NewValue)
{
	if (NewValue == ESlThreadingMode::TM_None)
	{
#if WITH_EDITOR
		ZED_CAMERA_LOG_E("EZEDThreadingMode::TM_None is not a valid mode");
#endif
		return;
	}

	GSlCameraProxy->EnableGrabThread(RenderingParameters.ThreadingMode == ESlThreadingMode::TM_MultiThreaded);
	RenderingParameters.ThreadingMode = NewValue;
}

void AZEDCamera::SetRuntimeParameters(const FSlRuntimeParameters& NewValue)
{
	RuntimeParameters = NewValue;
	GSlCameraProxy->SetRuntimeParameters(RuntimeParameters);
}

void AZEDCamera::SetCameraSettings(const FSlCameraSettings& NewValue)
{
	if (NewValue.bDefault)
	{
		CameraSettings = FSlCameraSettings();
		CameraSettings.bDefault = true;
	}
	else
	{
		CameraSettings = NewValue;
	}

	GSlCameraProxy->SetCameraSettings(CameraSettings);

	bool bAutoGainAndExposure = CameraSettings.bAutoGainAndExposure;
	bool bAutoWhiteBalance = CameraSettings.bAutoWhiteBalance;

	CameraSettings = GSlCameraProxy->GetCameraSettings();
	CameraSettings.bAutoGainAndExposure = bAutoGainAndExposure;
	CameraSettings.bAutoWhiteBalance = bAutoWhiteBalance;
}

void AZEDCamera::EnableTracking()
{
	GSlCameraProxy->EnableTracking(TrackingParameters);
}

void AZEDCamera::DisableTracking()
{
	GSlCameraProxy->DisableTracking();

	// Reset positional tracking if using HMD
	bPositionalTrackingInitialized = false;
}

void AZEDCamera::ResetTrackingOrigin()
{
	// If using an HMD, reset SDK tracking with current HMD tracking
	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		InitHMDTrackingData();
	}
	else
	{
		GSlCameraProxy->ResetTracking(TrackingParameters.Rotation, TrackingParameters.Location);
	}
}

void AZEDCamera::SaveSpatialMemoryArea()
{
	GSlCameraProxy->SaveSpatialMemoryArea(TrackingParameters.SpatialMemoryFileSavingPath);
}

void AZEDCamera::InitializeParameters(AZEDInitializer* ZedInitializer, bool bHMDEnabled)
{
	TrackingParameters = ZedInitializer->TrackingParameters;
	InitParameters = ZedInitializer->InitParameters;
	RuntimeParameters = ZedInitializer->RuntimeParameters;
	RenderingParameters = ZedInitializer->RenderingParameters;
	AntiDriftParameters = ZedInitializer->AntiDriftParameters;
	CameraSettings = ZedInitializer->CameraSettings;
	SVOParameters = ZedInitializer->SVOParameters;
	bUseHMDTrackingAsOrigin = ZedInitializer->bUseHMDTrackingAsOrigin;

	bCurrentDepthEnabled = RuntimeParameters.bEnableDepth;
	
	checkf(RuntimeParameters.ReferenceFrame == ESlReferenceFrame::RF_World, TEXT("Reference frame must be World when using the ZEDCamera"));

	if (InitParameters.bUseSVO)
	{
		RenderingParameters.ThreadingMode = ESlThreadingMode::TM_SingleThreaded;
	}

	if (bUseHMDTrackingAsOrigin)
	{
		TrackingParameters.Location = FVector::ZeroVector;
		TrackingParameters.Rotation = FRotator::ZeroRotator;
	}

	if (bHMDEnabled)
	{
		InitParameters.bEnableRightSideMeasure = true;
	}
}

void AZEDCamera::Init(bool bHMDEnabled)
{
	if (bInit)
	{
		return;
	}

	Batch = USlGPUTextureBatch::CreateGPUTextureBatch(FName("ZedCameraBatch"));

	if (SVOParameters.bLoop)
	{
		GSlCameraProxy->SetSVOPlaybackLooping(true);
	}

	if (bHMDEnabled)
	{
		sl::mr::latencyCorrectorInitialize();

		if (TrackingParameters.bEnableTracking)
		{
			InitHMDTrackingData();
		}

		// Set first delay to 1.0f to fix HMD planes wrong location at startup
		GetWorldTimerManager().SetTimer(PlanesAntiDriftTimerHandle, this, &AZEDCamera::CorrectHMDPlanesDrift, 10.0f, true, 1.0f);
	}

	SetCameraSettings(CameraSettings);
	if (RenderingMode == ESlRenderingMode::RM_None)
	{
		RenderingMode = bHMDEnabled ? ESlRenderingMode::RM_Stereo : ESlRenderingMode::RM_Mono;
	}
	SetRuntimeParameters(RuntimeParameters);
	SetThreadingMode(RenderingParameters.ThreadingMode);

	ZedLeftEyeMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(ZedSourceMaterial, nullptr);
	ZedLeftEyeMaterialInstanceDynamic->SetScalarParameterValue("MinDepth", InitParameters.DepthMinimumDistance);
	ZedLeftEyeMaterialInstanceDynamic->SetScalarParameterValue("MaxDepth", InitParameters.DepthMaximumDistance);

	CreateLeftTextures();
	ZedLeftEyeMaterialInstanceDynamic->SetTextureParameterValue("Color", LeftEyeColor->Texture);
	ZedLeftEyeMaterialInstanceDynamic->SetTextureParameterValue("Depth", LeftEyeDepth->Texture);
	ZedLeftEyeMaterialInstanceDynamic->SetTextureParameterValue("Normals", LeftEyeNormals->Texture);

	Batch->AddTexture(LeftEyeColor);

	if (!bHMDEnabled)
	{
		Batch->AddTexture(LeftEyeDepth);
		Batch->AddTexture(LeftEyeNormals);
	}
	else
	{
		GSlCameraProxy->HMDToCameraOffset = AntiDriftParameters.CalibrationTransform.GetLocation().X;

		HMDLeftEyeMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(HMDLeftEyeSourceMaterial, nullptr);
		HMDRightEyeMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(HMDRightEyeSourceMaterial, nullptr);

		ZedRightEyeMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(ZedSourceMaterial, nullptr);
		ZedRightEyeMaterialInstanceDynamic->SetScalarParameterValue("MinDepth", InitParameters.DepthMinimumDistance);
		ZedRightEyeMaterialInstanceDynamic->SetScalarParameterValue("MaxDepth", InitParameters.DepthMaximumDistance);

		CreateRightTextures();
		ZedRightEyeMaterialInstanceDynamic->SetTextureParameterValue("Color", RightEyeColor->Texture);
		if (bCurrentDepthEnabled)
		{
			ZedRightEyeMaterialInstanceDynamic->SetTextureParameterValue("Depth", RightEyeDepth->Texture);
			ZedRightEyeMaterialInstanceDynamic->SetTextureParameterValue("Normals", RightEyeNormals->Texture);
		}

		Batch->AddTexture(LeftEyeColor);
		Batch->AddTexture(RightEyeColor);
		if (bCurrentDepthEnabled)
		{
			Batch->AddTexture(LeftEyeDepth);
			Batch->AddTexture(RightEyeDepth);
			Batch->AddTexture(LeftEyeNormals);
			Batch->AddTexture(RightEyeNormals);
		}
	}

	GrabDelegateHandle = GSlCameraProxy->AddToGrabDelegate([this](ESlErrorCode ErrorCode, const FSlTimestamp& Timestamp)
	{
		GrabCallback(ErrorCode, Timestamp);
	});

	InitializeRendering();

	OnCameraActorInitialized.Broadcast();

	bInit = true;
}

void AZEDCamera::CameraClosed()
{
	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		sl::mr::driftCorrectorShutdown();
		sl::mr::latencyCorrectorShutdown();

		GetWorldTimerManager().ClearTimer(PlanesAntiDriftTimerHandle);
	}

	GSlCameraProxy->RemoveFromGrabDelegate(GrabDelegateHandle);

	UHeadMountedDisplayFunctionLibrary::SetSpectatorScreenMode(ESpectatorScreenMode::SingleEyeCroppedToFill);

	Batch->Clear();

	delete LeftEyeColor;
	LeftEyeColor = nullptr;
	delete LeftEyeNormals;
	LeftEyeNormals = nullptr;
	delete LeftEyeDepth;
	LeftEyeDepth = nullptr;
	delete RightEyeColor;
	RightEyeColor = nullptr;
	delete RightEyeNormals;
	RightEyeNormals = nullptr;
	delete RightEyeDepth;
	RightEyeDepth = nullptr;

	bInit = false;
}

ESlErrorCode AZEDCamera::EnableSVORecording()
{
	return GSlCameraProxy->EnableSVORecording(SVOParameters.RecordingFilePath, SVOParameters.CompressionMode);
}

void AZEDCamera::DisableSVORecording()
{
	GSlCameraProxy->DisableSVORecording();
}

void AZEDCamera::SetVOPlaybackLooping(bool bLooping)
{
	SVOParameters.bLoop = bLooping;
	GSlCameraProxy->SetSVOPlaybackLooping(bLooping);
}

void AZEDCamera::InitHMDTrackingData()
{
	sl::mr::driftCorrectorInitialize();

	sl::mr::driftCorrectorSetCalibrationTransform(sl::unreal::ToSlType(AntiDriftParameters.CalibrationTransform));
	if (!bUseHMDTrackingAsOrigin)
	{
		sl::mr::driftCorrectorSetTrackingOffsetTransfrom(sl::unreal::ToSlType(FTransform(TrackingParameters.Rotation, TrackingParameters.Location)));
	}
	else
	{
		sl::mr::driftCorrectorSetTrackingOffsetTransfrom(sl::unreal::ToSlType(FTransform()));
	}

	bHMDHasTrackers = (UHeadMountedDisplayFunctionLibrary::GetNumOfTrackingSensors() > 0);

	FVector HMDLocation;
	FRotator HMDRotation;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HMDRotation, HMDLocation);
	InitializeDriftCorrectorConstOffset(HMDLocation, HMDRotation);
}

bool AZEDCamera::InitializeDriftCorrectorConstOffset(const FVector& HMDLocation, const FRotator& HMDRotation)
{
	// If trackers and already initialized
	if (bPositionalTrackingInitialized)
	{
		return true;
	}

	bool bHasValidTrackingPosition = UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition();

	if (!bHMDHasTrackers ||
		bHMDHasTrackers && bHasValidTrackingPosition)
	{
		if (bUseHMDTrackingAsOrigin)
		{
			TrackingOriginFromHMD = FTransform(HMDRotation, HMDLocation) * AntiDriftParameters.CalibrationTransform;

			// Set HMD tracking data
			TrackingParameters.Location = TrackingOriginFromHMD.GetLocation();
			TrackingParameters.Rotation = TrackingOriginFromHMD.Rotator();
		}
		else
		{
			TrackingOriginFromHMD = FTransform(HMDRotation, HMDLocation) * FTransform(TrackingParameters.Rotation, TrackingParameters.Location) * AntiDriftParameters.CalibrationTransform;
		}

		GSlCameraProxy->ResetTracking(TrackingOriginFromHMD.Rotator(), TrackingOriginFromHMD.GetLocation());

		//if (GSlCameraProxy->GetCameraModel() == ESlModel::M_ZedM)
		//{
		//	sl::mr::driftCorrectorSetConstOffsetTransfrom(sl::unreal::ToSlType(AntiDriftParameters.CalibrationTransform * FTransform(FRotator::ZeroRotator, HMDLocation)));
		//}
		//else
		//{
			sl::mr::driftCorrectorSetConstOffsetTransfrom(sl::unreal::ToSlType(AntiDriftParameters.CalibrationTransform * FTransform(HMDRotation, HMDLocation)));
		//}

		bPositionalTrackingInitialized = true;

		return true;
	}

	return false;
}