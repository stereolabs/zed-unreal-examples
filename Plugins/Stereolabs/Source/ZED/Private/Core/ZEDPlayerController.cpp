// Fill out your copyright notice in the Description page of Project Settings.

#include "ZEDPrivatePCH.h"
#include "ZED/Public/Core/ZEDPlayerController.h"
#include "ZED/Public/Utilities/ZEDFunctionLibrary.h"
#include "ZED/Public/Core/ZEDInitializer.h"
#include "ZED/Classes/ZEDGameInstance.h"
#include "Stereolabs/Public/Core/StereolabsCoreGlobals.h"
#include "Stereolabs/Public/Core/StereolabsCameraProxy.h"
#include "Stereolabs/Public/Utilities/StereolabsFunctionLibrary.h"
#include "UMG.h"
#include "UnrealNetwork.h"

DEFINE_LOG_CATEGORY(ZEDPlayerController);

#define MONO_NOISE_OFFSET 0.85f
#define STEREO_NOISE_OFFSET 0.35f
#define STEREO_BLUR_SIGMA 0.5f

#define ADD_FVECTOR_2D(Vector, Value)\
	Vector.X += Value;\
	Vector.Y += Value;\

/** Activate/Deactivate noise */
static TAutoConsoleVariable<int32> CVarZEDNoise(
	TEXT("r.ZED.Noise"),
	0,
	TEXT("1 to enable noise, 0 to disable"),
	ECVF_RenderThreadSafe
);

// Show loading text while opening camera/enabling tracking
#define SHOW_LOADING_TEXT 0

AZEDPlayerController::AZEDPlayerController()
	:
	bTickZedCamera(false),
	bUseDefaultBeginPlay(true),
	bOpenZedCameraAtInit(true),
	bStereoRenderingSupport(true),
	bStartInVR(true),
	bIsFirstPlayer(false),
	bUseShowOnlyList(false),
	bHideWorldOpeningZedCamera(true),
	CurrentNoiseValue(0),
	ZedPawn(nullptr),
	ZedCamera(nullptr),
	PawnClass(AZEDPawn::StaticClass())
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PrePhysics;

	static ConstructorHelpers::FObjectFinder<UMaterial> PostProcessFadeMaterial(TEXT("Material'/Stereolabs/ZED/Materials/M_ZED_Fade.M_ZED_Fade'"));
	PostProcessFadeSourceMaterial = PostProcessFadeMaterial.Object;

	static ConstructorHelpers::FObjectFinder<UMaterial> PostProcessZedMaterial(TEXT("Material'/Stereolabs/ZED/Materials/M_ZED_PostProcess.M_ZED_PostProcess'"));
	PostProcessZedSourceMaterial = PostProcessZedMaterial.Object;

	static ConstructorHelpers::FObjectFinder<UClass> ZedCameraBlueprint(TEXT("'/Stereolabs/ZED/Blueprints/BP_ZED_Camera.BP_ZED_Camera_C'"));
	ZedCameraBlueprintClass = ZedCameraBlueprint.Object;
	
	static ConstructorHelpers::FObjectFinder<UCurveFloat> FadeCurve(TEXT("CurveFloat'/Stereolabs/ZED/Utility/C_Fade.C_Fade'"));
	FadeTimelineCurve = FadeCurve.Object;

	FadeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("FadeTimeline"));
	FadeTimeline->SetTimelineLength(0.75f);

	FadeFunction.BindUFunction(this, "Fading");
	FadeTimeline->AddInterpFloat(FadeTimelineCurve, FadeFunction);
}

void AZEDPlayerController::Tick(float DeltaSeconds)
{
	if (bTickZedCamera)
	{
		ZedCamera->Tick(DeltaSeconds);
	}

	int32 NoiseValue = CVarZEDNoise.GetValueOnGameThread();
	if (CurrentNoiseValue != NoiseValue)
	{
		CurrentNoiseValue = NoiseValue;

		if (CurrentNoiseValue)
		{
			PostProcessZedMaterialInstanceDynamic->SetScalarParameterValue("UseNoise", 1);
			GetWorldTimerManager().SetTimer(NoiseTimerHandle, this, &AZEDPlayerController::UpdateNoise, 5.0f, true);
		}
		else
		{
			PostProcessZedMaterialInstanceDynamic->SetScalarParameterValue("UseNoise", 0);
			GetWorldTimerManager().ClearTimer(NoiseTimerHandle);
		}
	}

	Super::Tick(DeltaSeconds);
}

