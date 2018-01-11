//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "StereolabsPrivatePCH.h"

#include "Stereolabs/Public/Threading/StereolabsRunnable.h"

FSlRunnable::FSlRunnable()
	:
	Thread(nullptr),
	bIsRunning(false),
	bIsPaused(false),
	bIsSleeping(false)
{
}

FSlRunnable::~FSlRunnable()
{
	delete Thread;
	Thread = nullptr;
}