//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "StereolabsPrivatePCH.h"
#include "Stereolabs/Public/Core/StereolabsCameraProxy.h"
#include "Stereolabs/Public/Core/StereolabsCoreGlobals.h"
#include "Stereolabs/Private/Threading/StereolabsGrabRunnable.h"
#include "Stereolabs/Private/Threading/StereolabsMeasureRunnable.h"
#include "Stereolabs/Public/Core/StereolabsTexture.h"
#include "WidgetLayoutLibrary.h"

DECLARE_CYCLE_STAT_EXTERN(TEXT("Grab"), STAT_Grab, STATGROUP_ZED, STEREOLABS_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("RetrieveMeasure"), STAT_RetrieveMeasure, STATGROUP_ZED, STEREOLABS_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("RetrieveImage"), STAT_RetrieveImage, STATGROUP_ZED, STEREOLABS_API);

DEFINE_STAT(STAT_Grab)
DEFINE_STAT(STAT_RetrieveMeasure)
DEFINE_STAT(STAT_RetrieveImage)

DEFINE_LOG_CATEGORY(SlCameraProxy);
#define SL_CAMERA_PROXY_LOG(Format, ...) SL_LOG(SlCameraProxy, Format, ##__VA_ARGS__)
#define SL_CAMERA_PROXY_LOG_W(Format, ...) SL_LOG_W(SlCameraProxy, Format, ##__VA_ARGS__)
#define SL_CAMERA_PROXY_LOG_E(Format, ...) SL_LOG_E(SlCameraProxy, Format, ##__VA_ARGS__)
#define SL_CAMERA_PROXY_LOG_F(Format, ...) SL_LOG_F(SlCameraProxy, Format, ##__VA_ARGS__)

void CreateSlCameraProxyInstance()
{
	check(IsInGameThread());

	if (!GSlCameraProxy)
	{
		GSlCameraProxy = NewObject<USlCameraProxy>();
		GSlCameraProxy->AddToRoot();
	}
}

void FreeSlCameraProxyInstance()
{
	check(IsInGameThread());

	if (GSlCameraProxy)
	{
		GSlCameraProxy->ConditionalBeginDestroy();
		GSlCameraProxy = nullptr;
	}
}