void AZEDPlayerController::BeginPlay()
{
	bIsFirstPlayer = (GetWorld()->GetFirstPlayerController() == this);

	bool bIsStandalone = UKismetSystemLibrary::IsStandalone(this);
	bool bIsLocal = IsLocalPlayerController();

#if WITH_EDITOR
	// Listen server or client
	if (bIsFirstPlayer && bIsLocal)
	{
		// Check game instance type
		UZEDGameInstance* ZedGameInstance = Cast<UZEDGameInstance>(GetGameInstance());

		checkf(ZedGameInstance, TEXT("Game instance must inherit from UZEDGameInstance"));
	}
#endif

	// User begin play
	Super::BeginPlay();

	if (bUseDefaultBeginPlay)
	{
		// Standalone
		if (bIsStandalone)
		{
			SpawnPawn(PawnClass);

			// First player
			if (bIsFirstPlayer)
			{
				SpawnZedCameraActor();

				Init();
			}
		}
		// Client or server
		else
		{
			// Server
			if (HasAuthority())
			{
				SpawnPawn(PawnClass);

				// Listen server controller and first player
				if (bIsLocal && bIsFirstPlayer)
				{
					SpawnZedCameraActor();

					Init();
				}
			}
			// Client
			else
			{
				// First player
				if (bIsFirstPlayer)
				{
					SpawnZedCameraActor();
				}
			}
		}
	}
}

void AZEDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (!bIsFirstPlayer || !IsLocalPlayerController())
	{
		return;
	}

	if (GSlCameraProxy)
	{
		if (GSlCameraProxy->IsCameraOpened())
		{
			GSlCameraProxy->CloseCamera();
		}

		GSlCameraProxy->OnCameraDisconnected.RemoveDynamic(this, &AZEDPlayerController::ZedCameraDisconnected);
		GSlCameraProxy->OnCameraClosed.RemoveDynamic(this, &AZEDPlayerController::ZedCameraClosed);
		GSlCameraProxy->OnTrackingEnabled.RemoveDynamic(this, &AZEDPlayerController::ZedCameraTrackingEnabled);
	}

	if (ZedCamera)
	{
		ZedCamera->OnCameraActorInitialized.RemoveDynamic(this, &AZEDPlayerController::ZedCameraActorInitialized);

		if (ZedPawn)
		{
			ZedCamera->OnTrackingDataUpdated.RemoveDynamic(ZedPawn, &AZEDPawn::ZedCameraTrackingUpdated);
		}
	}

	GetWorldTimerManager().ClearTimer(NoiseTimerHandle);
}

UObject* AZEDPlayerController::SpawnPawn(UClass* NewPawnClass, bool bPossess)
{
	// Spawn pawn
	APawn* SpawnedPawn = Cast<APawn>(GetWorld()->SpawnActor(NewPawnClass));

	// Set zed pawn
	ZedPawn = Cast<AZEDPawn>(SpawnedPawn);

	checkf(ZedPawn, TEXT("NewPawnClass must inherit from AZedPawn"));

	if (bPossess)
	{
		Possess(SpawnedPawn);
	}

	return ZedPawn;
}

void AZEDPlayerController::SpawnZedCameraActor()
{
	ZedCamera = GetWorld()->SpawnActor<AZEDCamera>(ZedCameraBlueprintClass);
}

