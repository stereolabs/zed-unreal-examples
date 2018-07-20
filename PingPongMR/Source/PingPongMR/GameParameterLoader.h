// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameParameterLoader.generated.h"


USTRUCT(BlueprintType, Category = "GameParameter|Struct")
struct FGameParameter
{
	GENERATED_BODY()

public:

	FGameParameter() {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Ball")
		int ballNumberLimit = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter")
		float globalTimeDilation = 0.85;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Ball")
		float linearDampingSlope = 0.000005;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Ball")
		float ballWeigth = 0.01;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Ball")
		float ballMaxSpeed = 1000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Ball")
		float magnusSlope = 0.03;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter")
		float raquetteServerMeshScale = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Restitution")
		float raquetteRestitution = 0.3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Friction")
		float raquetteFriction = 40.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Restitution")
		float ballRestitution = 0.8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Friction")
		float ballFriction = 5.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Restitution")
		float tableRestitution = 0.75;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameParameter|Friction")
		float tableFriction = 0.1;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PINGPONGMR_API UGameParameterLoader : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGameParameterLoader();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	UPROPERTY(BlueprintReadWrite, Category = "GameParameter")
	bool isValid = false;

	UPROPERTY(BlueprintReadWrite, Category = "GameParameter")
	FGameParameter gameParameters;

	UFUNCTION(BlueprintCallable, Category = "GameParameter")
	void LoadParameters();

	UFUNCTION(BlueprintCallable, Category = "GameParameter")
	static UPhysicalMaterial* ApplyPhysicalMaterialParameter(UPhysicalMaterial* physicalMaterial, const float& friction, const float& restitution);
		
	
};
