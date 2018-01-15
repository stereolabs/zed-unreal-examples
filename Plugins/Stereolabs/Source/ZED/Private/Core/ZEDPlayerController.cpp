//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

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
#include "Engine/Engine.h"
#include "IHeadMountedDisplay.h"
#include "IXRTrackingSystem.h"

DEFINE_LOG_CATEGORY(ZEDPlayerController);

#define MONO_NOISE_OFFSET 0.85f
#define STEREO_NOISE_OFFSET 0.35f
#define STEREO_BLUR_SIGMA 0.5f

#define ADD_FVECTOR_2D(Vector, Value)\
	Vector.X += Value;\
	Vector.Y += Value;\

#define SHOW_ZED_MESSAGE(Canvas, Font, TextItem, Position, RowHeight)\
	if (Font && Font->ImportOptions.bUseDistanceFieldAlpha)\
	{\
		TextItem.BlendMode = SE_BLEND_MaskedDistanceFieldShadowed;\
	}\
	else\
	{\
		TextItem.EnableShadow(FColor::Black);\
	}\
	Canvas->Canvas->DrawItem(String);\
	Position.Y += RowHeight;\

/** Activate/Deactivate noise */
static TAutoConsoleVariable<int32> CVarZEDNoise(
	TEXT("r.ZED.Noise"),
	0,
	TEXT("1 to enable noise, 0 to disable"),
	ECVF_RenderThreadSafe
);

/** Show ZED FPS */
static TAutoConsoleVariable<int32> CVarZEDShowFPS(
	TEXT("r.ZED.ShowFPS"),
	0,
	TEXT("1 to show, 0 to hide"),
	ECVF_RenderThreadSafe
);

AZEDPlayerController::AZEDPlayerController()
	:
	CurrentCameraFPSTimerBadFPS(0.0f),
	CurrentCameraFPSTimerGoodFPS(0.0f),
	CurrentFPSTimerBadFPS(0.0f),
	CurrentFPSTimerGoodFPS(0.0f),
	bShowLowCameraFPS(false),
	bShowLowAppFPS(false),
	bTickZedCamera(false),
	bUseDefaultBeginPlay(true),
	bOpenZedCameraAtInit(true),
	bStereoRenderingSupport(true),
	bHMDEnabled(false),
	bIsFirstPlayer(false),
	bUseShowOnlyList(false),
	bHideWorldOpeningZedCamera(true),
	bZedCameraDisconnected(false),
	bInit(false),
	CurrentNoiseValue(0),
	ZedPawn(nullptr),
	ZedCamera(nullptr),
	PawnClass(AZEDPawn::StaticClass())
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
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