UGrabCallbackInterface::UGrabCallbackInterface(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

class FOpenCameraAsyncTask : public FNonAbandonableTask
{
	friend class FAsyncTask<FOpenCameraAsyncTask>;

public:
	FOpenCameraAsyncTask( const FSlInitParameters& InitParameters) 
		:
		InitParameters(InitParameters)
	{}

protected:
	void DoWork()
	{
		SL_CAMERA_PROXY_LOG_W("Waiting for ZED to be connected");

		while (!USlCameraProxy::IsCameraConnected() && !GSlCameraProxy->bAbandonOpenTask)
		{
			GSlCameraProxy->SetOpenCameraErrorCode(ESlErrorCode::EC_CameraNotDetected);

			FPlatformProcess::SleepNoStats(0.01f);
		}

		if (!GSlCameraProxy->bAbandonOpenTask)
		{
			SL_CAMERA_PROXY_LOG_W("ZED connected");
			GSlCameraProxy->SetOpenCameraErrorCode(ESlErrorCode::EC_None);
		
			GSlCameraProxy->Internal_OpenCamera(InitParameters);
		}
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FOpenCameraAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

protected:
	FSlInitParameters InitParameters;
};

class FEnableTrackingAsyncTask : public FNonAbandonableTask
{
	friend class FAsyncTask<FEnableTrackingAsyncTask>;

public:
	FEnableTrackingAsyncTask(const FSlTrackingParameters& TrackingParameters)
		:
		TrackingParameters(TrackingParameters)
	{}

protected:
	void DoWork()
	{
		GSlCameraProxy->Internal_EnableTracking(TrackingParameters);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FEnableTrackingAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

protected:
	FSlTrackingParameters TrackingParameters;
};

USlCameraProxy::USlCameraProxy()
	:
	GrabWorker(nullptr),
	MeasuresWorker(nullptr),
	OpenCameraAsyncTask(nullptr),
	EnableTrackingAsyncTask(nullptr),
	bTrackingEnabled(false),
	bAbandonOpenTask(false),
	bHitTestDepthEnabled(true),
	bHitTestNormalsEnabled(true),
	bAutoGainAndExposure(true),
	bSpatialMemoryEnabled(false),
	bSpatialMappingEnabled(false),
	bGrabEnabled(false),
	bSVOPlaybackEnabled(false),
	bSVORecordingEnabled(false),
	bSVOPlaybackPaused(false),
	bSVORecordingFrames(false),
	bSVOLooping(false),
	bCameraOpened(false),
	OpenCameraErrorCode(ESlErrorCode::EC_None),
	HMDToCameraOffset(0.0f),
	RetrieveMatSize(GetSlTextureSizeFromPreset(0))
{
}

void USlCameraProxy::BeginDestroy()
{
	// Disable grab thread
	if (GrabWorker)
	{
		GrabWorker->EnsureCompletion();
		delete GrabWorker;
		GrabWorker = nullptr;
	}

	// Disable measures thread
	if (MeasuresWorker)
	{
		MeasuresWorker->EnsureCompletion();
		delete MeasuresWorker;
		MeasuresWorker = nullptr;
	}

	CloseCamera();

	Super::BeginDestroy();
}

FString USlCameraProxy::GetSDKVersion()
{
	return FString(sl::Camera::getSDKVersion().c_str());
}

void USlCameraProxy::OpenCamera(const FSlInitParameters& InitParameters)
{
	if (OpenCameraAsyncTask)
	{
		return;
	}
	else if (Zed.isOpened())
	{
		AsyncTask(ENamedThreads::GameThread, [this] ()
		{
			SL_CAMERA_PROXY_LOG_W("ZED camera already opened");
		});

		return;
	}

	OpenCameraAsyncTask = new FAsyncTask<FOpenCameraAsyncTask>(InitParameters);
	OpenCameraAsyncTask->StartBackgroundTask();
}

void USlCameraProxy::Internal_OpenCamera(const FSlInitParameters& InitParameters)
{
	SL_CAMERA_PROXY_LOG("Initializing");

	sl::InitParameters InitParams = sl::unreal::ToSlType(InitParameters);
	sl::ERROR_CODE ErrorCode;

	do
	{
		ErrorCode = Zed.open(InitParams);

		SetOpenCameraErrorCode(sl::unreal::ToUnrealType(ErrorCode));

		if (ErrorCode != sl::ERROR_CODE::SUCCESS)
		{
#if WITH_EDITOR
			FString ErrorString(sl::toString(ErrorCode).c_str());
			SL_CAMERA_PROXY_LOG_E("Error during initialization: \"%s\"", *ErrorString);
#endif
			if (ErrorCode != sl::ERROR_CODE::ERROR_CODE_CAMERA_NOT_DETECTED &&
				ErrorCode != sl::ERROR_CODE::ERROR_CODE_SENSOR_NOT_DETECTED)
			{
				bAbandonOpenTask = true;
			}
		}

		FPlatformProcess::Sleep(0.01f);
	} while (ErrorCode != sl::ERROR_CODE::SUCCESS && !bAbandonOpenTask);	

	if (bAbandonOpenTask)
	{
		AsyncTask(ENamedThreads::GameThread, [this] ()
		{
			SL_CAMERA_PROXY_LOG_W("Not initialized");
		});

		return;
	}

	Zed.setDepthMaxRangeValue(InitParameters.DepthMaximumDistance);

	SetOpenCameraErrorCode(ESlErrorCode::EC_None);

	AsyncTask(ENamedThreads::GameThread, [this, InitParameters] ()
	{
		if (!GSlCameraProxy)
		{
			return;
		}

		if (InitParameters.bUseSVO)
		{
			bSVOPlaybackEnabled = true;
		}

		SlCameraInformation = Zed.getCameraInformation();
		CameraInformation = sl::unreal::ToUnrealType(SlCameraInformation);
		RetrieveLeftCameraParameters = sl::unreal::ToUnrealType(Zed.getCameraInformation(sl::unreal::ToSlType2(RetrieveMatSize))).CalibrationParameters.LeftCameraParameters;

		GSlEyeHalfBaseline = CameraInformation.HalfBaseline;

		OpenCameraAsyncTask->EnsureCompletion(false);

		delete OpenCameraAsyncTask;
		OpenCameraAsyncTask = nullptr;

		SL_CAMERA_PROXY_LOG("Initialized");

		CUresult Result = cuCtxSetCurrent(Zed.getCUDAContext());

#if WITH_EDITOR
		if (Result != CUresult::CUDA_SUCCESS)
		{
			UE_LOG(LogTemp, Warning, TEXT("Can't set current CUDA context"));
		}
#endif
		bCameraOpened = true;

		OnCameraOpened.Broadcast();

		bGrabEnabled = true;
	});
}

void USlCameraProxy::CloseCamera()
{
	// Disable grab
	bGrabEnabled = false;

	bTrackingEnabled = false;
	bSpatialMappingEnabled = false;

	// Still trying to open the camera
	if (OpenCameraAsyncTask && !OpenCameraAsyncTask->IsDone())
	{
		bAbandonOpenTask = true;

		OpenCameraAsyncTask->EnsureCompletion(false);

		delete OpenCameraAsyncTask;
		OpenCameraAsyncTask = nullptr;
	}

	// Still trying to enable tracking
	if (EnableTrackingAsyncTask && !EnableTrackingAsyncTask->IsDone())
	{
		EnableTrackingAsyncTask->EnsureCompletion(false);

		delete EnableTrackingAsyncTask;
		EnableTrackingAsyncTask = nullptr;
	}

	if (bCameraOpened)
	{
		bCameraOpened = false;

		// Wait for current frame
		FlushRenderingCommands();

		// Disable grab thread
		if (GrabWorker)
		{
			EnableGrabThread(false);
		}

		// Disable measures thread
		if (MeasuresWorker)
		{
			EnableHitTest(false);
		}

		// Reset data
		CameraInformation = FSlCameraInformation();

		// Broadcast
		OnCameraClosed.Broadcast();
	}

	Zed.close();
}

void USlCameraProxy::EnableTracking(const FSlTrackingParameters& NewTrackingParameters)
{
	if (EnableTrackingAsyncTask)
	{
#if WITH_EDITOR
		SL_CAMERA_PROXY_LOG_E("Trying to enable tracking while tracking being enabled");
#endif
		return;
	}
	else if (bTrackingEnabled)
	{
#if WITH_EDITOR
		SL_CAMERA_PROXY_LOG_E("Trying to enable tracking but tracking already enabled");
#endif
		return;
	}

	EnableTrackingAsyncTask = new FAsyncTask<FEnableTrackingAsyncTask>(NewTrackingParameters);
	EnableTrackingAsyncTask->StartBackgroundTask();
}

void USlCameraProxy::Internal_EnableTracking(const FSlTrackingParameters& NewTrackingParameters)
{
	sl::ERROR_CODE ErrorCode;
	sl::ERROR_CODE IMUDataErrorCode;
	sl::IMUData IMUData;

	SL_SCOPE_LOCK(Lock, GrabSection)
		ErrorCode = Zed.enableTracking(sl::unreal::ToSlType(NewTrackingParameters));

		IMUDataErrorCode = Zed.getIMUData(IMUData, sl::TIME_REFERENCE_CURRENT);
	SL_SCOPE_UNLOCK

#if WITH_EDITOR
	if (ErrorCode != sl::ERROR_CODE::SUCCESS)
	{
		FString ErrorString(sl::toString(ErrorCode).c_str());
		SL_CAMERA_PROXY_LOG_E("Can't enable tracking: \"%s\"", *ErrorString);
	}

	if (IMUDataErrorCode != sl::ERROR_CODE::SUCCESS)
	{
		FString ErrorString(sl::toString(IMUDataErrorCode).c_str());
		SL_CAMERA_PROXY_LOG_E("Can't retrieve IMU Data after enable tracking: \"%s\"", *ErrorString);
	}
#endif
	
	FRotator IMURotation = sl::unreal::ToUnrealType(IMUData.getRotation()).Rotator();
	AsyncTask(ENamedThreads::GameThread, [this, ErrorCode, NewTrackingParameters, IMURotation] ()
	{
		if (!GSlCameraProxy)
		{
			return;
		}

		EnableTrackingAsyncTask->EnsureCompletion(false);

		delete EnableTrackingAsyncTask;
		EnableTrackingAsyncTask = nullptr;

		bTrackingEnabled = (ErrorCode == sl::ERROR_CODE::SUCCESS);
		bSpatialMemoryEnabled = NewTrackingParameters.bEnableSpatialMemory && bTrackingEnabled;

		OnTrackingEnabled.Broadcast(bTrackingEnabled, sl::unreal::ToUnrealType(ErrorCode), NewTrackingParameters.Location, IMURotation);
	});
}

void USlCameraProxy::DisableTracking()
{
	if (EnableTrackingAsyncTask)
	{
		SL_CAMERA_PROXY_LOG_E("Trying to disable tracking while tracking being enabled");
		return;
	}

	SL_SCOPE_LOCK(Lock, GrabSection)
		if (bTrackingEnabled)
		{
			Zed.disableTracking();
		
			bTrackingEnabled = false;
		}
	SL_SCOPE_UNLOCK

	bSpatialMemoryEnabled = false;

	OnTrackingDisabled.Broadcast();
}

void USlCameraProxy::ResetTracking(const FRotator& Rotation, const FVector& Location)
{
	sl::ERROR_CODE ErrorCode;
	sl::ERROR_CODE IMUDataErrorCode;
	sl::IMUData IMUData;

	SL_SCOPE_LOCK(Lock, GrabSection)
		ErrorCode = Zed.resetTracking(sl::unreal::ToSlType(FTransform(Rotation, Location)));	

		IMUDataErrorCode = Zed.getIMUData(IMUData, sl::TIME_REFERENCE_CURRENT);
	SL_SCOPE_UNLOCK

	bTrackingEnabled = (ErrorCode == sl::ERROR_CODE::SUCCESS);

#if WITH_EDITOR
		if (ErrorCode != sl::ERROR_CODE::SUCCESS)
		{
			FString ErrorString(sl::toString(ErrorCode).c_str());
			SL_CAMERA_PROXY_LOG_E("Can't reset tracking: \"%s\"", *ErrorString);
		}

		if (IMUDataErrorCode != sl::ERROR_CODE::SUCCESS)
		{
			FString ErrorString(sl::toString(IMUDataErrorCode).c_str());
			SL_CAMERA_PROXY_LOG_E("Can't retrieve IMU Data after reset tracking: \"%s\"", *ErrorString);
		}
#endif

	OnTrackingReset.Broadcast(bTrackingEnabled, sl::unreal::ToUnrealType(ErrorCode), Location, sl::unreal::ToUnrealType(IMUData.getRotation()).Rotator());
}

ESlTrackingState USlCameraProxy::GetPosition(FSlPose& Pose, ESlReferenceFrame ReferenceFrame)
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		sl::Pose SlPose;
		sl::TRACKING_STATE TrackingState = Zed.getPosition(SlPose, sl::unreal::ToSlType(ReferenceFrame));

#if WITH_EDITOR
		if (TrackingState == sl::TRACKING_STATE::TRACKING_STATE_FPS_TOO_LOW)
		{
			SL_CAMERA_PROXY_LOG_W("FPS too low for good tracking.");
		}
		else if (TrackingState == sl::TRACKING_STATE::TRACKING_STATE_SEARCHING)
		{
			SL_CAMERA_PROXY_LOG_W("Tracking trying to relocate.");
		}
#endif

		Pose = sl::unreal::ToUnrealType(SlPose);

		return sl::unreal::ToUnrealType(TrackingState);
	SL_SCOPE_UNLOCK
}

ESlErrorCode USlCameraProxy::GetIMUData(FSlIMUData& IMUData, ESlTimeReference TimeReference)
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		sl::IMUData SlIMUData;
		sl::ERROR_CODE ErrorCode = Zed.getIMUData(SlIMUData, sl::unreal::ToSlType(TimeReference));

#if WITH_EDITOR
		if (ErrorCode != sl::ERROR_CODE::SUCCESS)
		{
			FString ErrorString(sl::toString(ErrorCode).c_str());
			SL_CAMERA_PROXY_LOG_E("Error while retrieving IMU data: \"%s\"", *ErrorString);
		}
#endif
		IMUData = sl::unreal::ToUnrealType(SlIMUData);

		return sl::unreal::ToUnrealType(ErrorCode);
	SL_SCOPE_UNLOCK
}

void USlCameraProxy::SetIMUPrior(const FTransform& Transform)
{
	Zed.setIMUPrior(sl::unreal::ToSlType(Transform));
}

bool USlCameraProxy::IsTrackingEnabled()
{
	return bTrackingEnabled;
}

bool USlCameraProxy::SaveSpatialMemoryArea(const FString& AreaSavingPath)
{
	sl::ERROR_CODE ErrorCode = Zed.saveCurrentArea(TCHAR_TO_UTF8(*AreaSavingPath));

#if WITH_EDITOR
	if (ErrorCode != sl::ERROR_CODE::SUCCESS)
	{
		FString ErrorString(sl::toString(ErrorCode).c_str());
		SL_CAMERA_PROXY_LOG_E("Can't save spatial memory: \"%s\"", *ErrorString);

		return false;
	}

	return true;
#else
	return (Zed.saveCurrentArea(TCHAR_TO_UTF8(*AreaSavingPath)) == sl::ERROR_CODE::SUCCESS);
#endif
}

ESlSpatialMemoryExportState USlCameraProxy::GetSpatialMemoryExportState()
{
	return sl::unreal::ToUnrealType(Zed.getAreaExportState());
}

void USlCameraProxy::SetRuntimeParameters(const FSlRuntimeParameters& NewRuntimeParameters)
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		RuntimeParameters = sl::unreal::ToSlType(NewRuntimeParameters);
	SL_SCOPE_UNLOCK
}