void AZEDPlayerController::Init()
{
	// Attach Zed camera actor to pawn
	ZedCamera->AttachToComponent(ZedPawn->GetRootComponent(), FAttachmentTransformRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, false));

	// Create dynamic post process
	PostProcessFadeMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(PostProcessFadeSourceMaterial, nullptr);
	PostProcessZedMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(PostProcessZedSourceMaterial, nullptr);

	// Bind events to proxy
	GSlCameraProxy->OnCameraOpened.AddDynamic(this, &AZEDPlayerController::ZedCameraOpened);
	GSlCameraProxy->OnCameraDisconnected.AddDynamic(this, &AZEDPlayerController::ZedCameraDisconnected);
	GSlCameraProxy->OnCameraClosed.AddDynamic(this, &AZEDPlayerController::ZedCameraClosed);

	// Bind event to Zed camera actor
	ZedCamera->OnCameraActorInitialized.AddDynamic(this, &AZEDPlayerController::ZedCameraActorInitialized);

	// Pawn tracking
	ZedCamera->OnTrackingDataUpdated.AddDynamic(ZedPawn, &AZEDPawn::ZedCameraTrackingUpdated);

	// Enable fade post process
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);

	// Enable/Disable HMD
	if (bStereoRenderingSupport)
	{
#if WITH_EDITOR
		bHMDEnabled = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
#else
		if (bStartInVR)
		{
			bHMDEnabled = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayConnected();
			UHeadMountedDisplayFunctionLibrary::EnableHMD(bHMDEnabled);
		}
#endif
	}
	else
	{
		bHMDEnabled = false;

#if WITH_EDITOR
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			SL_LOG_E(ZEDPlayerController, "Play in VR but stereo rendering not supported");
		}
#else
		UHeadMountedDisplayFunctionLibrary::EnableHMD(false);
#endif
	}

	if (!bHMDEnabled)
	{
		ZedPawn->ZedLoadingWidget->WidgetComponent->SetGeometryMode(EWidgetGeometryMode::Plane);
		ZedPawn->ZedLoadingWidget->SetWorldScale3D(FVector(0.3f));

		ZedPawn->ZedErrorWidget->WidgetComponent->SetGeometryMode(EWidgetGeometryMode::Plane);
		ZedPawn->ZedErrorWidget->SetWorldScale3D(FVector(0.3f));
	}
	else
	{
		ZedPawn->ZedLoadingWidget->bAbsoluteLocation = true;
		ZedPawn->ZedLoadingWidget->bAbsoluteRotation = true;

		ZedPawn->ZedErrorWidget->bAbsoluteLocation = true;
		ZedPawn->ZedErrorWidget->bAbsoluteRotation = true;
	}

	// User init
	InitEvent();

	// Init controller next frame to let all objects initialize
	GetWorldTimerManager().SetTimer(InitTimerHandle, this, &AZEDPlayerController::Internal_Init, 1.0f, false, 0.0f);
}

void AZEDPlayerController::Internal_Init()
{
	GetWorldTimerManager().ClearTimer(InitTimerHandle);

	// Automatically open camera
	if (bOpenZedCameraAtInit)
	{
		OpenZedCamera(bHideWorldOpeningZedCamera);
	}
}

void AZEDPlayerController::CloseZedCamera()
{
	GSlCameraProxy->CloseCamera();
}

void AZEDPlayerController::OpenZedCamera(bool bHideWorld)
{
	if (bHideWorld)
	{
		bUseShowOnlyList = true;
		ShowOnlyPrimitiveComponents.Empty();
		ShowOnlyPrimitiveComponents.Add(ZedPawn->ZedLoadingWidget->WidgetComponent);
		ShowOnlyPrimitiveComponents.Add(ZedPawn->ZedErrorWidget->WidgetComponent);
	}

	// Get Zed initializer object
	TArray<AActor*> ZedInitializer;
	UGameplayStatics::GetAllActorsOfClass(this, AZEDInitializer::StaticClass(), ZedInitializer);

#if WITH_EDITOR
	if (!ZedInitializer.Num())
	{
		SL_LOG_E(ZEDPlayerController, "BP_ZED_Initializer must be placed in the world");
		return;
	}
#endif

	AZEDInitializer* Initializer = static_cast<AZEDInitializer*>(ZedInitializer[0]);

	// Load
	Initializer->LoadParametersAndSettings();

	// Do some work before Zed actor initialization
	OnPreZedCameraOpening.Broadcast();

	for (auto ChildrenIt = Initializer->ChildActors.CreateConstIterator(); ChildrenIt; ++ChildrenIt)
	{
		(*ChildrenIt)->AttachToActor(ZedPawn, FAttachmentTransformRules(EAttachmentRule::KeepRelative, true));
	}

	// Set parameters
	ZedCamera->InitializeParameters(Initializer, bHMDEnabled);

	// Open camera with parameters
	GSlCameraProxy->OpenCamera(ZedCamera->InitParameters);

	if (ZedCamera->TrackingParameters.bEnableTracking)
	{
		GSlCameraProxy->OnTrackingEnabled.AddDynamic(this, &AZEDPlayerController::ZedCameraTrackingEnabled);
	}

	UpdateHUDOpeningZed();

	GetWorldTimerManager().SetTimer(CameraOpeningTimerHandle, this, &AZEDPlayerController::UpdateHUDCheckOpeningZed, 1.0f, true, 2.0f);
}