void AZEDPlayerController::PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir)
{
	Super::PostRenderFor(PC, Canvas, CameraPosition, CameraDir);

	UFont* Font = GEngine->GetLargeFont();

	FLinearColor Color;
	FVector2D Position;

	const int32 RowHeight = FMath::TruncToInt(Font->GetMaxCharHeight() * 1.1f);
	
	if (bHMDEnabled)
	{
		EHMDDeviceType::Type Type = GEngine->XRSystem->GetHMDDevice()->GetHMDDeviceType();

		switch (Type)
		{
		
			case EHMDDeviceType::DT_SteamVR:
				Position = FVector2D(290.00f, 420.0f);
				break;
			case EHMDDeviceType::DT_OculusRift:
			default:
				Position = FVector2D(290.0f, 395.0f);
				break;
		}
	}

	const FVector2D Scale(1.1f, 1.1f);

	if (CVarZEDShowFPS.GetValueOnAnyThread() && GSlCameraProxy->IsCameraOpened())
	{
		Color = ZEDFPS >= 58.0f ? FColor::Green : ZEDFPS >= 45.0f ? FColor::Yellow : FColor::Red;

		{
			FCanvasTextItem String(Position, FText::FromString(FString::Printf(TEXT("%5.2f FPS"), ZEDFPS)), Font, Color);
			String.Scale = Scale;

			if (!bHMDEnabled)
			{
				Position = FVector2D(40.0f, FMath::TruncToInt(GetLocalPlayer()->ViewportClient->Viewport->GetSizeXY().Y * 0.20f));
				String.Position = Position;
			}

			SHOW_ZED_MESSAGE(Canvas, Font, String, Position, RowHeight);
		}

		{
			FCanvasTextItem String(Position, FText::FromString(FString::Printf(TEXT("%5.2f ms"), 1.0f / ZEDFPS * 100.0f)), Font, Color);
			String.Scale = Scale;

			SHOW_ZED_MESSAGE(Canvas, Font, String, Position, RowHeight);
		}
	}

	if (bHMDEnabled && GSlCameraProxy->IsCameraOpened())
	{
		Color = FColor::Green;

		if (GSlCameraProxy->GetCameraFPS() < 60.0f)
		{
			FCanvasTextItem String(Position, FText::FromString(FString("Selected ZED camera FPS is too low : Choose 60 FPS")), Font, Color);
			String.Scale = Scale;

			SHOW_ZED_MESSAGE(Canvas, Font, String, Position, RowHeight);
		}
		else
		{
			if (bShowLowCameraFPS)
			{
				FCanvasTextItem String(Position, FText::FromString(FString("Current ZED camera FPS is too low : Check graphics requirements or switch USB port")), Font, Color);
				String.Scale = Scale;

				SHOW_ZED_MESSAGE(Canvas, Font, String, Position, RowHeight);
			}
		}

		if (bShowLowAppFPS)
		{
			FCanvasTextItem String(Position, FText::FromString(FString("Application FPS is too low : Check graphics requirements")), Font, Color);
			String.Scale = Scale;

			SHOW_ZED_MESSAGE(Canvas, Font, String, Position, RowHeight);
		}
	}
}

