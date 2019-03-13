// Fill out your copyright notice in the Description page of Project Settings.

#include "GameParameterLoader.h"

#include<iostream>
#include<fstream>
#include<string>

#if WITH_EDITOR
#define PINGPONG_PARAMETER_PATH			FPaths::Combine(*FPaths::ProjectDir(), *FString("Saved/Config/ZED/GameParameter.conf"))
#else
#define PINGPONG_PARAMETER_PATH			FPaths::Combine(*FPaths::ConvertRelativePathToFull("../../"), *FString("Saved/Config/ZED/GameParameter.conf"))
#endif

// Sets default values for this component's properties
UGameParameterLoader::UGameParameterLoader()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGameParameterLoader::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGameParameterLoader::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UGameParameterLoader::LoadParameters()
{
	isValid = false;

	std::ifstream myFile(std::string(TCHAR_TO_UTF8(*PINGPONG_PARAMETER_PATH)));
	if (myFile.is_open())
	{
		std::string tmp;
		myFile >> tmp;
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "ballNumberLimit=%d", &gameParameters.ballNumberLimit);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "globalTimeDilation=%f", &gameParameters.globalTimeDilation);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "linearDampingSlope=%f", &gameParameters.linearDampingSlope);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "ballWeigth=%f", &gameParameters.ballWeigth);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "ballMaxSpeed=%f", &gameParameters.ballMaxSpeed);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "magnusSlope=%f", &gameParameters.magnusSlope);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "raquetteServerMeshScale=%f", &gameParameters.raquetteServerMeshScale);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "raquetteRestitution=%f", &gameParameters.raquetteRestitution);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "raquetteFriction=%f", &gameParameters.raquetteFriction);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "ballRestitution=%f", &gameParameters.ballRestitution);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "ballFriction=%f", &gameParameters.ballFriction);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "tableRestitution=%f", &gameParameters.tableRestitution);
		myFile >> tmp;
		std::sscanf(tmp.c_str(), "tableFriction=%f", &gameParameters.tableFriction);

		isValid = true;

		myFile.close();
	}
}

UPhysicalMaterial* UGameParameterLoader::ApplyPhysicalMaterialParameter(UPhysicalMaterial* physicalMaterial, const float& friction, const float& restitution)
{
	physicalMaterial->Friction = friction;
	physicalMaterial->Restitution = restitution;
	return physicalMaterial;
}