void AZEDPlayerController::ZedCameraOpened()
{
	GetWorldTimerManager().ClearTimer(CameraOpeningTimerHandle);

	if (!bHMDEnabled)
	{
		SetWidgetInFrontOfCamera(ZedPawn->ZedLoadingWidget);
		SetWidgetInFrontOfCamera(ZedPawn->ZedErrorWidget);
	}

	// Set fade post process
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);
	// Set camera field of view
	ZedPawn->Camera->SetFieldOfView(GSlCameraProxy->CameraInformation.CalibrationParameters.LeftCameraParameters.HFOV);

	// Init viewport helper
	UGameViewportClient* GameViewport = GetLocalPlayer()->ViewportClient;
	check(GameViewport);

	if (!bHMDEnabled)
	{
		// Add to event
		ViewportHelper.AddToViewportResizeEvent(GameViewport);
	}

	ViewportHelper.Update(GameViewport->Viewport->GetSizeXY());

	UpdateHUDCheckOpeningZed();

	// Enable tracking
	if (ZedCamera->TrackingParameters.bEnableTracking)
	{
		UpdateHUDEnablingTracking();

		ZedCamera->EnableTracking();

		/*FSlTrackingParameters TrackingParameters = ZedCamera->TrackingParameters;
		if (bHMDEnabled)
		{
			FVector HMDLocation;
			FRotator HMDRotation;
			FTransform TrackingOrigin;

			UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HMDRotation, HMDLocation);

			TrackingOrigin = FTransform(HMDRotation, HMDLocation) * ZedCamera->AntiDriftParameters.CalibrationTransform;

			TrackingParameters.Location = TrackingOrigin.GetLocation();
			TrackingParameters.Rotation = TrackingOrigin.Rotator();
		}

		GSlCameraProxy->EnableTracking(TrackingParameters);*/
	}
	else
	{
		UpdateHUDZedOpened();

		// Fade to hide zed camera actor init
		FadeIn();

		// Init zed camera actor
		GetWorldTimerManager().SetTimer(InitializeZedCameraActorTimerHandle, this, &AZEDPlayerController::Internal_InitializeZedCameraActor, 1.0f, false, 3.0f);
	}
}

void AZEDPlayerController::ZedCameraTrackingEnabled(bool bSuccess, ESlErrorCode ErrorCode, const FVector& Location, const FRotator& Rotation)
{
	UpdateHUDTrackingEnabled(bSuccess, ErrorCode);

	if (bSuccess)
	{
		// Fade to hide zed camera actor init
		FadeIn();

		// Reset HMD tracking origin. Reset is not immediate that's why the camera actor init 1s after
		GetWorldTimerManager().SetTimer(ResetHMDTrackingOriginTimerHandle, this, &AZEDPlayerController::ResetHMDTrackingOrigin, 1.0f, false, 2.0f);

		// Init zed camera actor
		GetWorldTimerManager().SetTimer(InitializeZedCameraActorTimerHandle, this, &AZEDPlayerController::Internal_InitializeZedCameraActor, 1.0f, false, 3.0f);
	}

	GSlCameraProxy->OnTrackingEnabled.RemoveDynamic(this, &AZEDPlayerController::ZedCameraTrackingEnabled);
}

void AZEDPlayerController::Internal_InitializeZedCameraActor()
{
	GetWorldTimerManager().ClearTimer(InitializeZedCameraActorTimerHandle);

	// Init zed camera actor
	ZedCamera->Init(bHMDEnabled);
}