FSlCameraInformation USlCameraProxy::GetCameraInformation(const FIntPoint& CustomResolution/* = FIntPoint::ZeroValue*/)
{
	if (CustomResolution == FIntPoint::ZeroValue)
	{
		return CameraInformation;
	}
	else
	{
		return sl::unreal::ToUnrealType(Zed.getCameraInformation(sl::unreal::ToSlType2(CustomResolution)));
	}
}

bool USlCameraProxy::IsCameraOpened()
{
	return OpenCameraAsyncTask ? false : bCameraOpened && Zed.isOpened();
}

bool USlCameraProxy::IsCameraConnected()
{
	return sl::Camera::isZEDconnected() > 0;
}

TArray<FSlDeviceProperties> USlCameraProxy::GetCameraList()
{
	TArray<FSlDeviceProperties> DevicePropetiesList;

#if PLATFORM_LINUX
	std::vector<sl::DeviceProperties> SlDevicePropetiesList = sl::Camera::getDeviceList();
	int32 Size = SlDevicePropetiesList.size();

	DevicePropetiesList.Reserve(Size);

	for (int32 Index = 0; Index < Size; ++Index)
	{
		DevicePropetiesList.Add(sl::unreal::ToUnrealType(SlDevicePropetiesList[Index]));
	}
#endif

	return DevicePropetiesList;
}

