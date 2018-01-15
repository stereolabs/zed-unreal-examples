//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#pragma once

#include "ZED/Public/Core/ZEDBaseTypes.h"
#include "ZED/Public/Core/ZEDInitializer.h"
#include "Stereolabs/Public/Core/StereolabsTexture.h"
#include "Stereolabs/Public/Core/StereolabsTextureBatch.h"

#include <sl_mr_core/defines.hpp>

#include "ZEDCamera.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(ZEDCamera, Log, All);

/*
 * Notify that the tracking data have been updated
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZEDTrackingDataUpdatedDelegate, const FZEDTrackingData&, NewTrackingData);

/*
 * Notify that the actor is initialized
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZEDCameraActorInitializedDelegate);

/*
 * Engine representation of the ZED. Spawnable in a level
 */
UCLASS(Category = "Stereolabs|Zed")
class ZED_API AZEDCamera : public AActor
{
	friend class AZEDPlayerController;

	GENERATED_BODY()

public:	
	AZEDCamera();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick( float DeltaSeconds ) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const UProperty* InProperty) const override;
#endif

public:
	/*
	 * Enable tracking using current tracking parameters
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed|Tracking")
	void EnableTracking();

	/*
	 * Disable tracking
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed|Tracking")
	void DisableTracking();

	/*
	 * Reset the tracking origin of the camera.
	 * If using an HMD, reset with its current transform.
	 * Else use the current tracking data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed|Tracking")
	void ResetTrackingOrigin();

	/*
	 * Save the tracking area using current tracking parameters path
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed|Tracking")
	void SaveSpatialMemoryArea();

	/*
	 * Set the threading mode and enable/disable the grab thread
	 * @param NewValue The new threading mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed|Rendering")
	void SetThreadingMode(ESlThreadingMode NewValue);

	/*
	 * Set the runtime parameters. Take effect next grab
	 * @param NewValue The news runtime parameters
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed|Runtime")
	void SetRuntimeParameters(const FSlRuntimeParameters& NewValue);

	/*
	 * Set the camera settings. Take effect next grab 
	 * @param NewValue The news camera settings
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed|Camera")
	void SetCameraSettings(const FSlCameraSettings& NewValue);

	/*
	 * Initialize cameras and planes
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Zed|Event")
	void InitializeRendering();

	/*
	 * Set the post process of the cameras
	 * @param NewPostProcess The post process to add
	 * @param NewWeight		 The weight of the post process
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Zed|Rendering")
	void AddOrUpdatePostProcess(UMaterialInterface* NewPostProcess, float NewWeight);

	/*
	 * Correct planes drift
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Zed|AntiDrift")
	void CorrectHMDPlanesDrift();

	/*
	 * Update planes size based on perception distance
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Zed|AntiDrift")
	void UpdatePlanesSize();
	
	/*
	 * Update HMD planes rotation
	 * @param Rotation The new rotation
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Zed|Event")
	void SetHMDPlanesRotation(const FRotator& Rotation);

	/*
	 * Set HMD render planes location
	 * @param NewLocation The new location
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Zed|AntiDrift")
	void SetHMDPlanesLocation(const FVector& NewLocation);
	
	/*
	 * Hide render planes
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Zed|Rendering")
	void DisableRendering();

	/*
	 * Enable recording using Init and SVO parameters
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed|SVO")
	ESlErrorCode EnableSVORecording();

	/*
	 * Disable SVO recording
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed|SVO")
	void DisableSVORecording();

	/*
	 * Enable/Disable SVO playback looping
	 * @param bLooping True to loop
	 */
	UFUNCTION(BlueprintCallable, Category = "Zed|SVO")
	void SetVOPlaybackLooping(bool bLooping);

	/*
	 * Initialize parameters
	 * @param ZedInitializer The initializer
	 * @param bHMDEnabled	 True if HMD enabled
	 */
	void InitializeParameters(AZEDInitializer* ZedInitializer, bool bHMDEnabled);

	/*
	 * Initialize actor
	 * @param bHMDEnabled True if the HMD is enabled
	 */
	void Init(bool bHMDEnabled);

	/*
	 * Initialize tracking data
	 */
	void InitHMDTrackingData();

	// ------------------------------------------------------------------

private:
	/*
	 * Callback function for the grab delegate
	 * @param ErrorCode Grab error code
	 * @param Timestamp Image timestamp
	 */
	UFUNCTION()
	void GrabCallback(ESlErrorCode ErrorCode, const FSlTimestamp& Timestamp);

	/*
	 * Zed closed
	 */
	UFUNCTION()
	void CameraClosed();

	// ------------------------------------------------------------------

private:
	/*
	 * Create left eye textures
	 *
	 * @param bCeateColorTexture True to create color texture
	 */
	void CreateLeftTextures(bool bCreateColorTexture = true);

	/*
	 * Create right eye textures
	 *
	 * @param bCeateColorTexture True to create color texture
	 */
	void CreateRightTextures(bool bCreateColorTexture = true);

	/*
	 * Initialize the drift corrector
	 */
	bool InitializeDriftCorrectorConstOffset(const FVector& HMDLocation, const FRotator& HMDRotation);

	// ------------------------------------------------------------------

