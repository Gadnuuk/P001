// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TraversalMovement.h"

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TraversalMovementComponent.generated.h"

class UActionPointComponent;

/**
 * 
 */
UCLASS()
class P001_API UTraversalMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public: 
	UTraversalMovementComponent();

	UPROPERTY(BlueprintAssignable)
	FTraverseDelegate InitJumpEvent;
	
	UPROPERTY(BlueprintAssignable)
	FTraverseDelegate OnStartLegRaise;

	UPROPERTY(BlueprintAssignable)
	FTraverseDelegate OnStopLegRaise;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LegRaiseHalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanEverRaiseLegs;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShowArrow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ActionPointActorClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
	FTraverseSettings ClimbStartSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
	FVector PlayerOffsetFromWall;

	UPROPERTY(BlueprintReadWrite)
	class USphereComponent* ActionPointDetectionCollision;

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
	void StartClimbing();
	
	UFUNCTION(BlueprintCallable, Category = "Move State")
	void StopClimbing();

	UFUNCTION(BlueprintCallable, Category = "Move State")
	void PlayMontageWithTargetMatch(const FMatchTargetData& Data);

	UFUNCTION(BlueprintCallable, Category = "Move State")
	bool SweepForGround(FVector& SweepVelocity) const;

	UFUNCTION(BlueprintPure, Category = "Move State")
	bool GetIsClimbing() {return bIsClimbing;}

	UFUNCTION(BlueprintCallable)
	bool InitActionPointDetection();

protected:
	UPROPERTY()
	TArray<FMatchTargetData> MatchTargets;

	UPROPERTY()
	UAnimInstance* AnimInstance;

	UPROPERTY()
	TArray<UActionPointComponent*> AllActionPoints;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<UActionPointComponent*> ActionPointsInClimbableSpace;
	
	UPROPERTY(BlueprintReadOnly)
	UActionPointComponent* CurrentActionPoint;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override;
	
	void TickMatchTarget(float DeltaTime);
	void SetCurrentActionPoint(UActionPointComponent* NewActionPoint);
	void TickClimbPosition(float DeltaTime);

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	bool bIsClimbing;
	bool bIsTransitioning;
	bool bHasLegsRaised;

	float CurrentMinHeightToClimb;
	float CurrentMaxHeightToClimb;
	
	float DefaultCapsuleHalfHeightUnscaled;
	float DefaultCapsuleRadiusUnscaled;
	
	float DefaultCapsuleHalfHeightScaled;
	float DefaultCapsuleRadiusScaled;
	
	FVector DefaultSkeletalMeshLocation;
};
