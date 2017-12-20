// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "StereolabsEditorPrivatePCH.h"
#include "StereolabsEditor/Public/ZED/ZEDInitializerDetails.h"
#include "StereolabsEditor/Public/ZED/ZEDCameraDetails.h"
#include "StereolabsEditor/Public/SpatialMapping/SpatialMappingManagerDetails.h"
#include "ZED/Public/Core/ZEDInitializer.h"
#include "ZED/Public/Core/ZEDCamera.h"

#define LOCTEXT_NAMESPACE "FStereolabsEditor"

void FStereolabsEditor::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		
	PropertyModule.RegisterCustomClassLayout(AZEDInitializer::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FZEDInitializerDetails::MakeInstance));

	PropertyModule.RegisterCustomClassLayout(AZEDCamera::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FZEDCameraDetails::MakeInstance));

	PropertyModule.RegisterCustomClassLayout(ASpatialMappingManager::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FSpatialMappingManagerDetails::MakeInstance));
}

void FStereolabsEditor::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout(AZEDCamera::StaticClass()->GetFName());
	PropertyModule.UnregisterCustomClassLayout(AZEDInitializer::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStereolabsEditor, StereolabsEditor)