public:
	/** Tracking data dispatcher */
	UPROPERTY(BlueprintAssignable, Category = "Zed|Tracking")
	FZEDTrackingDataUpdatedDelegate OnTrackingDataUpdated;

	/** Actor initialized */
	UPROPERTY(BlueprintAssignable, Category = "Zed|Camera")
	FZEDCameraActorInitializedDelegate OnCameraActorInitialized;

	// ------------------------------------------------------------------

	/** Anti drift parameters */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Tracking")
	FSlAntiDriftParameters AntiDriftParameters;

	// ------------------------------------------------------------------

	/** Left eye image texture */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Textures")
	USlTexture* LeftEyeColor;

	/** Left eye depth texture  */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Textures")
	USlTexture* LeftEyeDepth;

	/** Left eye normals texture */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Textures")
	USlTexture* LeftEyeNormals;

	/** Right eye image texture */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Textures")
	USlTexture* RightEyeColor;

	/** Right eye depth texture  */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Textures")
	USlTexture* RightEyeDepth;

	/** Right eye normals texture  */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Textures")
	USlTexture* RightEyeNormals;

	/** Render target left eye */
	UPROPERTY(BlueprintReadWrite, Category = "Zed|Textures")
	UTextureRenderTarget2D* LeftEyeRenderTarget;
	
	/** Render target right eye */
	UPROPERTY(BlueprintReadWrite, Category = "Zed|Textures")
	UTextureRenderTarget2D* RightEyeRenderTarget;

	// ------------------------------------------------------------------

	/** Runtime parameters */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zed")
	FSlRuntimeParameters RuntimeParameters;

	/** Camera settings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zed")
	FSlCameraSettings CameraSettings;

	// ------------------------------------------------------------------

	/** Config rendering parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	FSlRenderingParameters RenderingParameters;

	/**  Render distance of the ZED planes */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Rendering")
	float CameraRenderPlaneDistance;

	/** Render distance of the HMD planes */
	UPROPERTY(BlueprintReadOnly,  Category = "Zed|Rendering")
	float HMDRenderPlaneDistance;

	/** Offset of the HMD camera */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Rendering")
	float HMDCameraOffset;

	/** Rendering mode */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Rendering")
	ESlRenderingMode RenderingMode;

	// ------------------------------------------------------------------

	/** The current tracking data */
	UPROPERTY(BlueprintReadOnly, Category = "Zed|Tracking")
	FZEDTrackingData TrackingData;

	/** Tracking parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	FSlTrackingParameters TrackingParameters;

	// ------------------------------------------------------------------

	/** Dynamic left Zed eye material */
	UPROPERTY(BlueprintReadWrite, Category = "Zed|Rendering")
	UMaterialInstanceDynamic* ZedLeftEyeMaterialInstanceDynamic;

	/** Dynamic right Zed eye material */
	UPROPERTY(BlueprintReadWrite, Category = "Zed|Rendering")
	UMaterialInstanceDynamic* ZedRightEyeMaterialInstanceDynamic;

	/** Dynamic HMD left eye material */
	UPROPERTY(BlueprintReadWrite, Category = "Zed|Rendering")
	UMaterialInstanceDynamic* HMDLeftEyeMaterialInstanceDynamic;

	/** Dynamic HMD right eye material */
	UPROPERTY(BlueprintReadWrite, Category = "Zed|Rendering")
	UMaterialInstanceDynamic* HMDRightEyeMaterialInstanceDynamic;

	// ------------------------------------------------------------------

	/** SVO parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	FSlSVOParameters SVOParameters;

	// ------------------------------------------------------------------

	/** True if HMD transform is used as tracking origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zed")
	uint8 bUseHMDTrackingAsOrigin:1;

private:
	/** The tracking origin used if HMD enabled */
	FTransform TrackingOriginFromHMD;

	/** Current batch */
	UPROPERTY()
	USlTextureBatch* Batch;

	/** Init parameters */
	FSlInitParameters InitParameters;

	/** Zed material resource */
	UPROPERTY()
	UMaterial* ZedSourceMaterial;
	
	/** HMD left eye material resource */
	UPROPERTY()
	UMaterial* HMDLeftEyeSourceMaterial;
	
	/** HMD right eye material resource */
	UPROPERTY()
	UMaterial* HMDRightEyeSourceMaterial;

	/** The tracking data of the current grab frame */
	FZEDTrackingData CurrentFrameTrackingData;

	/** The grab delegate handle */
	FDelegateHandle GrabDelegateHandle;

	/** Update section if grab is threaded */
	FCriticalSection TrackingUpdateSection;

	/** Plane anti drift timer handle */
	FTimerHandle PlanesAntiDriftTimerHandle;

	/** Current depth texture quality preset */
	int32 CurrentDepthTextureQualityPreset;

	/** True if pass through is enabled */
	uint8 bPassThrough:1;

	/** True if positional tracking initialized from HMD positional tracking */
	uint8 bPositionalTrackingInitialized:1;

	/** True if HMD has trackers */
	uint8 bHMDHasTrackers:1;

	/** True if depth enabled */
	uint8 bCurrentDepthEnabled:1;

	/** True if initialized */
	uint8 bInit:1;
};