sl::Camera& USlCameraProxy::GetCamera()
{
	return Zed;
}

void USlCameraProxy::Grab()
{
	SCOPE_CYCLE_COUNTER(STAT_Grab);

	if (!bGrabEnabled)
	{
		return;
	}

	sl::ERROR_CODE ErrorCode;

	SL_SCOPE_LOCK(Lock, SVOSection)
		if (bSVOPlaybackEnabled)
		{
			if (bSVOPlaybackPaused)
			{
				Zed.setSVOPosition(CurrentSVOPlaybackPosition);
			}
			else if (bSVOLooping && Zed.getSVOPosition() >= Zed.getSVONumberOfFrames() - 1)
			{
				Zed.setSVOPosition(0);
			}
		}
	SL_SCOPE_UNLOCK

	SL_SCOPE_LOCK(Lock, GrabSection)
		ErrorCode = Zed.grab(RuntimeParameters);
	SL_SCOPE_UNLOCK

	if (ErrorCode == sl::ERROR_CODE::SUCCESS)
	{
		SL_SCOPE_LOCK(Lock, SVOSection)
			if (bSVORecordingEnabled && bSVORecordingFrames)
			{
				SlRecordingState = Zed.record();
#if WITH_EDITOR
				if (!SlRecordingState.status)
				{
					SL_CAMERA_PROXY_LOG_E("Can't record current frame");
				}
#endif
			}
		SL_SCOPE_UNLOCK
	}
	else
	{
#if WITH_EDITOR
		if (ErrorCode != sl::ERROR_CODE::ERROR_CODE_NOT_A_NEW_FRAME)
		{
			FString ErrorString(sl::toString(ErrorCode).c_str());
			SL_CAMERA_PROXY_LOG_E("Grab error: \"%s\"", *ErrorString);
		}
#endif

		if (ErrorCode == sl::ERROR_CODE::ERROR_CODE_CAMERA_NOT_DETECTED)
		{
			if (bGrabEnabled)
			{
				bGrabEnabled = false;

				AsyncTask(ENamedThreads::GameThread, [this] ()
				{
					OnCameraDisconnected.Broadcast();
				});

				return;
			}
		}
	}

	SL_SCOPE_LOCK(Lock, GrabDelegateSection)
		OnGrabDoneDelegate.Broadcast(sl::unreal::ToUnrealType(ErrorCode), FSlTimestamp(Zed.getTimestamp(sl::TIME_REFERENCE::TIME_REFERENCE_IMAGE)));
	SL_SCOPE_UNLOCK
}

FSlCameraSettings USlCameraProxy::GetCameraSettings()
{
	FSlCameraSettings CameraSettings;

	CameraSettings.Brightness			= Zed.getCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_BRIGHTNESS);
	CameraSettings.Contrast				= Zed.getCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_CONTRAST);
	CameraSettings.Hue					= Zed.getCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_HUE);
	CameraSettings.Saturation			= Zed.getCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_SATURATION);
	CameraSettings.WhiteBalance			= Zed.getCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_WHITEBALANCE);
	CameraSettings.Gain					= Zed.getCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_GAIN);
	CameraSettings.Exposure				= Zed.getCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_EXPOSURE);
	CameraSettings.bAutoWhiteBalance    = (Zed.getCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_AUTO_WHITEBALANCE) == 0 ? false : true);
	CameraSettings.bAutoGainAndExposure = bAutoGainAndExposure; // Can't retrieve from function
	CameraSettings.bDefault			    = false;

	return CameraSettings;
}

bool USlCameraProxy::EnableSpatialMapping(const FSlSpatialMappingParameters& SpatialMappingParameters)
{
	sl::ERROR_CODE ErrorCode;

	SL_SCOPE_LOCK(Lock, GrabSection)
		ErrorCode = Zed.enableSpatialMapping(sl::unreal::ToSlType(SpatialMappingParameters));
	SL_SCOPE_UNLOCK

	bSpatialMappingEnabled = (ErrorCode == sl::ERROR_CODE::SUCCESS);

#if WITH_EDITOR
	if (ErrorCode != sl::ERROR_CODE::SUCCESS)
	{
		FString ErrorString(sl::toString(ErrorCode).c_str());
		SL_CAMERA_PROXY_LOG_E("Can't enable spatial mapping: \"%s\"", *ErrorString);
	}
#endif

	OnSpatialMappingEnabled.Broadcast(bSpatialMappingEnabled, sl::unreal::ToUnrealType(ErrorCode));

	return bSpatialMappingEnabled;
}

