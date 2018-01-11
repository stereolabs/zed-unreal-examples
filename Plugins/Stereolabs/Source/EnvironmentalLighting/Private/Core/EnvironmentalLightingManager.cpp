//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "EnvironmentalLightingPrivatePCH.h"
#include "EnvironmentalLighting/Public/Core/EnvironmentalLightingManager.h"
#include "Stereolabs/Public/Core/StereolabsCameraProxy.h"
#include "Stereolabs/Public/Core/StereolabsCoreUtilities.h"

#include <sl_mr_core/EnvironmentalLighting.hpp>

/** Set environmental lighting max intensity */
static TAutoConsoleVariable<float> CVarEnvLightMaxIntensity(
	TEXT("r.ZED.EnvLightMaxIntensity"),
	1.0f,
	TEXT("Default to 1.0, range [0.005, 2.0]"),
	ECVF_SetByConsole
);

AEnvironmentalLightingManager::AEnvironmentalLightingManager()
	:
	Batch(nullptr),
	LeftEyeTexture(nullptr),
	Light(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEnvironmentalLightingManager::BeginPlay()
{
	Super::BeginPlay();

	Batch = USlCPUTextureBatch::CreateCPUTextureBatch(FName("EnvironmentalLightingBatch"));

	sl::mr::environmentalLightingInitialize();

	SetActorTickEnabled(false);

	GSlCameraProxy->OnCameraOpened.AddDynamic(this, &AEnvironmentalLightingManager::ToggleTick);
	GSlCameraProxy->OnCameraClosed.AddDynamic(this, &AEnvironmentalLightingManager::ToggleTick);
}

void AEnvironmentalLightingManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	FlushRenderingCommands();

	if (GSlCameraProxy)
	{
		GSlCameraProxy->RemoveFromGrabDelegate(GrabDelegateHandle);

		GSlCameraProxy->OnCameraOpened.RemoveDynamic(this, &AEnvironmentalLightingManager::ToggleTick);
		GSlCameraProxy->OnCameraClosed.RemoveDynamic(this, &AEnvironmentalLightingManager::ToggleTick);
	}
	
	Batch->Clear();

	sl::mr::environmentalLightingShutdown();
}

void AEnvironmentalLightingManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UDirectionalLightComponent* LightComponent = Light ? static_cast<UDirectionalLightComponent*>(Light->GetLightComponent()) : nullptr;

	if (LightComponent && LightComponent->bEnableEnvironmentalLighting)
	{
		if (!GrabDelegateHandle.IsValid())
		{
			GrabDelegateHandle = GSlCameraProxy->AddToGrabDelegate([this](ESlErrorCode ErrorCode, const FSlTimestamp& Timestamp) {
				GrabCallback(ErrorCode, Timestamp);
			});
		}
		
		bool bUpdateLighting = Batch->Tick();
		if (bUpdateLighting)
		{
			sl::mr::environmentalLightingComputeDiffuseCoefficients(&LeftEyeTexture->Mat.Mat);

			sl::Matrix4f Red;
			sl::Matrix4f Green;
			sl::Matrix4f Blue;
			sl::mr::environmentalLightingGetShmMatrix(&Red, 0);
			sl::mr::environmentalLightingGetShmMatrix(&Green, 1);
			sl::mr::environmentalLightingGetShmMatrix(&Blue, 2);

			EnvironmentalLightingSettings.Red = sl::unreal::ToUnrealType(Red);
			EnvironmentalLightingSettings.Green = sl::unreal::ToUnrealType(Green);
			EnvironmentalLightingSettings.Blue = sl::unreal::ToUnrealType(Blue);
		}

		EnvironmentalLightingSettings.Exposure = sl::mr::environmentalLightingGetExposure(DeltaSeconds);
		EnvironmentalLightingSettings.Exposure += (EnvironmentalLightingSettings.Exposure > 0.1f ? 0.2f : 0.0f);
		EnvironmentalLightingSettings.Exposure = FMath::Min(2.0f, EnvironmentalLightingSettings.Exposure * 2.0f);

		LightComponent->SetIntensity(FMath::Clamp(EnvironmentalLightingSettings.Exposure, 0.005f, CVarEnvLightMaxIntensity.GetValueOnGameThread()));
		LightComponent->SetEnvironmentalLightingSettings(EnvironmentalLightingSettings);
	}
	else
	{
		GSlCameraProxy->RemoveFromGrabDelegate(GrabDelegateHandle);
	}
}

void AEnvironmentalLightingManager::ToggleTick()
{
	if (GSlCameraProxy->IsCameraOpened())
	{
		LeftEyeTexture = USlViewTexture::CreateCPUViewTexture(FName("EnvLightingLeftView"), 128, 72, ESlView::V_Left);

		Batch->AddTexture(LeftEyeTexture);
	}
	else
	{
		Batch->Clear();

		delete LeftEyeTexture;
		LeftEyeTexture = nullptr;		
	}

	SetActorTickEnabled(GSlCameraProxy->IsCameraOpened());
}

void AEnvironmentalLightingManager::GrabCallback(ESlErrorCode ErrorCode, const FSlTimestamp& Timestamp)
{
	if (ErrorCode != ESlErrorCode::EC_Success)
	{
		return;
	}

	Batch->RetrieveCurrentFrame(Timestamp);
}