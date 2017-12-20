// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MotionControllerComponent.h"
#include "DevicesMotionController.generated.h"

/*
 *	Actor which can provide a MotionController transform with a delay in millisecond. AActor is needed for timer.
 */
UCLASS(Category = "Stereolabs|Controllers")
class DEVICES_API ADevicesMotionController : public AActor
{
	GENERATED_BODY()

public:
	ADevicesMotionController();
	~ADevicesMotionController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

private:
	UFUNCTION()
	void Start();

	UFUNCTION()
	void Stop();

private:
	/*
	 * Get the motion controller transform delayed of "LatencyTime" milliseconds 
	 * @return ControllerDelayedTransform The transform
	 */
	FTransform GetDelayedTransform();

	/** Function called by the timer. It add a new motion controller transform to the buffer and remove the old one if we have enough */
	void UpdateTransformBuffer();

	/** */
	FTransform GetTransform();

public:
	/** MotionController */
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	USceneComponent* DefaultSceneRoot;

	/** MotionController */
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UMotionControllerComponent* MotionController;

private:
	/** Latency time add to controller pose (in ms) */
	int32 LatencyTime;

	/** Buffer containing previous controller transforms (one transform each frame) */
	TArray<FTransform, TFixedAllocator<26>> TransformBuffer;

	/** Update transform timer handle */
	FTimerHandle TimerHandle;
};
