//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#pragma once

#include "Stereolabs/Public/Core/StereolabsBaseTypes.h"
#include "ZED/Public/Core/ZEDBaseTypes.h"

#include "ZEDInitializer.generated.h"


/*
 * Actor used to initialize Zed camera using configuration file
 */
UCLASS(Category = "Stereolabs|Zed")
class ZED_API AZEDInitializer : public AActor
{
	GENERATED_BODY()

public:
	AZEDInitializer();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const UProperty* InProperty) const override;
#endif

public:
	/* 
	 * Load all parameters and settings
	 */
	UFUNCTION(BlueprintCallable, Category="Zed")
	void LoadParametersAndSettings();

	/*
	 * Load config parameters 
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed")
	void LoadParameters();

	/* 
	 * Load camera settings 
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed")
	void LoadCameraSettings();

	/* 
	 * Load config parameters 
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed")
	void SaveParameters();

	/*
	 * Load camera settings 
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed")
	void SaveCameraSettings();

	/*
	 * Reset parameters
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed")
	void ResetParameters();

	/*
	 * Reset camera settings
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed")
	void ResetSettings();

private:
	/*
	 * Load anti drift parameters
	 */
	void LoadAntiDriftParameters();

public:
	/** Init parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	FSlInitParameters InitParameters;

	/** Tracking parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	FSlTrackingParameters TrackingParameters;

	/** Init parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	FSlSVOParameters SVOParameters;

	/** Runtime parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	FSlRuntimeParameters RuntimeParameters;

	/** Rendering parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	FSlRenderingParameters RenderingParameters;

	/** Camera settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	FSlCameraSettings CameraSettings;

	/** Actors that will be attached to the pawn at startup. Actor's Transform will be local, and body weld. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	TArray<AActor*> ChildActors;

	/** Load parameters at runtime from config file and override preset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	uint8 bLoadParametersFromConfigFile:1;

	/** Load camera settings at runtime from config file and override preset  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	uint8 bLoadCameraSettingsFromConfigFile:1;

	/*
	 * Use the HMD transform as tracking origin, else the HMD tracking origin is reset.
	 * Enable if the player must match his real world location/rotation. 
	 * Not loaded from config file
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	uint8 bUseHMDTrackingAsOrigin:1;

public:
	/** Calibration parameters */
	FSlAntiDriftParameters AntiDriftParameters;

	/** Parameters for external view */
	FZEDExternalViewParameters ExternalViewParameters;
};