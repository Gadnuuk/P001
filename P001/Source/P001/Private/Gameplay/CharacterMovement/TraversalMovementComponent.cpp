// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/Gameplay/CharacterMovement/TraversalMovementComponent.h"
#include "Public/Gameplay/CharacterMovement/TraversalMovement.h"
#include "Public/Gameplay/CharacterMovement/ActionPointComponent.h"
#include "P001.h"

#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

void UTraversalMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// find all action point components
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActionPointActorClass, FoundActors);
	for(AActor* Actor : FoundActors)
	{
		auto ActionComponents = Actor->GetComponentsByClass(ActionPointComponentClass);
		for(auto Comp : ActionComponents)
		{
			ActionPoints.Add(Cast<USceneComponent>(Comp));
		}
	}


	CameraBoom = Cast<USpringArmComponent>(CharacterOwner->GetComponentByClass(USpringArmComponent::StaticClass()));

	if(CameraBoom != nullptr)
	{
		DefaultCameraBoomLocation = CameraBoom->GetRelativeLocation();
	}

	DefaultCapsuleHalfHeightUnscaled = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	DefaultCapsuleRadiusUnscaled = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();

	DefaultCapsuleHalfHeightScaled = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	DefaultCapsuleRadiusScaled = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();

	DefaultSkeletalMeshLocation = CharacterOwner->GetMesh()->GetRelativeLocation();
}


void UTraversalMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction );

	TickMatchTarget(DeltaTime);

	// if we are in the walk state
	{
		// handle the players orientation
		if(IsFalling() && bOrientRotationToMovement)
		{
			// dont rotate with intput
			bOrientRotationToMovement = false;
		}
		else if(!IsFalling() && !bOrientRotationToMovement)
		{
			bOrientRotationToMovement = true;
		}
	}
}


FVector UTraversalMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const
{
	FVector Result = Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);

	SweepForGround(Result);

	return Result;
}

void UTraversalMovementComponent::TickMatchTarget(float DeltaTime)
{
	TArray<FMatchTargetData> DataToRemove;

	for(FMatchTargetData& Data : MatchTargets)
	{
		// check if the animation is playing
		UAnimMontage* ActiveMontage = CharacterOwner->GetMesh()->GetAnimInstance()->GetCurrentActiveMontage();
		if(ActiveMontage == Data.AnimationMontage)
		{
			// increment time
			Data.CurrentTime = FMath::Clamp(Data.CurrentTime + DeltaTime, 0.f, Data.MatchTime);
			if(Data.CurrentTime >= Data.MatchTime)
			{
				Data.bHandled = true;
			}
			
			if(!Data.bHandled)
			{
				float LerpAmount = FMath::Clamp(Data.CurrentTime / Data.MatchTime, 0.f, 1.f);

				FVector TargetLocation = Data.MatchTarget->GetComponentLocation();
				FVector CharacterLocation = CharacterOwner->GetActorLocation();
				FVector JointLocation = CharacterOwner->GetMesh()->GetSocketLocation(Data.SocketName);
				FVector Offset = JointLocation - CharacterLocation;

				TargetLocation = FMath::Lerp(CharacterLocation, TargetLocation - Offset, LerpAmount);

				// ensure we arent dropping the players capsule below the ground
				//if(SweepForGround(TargetLocation/*, Data*/))
				//{
				//	DEBUG_SCREEN_LOG("Attempted to enter the ground...", 1);
				//}

				CharacterOwner->SetActorLocation(TargetLocation);
			
				// make sure we are controlling the characters collision
				if(!CharacterOwner->bIsCrouched && Data.bCrouch)
				{
					CharacterOwner->Crouch();
				}
				if(!bHasLegsRaised && Data.bRaiseLegs)
				{
					RaiseLegs();
				}
			}
		}
		else
		{
			DataToRemove.Add(Data);
		}
	}

	for(FMatchTargetData& Data : DataToRemove)
	{
		MatchTargets.Remove(Data);

		if(Data.bCrouch)
		{
			CharacterOwner->UnCrouch();
		}

		if(Data.bRaiseLegs)
		{
			DropLegs();
		}
	}
}