void USlCameraProxy::DisableSpatialMapping()
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		Zed.disableSpatialMapping();
	SL_SCOPE_UNLOCK

	bSpatialMappingEnabled = false;

	OnSpatialMappingDisabled.Broadcast();
}

void USlCameraProxy::PauseSpatialMapping(bool bPause)
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		Zed.pauseSpatialMapping(bPause);
	SL_SCOPE_UNLOCK

	OnSpatialMappingPaused.Broadcast(bPause);
}

void USlCameraProxy::RequestMeshAsync()
{
	Zed.requestMeshAsync();
}

bool USlCameraProxy::GetMeshIsReadyAsync()
{
	return (Zed.getMeshRequestStatusAsync() == sl::ERROR_CODE::SUCCESS);
}

bool USlCameraProxy::RetrieveMeshAsync(USlMesh* Mesh)
{
#if WITH_EDITOR
	sl::ERROR_CODE ErrorCode = Zed.retrieveMeshAsync(Mesh->Mesh);

	if (ErrorCode != sl::ERROR_CODE::SUCCESS)
	{
		FString ErrorString(sl::toString(ErrorCode).c_str());
		SL_CAMERA_PROXY_LOG_E("Can't retrieve mesh: \"%s\"", *ErrorString);
		return false;
	}

	return true;
#else
	return (Zed.retrieveMeshAsync(Mesh->Mesh) == sl::ERROR_CODE::SUCCESS);
#endif
}

bool USlCameraProxy::ExtractWholeMesh(USlMesh* Mesh)
{
#if WITH_EDITOR
	sl::ERROR_CODE ErrorCode = Zed.extractWholeMesh(Mesh->Mesh);

	if (ErrorCode != sl::ERROR_CODE::SUCCESS)
	{
		FString ErrorString(sl::toString(ErrorCode).c_str());
		SL_CAMERA_PROXY_LOG_E("Can't extract the mesh: \"%s\"", *ErrorString);
		return false;
	}

	return true;
#else
	return  (Zed.extractWholeMesh(Mesh->Mesh) == sl::ERROR_CODE::SUCCESS);
#endif
}

bool USlCameraProxy::RetrieveTexture(USlTexture* Texture)
{
	return (
		Texture->IsTypeOf(ESlTextureType::TT_Measure) ? 
		RetrieveMeasure(Texture->Mat, static_cast<USlMeasureTexture*>(Texture)->MeasureType, Texture->GetMemoryType(), FIntPoint(Texture->Width, Texture->Height)) :
		RetrieveImage(Texture->Mat, static_cast<USlViewTexture*>(Texture)->ViewType, Texture->GetMemoryType(), FIntPoint(Texture->Width, Texture->Height))
	);
}

bool USlCameraProxy::RetrieveImage(FSlMat& Mat, ESlView ViewType, ESlMemoryType MemoryType, const FIntPoint& Resolution)
{
	return RetrieveImage(Mat.Mat, ViewType, MemoryType, Resolution);
}

bool USlCameraProxy::RetrieveMeasure(FSlMat& Mat, ESlMeasure MeasureType, ESlMemoryType MemoryType, const FIntPoint& Resolution)
{
	return RetrieveMeasure(Mat.Mat, MeasureType, MemoryType, Resolution);
}

bool USlCameraProxy::RetrieveImage(sl::Mat& Mat, ESlView ViewType, ESlMemoryType MemoryType, const FIntPoint& Resolution)
{
	SCOPE_CYCLE_COUNTER(STAT_RetrieveImage);

#if WITH_EDITOR
	sl::ERROR_CODE ErrorCode = Zed.retrieveImage(Mat, sl::unreal::ToSlType(ViewType), sl::unreal::ToSlType(MemoryType), Resolution.X, Resolution.Y);

	if (ErrorCode != sl::ERROR_CODE::SUCCESS)
	{
		FString ErrorString(sl::toString(ErrorCode).c_str());
		FString MatName(Mat.name.c_str());
		SL_CAMERA_PROXY_LOG_E("Error while retrieving texture image %s: \"%s\"", *MatName, *ErrorString);

		return false;
	}

	return true;
#else
	return (Zed.retrieveImage(Mat, sl::unreal::ToSlType(ViewType), sl::unreal::ToSlType(MemoryType), Resolution.X, Resolution.Y) == sl::ERROR_CODE::SUCCESS);
#endif
}

bool USlCameraProxy::RetrieveMeasure(sl::Mat& Mat, ESlMeasure MeasureType, ESlMemoryType MemoryType, const FIntPoint& Resolution)
{
	SCOPE_CYCLE_COUNTER(STAT_RetrieveMeasure);

#if WITH_EDITOR
	sl::ERROR_CODE ErrorCode = Zed.retrieveMeasure(Mat, sl::unreal::ToSlType(MeasureType), sl::unreal::ToSlType(MemoryType), Resolution.X, Resolution.Y);

	if (ErrorCode != sl::ERROR_CODE::SUCCESS)
	{
		FString ErrorString(sl::toString(ErrorCode).c_str());
		FString MatName(Mat.name.c_str());
		SL_CAMERA_PROXY_LOG_E("Error while retrieving texture measure %s: \"%s\"", *MatName, *ErrorString);
	}

	return (ErrorCode == sl::ERROR_CODE::SUCCESS);
#else
	return (Zed.retrieveMeasure(Mat, sl::unreal::ToSlType(MeasureType), sl::unreal::ToSlType(MemoryType), Resolution.X, Resolution.Y) == sl::ERROR_CODE::SUCCESS);
#endif
}