void AZEDPlayerController::Tick(float DeltaSeconds)
{
	if (bTickZedCamera)
	{
		ZedCamera->Tick(DeltaSeconds);

		ZEDFPS = GSlCameraProxy->GetCurrentFPS();
		if (ZEDFPS <= 55.0f)
		{
			// Hitch
			if (CurrentCameraFPSTimerGoodFPS < 4.0f && CurrentCameraFPSTimerBadFPS == 2.0f)
			{
				// Reset
				CurrentCameraFPSTimerGoodFPS = 0.0f;
			}
			else
			{
				CurrentCameraFPSTimerBadFPS = FMath::Min(2.0f, CurrentCameraFPSTimerBadFPS + DeltaSeconds);
				if (CurrentCameraFPSTimerBadFPS == 2.0f)
				{
					CurrentCameraFPSTimerGoodFPS = 0.0f;

					bShowLowCameraFPS = true;
				}
			}
		}
		else
		{
			// Hitch
			if (CurrentCameraFPSTimerBadFPS < 2.0f && CurrentCameraFPSTimerGoodFPS == 4.0f)
			{
				// Reset
				CurrentCameraFPSTimerBadFPS = 0.0f;
			}
			else
			{
				CurrentCameraFPSTimerGoodFPS = FMath::Min(4.0f, CurrentCameraFPSTimerGoodFPS + DeltaSeconds);
				if (CurrentCameraFPSTimerGoodFPS == 4.0f)
				{
					CurrentCameraFPSTimerBadFPS = 0.0f;

					bShowLowCameraFPS = false;
				}
			}
		}

		if (1.0f / DeltaSeconds <= 60.0f)
		{
			// Hitch
			if (CurrentFPSTimerGoodFPS < 4.0f && CurrentFPSTimerBadFPS == 2.0f)
			{
				// Reset
				CurrentFPSTimerGoodFPS = 0.0f;
			}
			else
			{
				CurrentFPSTimerBadFPS = FMath::Min(2.0f, CurrentFPSTimerBadFPS + DeltaSeconds);
				if (CurrentFPSTimerBadFPS == 2.0f)
				{
					CurrentFPSTimerGoodFPS = 0.0f;

					bShowLowAppFPS = true;
				}
			}
		}
		else
		{
			// Hitch
			if (CurrentFPSTimerBadFPS < 2.0f && CurrentFPSTimerGoodFPS == 4.0f)
			{
				// Reset
				CurrentFPSTimerBadFPS = 0.0f;
			}
			else
			{
				CurrentFPSTimerGoodFPS = FMath::Min(4.0f, CurrentFPSTimerGoodFPS + DeltaSeconds);
				if (CurrentFPSTimerGoodFPS == 4.0f)
				{
					CurrentFPSTimerBadFPS = 0.0f;

					bShowLowAppFPS = false;
				}
			}
		}
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
					// Dedicated server spawn pawn before begin play
					if (ZedPawn)
					{
						OnPawnSpawned.Broadcast();
					}

					SpawnZedCameraActor();

					// Dedicated server spawn pawn before begin play
					if (ZedPawn)
					{
						Init();
					}
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
	ZedPawn = Cast<AZEDPawn>(GetWorld()->SpawnActor(NewPawnClass));

	checkf(ZedPawn, TEXT("NewPawnClass must inherit from AZedPawn"));

	if (bPossess)
	{
		Possess(ZedPawn);
	}

	OnPawnSpawned.Broadcast();

	return ZedPawn;
}

void AZEDPlayerController::SpawnZedCameraActor()
{
	ZedCamera = GetWorld()->SpawnActor<AZEDCamera>(ZedCameraBlueprintClass);
}

void AZEDPlayerController::Init()
{
	if (bInit)
	{
		return;
	}

	// Attach Zed camera actor to pawn
	ZedCamera->AttachToComponent(ZedPawn->GetRootComponent(), FAttachmentTransformRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, false));

	// Create dynamic post process
	PostProcessFadeMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(PostProcessFadeSourceMaterial, nullptr);
	PostProcessZedMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(PostProcessZedSourceMaterial, nullptr);

	// Bind events to proxy
	GSlCameraProxy->OnCameraOpened.AddDynamic(this, &AZEDPlayerController::ZedCameraOpened);
	GSlCameraProxy->OnCameraDisconnected.AddDynamic(this, &AZEDPlayerController::ZedCameraDisconnected);
	GSlCameraProxy->OnCameraClosed.AddDynamic(this, &AZEDPlayerController::ZedCameraClosed);
	GSlCameraProxy->OnTrackingEnabled.AddDynamic(this, &AZEDPlayerController::ZedCameraTrackingEnabled);

	// Bind event to Zed camera actor
	ZedCamera->OnCameraActorInitialized.AddDynamic(this, &AZEDPlayerController::ZedCameraActorInitialized);

	// Pawn tracking
	ZedCamera->OnTrackingDataUpdated.AddDynamic(ZedPawn, &AZEDPawn::ZedCameraTrackingUpdated);

	// Enable fade post process
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);

	// User init
	InitEvent();

	bInit = true;

	// Open camera next frame
	if (bOpenZedCameraAtInit)
	{
		GetWorldTimerManager().SetTimer(InitTimerHandle, this, &AZEDPlayerController::Internal_Init, 0.001f, false);
	}
}

void AZEDPlayerController::Internal_Init()
{
	OpenZedCamera(bHideWorldOpeningZedCamera);
}

void AZEDPlayerController::CloseZedCamera()
{
	GetWorldTimerManager().ClearTimer(DisableFadePostProcessTimerHandle);
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);

	FadeIn();

	GetWorldTimerManager().SetTimer(CloseZedCameraTimerHandle, this, &AZEDPlayerController::Internal_CloseZedCamera, 1.5f, false);
}

void AZEDPlayerController::Internal_CloseZedCamera()
{
	GSlCameraProxy->CloseCamera();

	ZedCamera->DisableRendering();

	FadeOut();
}

