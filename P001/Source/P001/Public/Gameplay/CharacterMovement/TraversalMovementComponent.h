// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TraversalMovement.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"

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

	UPROPERTY(BlueprintAssignable)
	FTraverseDelegate OnWallClimbStart;
	
	UPROPERTY(BlueprintAssignable)
	FTraverseWallClimbDelegate OnWallClimbStartTransition;

	UPROPERTY(BlueprintAssignable)
	FTraverseDelegate OnWallClimbEndTransition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TestLerp = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ClimbTransitionInputTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ClimbHandWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LegRaiseHalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanEverRaiseLegs;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D ClimbTransitionMatchTargetRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debugging")
	bool bShowArrow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debugging")
	bool bFreezeClimbTransitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debugging")
	bool bLogPossibleClimbPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debugging")
	bool bLogClimbTransitionAnimationValues;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debugging")
	TEnumAsByte<EDrawDebugTrace::Type> CollisionCheckDrawMode;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debugging")
	TEnumAsByte<EDrawDebugTrace::Type> LedgeCheckDrawMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debugging")
	float CollisionCheckDrawTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ActionPointActorClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
	FTraverseSettings ClimbStartSettings;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
	FTraverseSettings ClimbTransitionSettings;

	UPROPERTY(BlueprintReadOnly, Category = "Debugging")
	bool bIsLeaning;

	UPROPERTY(BlueprintReadOnly, Category = "Debugging")
	bool bIsLeaningPaused;

	UPROPERTY(BlueprintReadOnly, Category = "Debugging")
	bool bAnchorRightHand;

	UPROPERTY(BlueprintReadOnly, Category = "Debugging")
	FVector LeanDirection;

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

	UFUNCTION(BlueprintCallable)
	void AddClimbHorizontalInput(float NewHorizontal);
	
	UFUNCTION(BlueprintCallable)
	void AddClimbVerticalInput(float NewVertical);

	UFUNCTION(BlueprintCallable)
	void StartClimbTransition(UActionPointComponent* NewActionPoint, FVector ClimbDirection, float TransitionTime);
	
	UFUNCTION(BlueprintCallable)
	void EndClimbTransition();

	UFUNCTION(BlueprintCallable)
	void PauseClimb(float AmountOfTime);
	
	UFUNCTION(BlueprintCallable, Exec)
	void FreezeClimb(bool bFreeze);

	UFUNCTION(BlueprintCallable, Exec)
	void ShowArrow(bool bShow);

	UFUNCTION(BlueprintPure)
	FVector GetLocationFromActionPoint(UActionPointComponent* ActionPoint, bool bAnchorRight, float Lerp = 1.f);
	
	UFUNCTION(BlueprintPure)
	FQuat GetRotationFromActionPoint(UActionPointComponent* ActionPoint);

	UFUNCTION(BlueprintCallable)
	UActionPointComponent* FindNextActionPoint(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	FVector SearchForLedge(float DeltaTime);

	UFUNCTION(BlueprintPure)
	bool CanReachActionPoint(UActionPointComponent* ActionPoint);

protected:
	UPROPERTY()
	TArray<FMatchTargetData> MatchTargets;

	UPROPERTY()
	UAnimInstance* AnimInstance;

	UPROPERTY(BlueprintReadOnly)
	TArray<UActionPointComponent*> ActionPointsInClimbableSpace;
	
	UPROPERTY(BlueprintReadOnly)
	UActionPointComponent* CurrentActionPoint;
	
	UPROPERTY(BlueprintReadOnly)
	UActionPointComponent* CurrentActionPointTransitioningTo;

	UPROPERTY(BlueprintReadWrite)
	float Horizontal;
	UPROPERTY(BlueprintReadWrite)
	float PreviousHorizontal;
	
	UPROPERTY(BlueprintReadWrite)
	float Vertical;

	UPROPERTY(BlueprintReadWrite)
	float PreviousVertical;

	UFUNCTION(BlueprintPure)
	FVector GetClimbInputVector();

	UFUNCTION(BlueprintPure)
	TArray<AActor*> GetClimbActorsToIgnore();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override;
	
	void TickMatchTarget(float DeltaTime);
	void SetCurrentActionPoint(UActionPointComponent* NewActionPoint);
	void TickClimbInput(float DeltaTime);
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

	float CurrentClimbTransitionInputTime;
	float ClimbTransitionTime;
	float CurrentTransitionTime;

	bool bClimbingPaused;
	float ClimbPauseTime;
	float CurrentClimbPauseTime;

	float CurrentMinHeightToClimb;
	float CurrentMaxHeightToClimb;
	
	float DefaultCapsuleHalfHeightUnscaled;
	float DefaultCapsuleRadiusUnscaled;
	
	float DefaultCapsuleHalfHeightScaled;
	float DefaultCapsuleRadiusScaled;

	float RaiseLegsCapsuleHalfHeight;
	float RaiseLegsCapsuleRadius;

	float LastNonZeroHorizontal;
	float PreviousNonZeroHorizontal;
	
	FVector DefaultSkeletalMeshLocation;

	FString ClimbTransitionSocketName;
	bool bClimbTransitionAnchorRight;


	ECollisionEnabled::Type OriginalCollisionSetting;
};