void USlCameraProxy::SetCameraSettings(const FSlCameraSettings& NewCameraSettings)
{
	bool bDefault = NewCameraSettings.bDefault;

	if (!bDefault)
	{
		FMath::Clamp(NewCameraSettings.Brightness,   0, 8);
		FMath::Clamp(NewCameraSettings.Contrast,	 0, 8);
		FMath::Clamp(NewCameraSettings.Hue,			 0, 11);
		FMath::Clamp(NewCameraSettings.Saturation,   0, 8);
		FMath::Clamp(NewCameraSettings.WhiteBalance, 2800, 6500);
		FMath::Clamp(NewCameraSettings.Gain,		 0, 100);
		FMath::Clamp(NewCameraSettings.Exposure,	 0, 100);
	}

	Zed.setCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_BRIGHTNESS,		   NewCameraSettings.Brightness,		bDefault);
	Zed.setCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_CONTRAST,		   NewCameraSettings.Contrast,			bDefault);
	Zed.setCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_HUE,				   NewCameraSettings.Hue,				bDefault);
	Zed.setCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_SATURATION,		   NewCameraSettings.Saturation,		bDefault);
	Zed.setCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_GAIN,			   NewCameraSettings.Gain,				NewCameraSettings.bAutoGainAndExposure);
	Zed.setCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_EXPOSURE,		   NewCameraSettings.Exposure,			NewCameraSettings.bAutoGainAndExposure);
	Zed.setCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_AUTO_WHITEBALANCE,  NewCameraSettings.bAutoWhiteBalance, NewCameraSettings.bAutoWhiteBalance || bDefault);
	Zed.setCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_WHITEBALANCE,	   NewCameraSettings.WhiteBalance,		NewCameraSettings.bAutoWhiteBalance || bDefault);

	bAutoGainAndExposure = NewCameraSettings.bAutoGainAndExposure;
}

FSlTimestamp USlCameraProxy::GetTimestamp(ESlTimeReference TimeReference)
{
	if (TimeReference == ESlTimeReference::TR_Image)
	{
		SL_SCOPE_LOCK(Lock, GrabSection)
			return Zed.getTimestamp(sl::unreal::ToSlType(ESlTimeReference::TR_Image));
		SL_SCOPE_UNLOCK
	}

	return Zed.getTimestamp(sl::unreal::ToSlType(TimeReference));
}

void USlCameraProxy::EnableGrabThread(bool bEnable)
{
	if (bEnable)
	{
		if (GrabWorker)
		{
			return;
		}

		GrabWorker = new FSlGrabRunnable();
		GrabWorker->Start(0.001f);

		OnGrabThreadEnabled.Broadcast(true);
	}
	else
	{
		if (!GrabWorker)
		{
			return;
		}

		GrabWorker->EnsureCompletion();
		delete GrabWorker;
		GrabWorker = nullptr;

		OnGrabThreadEnabled.Broadcast(false);
	}
}

void USlCameraProxy::SetOpenCameraErrorCode(ESlErrorCode ErrorCode)
{
	SL_SCOPE_LOCK(SubLock, AsyncStatusSection)
		OpenCameraErrorCode = ErrorCode;
	SL_SCOPE_UNLOCK
}


ESlErrorCode USlCameraProxy::GetOpenCameraErrorCode()
{
	SL_SCOPE_LOCK(Lock, AsyncStatusSection)
		return OpenCameraErrorCode;
	SL_SCOPE_UNLOCK
}

float USlCameraProxy::GetDepth(const FSlViewportHelper& ViewportHelper, const FIntPoint& ScreenPosition)
{
	checkf(MeasuresWorker && bHitTestDepthEnabled, TEXT("Depth hit tests must be enabled"));

	float Depth = MeasuresWorker->GetDepth(ScreenPosition, ViewportHelper.RangeX, ViewportHelper.RangeY);

	if (!FMath::IsFinite(Depth))
	{
		if (FMath::IsNaN(Depth) || Depth > 0.0f)
		{
			Depth = Zed.getDepthMaxRangeValue();
		}
		else
		{
			Depth = Zed.getDepthMinRangeValue();
		}
	}

	return (Depth + HMDToCameraOffset);
}

TArray<float> USlCameraProxy::GetDepths(const FSlViewportHelper& ViewportHelper, const TArray<FIntPoint>& ScreenPositions)
{
	checkf(MeasuresWorker && bHitTestDepthEnabled, TEXT("Depth hit tests must be enabled"));

	TArray<float> Depths = MeasuresWorker->GetDepths(ScreenPositions, ViewportHelper.RangeX, ViewportHelper.RangeY);

	for (auto Iterator = Depths.CreateIterator(); Iterator; ++Iterator)
	{
		float& Depth = (*Iterator);

		if (!FMath::IsFinite(Depth))
		{
			if (FMath::IsNaN(Depth) || Depth > 0.0f)
			{
				Depth = Zed.getDepthMaxRangeValue();
			}
			else
			{
				Depth = Zed.getDepthMinRangeValue();
			}
		}

		Depth += HMDToCameraOffset;
	}

	return Depths;
}

FVector USlCameraProxy::GetNormal(const FSlViewportHelper& ViewportHelper, const FIntPoint& ScreenPosition)
{
	checkf(MeasuresWorker && bHitTestNormalsEnabled, TEXT("Normals hit tests must be enabled"));

	FVector Normal = MeasuresWorker->GetNormal(ScreenPosition, ViewportHelper.RangeX, ViewportHelper.RangeY);

	float Size = Normal.SizeSquared();
	if (!FMath::IsFinite(Size) || FMath::IsNaN(Size))
	{
		Normal = FVector::ZeroVector;
	}

	return Normal;
}

