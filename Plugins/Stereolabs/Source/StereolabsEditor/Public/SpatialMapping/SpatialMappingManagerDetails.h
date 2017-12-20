#pragma once

#include "SpatialMapping/Public/Core/SpatialMappingManager.h"
#include "Stereolabs/Public/Core/StereolabsCameraProxy.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"


DECLARE_LOG_CATEGORY_EXTERN(SpatialMappingManagerDetails, Log, All);

class FSpatialMappingManagerDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	/** Button enabled */
	bool IsSpatialMappingEnabled() const 
	{ 
		ASpatialMappingManager* SpatialMappingManager = static_cast<ASpatialMappingManager*>(SelectedObjects[0].Get());
	
		return SpatialMappingManager->IsSpatialMappingEnabled()/* && GSlCameraProxy->bSpatialMappingEnabled*/;
	}

	/** Button enabled */
	bool IsSpatialMappingDisabled() const { return !IsSpatialMappingEnabled(); }

	/** Button enabled */
	bool IsSpatialMappingStarted() const { return IsSpatialMappingEnabled() && GSlCameraProxy &&  GSlCameraProxy->bSpatialMappingEnabled; }

	/** Button enabled */
	bool IsSpatialMappingPaused() const
	{ 
		ASpatialMappingManager* SpatialMappingManager = static_cast<ASpatialMappingManager*>(SelectedObjects[0].Get());

		return IsSpatialMappingStarted() && SpatialMappingManager->IsSpatialMappingPaused();
	}

	/** Button enabled */
	bool IsSpatialMappingNotPaused() const
	{
		ASpatialMappingManager* SpatialMappingManager = static_cast<ASpatialMappingManager*>(SelectedObjects[0].Get());

		return IsSpatialMappingStarted() && !SpatialMappingManager->IsSpatialMappingPaused();
	}

	/** Button enabled */
	bool IsSpatialMappingStopped() const { return IsSpatialMappingEnabled() && GSlCameraProxy && !GSlCameraProxy->bSpatialMappingEnabled; }

	/** Button enabled */
	bool HasMeshVertices() const 
	{
		ASpatialMappingManager* SpatialMappingManager = static_cast<ASpatialMappingManager*>(SelectedObjects[0].Get());

		return IsSpatialMappingEnabled() && SpatialMappingManager->GetMeshVerticesNumber() > 0;
	}

	/** Button enabled */
	bool IsTexturingEnabled() const
	{
		ASpatialMappingManager* SpatialMappingManager = static_cast<ASpatialMappingManager*>(SelectedObjects[0].Get());

		return IsSpatialMappingEnabled() && HasMeshVertices() && SpatialMappingManager->SpatialMappingParameters.bSaveTexture;
	}


	/** Clicking the enable SpatialMapping button */
	FReply OnClickEnableSpatialMapping();

	/** Clicking the disable SpatialMapping button */
	FReply OnClickDisableSpatialMapping();

	/** Clicking the reset SpatialMapping button */
	FReply OnClickResetSpatialMapping();

	/** Clicking the start SpatialMapping button */
	FReply OnClickStartSpatialMapping();

	/** Clicking the pause SpatialMapping button */
	FReply OnClickPauseSpatialMapping();

	/** Clicking the resume SpatialMapping button */
	FReply OnClickResumeSpatialMapping();

	/** Clicking the stop SpatialMapping button */
	FReply OnClickStopSpatialMapping();

	/** Clicking the filter mesh button */
	FReply OnClickFilterMesh();

	/** Clicking the texture mesh button */
	FReply OnClickTextureMesh();

	/** Clicking the load mesh button */
	FReply OnClickLoadMesh();

	/** Clicking the save mesh button */
	FReply OnClickSaveMesh();

private:
	/** Can only be one camera actor */
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;

	/** Detail builder used to draw */
	IDetailLayoutBuilder* CachedDetailBuilder;
};