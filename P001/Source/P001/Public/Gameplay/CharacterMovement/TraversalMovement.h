// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TraversalMovement.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTraverseDelegate);

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