TArray<FVector> USlCameraProxy::GetNormals(const FSlViewportHelper& ViewportHelper, const TArray<FIntPoint>& ScreenPositions)
{
	checkf(MeasuresWorker && bHitTestNormalsEnabled, TEXT("Normals hit tests must be enabled"));

	TArray<FVector> Normals = MeasuresWorker->GetNormals(ScreenPositions, ViewportHelper.RangeX, ViewportHelper.RangeY);

	for (auto Iterator = Normals.CreateIterator(); Iterator; ++Iterator)
	{
		FVector& Normal = (*Iterator);

		float Size = Normal.SizeSquared();
		if (!FMath::IsFinite(Size) || FMath::IsNaN(Size))
		{
			Normal = FVector::ZeroVector;
		}
	}

	return Normals;
}

void USlCameraProxy::GetDepthAndNormal(const FSlViewportHelper& ViewportHelper, const FIntPoint& ScreenPosition, float& Depth, FVector& Normal)
{
	checkf(MeasuresWorker && bHitTestDepthEnabled && bHitTestNormalsEnabled, TEXT("Depth and Normals hit tests must be enabled"));

	FVector4 DepthAndNormal = MeasuresWorker->GetDepthAndNormal(ScreenPosition, ViewportHelper.RangeX, ViewportHelper.RangeY);
	
	Depth = DepthAndNormal.W;
	if (!FMath::IsFinite(Depth))
	{
		if (FMath::IsNaN(Depth) || Depth > 0.0f)
		{
			Depth = Zed.getDepthMaxRangeValue();
		}
		else
		{
			Depth = Zed.getDepthMinRangeValue();
		}
	}

	Depth += HMDToCameraOffset;

	Normal = FVector(DepthAndNormal.X, DepthAndNormal.Y, DepthAndNormal.Z);
	float Size = Normal.SizeSquared();
	if (!FMath::IsFinite(Size) || FMath::IsNaN(Size))
	{
		Normal = FVector::ZeroVector;
	}
}

void USlCameraProxy::GetDepthsAndNormals(const FSlViewportHelper& ViewportHelper, const TArray<FIntPoint>& ScreenPositions, TArray<float>& Depths, TArray<FVector>& Normals)
{
	checkf(MeasuresWorker && bHitTestDepthEnabled && bHitTestNormalsEnabled, TEXT("Depth and Normals hit tests must be enabled"));

	TArray<FVector4> DepthsAndNormals = MeasuresWorker->GetDepthsAndNormals(ScreenPositions, ViewportHelper.RangeX, ViewportHelper.RangeY);

	int ScreenPositionsNums = ScreenPositions.Num();
	Depths.Reserve(ScreenPositionsNums);
	Normals.Reserve(ScreenPositionsNums);

	for (int ScreenPositionsIndex = 0; ScreenPositionsIndex < ScreenPositionsNums; ++ScreenPositionsIndex)
	{
		float& Depth = DepthsAndNormals[ScreenPositionsIndex].Z;
		if (!FMath::IsFinite(Depth))
		{
			if (FMath::IsNaN(Depth) || Depth > 0.0f)
			{
				Depth = Zed.getDepthMaxRangeValue();
			}
			else
			{
				Depth = Zed.getDepthMinRangeValue();
			}
		}
		Depths.Add(Depth + HMDToCameraOffset);

		FVector Normal(DepthsAndNormals[ScreenPositionsIndex]);
		float Size = Normal.SizeSquared();
		if (!FMath::IsFinite(Size) || FMath::IsNaN(Size))
		{
			Normal = FVector::ZeroVector;
		}
		Normals.Add(Normal);
	}
}

void USlCameraProxy::SetHitTestDepthAndNormals(bool bEnableDepth, bool bEnableNormals)
{
	bHitTestDepthEnabled = bEnableDepth;
	bHitTestNormalsEnabled = bEnableNormals;

	if (MeasuresWorker)
	{
		MeasuresWorker->SetDepthAndNormals(bEnableDepth, bEnableNormals);
	}
}

void USlCameraProxy::EnableHitTest(bool bEnabled)
{
	if (bEnabled)
	{
		if (MeasuresWorker)
		{
			return;
		}

		MeasuresWorker = new FSlMeasureRunnable();
		MeasuresWorker->SetDepthAndNormals(bHitTestDepthEnabled, bHitTestNormalsEnabled);
		MeasuresWorker->Start(30.0f);

		OnHitTestThreadEnabled.Broadcast(true);
	}
	else
	{
		if (!MeasuresWorker)
		{
			return;
		}

		MeasuresWorker->EnsureCompletion();
		delete MeasuresWorker;
		MeasuresWorker = nullptr;

		OnHitTestThreadEnabled.Broadcast(false);
	}
}

FDelegateHandle USlCameraProxy::AddToGrabDelegate(TFunction<void(ESlErrorCode, const FSlTimestamp&)> Lambda)
{
	SL_SCOPE_LOCK(Lock, GrabDelegateSection)
		return OnGrabDoneDelegate.AddLambda(Lambda);
	SL_SCOPE_UNLOCK
}

FGrabDelegateHandle USlCameraProxy::BP_AddToGrabDelegate(UObject* Object)
{
	checkf(Object->GetClass()->ImplementsInterface(UGrabCallbackInterface::StaticClass()), TEXT("Object must implement IGrabCallbackInterface interface"));

	FGrabDelegateHandle Handle = AddToGrabDelegate([Object](ESlErrorCode ErrorCode, const FSlTimestamp& Timestamp) {
		IGrabCallbackInterface::Execute_GrabCallback(Object, ErrorCode, Timestamp);
	});

	return Handle;
}

void USlCameraProxy::RemoveFromGrabDelegate(FDelegateHandle& Handle)
{
	SL_SCOPE_LOCK(Lock, GrabDelegateSection)
		if (Handle.IsValid())
		{
			OnGrabDoneDelegate.Remove(Handle);
			Handle.Reset();
		}
	SL_SCOPE_UNLOCK
}