void AZEDPlayerController::OpenZedCamera(bool bHideWorld)
{
	checkf(bInit, TEXT("Init() not called before opening the camera"));

	GetWorldTimerManager().ClearTimer(CameraOpeningTimerHandle);

	ZedPawn->ZedLoadingWidget->SetVisibility(false);
	ZedPawn->ZedErrorWidget->SetVisibility(false);

	if (bHideWorld)
	{
		bUseShowOnlyList = true;
		ShowOnlyPrimitiveComponents.Empty();
		ShowOnlyPrimitiveComponents.Add(ZedPawn->ZedLoadingWidget->WidgetComponent);
		ShowOnlyPrimitiveComponents.Add(ZedPawn->ZedErrorWidget->WidgetComponent);
	}

	// Enable/Disable HMD
	if (bStereoRenderingSupport)
	{
		// Need to enable the HMD
		if (!UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			// HMD connected
			if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayConnected())
			{
				// Enable
				UHeadMountedDisplayFunctionLibrary::EnableHMD(true);
			}
		}

		GetWorldTimerManager().SetTimer(OpenZedCameraTimerHandle, this, &AZEDPlayerController::Internal_OpenZedCamera, 2.0f, false);
	}
	else
	{
#if WITH_EDITOR
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			SL_LOG_E(ZEDPlayerController, "Playing in VR but stereo rendering not supported");
		}
#else
		UHeadMountedDisplayFunctionLibrary::EnableHMD(false);
#endif

		Internal_OpenZedCamera();
	}
}

void AZEDPlayerController::Internal_OpenZedCamera()
{
	// Test if enable succeed
	bHMDEnabled = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();

	if (!bHMDEnabled)
	{
		ZedPawn->ZedLoadingWidget->WidgetComponent->SetGeometryMode(EWidgetGeometryMode::Plane);
		ZedPawn->ZedLoadingWidget->SetWorldScale3D(FVector(0.3f));

		ZedPawn->ZedErrorWidget->WidgetComponent->SetGeometryMode(EWidgetGeometryMode::Plane);
		ZedPawn->ZedErrorWidget->SetWorldScale3D(FVector(0.3f));
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

	UpdateHUDOpeningZed();

	GetWorldTimerManager().SetTimer(CameraOpeningTimerHandle, this, &AZEDPlayerController::UpdateHUDCheckOpeningZed, 1.0f, true, 2.0f);
}

void AZEDPlayerController::ZedCameraOpened()
{
	GetWorldTimerManager().ClearTimer(CameraOpeningTimerHandle);

	// Set fade post process
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);
	// Set camera field of view
	ZedPawn->Camera->SetFieldOfView(GSlCameraProxy->CameraInformation.CalibrationParameters.LeftCameraParameters.HFOV);

	// Init viewport helper
	UGameViewportClient* GameViewport = GetLocalPlayer()->ViewportClient;
	check(GameViewport);

	if (!bHMDEnabled)
	{
		ViewportHelper.AddToViewportResizeEvent(GameViewport);
	}

	ViewportHelper.Update(GameViewport->Viewport->GetSizeXY());

	UpdateHUDCheckOpeningZed();

	// Enable tracking
	if (ZedCamera->TrackingParameters.bEnableTracking)
	{
		UpdateHUDEnablingZedTracking();

		ZedCamera->EnableTracking();
	}
	else
	{
		UpdateHUDZedOpened();

		// Fade to hide zed camera actor init
		FadeIn();

		// Init zed camera actor
		GetWorldTimerManager().SetTimer(InitializeZedCameraActorTimerHandle, this, &AZEDPlayerController::Internal_InitializeZedCameraActor, 1.5f, false);
	}
}

