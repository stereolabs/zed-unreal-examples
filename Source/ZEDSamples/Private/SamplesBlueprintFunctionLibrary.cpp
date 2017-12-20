// Fill out your copyright notice in the Description page of Project Settings.

#include "SamplesBlueprintFunctionLibrary.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "IHeadMountedDisplay.h"
#include "IXRTrackingSystem.h"

EHMDType USamplesBlueprintFunctionLibrary::GetHMDType()
{
	if (GEngine->XRSystem.IsValid() && GEngine->XRSystem->GetHMDDevice())
	{
		if (GEngine->StereoRenderingDevice.IsValid())
		{
			EHMDDeviceType::Type Type = GEngine->XRSystem->GetHMDDevice()->GetHMDDeviceType();

			switch (Type)
			{
				case EHMDDeviceType::DT_OculusRift:
					return EHMDType::T_Oculus;
				case EHMDDeviceType::DT_SteamVR:
					return EHMDType::T_Vive;
			}
		}
	}

	return EHMDType::T_Unknown;
}