void AZEDPlayerController::ZedCameraActorInitialized()
{
	if (!bHMDEnabled)
	{
		ZedPawn->Camera->AddOrUpdateBlendable(PostProcessZedMaterialInstanceDynamic, 1.0f);

		ZedPawn->Camera->PostProcessSettings.bVirtualObjectsPostProcess = true;
	}
	else
	{
		// Set HMD camera offset
		ZedPawn->SpringArm->SetRelativeLocation(FVector(ZedCamera->FinalCameraOffset, 0.0f, 0.0f));

		ZedCamera->AddOrUpdatePostProcess(PostProcessZedMaterialInstanceDynamic,  1.0f);

		ZedPawn->Camera->PostProcessSettings.bDeferredAA = false;
		ZedPawn->Camera->PostProcessSettings.bPostProcessing = false;
		ZedPawn->Camera->CameraRenderingSettings.bLighting = false;

		FString Command = FString::Printf(TEXT("r.VirtualObjects.BlurSigma %f"), STEREO_BLUR_SIGMA);
		ConsoleCommand(Command);
	}

	// Full HD or 2k
	if (GSlCameraProxy->CameraInformation.CalibrationParameters.LeftCameraParameters.Resolution.X >= 1920)
	{
		// Disable blur
		FString Command = FString::Printf(TEXT("r.VirtualObjects.BlurSigma %f"), 0);
		ConsoleCommand(Command);
	}

	ZedPawn->Camera->CameraRenderingSettings.bVelocity = false;

	ZedPawn->ZedLoadingWidget->SetVisibility(false);
	ZedPawn->ZedErrorWidget->SetVisibility(false);

	bUseShowOnlyList = false;
	ShowOnlyPrimitiveComponents.Empty();

	GetWorldTimerManager().SetTimer(FadeOutTimerHandle, this, &AZEDPlayerController::FadeOutToGame, 1.0f, false, 1.0f);

	bTickZedCamera = true;
}

void AZEDPlayerController::ResetHMDTrackingOrigin()
{
	GetWorldTimerManager().ClearTimer(ResetHMDTrackingOriginTimerHandle);

	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);

	if (!ZedCamera->bUseHMDTrackingAsOrigin)
	{
		UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition(0.0f, EOrientPositionSelector::OrientationAndPosition);
	}
}

void AZEDPlayerController::UpdateNoise()
{
	FZEDNoiseFactors NoiseFactors = sl::unreal::ToUnrealType(sl::mr::computeNoiseFactors(GSlCameraProxy->GetCamera().getCameraSettings(sl::CAMERA_SETTINGS::CAMERA_SETTINGS_GAIN)));

	ADD_FVECTOR_2D(NoiseFactors.R, bHMDEnabled ? STEREO_NOISE_OFFSET : MONO_NOISE_OFFSET);
	ADD_FVECTOR_2D(NoiseFactors.G, bHMDEnabled ? STEREO_NOISE_OFFSET : MONO_NOISE_OFFSET);
	ADD_FVECTOR_2D(NoiseFactors.B, bHMDEnabled ? STEREO_NOISE_OFFSET : MONO_NOISE_OFFSET);

	if (NoiseFactors.R == LastNoiseFactors.R)
	{
		return;
	}
	
	LastNoiseFactors = NoiseFactors;
	
	FLinearColor Red(NoiseFactors.R.X, NoiseFactors.R.Y, 0.0f);
	PostProcessZedMaterialInstanceDynamic->SetVectorParameterValue("RedFactors", Red);

	FLinearColor Green(NoiseFactors.G.X, NoiseFactors.G.Y, 0.0f);
	PostProcessZedMaterialInstanceDynamic->SetVectorParameterValue("GreenFactors", Green);

	FLinearColor Blue(NoiseFactors.B.X, NoiseFactors.B.Y, 0.0f);
	PostProcessZedMaterialInstanceDynamic->SetVectorParameterValue("BlueFactors", Blue);
}

void AZEDPlayerController::Fading(float FadingFactor)
{
	PostProcessFadeMaterialInstanceDynamic->SetScalarParameterValue("FadingFactor", FadingFactor);
}

void AZEDPlayerController::FadeIn()
{
	GetWorldTimerManager().ClearTimer(DisableFadePostProcessTimerHandle);
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);

	FadeTimeline->ReverseFromEnd();
}

