//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#pragma once

#include "ModuleManager.h"

class FStereolabs : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};