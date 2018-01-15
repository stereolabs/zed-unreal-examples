//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "SpatialMappingPrivatePCH.h"
#include "SpatialMapping/Public/Core/SpatialMappingCubemapManager.h"

ASpatialMappingCubemapManager::ASpatialMappingCubemapManager()
	:
	bCanUpdateTextureTarget(true),
	CubemapWorker(nullptr),
	Cubemap(nullptr),
	TextureTarget(nullptr),
	Camera(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PrePhysics;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	Camera = CreateDefaultSubobject<USceneCaptureComponentCube>(TEXT("MainCamera"), true);
	Camera->SetupAttachment(RootComponent);
	Camera->bCaptureEveryFrame = false;
	Camera->bCaptureOnMovement = false;
}

void ASpatialMappingCubemapManager::BeginPlay()
{
	Super::BeginPlay();

	if (TextureTarget && TextureTarget->IsValidLowLevel())
	{
		SetTextureTarget(TextureTarget);
	}

	CubemapWorker = new FSpatialMappingCubemapRunnable(&CubemapProxy);
	CubemapWorker->Start(1.0f);
	CubemapWorker->Sleep();
}

void ASpatialMappingCubemapManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (CubemapWorker)
	{
		CubemapWorker->EnsureCompletion();
		delete CubemapWorker;
		CubemapWorker = nullptr;
	}
}

void ASpatialMappingCubemapManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CubemapProxy.bComplete)
	{
		// Suspend thread
		CubemapWorker->Sleep();
		// Building complete can update texture target
		bCanUpdateTextureTarget = true;
		// Reset flag
		CubemapProxy.bComplete = false;
		// Reset proxy cubemap
		CubemapProxy.Cubemap = nullptr;
		// Update cubemap resource
		Cubemap->UpdateResource();
		// Notify
		OnCubemapBuildComplete.Broadcast(Cubemap);
	}
}

#if WITH_EDITOR
void ASpatialMappingCubemapManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASpatialMappingCubemapManager, TextureTarget))
	{
		SetTextureTarget(TextureTarget);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool ASpatialMappingCubemapManager::CanEditChange(const UProperty* InProperty) const
{
	FName PropertyName = InProperty->GetFName();


	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASpatialMappingCubemapManager, TextureTarget))
	{
		return CubemapWorker == nullptr;
	}

	return Super::CanEditChange(InProperty);
}
#endif

bool ASpatialMappingCubemapManager::BuildCubemap(const FName& Name)
{
	// Cubemap build not finished
	if (!CubemapWorker->IsSleeping())
	{
		return false;
	}

	bCanUpdateTextureTarget = false;

	// create the cube texture or retrieve the existing one
	Cubemap = NewObject<UTextureCube>(
		GetTransientPackage(),
		Name,
		RF_Transient);
	
	CubemapProxy.Cubemap = Cubemap;
	CubemapWorker->GetPixels();
	CubemapWorker->Awake();

	return true;
}

void ASpatialMappingCubemapManager::CaptureScene()
{
	Camera->CaptureScene();
}

bool ASpatialMappingCubemapManager::SetTextureTarget(UTextureRenderTargetCube* NewTextureTarget)
{
	if (!bCanUpdateTextureTarget)
	{
		return false;
	}

	Camera->TextureTarget = NewTextureTarget;
	CubemapProxy.TextureTarget = NewTextureTarget;

	return true;
}