void AZEDPlayerController::FadeOut()
{
	GetWorldTimerManager().ClearTimer(DisableFadePostProcessTimerHandle);
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);

	FadeTimeline->PlayFromStart();
}

void AZEDPlayerController::DisableFadePostProcess()
{
	GetWorldTimerManager().ClearTimer(DisableFadePostProcessTimerHandle);

	// If camera disconnected right before disabling fade post process
	if (!GSlCameraProxy->IsCameraConnected())
	{
		return;
	}
	
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 0.0f);
}

void AZEDPlayerController::Internal_CameraDisconnected()
{
	GetWorldTimerManager().ClearTimer(CameraDisconnectedTimerHandle);

	if (!bHMDEnabled)
	{
		ZedPawn->Camera->AddOrUpdateBlendable(PostProcessZedMaterialInstanceDynamic, 0.0f);
	}


	if (bHMDEnabled)
	{
		ZedPawn->Camera->PostProcessSettings.bDeferredAA = true;
		ZedPawn->Camera->PostProcessSettings.bPostProcessing = true;
		ZedPawn->Camera->CameraRenderingSettings.bLighting = true;
		ZedPawn->Camera->CameraRenderingSettings.bVelocity = true;

		ConsoleCommand(TEXT("vr.SpectatorScreenMode 3"), true);
	}

	ZedPawn->Camera->PostProcessSettings.bVirtualObjectsPostProcess = false;

	UpdateHUDZedDisconnected();

	bUseShowOnlyList = true;
	ShowOnlyPrimitiveComponents.Empty();
	ShowOnlyPrimitiveComponents.Add(ZedPawn->ZedErrorWidget->WidgetComponent);

	FadeOut();
}

void AZEDPlayerController::ZedCameraDisconnected()
{
	bTickZedCamera = false;

	GSlCameraProxy->CloseCamera();

	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);

	FadeIn();

	GetWorldTimerManager().SetTimer(CameraDisconnectedTimerHandle, this, &AZEDPlayerController::Internal_CameraDisconnected, 1.0f, false, 2.0f);
}

void AZEDPlayerController::ZedCameraClosed()
{
	GetWorldTimerManager().ClearTimer(NoiseTimerHandle);
}

void AZEDPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(AZEDPlayerController, ZedPawn, COND_OwnerOnly, REPNOTIFY_OnChanged);
}

void AZEDPlayerController::OnRep_ZedPawn()
{
	if (bUseDefaultBeginPlay)
	{
		bool bIsLocal = IsLocalPlayerController();

		if (bIsLocal && bIsFirstPlayer)
		{
			Init();
		}
	}

	OnPawnSpawned.Broadcast();
}

void AZEDPlayerController::SetWidgetInFrontOfCamera(UZEDWidget* Widget)
{
	FRotator CameraRotation = ZedPawn->Camera->GetComponentRotation();

	if (bHMDEnabled)
	{
		FVector Location = ZedPawn->Camera->GetComponentLocation() + UKismetMathLibrary::GetForwardVector(FRotator(0.0f, CameraRotation.Yaw, 0.0f)) * 300.0f;
		Widget->SetWorldLocation(Location);

		FRotator Rotation = FRotator(0.0f, CameraRotation.Yaw, 0) + FRotator(0.0f, 180.0f, 0.0f);
		Widget->SetWorldRotation(Rotation);
	}
	else
	{
		FVector Location = FVector(300.0f, 0.0f, 0.0f);

		if (GSlCameraProxy->IsCameraOpened())
		{
			Location = UZEDFunctionLibrary::GetCustomLocationInFrontOfPlayer(ZedPawn->Camera->GetComponentLocation(), FRotator(0.0f, CameraRotation.Yaw, 0.0f), 300.0f);
		}

		Widget->SetRelativeLocation(Location);
		Widget->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	}
}

void AZEDPlayerController::FadeOutToGame()
{
	GetWorldTimerManager().ClearTimer(FadeOutTimerHandle);

	FadeOut();

	GetWorldTimerManager().SetTimer(DisableFadePostProcessTimerHandle, this, &AZEDPlayerController::DisableFadePostProcess, 1.0f, false, 2.0f);
}

