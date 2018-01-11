///======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "ZEDPrivatePCH.h"
#include "ZED/Public/Core/ZEDCoreGlobals.h"

FZEDTrackingData GZedTrackingData = FZEDTrackingData();

FRotator GZedRawRotation = FRotator::ZeroRotator;

FVector GZedRawLocation = FVector::ZeroVector;

FRotator GZedViewPointRotation = FRotator::ZeroRotator;

FVector GZedViewPointLocation = FVector::ZeroVector;