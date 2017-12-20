// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SamplesBlueprintFunctionLibrary.generated.h"

UENUM(BlueprintType)
enum class EHMDType : uint8
{
	T_Oculus  UMETA(DisplayName = "Oculus"),
	T_Vive	  UMETA(DisplayName = "Vive"),
	T_Unknown UMETA(DisplayName = "Unknown")
};

/**
 * 
 */
UCLASS()
class USamplesBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	UFUNCTION(BlueprintPure)
	static EHMDType GetHMDType();

};