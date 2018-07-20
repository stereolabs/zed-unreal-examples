// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpectatorCalibrationLoader.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PINGPONGMR_API USpectatorCalibrationLoader : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USpectatorCalibrationLoader();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	UPROPERTY(BlueprintReadWrite, Category = "Spectator|CalibrationViveTracker")
	float x = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Spectator|CalibrationViveTracker")
	float y = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Spectator|CalibrationViveTracker")
	float z = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Spectator|CalibrationViveTracker")
	float rx = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Spectator|CalibrationViveTracker")
	float ry = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Spectator|CalibrationViveTracker")
	float rz = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Spectator|CalibrationViveTracker")
	float rw = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Spectator|CalibrationViveTracker")
	bool isValid = false;

	UFUNCTION(BlueprintCallable, Category = "Spectator")
	FRotator ConvertToRotator();
		
	
};
