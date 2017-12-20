// Fill out your copyright notice in the Description page of Project Settings.

#include "ZEDPrivatePCH.h"
#include "ZED/Public/Core/ZEDPawn.h"
#include "ZED/Public/Core/ZEDPlayerController.h"
#include "UMG.h"

AZEDPawn::AZEDPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	SpringArm = CreateDefaultSubobject<USceneComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	Camera->SetupAttachment(SpringArm);
	Camera->bConstrainAspectRatio = true;
	Camera->PostProcessSettings.VignetteIntensity = 0.0f;
	Camera->PostProcessSettings.bOverride_VignetteIntensity = true;

	// Widget material
	static ConstructorHelpers::FObjectFinder<UMaterial> ZedWidgetMaterial(TEXT("Material'/Stereolabs/ZED/Materials/M_ZED_3DWidgetPassthroughNoDepth.M_ZED_3DWidgetPassthroughNoDepth'"));
	ZedWidgetSourceMaterial = ZedWidgetMaterial.Object;

	// Zed loading source widget
	static ConstructorHelpers::FObjectFinder<UClass> ZedLoadingWidgetBlueprint(TEXT("'/Stereolabs/ZED/Blueprints/HUD/Loading/W_ZED_Loading.W_ZED_Loading_C'"));
	ZedLoadingSourceWidget = ZedLoadingWidgetBlueprint.Object;

	// Zed error source widget
	static ConstructorHelpers::FObjectFinder<UClass> ZedErrorWidgetBlueprint(TEXT("'/Stereolabs/ZED/Blueprints/HUD/Error/W_ZED_Error.W_ZED_Error_C'"));
	ZedErrorSourceWidget = ZedErrorWidgetBlueprint.Object;

	// Zed loading widget
	ZedLoadingWidget = CreateDefaultSubobject<UZEDWidget>(TEXT("LoadingMessage"));
	ZedLoadingWidget->SetupAttachment(SpringArm);
	ZedLoadingWidget->SetWorldScale3D(FVector(0.5f));
	ZedLoadingWidget->WidgetComponent->SetMaterial(0, ZedWidgetSourceMaterial);
	ZedLoadingWidget->WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	ZedLoadingWidget->WidgetComponent->SetWidgetClass(ZedLoadingSourceWidget);
	ZedLoadingWidget->WidgetComponent->SetDrawSize(FVector2D(1920, 1080));
	ZedLoadingWidget->WidgetComponent->SetGeometryMode(EWidgetGeometryMode::Cylinder);
	ZedLoadingWidget->WidgetComponent->SetCylinderArcAngle(80.0f);
	ZedLoadingWidget->WidgetComponent->SetBlendMode(EWidgetBlendMode::Transparent);
	ZedLoadingWidget->SetVisibility(false);

	// Zed error widget
	ZedErrorWidget = CreateDefaultSubobject<UZEDWidget>(TEXT("ErrorMessage"));
	ZedErrorWidget->SetupAttachment(SpringArm);
	ZedErrorWidget->SetWorldScale3D(FVector(0.5f));
	ZedErrorWidget->WidgetComponent->SetMaterial(0, ZedWidgetSourceMaterial);
	ZedErrorWidget->WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	ZedErrorWidget->WidgetComponent->SetWidgetClass(ZedErrorSourceWidget);
	ZedErrorWidget->WidgetComponent->SetDrawSize(FVector2D(1920, 1080));
	ZedErrorWidget->WidgetComponent->SetGeometryMode(EWidgetGeometryMode::Cylinder);
	ZedErrorWidget->WidgetComponent->SetCylinderArcAngle(80.0f);
	ZedErrorWidget->WidgetComponent->SetBlendMode(EWidgetBlendMode::Transparent);
	ZedErrorWidget->SetVisibility(false);

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

AZEDPawn::~AZEDPawn()
{
}

void AZEDPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AZEDPawn::ZedCameraTrackingUpdated(const FZEDTrackingData& NewTrackingData)
{
	SetActorTransform(NewTrackingData.OffsetZedWorldTransform);
}