void AZEDPlayerController::UpdateHUDOpeningZed_Implementation()
{
#if SHOW_LOADING_TEXT
	ZedPawn->ZedLoadingWidget->SetText(FText::FromString("Searching for ZED camera"));
#endif
	ZedPawn->ZedLoadingWidget->SetVisibility(true);

	SetWidgetInFrontOfCamera(ZedPawn->ZedLoadingWidget);
	SetWidgetInFrontOfCamera(ZedPawn->ZedErrorWidget);

	ZedPawn->ZedLoadingWidget->FadeIn();
}

void AZEDPlayerController::UpdateHUDCheckOpeningZed_Implementation()
{
	ESlErrorCode OpenErrorCode = GSlCameraProxy->GetOpenCameraErrorCode();

	if (OpenErrorCode != ESlErrorCode::EC_Success && OpenErrorCode != ESlErrorCode::EC_None)
	{
		ZedPawn->ZedLoadingWidget->SetVisibility(false);

		ZedPawn->ZedErrorWidget->SetVisibility(true);
		ZedPawn->ZedErrorWidget->SetText(FText::FromString(USlFunctionLibrary::ErrorCodeToString(GSlCameraProxy->GetOpenCameraErrorCode())));
	}
	else
	{
		ZedPawn->ZedErrorWidget->SetVisibility(false);

		ZedPawn->ZedLoadingWidget->SetVisibility(true);
#if SHOW_LOADING_TEXT
		ZedPawn->ZedLoadingWidget->SetText(FText::FromString("Opening camera"));
#endif
	}
}

void AZEDPlayerController::UpdateHUDZedOpened_Implementation()
{
#if SHOW_LOADING_TEXT
	ZedPawn->ZedLoadingWidget->SetText(FText::FromString("Camera opened"));
#endif
	ZedPawn->ZedLoadingWidget->FadeOut();
}

void AZEDPlayerController::UpdateHUDEnablingTracking_Implementation()
{
#if SHOW_LOADING_TEXT
	ZedPawn->ZedLoadingWidget->SetText(FText::FromString("Enabling tracking"));
#endif
}

void AZEDPlayerController::UpdateHUDTrackingEnabled_Implementation(bool bSuccess, ESlErrorCode ErrorCode)
{
	if (!bSuccess)
	{
		ZedPawn->ZedLoadingWidget->SetVisibility(false);

		ZedPawn->ZedErrorWidget->SetText(FText::FromString(USlFunctionLibrary::ErrorCodeToString(ErrorCode)));
		ZedPawn->ZedErrorWidget->SetVisibility(true);
	}
	else
	{
#if SHOW_LOADING_TEXT
		ZedPawn->ZedLoadingWidget->SetText(FText::FromString("Tracking enabled"));
#endif
		ZedPawn->ZedLoadingWidget->FadeOut();
	}
}

void AZEDPlayerController::UpdateHUDZedDisconnected_Implementation()
{
	ZedPawn->ZedErrorWidget->SetText(FText::FromString(USlFunctionLibrary::ErrorCodeToString(ESlErrorCode::EC_CameraNotDetected)));
	ZedPawn->ZedErrorWidget->SetVisibility(true);
	ZedPawn->ZedErrorWidget->FadeIn();

	SetWidgetInFrontOfCamera(ZedPawn->ZedErrorWidget);
}

void AZEDPlayerController::AddShowOnlyComponent(UPrimitiveComponent* InComponent)
{
	if (InComponent)
	{
		ShowOnlyPrimitiveComponents.Add(InComponent);
	}
}

void AZEDPlayerController::BuildShowOnlyComponentList(TSet<FPrimitiveComponentId>& ShowOnlyComponentsOut)
{
	for (int32 ComponentIndx = ShowOnlyPrimitiveComponents.Num() - 1; ComponentIndx >= 0; --ComponentIndx)
	{
		TWeakObjectPtr<UPrimitiveComponent> ComponentPtr = ShowOnlyPrimitiveComponents[ComponentIndx];
		if (ComponentPtr.IsValid())
		{
			UPrimitiveComponent* Component = ComponentPtr.Get();
			if (Component->IsRegistered())
			{
				ShowOnlyComponentsOut.Add(Component->ComponentId);
			}
		}
		else
		{
			ShowOnlyPrimitiveComponents.RemoveAt(ComponentIndx);
		}
	}
}