//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#pragma once

#include "Zed/Public/HUD/ZEDWidget.h"
#include "ZED/Public/Core/ZEDCamera.h"
#include "ZED/Public/Core/ZEDBaseTypes.h"

#include "ZEDPawn.generated.h"

/*
 * Base class for pawn using the Zed.
 * Inherit from this class and set the PawnClassType variable in the controller to spawn a pawn of your type.
 */
UCLASS(Category = "Stereolabs|Zed")
class ZED_API AZEDPawn : public APawn
{
	friend class AZEDPlayerController;

	GENERATED_BODY()

public:
	AZEDPawn();

private:
	/*
	 * Event binded to OnTrackingDataUpdated
	 * @param NewTrackingData The new tracking data
	 */
	UFUNCTION()
	void ZedCameraTrackingUpdated(const FZEDTrackingData& NewTrackingData);

public:
	/** Custom spring arm that offset the camera */
	UPROPERTY()
	USceneComponent* SpringArm;

	/** Main camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCameraComponent* Camera;

	/** Zed loading widget */
	UPROPERTY()
	UZEDWidget* ZedLoadingWidget;

	/** Zed error widget */
	UPROPERTY()
	UZEDWidget* ZedErrorWidget;

private:
	/** Zed loading source widget */
	UPROPERTY()
	UClass* ZedLoadingSourceWidget;

	/** Zed error source widget */
	UPROPERTY()
	UClass* ZedErrorSourceWidget;

	/** Zed widget material */
	UPROPERTY()
	UMaterial* ZedWidgetSourceMaterial;
};
