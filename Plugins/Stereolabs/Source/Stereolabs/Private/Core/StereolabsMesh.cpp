//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#include "StereolabsPrivatePCH.h"
#include "Stereolabs/Public/Core/StereolabsMesh.h"
#include "Stereolabs/Public/Core/StereolabsCoreGlobals.h"

USlMesh::USlMesh()
{
}

void USlMesh::UpdateMeshData()
{
	int VerticesNum = Mesh.vertices.size();
	int IndicesNum = Mesh.triangles.size();
	int UVNum = 0;

	MeshData.Vertices.Reset(VerticesNum);
	MeshData.Indices.Reset(IndicesNum * 3);

	if (MeshData.Texture)
	{
		UVNum = Mesh.uv.size();

		MeshData.UV0.Reset(UVNum);
	}

	int MaxNum = FMath::Max(VerticesNum, IndicesNum);

	for (int i = 0; i < MaxNum; ++i)
	{
		if (i < VerticesNum)
		{
			MeshData.Vertices.Add(FVector(
				Mesh.vertices[i].x,
				Mesh.vertices[i].y,
				Mesh.vertices[i].z));

			MeshData.Normals.Add(FVector(
				Mesh.normals[i].x,
				Mesh.normals[i].y,
				Mesh.normals[i].z));
		}

		if (i < IndicesNum)
		{
			MeshData.Indices.Add(Mesh.triangles[i][0]);
			MeshData.Indices.Add(Mesh.triangles[i][1]);
			MeshData.Indices.Add(Mesh.triangles[i][2]);
		}

		if (UVNum && i < UVNum)
		{
			MeshData.UV0.Add(FVector2D(Mesh.uv[i][0], Mesh.uv[i][1]));
		}
	}
}

void USlMesh::UpdateMeshFromChunks(const TArray<int32>& ChunksIDs)
{
	Mesh.updateMeshFromChunkList(sl::unreal::arrays::ToSlType<std::size_t, int32>(ChunksIDs));
}

TArray<int32> USlMesh::GetVisibleChunks(const FTransform& ViewPoint)
{
	return sl::unreal::arrays::ToUnrealType<int32, std::size_t>(Mesh.getVisibleList(sl::unreal::ToSlType(ViewPoint)));
}

TArray<int32> USlMesh::GetSurroundingChunks(const FTransform& ViewPoint, float Radius)
{
	return  sl::unreal::arrays::ToUnrealType<int32, std::size_t>(Mesh.getSurroundingList(sl::unreal::ToSlType(ViewPoint), Radius));
}

bool USlMesh::Filter(const FSlMeshFilterParameters& MeshFilterParameters/* = FSlMeshFilterParameters()*/, bool bUpdateChunksOnly/* = false*/)
{
	return Mesh.filter(sl::unreal::ToSlType(MeshFilterParameters), bUpdateChunksOnly);
}

bool USlMesh::ApplyTexture(ESlMeshTextureFormat TextureFormat/* = ESlMeshTextureFormat::MTF_RGBA*/, bool bSRGB/* = false*/)
{
	bool bIsMeshTextured = Mesh.texture.data.isInit();

	if (!bIsMeshTextured)
	{
		bIsMeshTextured = Mesh.applyTexture(sl::unreal::ToSlType(TextureFormat));
	}

	if (!bIsMeshTextured)
	{
		return false;
	}

	// Texture data
	sl::Mat& MeshTexture = Mesh.texture.data;

	// Create texture
	UTexture2D* Texture = UTexture2D::CreateTransient(MeshTexture.getWidth(), MeshTexture.getHeight());

	// Populate texture
	FTexture2DMipMap& Mip = Texture->PlatformData->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, MeshTexture.getPtr<sl::float4>(sl::MEM_CPU), MeshTexture.getStepBytes(sl::MEM_CPU) * MeshTexture.getHeight());
	Mip.BulkData.Unlock();

	// Set texture settings
#if WITH_EDITORONLY_DATA
	Texture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	Texture->CompressionNone = true;
	Texture->CompressionNoAlpha = true;
	Texture->DeferCompression = false;
#endif	
	Texture->SRGB = bSRGB;
	Texture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	Texture->bNoTiling = true;
	Texture->bIsStreamable = false;
	Texture->Filter = TextureFilter::TF_Bilinear;
	Texture->AddressX = TextureAddress::TA_Clamp;
	Texture->AddressY = TextureAddress::TA_Clamp;
	Texture->LODGroup = TextureGroup::TEXTUREGROUP_World;

	if (IsInRenderingThread())
	{
		Texture->UpdateResource();
	}

	MeshData.Texture = Texture;

	return true;
}

void USlMesh::MergeChunks(int NumberOfFacesPerChunk)
{
	Mesh.mergeChunks(NumberOfFacesPerChunk);
}

FVector USlMesh::GetGravityVector()
{
	return sl::unreal::ToUnrealType(Mesh.getGravityEstimate());
}

bool USlMesh::Save(const FString& FilePath, const TArray<int32>& ChunksIDs, ESlMeshFileFormat FileFormat/* = ESlMeshFileFormat::MFF_OBJ*/)
{
	bool bSaved = Mesh.save(TCHAR_TO_UTF8(*FilePath), sl::unreal::ToSlType(FileFormat), sl::unreal::arrays::ToSlType<std::size_t, int32>(ChunksIDs));

	return bSaved;
}

bool USlMesh::Load(const FString& FilePath, bool bUpdateChunksOnly/* = false*/)
{
	return Mesh.load(TCHAR_TO_UTF8(*FilePath), bUpdateChunksOnly);
}

void USlMesh::Clear()
{
	Mesh.clear();
	MeshData.Clear();
}