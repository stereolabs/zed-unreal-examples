//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#pragma once
#include "Engine.h"
#include "kismet/BlueprintFunctionLibrary.h"

#include "Stereolabs/Public/Core/StereolabsBaseTypes.h"
#include "Stereolabs/Public/Core/StereolabsCoreGlobals.h"
#include "Stereolabs/Public/Core/StereolabsCameraProxy.h"

#include <sl/Core.hpp>

#include "StereolabsMatFunctionLibrary.generated.h"

UCLASS(BlueprintType)
class STEREOLABS_API USlMatFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*
	 * Sets the name of this mat
	 * @param Name The new name
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static void SetName(UPARAM(ref) FSlMat& Mat, const FName& Name)
	{
		Mat.Mat.name = TCHAR_TO_UTF8(*Name.ToString());
	}

	/*
	 * Whether the MAT can display informations or not
	 * @param bVerbose True if the mat should display informations
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static void SetVerbose(UPARAM(ref) FSlMat& Mat, bool bVerbose)
	{
		Mat.Mat.verbose = bVerbose;
	}

	/*
	 * @return The name of the mat
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static FName GetName(UPARAM(ref) FSlMat& Mat)
	{
		return FName(Mat.Mat.name.c_str());
	}

	/*
	 * @return True if verbose enabled
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static bool GetVerbose(UPARAM(ref) FSlMat& Mat)
	{
		return Mat.Mat.verbose;
	}

	/*
	 * Allocates the Mat memory
	 * @param Resolution The size of the matrix in pixels
	 * @param Type		 The type of the matrix (ESlMatType::MT_32F_C1, ESlMatType::MT__8U_C4)
	 * @param MemoryType Defines where the buffer will be stored (sl::MEM_CPU and/or sl::MEM_GPU)
	 * @warning			 It erases previously allocated memory
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static void Allocate(UPARAM(ref) FSlMat& Mat, const FIntPoint& Resolution, ESlMatType Type, UPARAM(DisplayName = "MemoryType", meta = (Bitmask, BitmaskEnum = ESlMemoryType)) ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		checkf(GSlCameraProxy && GSlCameraProxy->IsCameraOpened(), TEXT("Camera must be opened before allocating a Mat"));

		Mat.Mat.alloc(sl::unreal::ToSlType2(Resolution), sl::unreal::ToSlType(Type), sl::unreal::ToSlType(MemoryType));
	}

	/*
	 * Free the owned memory
	 * @param MemoryType Specify whether you want to free the  MEM_CPU and/or  MEM_GPU memory
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static void Free(UPARAM(ref) FSlMat& Mat, UPARAM(DisplayName = "MemoryType", meta = (Bitmask, BitmaskEnum = ESlMemoryType)) ESlMemoryType MemoryType)
	{
		Mat.Mat.free(sl::unreal::ToSlType(MemoryType));
	}

	/*
	 * Downloads data from DEVICE (GPU) to HOST (CPU), if possible
	 * @return  EC_Success if everything went well,  EC_Failure otherwise
	 * @note If no CPU or GPU memory are available for this Mat, some are directly allocated
	 * @note If verbose sets, you have informations in case of failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode UpdateCPUFromGPU(UPARAM(ref) FSlMat& Mat)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.updateCPUfromGPU());
	}

	/*
	 * Uploads data from HOST (CPU) to DEVICE (GPU), if possible
	 * @return  EC_Success if everything went well,  EC_Failure otherwise
	 * @note If no CPU or GPU memory are available for this Mat, some are directly allocated
	 * @note If verbose sets, you have informations in case of failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode UpdateGPUFromCPU(UPARAM(ref) FSlMat& Mat)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.updateGPUfromCPU());
	}

	/*
	 * Copies data in other Mat (deep copy)
	 * @param Dst	   The Mat where the data will be copied
	 * @param CopyType Specify the memories that will be used for the copy
	 * @return		   EC_Success if everything went well,  EC_Failure otherwise
	 * @note If the Destination is not allocated or has a not a compatible  MAT_TYPE or  Resolution,
	 * current memory is freed and new memory is directly allocated
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode CopyTo(const  FSlMat& Mat, UPARAM(ref) FSlMat& Dst, ESlCopyType CopyType = ESlCopyType::CT_CPUToCPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.copyTo(Dst.Mat, sl::unreal::ToSlType(CopyType)));
	}

	/*
	 * Copies data from an other Mat (deep copy)
	 * @param Src	   The Mat where the data will be copied from
	 * @param CopyType Specify the memories that will be used for the update
	 * @return		   EC_Success if everything went well,  EC_Failure otherwise
	 * @note If the current Mat is not allocated or has a not a compatible  MAT_TYPE or  Resolution with the Src,
	 * current memory is freed and new memory is directly allocated
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetFrom(UPARAM(ref) FSlMat& Mat, const FSlMat& Src, ESlCopyType CopyType = ESlCopyType::CT_CPUToCPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setFrom(Src.Mat, sl::unreal::ToSlType(CopyType)));
	}

	/*
	 * Reads an image from a file (only if  MEM_CPU is available on the current  Mat).
	 * Supported input files format are PNG and JPEG
	 * @param Path File path including the name and extension
	 * @return     EC_Success if everything went well,  EC_Failure otherwise
	 * @note Supported ESlMatType are : MT_8U_C1,  MT_8U_C3 and  MT_8U_C4
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode Read(UPARAM(ref) FSlMat& Mat, const FString& Path)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.read(TCHAR_TO_UTF8(*Path)));
	}

	/*
	 * Writes the  Mat (only if  MEM_CPU is available) into a file as an image
	 * Supported output files format are PNG and JPEG
	 * @param Path File path including the name and extension
	 * @return	   EC_Success if everything went well,  EC_Failure otherwise
	 * @note Supported  ESlMatType are : MT_8U_C1,  MT_8U_C3 and  MT_8U_C4
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode Write(UPARAM(ref) FSlMat& Mat, const FString& Path)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.write(TCHAR_TO_UTF8(*Path)));
	}

	/*
	 * Fills the Mat with the given value.
	 * This function overwrite all the matrix.
	 * @param Value      The value to be copied all over the matrix
	 * @param MemoryType Defines which buffer to fill, CPU and/or GPU
	 * @return		     EC_Success if everything went well, EC_Failure otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetToFloat(UPARAM(ref) FSlMat& Mat, float Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setTo(Value, sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Fills the Mat with the given value.
	 * This function overwrite all the matrix.
	 * @param Value      The value to be copied all over the matrix
	 * @param MemoryType Defines which buffer to fill, CPU and/or GPU
	 * @return		     EC_Success if everything went well, EC_Failure otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetToFVector2D(UPARAM(ref) FSlMat& Mat, const FVector2D& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setTo(sl::unreal::ToSlType(Value), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Fills the Mat with the given value.
	 * This function overwrite all the matrix.
	 * @param Value      The value to be copied all over the matrix
	 * @param MemoryType Defines which buffer to fill, CPU and/or GPU
	 * @return		     EC_Success if everything went well, EC_Failure otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetToFVector(UPARAM(ref) FSlMat& Mat, const FVector& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setTo(sl::unreal::ToSlType(Value), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Fills the Mat with the given value.
	 * This function overwrite all the matrix.
	 * @param Value      The value to be copied all over the matrix
	 * @param MemoryType Defines which buffer to fill, CPU and/or GPU
	 * @return		     EC_Success if everything went well, EC_Failure otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetToFVector4(UPARAM(ref) FSlMat& Mat, const FVector4& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setTo(sl::unreal::ToSlType(Value), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Fills the Mat with the given value.
	 * This function overwrite all the matrix.
	 * Clamped to (0, 255) and converted to sl::uchar.
	 * @param Value      The value to be copied all over the matrix
	 * @param MemoryType Defines which buffer to fill, CPU and/or GPU
	 * @return		     EC_Success if everything went well, EC_Failure otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetToInt32(UPARAM(ref) FSlMat& Mat, int32 Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setTo(static_cast<sl::uchar1>(FMath::Clamp(Value, 0, 255)), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Fills the Mat with the given value.
	 * This function overwrite all the matrix.
	 * Clamped to (0, 255) and converted to sl::uchar2.
	 * @param Value      The value to be copied all over the matrix
	 * @param MemoryType Defines which buffer to fill, CPU and/or GPU
	 * @return		     EC_Success if everything went well, EC_Failure otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetToFIntPoint(UPARAM(ref) FSlMat& Mat, const FIntPoint& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		sl::uchar2 SlValue(FMath::Clamp(Value.X, 0, 255), FMath::Clamp(Value.Y, 0, 255));
		return sl::unreal::ToUnrealType(Mat.Mat.setTo(SlValue, sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Fills the Mat with the given value.
	 * This function overwrite all the matrix.
	 * Clamped to (0, 255) and converted to sl::uchar3.
	 * @param Value      The value to be copied all over the matrix
	 * @param MemoryType Defines which buffer to fill, CPU and/or GPU
	 * @return		     EC_Success if everything went well, EC_Failure otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetToFIntVector(UPARAM(ref) FSlMat& Mat, const FIntVector& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setTo(sl::unreal::ToSlType(Value), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Fills the Mat with the given value.
	 * This function overwrite all the matrix.
	 * Converted to sl::uchar4.
	 * @param Value      The value to be copied all over the matrix
	 * @param MemoryType Defines which buffer to fill, CPU and/or GPU
	 * @return		     EC_Success if everything went well, EC_Failure otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetToFColor(UPARAM(ref) FSlMat& Mat, const FColor& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setTo(sl::unreal::ToSlType(Value), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Sets a value to a specific point in the matrix.
	 * @param Coordinates The coordinate to write to.
	 * @param Value		  The value to be set
	 * @param MemoryType  Defines which memory will be updated
	 * @return			  EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetValueFloat(UPARAM(ref) FSlMat& Mat, const FIntPoint& Coordinates, float Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setValue(Coordinates.X, Coordinates.Y, Value, sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Sets a value to a specific point in the matrix.
	 * @param Coordinates The coordinate to write to
	 * @param Value		  The value to be set
	 * @param MemoryType  Defines which memory will be updated
	 * @return			  EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetValueFVector2D(UPARAM(ref) FSlMat& Mat, const FIntPoint& Coordinates, const FVector2D& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setValue(Coordinates.X, Coordinates.Y, sl::unreal::ToSlType(Value), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Sets a value to a specific point in the matrix.
	 * @param Coordinates The coordinate to write to
	 * @param Value		  The value to be set
	 * @param MemoryType  Defines which memory will be updated
	 * @return			  EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetValueFVector(UPARAM(ref) FSlMat& Mat, const FIntPoint& Coordinates, const FVector& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setValue(Coordinates.X, Coordinates.Y, sl::unreal::ToSlType(Value), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Sets a value to a specific point in the matrix.
	 * @param Coordinates The coordinate to write to
	 * @param Value		  The value to be set
	 * @param MemoryType  Defines which memory will be updated
	 * @return			  EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetValueFVector4(UPARAM(ref) FSlMat& Mat, const FIntPoint& Coordinates, const FVector4& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setValue(Coordinates.X, Coordinates.Y, sl::unreal::ToSlType(Value), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Sets a value to a specific point in the matrix.
	 * @param Coordinates The coordinate to write to.
	 * Clamped to (0, 255) and converted to sl::uchar.
	 * @param Value		  The value to be set
	 * @param MemoryType  Defines which memory will be updated
	 * @return			  EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetValueInt32(UPARAM(ref) FSlMat& Mat, const FIntPoint& Coordinates, int32 Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setValue(Coordinates.X, Coordinates.Y, static_cast<sl::uchar1>(FMath::Clamp(Value, 0, 255)), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Sets a value to a specific point in the matrix.
	 * Clamped to (0, 255) and converted to sl::uchar2.
	 * @param Coordinates The coordinate to write to
	 * @param Value		  The value to be set
	 * @param MemoryType  Defines which memory will be updated
	 * @return			  EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetValueFIntPoint(UPARAM(ref) FSlMat& Mat, const FIntPoint& Coordinates, const FIntPoint& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		sl::uchar2 SLValue(FMath::Clamp(Value.X, 0, 255), FMath::Clamp(Value.Y, 0, 255));
		return sl::unreal::ToUnrealType(Mat.Mat.setValue(Coordinates.X, Coordinates.Y, SLValue, sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Sets a value to a specific point in the matrix.
	 * Clamped to (0, 255) and converted to sl::uchar3.
	 * @param Coordinates The coordinate to write to
	 * @param Value		  The value to be set
	 * @param MemoryType  Defines which memory will be updated
	 * @return			  EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetValueFIntVector(UPARAM(ref) FSlMat& Mat, const FIntPoint& Coordinates, const FIntVector& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setValue(Coordinates.X, Coordinates.Y, sl::unreal::ToSlType(Value), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Sets a value to a specific point in the matrix.
	 * Converted to sl::uchar4.
	 * @param Coordinates The coordinate to write to
	 * @param Value		  The value to be set
	 * @param MemoryType  Defines which memory will be updated
	 * @return			  EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode SetValueFColor(UPARAM(ref) FSlMat& Mat, const FIntPoint& Coordinates, const FColor& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.setValue(Coordinates.X, Coordinates.Y, sl::unreal::ToSlType(Value), sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Returns the value of a specific point in the matrix.
	 * @param Coordinates The coordinate to read from
	 * @param MemoryType  Defines which memory should be read.
	 * @return EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for  MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static ESlErrorCode GetValueFloat(const FSlMat& Mat, const FIntPoint& Coordinates, float& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.getValue(static_cast<size_t>(Coordinates.X), static_cast<size_t>(Coordinates.Y), &Value, sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Returns the value of a specific point in the matrix.
	 * @param Coordinates The coordinate to read from
	 * @param MemoryType  Defines which memory should be read.
	 * @return EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for  MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static ESlErrorCode GetValueFVector2D(const FSlMat& Mat, const FIntPoint& Coordinates, FVector2D& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		sl::float2 SlValue;
		sl::ERROR_CODE ErrorCode = Mat.Mat.getValue(static_cast<size_t>(Coordinates.X), static_cast<size_t>(Coordinates.Y), &SlValue, sl::unreal::ToSlType(MemoryType));

		Value = sl::unreal::ToUnrealType(SlValue);

		return sl::unreal::ToUnrealType(ErrorCode);
	}

	/*
	 * Returns the value of a specific point in the matrix.
	 * @param Coordinates The coordinate to read from
	 * @param MemoryType  Defines which memory should be read.
	 * @return EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for  MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static ESlErrorCode GetValueFVector(const FSlMat& Mat, const FIntPoint& Coordinates, FVector& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		sl::float3 SlValue;
		sl::ERROR_CODE ErrorCode = Mat.Mat.getValue(static_cast<size_t>(Coordinates.X), static_cast<size_t>(Coordinates.Y), &SlValue, sl::unreal::ToSlType(MemoryType));

		Value = sl::unreal::ToUnrealType(SlValue);

		return sl::unreal::ToUnrealType(ErrorCode);
	}

	/*
	 * Returns the value of a specific point in the matrix.
	 * @param Coordinates The coordinate to read from
	 * @param MemoryType  Defines which memory should be read.
	 * @return EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for  MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static ESlErrorCode GetValueFVector4(const FSlMat& Mat, const FIntPoint& Coordinates, FVector4& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		sl::float4 SlValue;
		sl::ERROR_CODE ErrorCode = Mat.Mat.getValue(static_cast<size_t>(Coordinates.X), static_cast<size_t>(Coordinates.Y), &SlValue, sl::unreal::ToSlType(MemoryType));

		Value = sl::unreal::ToUnrealType(SlValue);

		return sl::unreal::ToUnrealType(ErrorCode);
	}

	/*
	 * Returns the value of a specific point in the matrix.
	 * @param Coordinates The coordinate to read from
	 * @param MemoryType  Defines which memory should be read.
	 * @return EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for  MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static ESlErrorCode GetValueInt32(const FSlMat& Mat, const FIntPoint& Coordinates, int32& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		sl::uchar1 SlValue;
		sl::ERROR_CODE ErrorCode = Mat.Mat.getValue(static_cast<size_t>(Coordinates.X), static_cast<size_t>(Coordinates.Y), &SlValue, sl::unreal::ToSlType(MemoryType));

		Value = (int32)SlValue;

		return sl::unreal::ToUnrealType(ErrorCode);
	}

	/*
	 * Returns the value of a specific point in the matrix.
	 * @param Coordinates The coordinate to read from
	 * @param MemoryType  Defines which memory should be read.
	 * @return EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for  MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static ESlErrorCode GetValueFIntPoint(const FSlMat& Mat, const FIntPoint& Coordinates, FIntPoint& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		sl::uchar2 SlValue;
		sl::ERROR_CODE ErrorCode = Mat.Mat.getValue(static_cast<size_t>(Coordinates.X), static_cast<size_t>(Coordinates.Y), &SlValue, sl::unreal::ToSlType(MemoryType));

		Value = sl::unreal::ToUnrealType(SlValue);

		return sl::unreal::ToUnrealType(ErrorCode);
	}

	/*
	 * Returns the value of a specific point in the matrix.
	 * @param Coordinates The coordinate to read from
	 * @param MemoryType  Defines which memory should be read.
	 * @return EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for  MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static ESlErrorCode GetValueFIntVector(const FSlMat& Mat, const FIntPoint& Coordinates, FIntVector& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		sl::uchar3 SlValue;
		sl::ERROR_CODE ErrorCode = Mat.Mat.getValue(static_cast<size_t>(Coordinates.X), static_cast<size_t>(Coordinates.Y), &SlValue, sl::unreal::ToSlType(MemoryType));

		Value = sl::unreal::ToUnrealType(SlValue);

		return sl::unreal::ToUnrealType(ErrorCode);
	}

	/*
	 * Returns the value of a specific point in the matrix.
	 * @param Coordinates The coordinate to read from
	 * @param MemoryType  Defines which memory should be read.
	 * @return EC_Success if everything went well, EC_Failure otherwise.
	 *
	 * @warning Not efficient for  MEM_GPU, use it on sparse data.
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static ESlErrorCode GetValueFColor(const FSlMat& Mat, const FIntPoint& Coordinates, FColor& Value, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		sl::uchar4 SlValue;
		sl::ERROR_CODE ErrorCode = Mat.Mat.getValue(static_cast<size_t>(Coordinates.X), static_cast<size_t>(Coordinates.Y), &SlValue, sl::unreal::ToSlType(MemoryType));

		Value = sl::unreal::ToUnrealType(SlValue);

		return sl::unreal::ToUnrealType(ErrorCode);
	}

	/*
	 * Returns the width of the matrix
	 * @return The width of the matrix in pixels
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static  int32 GetWidth(const FSlMat& Mat)
	{
		return static_cast<int32>(Mat.Mat.getWidth());
	}

	/*
	 * Returns the height of the matrix
	 * @return The height of the matrix in pixels
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static int32 GetHeight(const FSlMat& Mat)
	{
		return static_cast<int32>(Mat.Mat.getHeight());
	}

	/*
	 * Returns the height of the matrix
	 * @return The height of the matrix in pixels
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static FIntPoint GetResolution(const FSlMat& Mat)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.getResolution());
	}
	/*
	 * Returns the number of values stored in one pixel
	 * @return The number of values in a pixel
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static int32 GetChannels(const FSlMat& Mat)
	{
		return static_cast<int32>(Mat.Mat.getChannels());
	}

	/*
	 * Returns the format of the matrix
	 * @return The format of the current Mat
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static ESlMatType GetDataType(const FSlMat& Mat)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.getDataType());
	}

	/*
	 * Returns the type of memory (CPU and/or GPU)
	 * @return The type of allocated memory
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static ESlMemoryType GetMemoryType(const FSlMat& Mat)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.getMemoryType());
	}

	/*
	 * Returns the memory step in Bytes (the Bytes size of one pixel row)
	 * @param MemoryTYpe Specify whether you want  MEM_CPU or  MEM_GPU step
	 * @return The step in bytes of the specified memory
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static int32 GetStepBytes(const FSlMat& Mat, ESlMemoryType MemoryTYpe = ESlMemoryType::MT_CPU)
	{
		return static_cast<int32>(Mat.Mat.getStepBytes(sl::unreal::ToSlType(MemoryTYpe)));
	}

	/*
	 * Returns the size in bytes of one pixel
	 * @return The size in bytes of a pixel
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static int32 GetPixelBytes(const FSlMat& Mat)
	{
		return static_cast<int32>(Mat.Mat.getPixelBytes());
	}

	/*
	 * Returns the memory step in number of elements (the number of values in one pixel row)
	 * @param MemoryType Specify whether you want  MEM_CPU or  MEM_GPU step
	 * @return			  The step in number of elements.
	 */
	inline int32 getStep(const FSlMat& Mat, ESlMemoryType MemoryType = ESlMemoryType::MT_CPU)
	{
		return static_cast<int32>(Mat.Mat.getStep(sl::unreal::ToSlType(MemoryType)));
	}

	/*
	 * Returns the size in bytes of a row
	 * @return The size in bytes of a row
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static int32 getWidthBytes(const FSlMat& Mat)
	{
		return GetPixelBytes(Mat) * GetWidth(Mat);
	}

	/*
	 * Returns the informations about the Mat into a FString
	 * @return A string containing the Mat informations
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static FString GetInfos(UPARAM(ref) FSlMat& Mat)
	{
		return FString(Mat.Mat.getInfos().c_str());
	}

	/*
	 * Defines whether the Mat is initialized or not
	 * @return True if current Mat has been allocated (by the constructor or therefore)
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static bool IsInit(const FSlMat& Mat)
	{
		return Mat.Mat.isInit();
	}

	/*
	 * Returns whether the Mat is the owner of the memory it access
	 * If not, the memory won't be freed if the Mat is destroyed
	 * @return True if the Mat is owning its memory, else false
	 */
	UFUNCTION(BlueprintPure, Category = "Stereolabs|Mat")
	static bool IsMemoryOwner(const FSlMat& Mat)
	{
		return Mat.Mat.isMemoryOwner();
	}

	/*
	 * Duplicates Mat by copy (deep copy)
	 * @param Src The reference to the Mat to copy
	 * This function copies the data array(s), it mark the new Mat as the memory owner
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode Clone(UPARAM(ref) FSlMat& Mat, UPARAM(ref) FSlMat& Src)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.clone(Src.Mat));
	}

	/*
	 * Moves Mat data to another Mat.
	 * This function gives the attribute of the current Mat to the specified one (No copy)
	 * @param Dst The reference to the Mat to move
	 * @note The current Mat is then no more usable since its loose its attributes
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static ESlErrorCode Move(UPARAM(ref) FSlMat& Mat, UPARAM(ref) FSlMat& Dst)
	{
		return sl::unreal::ToUnrealType(Mat.Mat.move(Dst.Mat));
	}

	/*
	 * Swaps the content of the provided Mat (only swaps the pointers, no data copy).
	 * This function swaps the pointers of the given Mat
	 * @param Left  The first mat
	 * @param Right The second mat
	 */
	UFUNCTION(BlueprintCallable, Category = "Stereolabs|Mat")
	static void Swap(UPARAM(ref) FSlMat& Left, UPARAM(ref) FSlMat& Right)
	{
		sl::Mat::swap(Left.Mat, Right.Mat);
	}
};