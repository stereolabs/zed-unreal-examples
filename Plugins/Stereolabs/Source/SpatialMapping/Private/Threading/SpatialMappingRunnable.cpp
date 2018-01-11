//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "SpatialMappingPrivatePCH.h"
#include "SpatialMapping/Private/Threading/SpatialMappingRunnable.h"
#include "SpatialMapping/Public/Core/SpatialMappingManager.h"
#include "Stereolabs/Public/Core/StereolabsCoreUtilities.h"

#include "RenderUtils.h"

DEFINE_STAT(STAT_UpdateMesh)
DEFINE_LOG_CATEGORY(SpatialMappingThread);

FSpatialMappingRunnable::FSpatialMappingRunnable(ASpatialMappingManager* SpatialMappingManager, USlMesh* Mesh)
	:
	SpatialMappingManager(SpatialMappingManager),
	Mesh(Mesh)
{
}

bool FSpatialMappingRunnable::Init()
{
	return FSlRunnable::Init();
}

uint32 FSpatialMappingRunnable::Run()
{
	FPlatformProcess::SleepNoStats(0.0f);

	while (bIsRunning)
	{
		if (bIsSleeping)
		{
			FPlatformProcess::ConditionalSleep([this]() -> bool {
				return !bIsSleeping;
			});
		}

		Timer.Start();

		SL_SCOPE_LOCK(Lock, StepSection)
			CurrentStep = Step;
		SL_SCOPE_UNLOCK

		switch (CurrentStep)
		{
			case ESpatialMappingStep::SS_Scan:
				{
					int Retrieved = RetrieveMesh();

					if (Retrieved == 1)
					{
						UpdateMesh();
					}
					else if(Retrieved == -1)
					{
						SpatialMappingManager->StepFailed(CurrentStep);
						SL_LOG_E(SpatialMappingThread, "Can't retrieve mesh");
					}
				}
				break;
			case ESpatialMappingStep::SS_Filter:
				{
					bool bFiltered = FilterMesh();

					if (bFiltered)
					{
						UpdateMesh();
					}
					else
					{
						SpatialMappingManager->StepFailed(CurrentStep);
						SL_LOG_E(SpatialMappingThread, "Can't filter mesh");
					}

					Step = ESpatialMappingStep::SS_None;
				}
				break;
			case ESpatialMappingStep::SS_Texture:
				{
					bool bTextured = TextureMesh();

					if (!bTextured)
					{
						SpatialMappingManager->StepFailed(CurrentStep);
						SL_LOG_E(SpatialMappingThread, "Can't texture mesh");
					}

					Step = ESpatialMappingStep::SS_None;
				}
				break;
			case ESpatialMappingStep::SS_Load:
				{
					bool bLoaded = LoadMesh();

					if (bLoaded)
					{
						UpdateMesh();
					}
					else
					{
						SpatialMappingManager->StepFailed(CurrentStep);
						SL_LOG_E(SpatialMappingThread, "Can't load mesh");
					}

					Step = ESpatialMappingStep::SS_None;
				}
				break;
			case ESpatialMappingStep::SS_Save:
				{
					bool bSaved = SaveMesh();

					if(!bSaved)
					{
						SpatialMappingManager->StepFailed(CurrentStep);
						SL_LOG_E(SpatialMappingThread, "Can't save mesh");
					}

					Step = ESpatialMappingStep::SS_None;
				}
				break;
		}

		if (Timer.CanSleep())
		{
			FPlatformProcess::Sleep(Timer.GetSleepingTimeSeconds());
		}
	}

	return 0;
}

void FSpatialMappingRunnable::Stop()
{
	FSlRunnable::Stop();
}

void FSpatialMappingRunnable::Exit()
{
}

void FSpatialMappingRunnable::Start(float Frequency)
{
	static uint64 ThreadCounter = 0;

	Timer.SetFrequency(Frequency);

	FString ThreadName("SLSpatialMappingThread");
	ThreadName.AppendInt(ThreadCounter++);

	Thread = FRunnableThread::Create(this, *ThreadName, 0, TPri_BelowNormal);
}

void FSpatialMappingRunnable::SetStep(ESpatialMappingStep NewStep)
{
	SL_SCOPE_LOCK(Lock, StepSection)
		Step = NewStep;
	SL_SCOPE_UNLOCK
}

ESpatialMappingStep FSpatialMappingRunnable::GetStep()
{
	SL_SCOPE_LOCK(Lock, StepSection)
		return CurrentStep;
	SL_SCOPE_UNLOCK
}

int FSpatialMappingRunnable::RetrieveMesh()
{
	GSlCameraProxy->RequestMeshAsync();

	return (GSlCameraProxy->GetMeshIsReadyAsync() ? (GSlCameraProxy->RetrieveMeshAsync(Mesh) ? 1 : -1) : 0);
}

bool FSpatialMappingRunnable::FilterMesh()
{
	// Sleep in case no buffer available
	FPlatformProcess::Sleep(0.5f);

	return Mesh->Filter(MeshFilterParameters, false);
}

bool FSpatialMappingRunnable::TextureMesh()
{
	bool bIsMeshTextured = Mesh->ApplyTexture(ESlMeshTextureFormat::MTF_RGBA, TexturingMode == ESpatialMappingTexturingMode::TM_Cubemap);
	if (bIsMeshTextured)
	{
		UpdateMesh();

		return true;
	}

	return false;
}

void FSpatialMappingRunnable::UpdateMesh()
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateMesh);

	Mesh->UpdateMeshFromChunks(TArray<int32>());

	SL_SCOPE_LOCK(Lock, SpatialMappingManager->MeshDataUpdateSection)
		Mesh->UpdateMeshData();

		SpatialMappingManager->Step = Step;
	SL_SCOPE_UNLOCK
}

bool FSpatialMappingRunnable::LoadMesh()
{
	FString Path;

	SL_SCOPE_LOCK(Lock, MeshOperationSection)
		Path = MeshLoadingPath;
	SL_SCOPE_UNLOCK

	return Mesh->Load(Path);
}

bool FSpatialMappingRunnable::SaveMesh()
{
	FString Path;
	ESlMeshFileFormat Format;

	SL_SCOPE_LOCK(Lock, MeshOperationSection)
		Path = MeshSavingPath;
		Format = MeshFileFormat;
	SL_SCOPE_UNLOCK

	Mesh->Mesh.applyTexture(sl::MESH_TEXTURE_FORMAT::MESH_TEXTURE_RGBA);

	bool bSaved = Mesh->Save(*Path, TArray<int32>(), Format);

	Mesh->Mesh.texture.clear();

	SL_SCOPE_LOCK(Lock, SpatialMappingManager->MeshDataUpdateSection)
		SpatialMappingManager->Step = Step;
	SL_SCOPE_UNLOCK

	return bSaved;
}

void FSpatialMappingRunnable::SetSaveMeshData(FString NewMeshSavingPath, ESlMeshFileFormat NewMeshFileFormat)
{
	SL_SCOPE_LOCK(Lock, MeshOperationSection)
		MeshSavingPath = NewMeshSavingPath;
		MeshFileFormat = NewMeshFileFormat;
	SL_SCOPE_UNLOCK
}

void FSpatialMappingRunnable::SetLoadMeshData(FString NewMeshLoadingPath)
{
	SL_SCOPE_LOCK(Lock, MeshOperationSection)
		MeshLoadingPath = NewMeshLoadingPath;
	SL_SCOPE_UNLOCK
}
