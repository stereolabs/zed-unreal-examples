//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "DevicesPrivatePCH.h"
#include "Devices/Public/Core/DevicesMotionController.h"
#include "Stereolabs/Public/Core/StereolabsBaseTypes.h"
#include "ZED/Public/Utilities/ZEDFunctionLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"

#include <sl_mr_core/latency.hpp>

ADevicesMotionController::ADevicesMotionController()
	:
	LatencyTime(25)
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>("MotionController");
	MotionController->bDisableLowLatencyUpdate = true;
	MotionController->SetCanEverAffectNavigation(false);
	MotionController->SetupAttachment(RootComponent);

	AddTickPrerequisiteComponent(MotionController);

	TransformBuffer.Reserve(LatencyTime);
}

void ADevicesMotionController::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickEnabled(false);

	AZEDCamera* ZedCameraActor = UZEDFunctionLibrary::GetCameraActor(this);
	if (ZedCameraActor)
	{
		UZEDFunctionLibrary::GetCameraActor(this)->OnCameraActorInitialized.AddDynamic(this, &ADevicesMotionController::Start);
	}

	GSlCameraProxy->OnCameraClosed.AddDynamic(this, &ADevicesMotionController::Stop);
}

void ADevicesMotionController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AZEDCamera* ZedCameraActor = UZEDFunctionLibrary::GetCameraActor(this);
	if (ZedCameraActor)
	{
		ZedCameraActor->OnCameraActorInitialized.RemoveDynamic(this, &ADevicesMotionController::Start);
	}

	if (GSlCameraProxy)
	{
		GSlCameraProxy->OnCameraClosed.RemoveDynamic(this, &ADevicesMotionController::Stop);
	}
}

void ADevicesMotionController::Tick(float DeltaTime)
{
	FTransform Transform;
	if (GetTransform(Transform))
	{
		SetActorTransform(Transform);
	}

	Super::Tick(DeltaTime);
}

void ADevicesMotionController::Start()
{
	if (!UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		return;
	}

	UZEDFunctionLibrary::GetCameraActor(this)->OnCameraActorInitialized.RemoveDynamic(this, &ADevicesMotionController::Start);
	
	UpdateTransformBuffer();

	GetWorldTimerManager().SetTimer(TimerHandle, this, &ADevicesMotionController::UpdateTransformBuffer, 0.001f, true);
	SetActorTickEnabled(true);
}

void ADevicesMotionController::Stop()
{
	GetWorldTimerManager().ClearTimer(TimerHandle);
	SetActorTickEnabled(false);
}

FTransform ADevicesMotionController::GetDelayedTransform()
{
	return TransformBuffer[0];
}

void ADevicesMotionController::UpdateTransformBuffer()
{
	check(MotionController);

	if (TransformBuffer.Num() + 1 > LatencyTime)
	{
		TransformBuffer.RemoveAt(0, 1, false);
	}

	TransformBuffer.Add(MotionController->GetRelativeTransform());
}

bool ADevicesMotionController::GetTransform(FTransform& Transform)
{
	FZEDTrackingData TrackingData = UZEDFunctionLibrary::GetTrackingData();
	FTransform DelayedTransform = GetDelayedTransform();
	sl::Transform SlLatencyTransform;

	bool bTransform = sl::mr::latencyCorrectorGetTransform(GSlCameraProxy->GetCamera().getTimestamp(sl::TIME_REFERENCE::TIME_REFERENCE_CURRENT) - sl::timeStamp(LatencyTime * 1000000), SlLatencyTransform, false);
	if (!bTransform)
	{
		return false;
	}

	Transform = (DelayedTransform * sl::unreal::ToUnrealType(SlLatencyTransform).Inverse()) * TrackingData.OffsetZedWorldTransform;

	return true;
}