void AZEDPlayerController::ZedCameraTrackingEnabled(bool bSuccess, ESlErrorCode ErrorCode, const FVector& Location, const FRotator& Rotation)
{
	UpdateHUDZedTrackingEnabled(bSuccess, ErrorCode);

	if (bSuccess)
	{
		if (!ZedCamera->bInit)
		{
			// Fade to hide zed camera actor init
			FadeIn();
		}

		if (bHMDEnabled)
		{
			SL_LOG_W(ZEDPlayerController, "You are using an HMD, bind delegate to OnTrackingReset instead of OnTrackingEnabled to get the right tracking origin.");

			// Reset HMD tracking origin. Reset is not immediate that's why the camera actor init 2s after
			GetWorldTimerManager().SetTimer(ResetHMDTrackingOriginTimerHandle, this, &AZEDPlayerController::ResetHMDTrackingOrigin, 1.5f, false);
			// Init zed camera actor
			GetWorldTimerManager().SetTimer(InitializeZedCameraActorTimerHandle, this, &AZEDPlayerController::Internal_InitializeZedCameraActor, 3.5f, false);
		}
		else
		{
			// Init zed camera actor
			GetWorldTimerManager().SetTimer(InitializeZedCameraActorTimerHandle, this, &AZEDPlayerController::Internal_InitializeZedCameraActor, 1.5f, false);
		}
	}
}

void AZEDPlayerController::Internal_InitializeZedCameraActor()
{
	if (!ZedCamera->bInit)
	{
		// Init zed camera actor
		ZedCamera->Init(bHMDEnabled);
	}
	else
	{
		if (bHMDEnabled)
		{
			ZedCamera->InitHMDTrackingData();
		}
	}
}

void AZEDPlayerController::ZedCameraActorInitialized()
{
	// Enable messages display
	GetHUD()->AddPostRenderedActor(this);
	GetHUD()->bShowOverlays = true;

	if (!bHMDEnabled)
	{
		ZedPawn->Camera->AddOrUpdateBlendable(PostProcessZedMaterialInstanceDynamic, 1.0f);

		ZedPawn->Camera->PostProcessSettings.bVirtualObjectsPostProcess = true;
	}
	else
	{
		// Set HMD camera offset
		ZedPawn->SpringArm->SetRelativeLocation(FVector(ZedCamera->HMDCameraOffset, 0.0f, 0.0f));

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

	GetWorldTimerManager().SetTimer(FadeOutTimerHandle, this, &AZEDPlayerController::FadeOutToGame, 1.0f, false);

	bTickZedCamera = true;
}

void AZEDPlayerController::ResetHMDTrackingOrigin()
{
	EHMDDeviceType::Type Type = GEngine->XRSystem->GetHMDDevice()->GetHMDDeviceType();

	// If Oculus reset to eye level or offset in tracking
	if (Type == EHMDDeviceType::DT_OculusRift)
	{
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Eye);
	}
	else
	{
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	}

	// If not using HMD Tracking origin, reset to 0
	if (!ZedCamera->bUseHMDTrackingAsOrigin)
	{
		UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition(0.0f, EOrientPositionSelector::OrientationAndPosition);
	}
	// If using HMD origin but no trackers, reset location
	else if (!UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition())
	{
		FRotator HMDRotation;
		FVector HMDLocation;
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HMDRotation, HMDLocation);

		UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition(HMDRotation.Yaw, EOrientPositionSelector::Position);
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
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);

	FadeTimeline->ReverseFromEnd();
}

void AZEDPlayerController::FadeOut()
{
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);

	FadeTimeline->PlayFromStart();
}

void AZEDPlayerController::DisableFadePostProcess()
{
	// If camera disconnected right before disabling fade post process
	if (!GSlCameraProxy->IsCameraConnected())
	{
		return;
	}
	
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 0.0f);
}