void UTraversalMovementComponent::Jump()
{	
	if(IsFalling())
	{
		return;
	}

	InitJumpEvent.Broadcast();
	CharacterOwner->Jump();
}


void UTraversalMovementComponent::RaiseLegs()
{
	if(!bCanEverRaiseLegs)
	{
		return;
	}

	if(bHasLegsRaised)
	{
		return;
	}

	// set the movement move to flying
	SetMovementMode(MOVE_Flying);

	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float OldUnscaledRadius = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();

	// Height is not allowed to be smaller than radius.
	const float ClampedCrouchedHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, CrouchedHalfHeight);
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, ClampedCrouchedHalfHeight);

	// move skeletal mesh down
	float HalfHeightAdjust = (OldUnscaledHalfHeight - ClampedCrouchedHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	FVector CurrentLoc = CharacterOwner->GetMesh()->GetRelativeLocation();
	CharacterOwner->GetMesh()->SetRelativeLocation(CurrentLoc + FVector(0.f, 0.f, -ScaledHalfHeightAdjust));
	
	CharacterOwner->GetCapsuleComponent()->MoveComponent(
		FVector(0.f, 0.f, ScaledHalfHeightAdjust),
		UpdatedComponent->GetComponentQuat(),
		true,
		nullptr,
		EMoveComponentFlags::MOVECOMP_NoFlags,
		ETeleportType::TeleportPhysics);

	bHasLegsRaised = true;
	OnStartLegRaise.Broadcast();
}


void UTraversalMovementComponent::DropLegs()
{
	if (!bCanEverRaiseLegs)
	{
		return;
	}

	if (!bHasLegsRaised)
	{
		return;
	}

	SetMovementMode(MOVE_Walking);
	
	CharacterOwner->GetMesh()->SetRelativeLocation(DefaultSkeletalMeshLocation);
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCapsuleRadiusUnscaled, DefaultCapsuleHalfHeightUnscaled);

	bHasLegsRaised = false;
	OnStopLegRaise.Broadcast();
}


void UTraversalMovementComponent::PlayMontageWithTargetMatch(const FMatchTargetData& Data)
{
	if(Data.AnimationMontage == nullptr || Data.MatchTarget == nullptr)
	{
		DEBUG_SCREEN_ERROR("Cannot Play Montage with no montage or match target...", 10);
		return;
	}

	// add to list
	MatchTargets.Add(Data);

	// play montage
	CharacterOwner->GetMesh()->GetAnimInstance()->Montage_Play(Data.AnimationMontage);

	if(Data.bCrouch)
	{
		CharacterOwner->Crouch();
	}

	if(Data.bRaiseLegs)
	{
		RaiseLegs();
	}
}

bool UTraversalMovementComponent::SweepForGround(FVector& SweepVelocity) const
{
	FHitResult HitResult;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(CharacterOwner);
	for(auto Actor : ActionPoints)
	{
		ActorsToIgnore.Add(Actor->GetOwner());
	}

	FVector Start = CharacterOwner->GetActorLocation();

	// do a collision sweep and offset the in vector
	bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(),
	Start, Start + SweepVelocity,
	DefaultCapsuleRadiusScaled,
	DefaultCapsuleHalfHeightScaled, 
	ETraceTypeQuery::TraceTypeQuery1,
	true,
	ActorsToIgnore,
	EDrawDebugTrace::Type::ForDuration,
	HitResult,
	true,
	FLinearColor::Red, FLinearColor::Green, 1
	);

	if(bHit && FVector::DotProduct(HitResult.ImpactNormal.GetSafeNormal(), FVector::UpVector) >= 1)
	{
		SweepVelocity.Z = 0;
		DRAW_SPHERE_GREEN(HitResult.ImpactPoint, 30, 1);
	}

	return bHit;
}