void USlCameraProxy::BP_RemoveFromGrabDelegate(FGrabDelegateHandle GrabDelegateHandle)
{
	RemoveFromGrabDelegate(GrabDelegateHandle.Handle);
}

ESlErrorCode USlCameraProxy::EnableSVORecording(FString SVOFilePath, ESlSVOCompressionMode CompressionMode)
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		sl::ERROR_CODE ErrorCode = Zed.enableRecording(TCHAR_TO_UTF8(*SVOFilePath), sl::unreal::ToSlType(CompressionMode));

		SL_SCOPE_LOCK(SubLock, SVOSection)
			bSVORecordingEnabled = (ErrorCode == sl::ERROR_CODE::SUCCESS);
		SL_SCOPE_UNLOCK

#if WITH_EDITOR
		if (!bSVORecordingEnabled)
		{
			FString ErrorString(sl::toString(ErrorCode).c_str());
			SL_CAMERA_PROXY_LOG_E("Can't enable SVO recording : \"%s\"", *ErrorString);
		}
#endif

		return sl::unreal::ToUnrealType(ErrorCode);
	SL_SCOPE_UNLOCK
}

void USlCameraProxy::DisableSVORecording()
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		Zed.disableRecording();

		SL_SCOPE_LOCK(SubLock, SVOSection)
			bSVORecordingEnabled = false;
		SL_SCOPE_UNLOCK	
	SL_SCOPE_UNLOCK
}

void USlCameraProxy::SetSVOPlaybackPosition(int Position)
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		Position = FMath::Max(0, FMath::Min(Zed.getSVONumberOfFrames() - 1, Position));

		SL_SCOPE_LOCK(SubLock, SVOSection)
			if (bSVOPlaybackPaused)
			{
				CurrentSVOPlaybackPosition = Position;
			}
			else
			{
				Zed.setSVOPosition(Position);
			}
		SL_SCOPE_UNLOCK
	SL_SCOPE_UNLOCK
}

int USlCameraProxy::GetSVOPlaybackPosition()
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		SL_SCOPE_LOCK(SubLock, SVOSection)
			return Zed.getSVOPosition();
		SL_SCOPE_UNLOCK
	SL_SCOPE_UNLOCK
}

int USlCameraProxy::GetSVONumberOfFrames()
{
	return Zed.getSVONumberOfFrames();
}

FSlRecordingState USlCameraProxy::RecordCurrentFrame()
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		SL_SCOPE_LOCK(SubLock, SVOSection)
			SlRecordingState = Zed.record();

#if WITH_EDITOR
	if (!SlRecordingState.status)
	{
		SL_CAMERA_PROXY_LOG_E("Can't record current frame");
	}
#endif

			return sl::unreal::ToUnrealType(SlRecordingState);
		SL_SCOPE_UNLOCK
	SL_SCOPE_UNLOCK
}

void USlCameraProxy::PauseSVOplayback(bool bPause, int NewSVOPosition/* = -1*/)
{
	SL_SCOPE_LOCK(SubLock, SVOSection)
		if (bPause)
		{
			CurrentSVOPlaybackPosition = NewSVOPosition >= 0 ? NewSVOPosition : Zed.getSVOPosition() - 1;
		}
		
		bSVOPlaybackPaused = bPause;
	SL_SCOPE_UNLOCK
}

void USlCameraProxy::SetSVOPlaybackLooping(bool bLoop)
{
	SL_SCOPE_LOCK(Lock, SVOSection)
		bSVOLooping = bLoop;
	SL_SCOPE_UNLOCK
}

void USlCameraProxy::SetSVORecordFrames(bool bRecord)
{
	SL_SCOPE_LOCK(Lock, SVOSection)
		bSVORecordingFrames = bRecord;
	SL_SCOPE_UNLOCK
}

FSlRecordingState USlCameraProxy::GetSVORecordingState()
{
	SL_SCOPE_LOCK(Lock, SVOSection)
		return sl::unreal::ToUnrealType(SlRecordingState);
	SL_SCOPE_UNLOCK
}

void USlCameraProxy::PushCudaContext()
{
	cuCtxPushCurrent(Zed.getCUDAContext());
}

void USlCameraProxy::PopCudaContext()
{
	CUcontext CudaContext = Zed.getCUDAContext();
	cuCtxPopCurrent(&CudaContext);
}

int32 USlCameraProxy::GetConfidenceThreshold()
{
	return Zed.getConfidenceThreshold();
}

void USlCameraProxy::SetConfidenceThreshold(int32 NewConfidenceThreshold)
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		Zed.setConfidenceThreshold(NewConfidenceThreshold);
	SL_SCOPE_UNLOCK
}

float USlCameraProxy::GetDepthMaxRangeValue()
{
	return Zed.getDepthMaxRangeValue();
}

void USlCameraProxy::SetDepthMaxRangeValue(float NewDepthMaxRange)
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		Zed.setDepthMaxRangeValue(NewDepthMaxRange);
	SL_SCOPE_UNLOCK
}

float USlCameraProxy::GetDepthMinRangeValue()
{
	return Zed.getDepthMinRangeValue();
}

float USlCameraProxy::GetCameraFPS()
{
	return Zed.getCameraFPS();
}

void USlCameraProxy::SetCameraFPS(int NewFPS)
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		Zed.setCameraFPS(NewFPS);
	SL_SCOPE_UNLOCK
}

float USlCameraProxy::GetCurrentFPS()
{
	return Zed.getCurrentFPS();
}

float USlCameraProxy::GetFrameDroppedCount()
{
	return Zed.getFrameDroppedCount();
}

void USlCameraProxy::ResetSelfCalibration()
{
	SL_SCOPE_LOCK(Lock, GrabSection)
		Zed.resetSelfCalibration();
	SL_SCOPE_UNLOCK
}

ESlSelfCalibrationState USlCameraProxy::GetSelfCalibratioState()
{
	return sl::unreal::ToUnrealType(Zed.getSelfCalibrationState());
}