void AZEDPlayerController::Internal_ZedCameraDisconnected()
{
	if (bHMDEnabled)
	{
		ZedPawn->Camera->PostProcessSettings.bDeferredAA = true;
		ZedPawn->Camera->PostProcessSettings.bPostProcessing = true;
		ZedPawn->Camera->CameraRenderingSettings.bLighting = true;
		ZedPawn->Camera->CameraRenderingSettings.bVelocity = true;
	}

	ZedPawn->Camera->PostProcessSettings.bVirtualObjectsPostProcess = false;
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessZedMaterialInstanceDynamic, 0.0f);

	ZedCamera->DisableRendering();

	UpdateHUDZedDisconnected();

	bUseShowOnlyList = true;
	ShowOnlyPrimitiveComponents.Empty();
	ShowOnlyPrimitiveComponents.Add(ZedPawn->ZedErrorWidget->WidgetComponent);

	FadeOut();
}

void AZEDPlayerController::ZedCameraDisconnected()
{
	GetWorldTimerManager().ClearTimer(DisableFadePostProcessTimerHandle);
	ZedPawn->Camera->AddOrUpdateBlendable(PostProcessFadeMaterialInstanceDynamic, 1.0f);

	FadeIn();

	bZedCameraDisconnected = true;

	GSlCameraProxy->CloseCamera();

	GetWorldTimerManager().SetTimer(CameraDisconnectedTimerHandle, this, &AZEDPlayerController::Internal_ZedCameraDisconnected, 1.5f, false);
}

void AZEDPlayerController::ZedCameraClosed()
{
	bTickZedCamera = false;

	if (!bZedCameraDisconnected)
	{
		if (bHMDEnabled)
		{
			ZedPawn->Camera->PostProcessSettings.bDeferredAA = true;
			ZedPawn->Camera->PostProcessSettings.bPostProcessing = true;
			ZedPawn->Camera->CameraRenderingSettings.bLighting = true;
			ZedPawn->Camera->CameraRenderingSettings.bVelocity = true;

			ConsoleCommand(TEXT("vr.SpectatorScreenMode 3"), true);
		}

		ZedPawn->Camera->PostProcessSettings.bVirtualObjectsPostProcess = false;
		ZedPawn->Camera->AddOrUpdateBlendable(PostProcessZedMaterialInstanceDynamic, 0.0f);
	}
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

void AZEDPlayerController::FadeOutToGame()
{
	FadeOut();

	GetWorldTimerManager().SetTimer(DisableFadePostProcessTimerHandle, this, &AZEDPlayerController::DisableFadePostProcess, 2.0f, false);
}

void AZEDPlayerController::UpdateHUDOpeningZed_Implementation()
{
#if WITH_EDITOR
	ZedPawn->ZedLoadingWidget->SetText(FText::FromString("Searching for ZED camera"));
#endif
	ZedPawn->ZedLoadingWidget->SetVisibility(true);
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
#if WITH_EDITOR
		ZedPawn->ZedLoadingWidget->SetText(FText::FromString("Opening camera"));
#endif
	}
}

void AZEDPlayerController::UpdateHUDZedOpened_Implementation()
{
#if WITH_EDITOR
	ZedPawn->ZedLoadingWidget->SetText(FText::FromString("Camera opened"));
#endif
	ZedPawn->ZedLoadingWidget->FadeOut();
}

void AZEDPlayerController::UpdateHUDEnablingZedTracking_Implementation()
{
#if WITH_EDITOR
	ZedPawn->ZedLoadingWidget->SetText(FText::FromString("Enabling tracking"));
#endif
}

void AZEDPlayerController::UpdateHUDZedTrackingEnabled_Implementation(bool bSuccess, ESlErrorCode ErrorCode)
{
	if (!bSuccess)
	{
		ZedPawn->ZedLoadingWidget->SetVisibility(false);

		ZedPawn->ZedErrorWidget->SetText(FText::FromString(USlFunctionLibrary::ErrorCodeToString(ErrorCode)));
		ZedPawn->ZedErrorWidget->SetVisibility(true);
	}
	else
	{
#if WITH_EDITOR
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