// Fill out your copyright notice in the Description page of Project Settings.

#include "SpectatorCalibrationLoader.h"


#include<iostream>
#include<fstream>
#include<string>


#if WITH_EDITOR
#define ZED_CONFIG_FILE_PATH			FPaths::Combine(*FPaths::ProjectDir(), *FString("Saved/Config/ZED/ZED_Position_Offset.conf"))
#else
#define ZED_CONFIG_FILE_PATH			FPaths::Combine(*FPaths::ConvertRelativePathToFull("../../"), *FString("Saved/Config/ZED/ZED_Position_Offset.conf"))
#endif

// Sets default values for this component's properties
USpectatorCalibrationLoader::USpectatorCalibrationLoader()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USpectatorCalibrationLoader::BeginPlay()
{
	std::ifstream myFile(std::string(TCHAR_TO_UTF8(*ZED_CONFIG_FILE_PATH)));
	if (myFile.is_open())
	{
		std::string tmp;
		myFile >> tmp;
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "x=%f", &x);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "y=%f", &y);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "z=%f", &z);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "rx=%f", &rx);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "ry=%f", &ry);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "rz=%f", &rz);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "rw=%f", &rw);

		isValid = true;

		myFile.close();
	}

	Super::BeginPlay();
	
}


// Called every frame
void USpectatorCalibrationLoader::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


FRotator USpectatorCalibrationLoader::ConvertToRotator()
{
	FQuat tempQuat = FQuat(rx, ry, rz, rw);
	FVector axis;
	float angle;
	tempQuat.ToAxisAndAngle(axis, angle);
	FVector axisUnreal(axis.Z, axis.X, axis.Y); // From unity coordinate system
	return FRotator(FQuat(axisUnreal, angle));
}
