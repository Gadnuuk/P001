// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TraversalMovement.h"

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TraversalMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class P001_API UTraversalMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public: 
	UPROPERTY(BlueprintAssignable)
	FTraverseDelegate InitJumpEvent;
	
	UPROPERTY(BlueprintAssignable)
	FTraverseDelegate OnStartLegRaise;

	UPROPERTY(BlueprintAssignable)
	FTraverseDelegate OnStopLegRaise;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanEverRaiseLegs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ActionPointActorClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USceneComponent> ActionPointComponentClass;

public: 
	UFUNCTION(BlueprintPure, Category = "Move State")
	TEnumAsByte<EMovementMode> GetMainMoveState(){return MovementMode;}

	UFUNCTION(BlueprintCallable, Category = "Move State")
	void Jump();

	UFUNCTION(BlueprintCallable, Category = "Move State")
	void RaiseLegs();
	
	UFUNCTION(BlueprintCallable, Category = "Move State")
	void DropLegs();

	UFUNCTION(BlueprintCallable, Category = "Move State")
	void PlayMontageWithTargetMatch(const FMatchTargetData& Data);

	UFUNCTION(BlueprintCallable, Category = "Move State")
	bool SweepForGround(FVector& SweepVelocity) const;

protected:
	UPROPERTY()
	class USpringArmComponent* CameraBoom;

	UPROPERTY()
	TArray<FMatchTargetData> MatchTargets;

	UPROPERTY()
	UAnimInstance* AnimInstance;

	TArray<class USceneComponent*> ActionPoints;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override;
	virtual void TickMatchTarget(float DeltaTime);

private:
	bool bHasLegsRaised;
	
	float DefaultCapsuleHalfHeightUnscaled;
	float DefaultCapsuleRadiusUnscaled;
	
	float DefaultCapsuleHalfHeightScaled;
	float DefaultCapsuleRadiusScaled;
	
	FVector DefaultSkeletalMeshLocation;
	FVector DefaultCameraBoomLocation;
};
