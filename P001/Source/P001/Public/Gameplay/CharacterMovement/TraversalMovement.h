// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TraversalMovement.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTraverseDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTraverseWallClimbDelegate, float, X, float, Y);

USTRUCT(BlueprintType, meta = (BlueprintSpawnableComponent))
struct FTraverseSettings
{
	GENERATED_USTRUCT_BODY()
	FTraverseSettings(){}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxDistanceThreshold;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinDistanceThreshold;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxDotThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHeightThreshold;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHeightThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LateralThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FallingHeightScalar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PlayerOffsetFromWall;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WallStepDepth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxDistanceScalar;

};

UENUM(BlueprintType)
enum EActionPointType
{
	AP_None,
	AP_WallClimb,
	AP_LedgeClimb
};

USTRUCT(BlueprintType, meta = (BlueprintSpawnableComponent))
struct FMatchTargetData
{
	GENERATED_USTRUCT_BODY()
	FMatchTargetData(){}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* AnimationMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* MatchTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCrouch;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRaiseLegs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MatchTime;
	
	UPROPERTY(BlueprintReadOnly)
	float CurrentTime;

	UPROPERTY(BlueprintReadOnly)
	bool bHandled;

	bool operator==(const FMatchTargetData& Rhs)
	{
		return (AnimationMontage == Rhs.AnimationMontage
			&&  SocketName == Rhs.SocketName
			&&  MatchTarget == Rhs.MatchTarget
			&&  MatchTime == Rhs.MatchTime);
	}
};

USTRUCT(BlueprintType)
struct FTraverseActionPointData
{	
	GENERATED_USTRUCT_BODY();
	FTraverseActionPointData(){}

	UPROPERTY(BlueprintReadWrite)
	float Distance;
	
	UPROPERTY(BlueprintReadWrite)
	float LateralDistance;

	UPROPERTY(BlueprintReadWrite)
	float Score;

	UPROPERTY(BlueprintReadWrite)
	FVector Direction;
};