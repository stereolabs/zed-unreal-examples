#pragma once

#include "Stereolabs/Public/Core/StereolabsCoreGlobals.h"
#include "Stereolabs/Public/Core/StereolabsTexture.h"

#include "StereolabsTextureBatch.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(SlTextureBatch, Log, All);

#define TB_BUFFERS_POOL_SIZE 2

struct FSlTextueBatchMatBuffer
{
	FSlTextueBatchMatBuffer();
	~FSlTextueBatchMatBuffer();

	/** Timestamp of the buffer */
	sl::timeStamp Timestamp;

	/** True if the buffer is free to use */
	bool bIsFree;

	/** True if the buffer is updated */
	bool bIsUpdated;

	/** Current mats used by the buffer */
	TArray<sl::Mat> Mats;
};

/*
 * A batch that retrieve and update textures.
 */
UCLASS(BlueprintType, Category = "Stereolabs|Texture")
class STEREOLABS_API USlTextureBatch : public UObject
{
	GENERATED_BODY()

public:
	USlTextureBatch();

	virtual void BeginDestroy() override;

	/*
	 * Retrieve the textures. Called inside the grab thread.
	 * @param ImageTimestamp The Zed image timestamp
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	void RetrieveCurrentFrame(const FSlTimestamp& ImageTimestamp);

	/*
	 * Tick the batch. Update the textures.
	 * @return Single thread : Always return true.
	 *		   Multi thread  : True if textures updated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	virtual bool Tick();

	/*
	 * Add a texture
	 * @param Texture The texture to add
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	virtual void AddTexture(USlTexture* Texture);

	/*
	 * Remove a texture
	 * @param Texture The texture to remove
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	virtual void RemoveTexture(USlTexture* Texture);

	/*
	 * Remove all textures
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs")
	virtual void Clear();

protected:
	/*
	 * Enable/Disable async retrieve
	 * @param bEnabled True to enable
	 */
	UFUNCTION()
	virtual void SetAsyncRetrieveEnabled(bool bEnabled);

	/*
	 * Create a batch
	 * @param Name	  The new name of the batch
	 * @param Type	  The type of batch
	 * @param MinSize The minimal size of the batch
	 */
	static USlTextureBatch* CreateTextureBatch(const FName& Name, ESlMemoryType Type, int32 MinSize = 1);

public:
	/** Name of the batch */
	FName Name;

protected:
	/** Pool of textures */
	TArray<USlTexture*> TexturesPool;

	/** Buffers for async retrieve */
	TArray<FSlTextueBatchMatBuffer, TFixedAllocator<TB_BUFFERS_POOL_SIZE>> BuffersPool;

	/** Section to synchronize with buffers */
	FCriticalSection BuffersSection;

	/** Section to synchronize with retrieves */
	FCriticalSection RetrieveSection;

	/** Section to synchronize with mat swaping */
	FCriticalSection SwapSection;

	/** True if the batch will retrieve in grab thread. Set to true before calling RetrieveCurrentFrame().  */
	FThreadSafeBool bAsyncRetrieveEnabled;

	/** Buffers for double buffering in async retrieve */
	FSlTextueBatchMatBuffer* Buffers[2];

	/** Current frame timestamp*/
	FSlTimestamp CurrentFrameTimestamp;

	/** The minimal size of the batch */
	int32 MinSize;

private:
	/** True to automatically add this batch to the OnGrabThreadEnabled delegate */
	bool bIsAutoAddToGrabDelegate;
};


/*
 * Batch for GPU textures with Texture2D
 */
UCLASS(BlueprintType, Category = "Stereolabs|Texture")
class STEREOLABS_API USlGPUTextureBatch : public USlTextureBatch
{
	GENERATED_BODY()

public:
	/*
	 * Create a GPU batch
	 * @param Name	  The new name of the batch
	 * @param MinSize The minimal size of the batch
	 * @return		  The batch
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	static USlGPUTextureBatch* CreateGPUTextureBatch(const FName& Name, int32 MinSize = 1)
	{
		return static_cast<USlGPUTextureBatch*>(USlTextureBatch::CreateTextureBatch(Name, ESlMemoryType::MT_GPU, MinSize));
	}

	/*
	 * Tick the batch. Update the textures.
	 * @return Single thread : Always return true.
	 *	       Multi thread  : True if render command enqueued.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	virtual bool Tick() override;

	/*
	 * Add a texture
	 * @param Texture The texture to add
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	virtual void AddTexture(USlTexture* Texture) override;

	/*
	 * Remove a texture. Sync with render thread.
	 * @param Texture The texture to remove
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs")
	virtual void RemoveTexture(USlTexture* Texture) override;

	/*
	 * Remove all textures. Sync with render thread.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	virtual void Clear() override;

private:
	/*
	 * Enable/Disable async retrieve
	 * @param bEnabled True to enable
	 */
	UFUNCTION()
	virtual void SetAsyncRetrieveEnabled(bool bEnabled) override;

private:
	/** Pool of CUDA resources associated with textures */
	TArray<cudaGraphicsResource_t> CudaResourcesPool;
};

/*
 * Batch for GPU textures without Texture2D
 */
UCLASS(BlueprintType, Category = "Stereolabs|Texture")
class STEREOLABS_API USlSimpleGPUTextureBatch : public USlTextureBatch
{
	GENERATED_BODY()

public:
	/*
	 * Create a GPU batch
	 * @param Name	  The new name of the batch
	 * @param MinSize The minimal size of the batch
	 * @return		  The batch
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	static USlSimpleGPUTextureBatch* CreateSimpleGPUTextureBatch(const FName& Name, int32 MinSize = 1)
	{
		return static_cast<USlSimpleGPUTextureBatch*>(USlTextureBatch::CreateTextureBatch(Name, ESlMemoryType::MT_GPU, MinSize));
	}

	/*
	 * Add a texture
	 * @param Texture The texture to add
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	virtual void AddTexture(USlTexture* Texture) override;
};

/*
 * Batch for CPU textures
 */
UCLASS(BlueprintType, Category = "Stereolabs|Texture")
class STEREOLABS_API USlCPUTextureBatch : public USlTextureBatch
{
	GENERATED_BODY()

public:
	/*
	 * Create a CPU batch
	 * @param Name	  The new name of the batch
	 * @param MinSize The minimal size of the batch
	 * @return		  The batch
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	static USlCPUTextureBatch* CreateCPUTextureBatch(const FName& Name, int32 MinSize = 1)
	{
		return static_cast<USlCPUTextureBatch*>(USlTextureBatch::CreateTextureBatch(Name, ESlMemoryType::MT_CPU, MinSize));
	}

	/*
	 * Add a texture
	 * @param Texture The texture to add
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Texture")
	virtual void AddTexture(USlTexture* Texture) override;
};