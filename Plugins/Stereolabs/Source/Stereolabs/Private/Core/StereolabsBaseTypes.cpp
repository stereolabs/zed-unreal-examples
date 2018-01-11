//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "StereolabsPrivatePCH.h"
#include "Stereolabs/Public/Core/StereolabsBaseTypes.h"
#include "Stereolabs/Public/Core/StereolabsCoreGlobals.h"

FSlSpatialMappingParameters::FSlSpatialMappingParameters()
	:
	MaxMemoryUsage(2048),
	PresetResolution(ESlSpatialMappingResolution::SMR_VeryLow),
	PresetRange(ESlSpatialMappingRange::SMR_Medium),
	bSaveTexture(false)/*,
	bUseChunkOnly(bUseChunkOnly)*/
{
	MaxRange = sl::SpatialMappingParameters::get(sl::unreal::ToSlType(PresetRange));

	if (PresetResolution == ESlSpatialMappingResolution::SMR_VeryLow)
	{
		sl::SpatialMappingParameters Dummy;

		Resolution = Dummy.allowed_resolution.second;
	}
	else
	{
		Resolution = sl::SpatialMappingParameters::get(sl::unreal::ToSlType(PresetResolution));
	}
}

void FSlSpatialMappingParameters::SetMaxRange(ESlSpatialMappingRange NewPresetRange)
{
	if (NewPresetRange == ESlSpatialMappingRange::SMR_Custom)
	{
		return;
	}

	PresetRange = NewPresetRange;
	MaxRange = sl::SpatialMappingParameters::get(sl::unreal::ToSlType(PresetRange));
}

void FSlSpatialMappingParameters::SetResolution(ESlSpatialMappingResolution NewPresetResolution)
{
	if (NewPresetResolution == ESlSpatialMappingResolution::SMR_Custom)
	{
		return;
	}

	PresetResolution = NewPresetResolution;
	if (PresetResolution == ESlSpatialMappingResolution::SMR_VeryLow)
	{
		sl::SpatialMappingParameters Dummy;

		Resolution = 0.1f;
	}
	else
	{
		Resolution = sl::SpatialMappingParameters::get(sl::unreal::ToSlType(PresetResolution));
	}
}