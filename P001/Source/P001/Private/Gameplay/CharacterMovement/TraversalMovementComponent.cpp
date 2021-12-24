// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/Gameplay/CharacterMovement/TraversalMovementComponent.h"
#include "Public/Gameplay/CharacterMovement/TraversalMovement.h"
#include "Public/Gameplay/CharacterMovement/ActionPointComponent.h"
#include "P001.h"

// used for a float limit
#include <limits>

#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

UTraversalMovementComponent::UTraversalMovementComponent()
{
	LegRaiseHalfHeight = 50.f;
}

void UTraversalMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if(ActionPointDetectionCollision == nullptr)
	{
		DEBUG_SCREEN_ERROR("Please Assign a sphere component to the Traversal Movement Component to use it...", 10);
	}
	else
	{
		InitActionPointDetection();
	}

	// tell the arrow component to show, this is usfull for displaying the characters center mass.
	CharacterOwner->GetArrowComponent()->SetHiddenInGame(!bShowArrow);

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
	TickClimbPosition(DeltaTime);

	// if we are in the walk state
	{
		// handle the players orientation
		if( (IsFalling()|| bIsClimbing) && bOrientRotationToMovement)
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

	//SweepForGround(Result);

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

				CharacterOwner->SetActorLocation(TargetLocation);
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
	}
}


void UTraversalMovementComponent::SetCurrentActionPoint(UActionPointComponent* NewActionPoint)
{
	// allow nullptr to be passed here to allow for clean up

	if(CurrentActionPoint != nullptr)
	{
		CurrentActionPoint->SetIsBusy(false);
	}

	CurrentActionPoint = NewActionPoint;
	if(CurrentActionPoint != nullptr)
	{
		CurrentActionPoint->SetIsBusy(true);
	}
}

void UTraversalMovementComponent::TickClimbPosition(float DeltaTime)
{
	// move the player to the current point
	if(bIsClimbing)
	{
		if(CurrentActionPoint != nullptr && !bIsTransitioning)
		{	
			// use start settings here because it denotes our idle
			FVector PlayerOffsetFromWall = ClimbStartSettings.PlayerOffsetFromWall;

			// glue the player to the current point
			FVector PlayerOffset = CurrentActionPoint->GetForwardVector() * PlayerOffsetFromWall.X 
								 + CurrentActionPoint->GetRightVector() * PlayerOffsetFromWall.Y
								 + CurrentActionPoint->GetUpVector() * PlayerOffsetFromWall.Z;

			FVector TargetLocation = CurrentActionPoint->GetComponentLocation() + PlayerOffset;
			FQuat TargetRotation = (-CurrentActionPoint->GetForwardVector()).ToOrientationQuat();

			CharacterOwner->SetActorLocation(TargetLocation);
			CharacterOwner->SetActorRotation(TargetRotation);

			// look for the next location the player can move to
			if(Horizontal != 0 || Vertical != 0)
			{
				//search radius should scale down with input - useful for joystick
				float MaxSearchRadius = FMath::Clamp(ClimbTransitionSettings.MaxDistanceThreshold * FVector2D(Horizontal, Vertical).Size(),
										0.f, ClimbTransitionSettings.MaxDistanceThreshold);
				FVector InputVector = (CharacterOwner->GetActorRightVector() * Horizontal
									+ CharacterOwner->GetActorUpVector() * Vertical)
									.GetSafeNormal();
				FVector Start = CharacterOwner->GetActorLocation() + (CharacterOwner->GetActorForwardVector() * -ClimbTransitionSettings.WallStepDepth);
				FVector End = (InputVector * MaxSearchRadius) + Start;

				// sweep for collision in the shape of our capsule to see if we need to adjust where we are looking
				FHitResult HitResult;
				TArray<AActor*> ActorsToIgnore;
				ActorsToIgnore.Add(CharacterOwner);
				for (auto Actor : ActionPointsInClimbableSpace)
				{
					ActorsToIgnore.Add(Actor->GetOwner());
				}
				

				if(UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(),
					Start, End,
					CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(),
					CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
					ETraceTypeQuery::TraceTypeQuery1,
					true,
					ActorsToIgnore,
					EDrawDebugTrace::Type::ForDuration,
					HitResult,
					true,
					FLinearColor::Green, FLinearColor::Red, 0.1f
				))
				{
					MaxSearchRadius = FVector::Distance(Start, HitResult.Location);
				}

				//DrawDebugLine(GetWorld(), Start, End, FColor::Red);
				//DRAW_SPHERE_BLUE(Start, MaxSearchRadius, -1.0);
				DrawDebugCylinder(GetWorld(), Start, Start + -CharacterOwner->GetActorForwardVector() * ClimbTransitionSettings.WallStepDepth, MaxSearchRadius, 30, FColor::Blue, false, -1.0f, (uint8)0U, 10);
			}
		}
	}
	else
	{
		if(IsFalling())
		{
			FVector Vel = CharacterOwner->GetVelocity() * DeltaTime * ClimbStartSettings.FallingHeightScalar;

			FVector TopVel = Vel;
			if(TopVel.Z < 0)
			{
				TopVel *= 2.;
			}

			// draw top line
			FVector Start = CharacterOwner->GetActorLocation() + FVector(0.f, 0.f, FMath::Clamp(ClimbStartSettings.MaxHeightThreshold + Vel.Z, ClimbStartSettings.MaxHeightThreshold,  std::numeric_limits<float>::max()));
			FVector End = (Start + CharacterOwner->GetActorForwardVector() * 100);
			DrawDebugLine(GetWorld(), Start, End, FColor::Red);

			CurrentMaxHeightToClimb = Start.Z;

			// draw bottom line
			Start = CharacterOwner->GetActorLocation() + FVector(0.f, 0.f, ClimbStartSettings.MinHeightThreshold + Vel.Z);
			End = (Start + CharacterOwner->GetActorForwardVector() * 100);
			DrawDebugLine(GetWorld(), Start, End, FColor::Red);

			CurrentMinHeightToClimb = Start.Z;
		}
		else
		{
			FVector Start = CharacterOwner->GetActorLocation() + FVector(0.f, 0.f, ClimbStartSettings.MaxHeightThreshold);
			FVector End = Start + CharacterOwner->GetActorForwardVector() * 100;
		
			Start = CharacterOwner->GetActorLocation() + FVector(0.f, 0.f, ClimbStartSettings.MinHeightThreshold);
			End = Start + CharacterOwner->GetActorForwardVector() * 100;
		}
	}
}

void UTraversalMovementComponent::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	auto PossibleActionPoint = Cast<UActionPointComponent>(OtherComp);
	if(PossibleActionPoint != nullptr)
	{
		ActionPointsInClimbableSpace.AddUnique(PossibleActionPoint);
	}
}	

void UTraversalMovementComponent::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	auto PossibleActionPoint = Cast<UActionPointComponent>(OtherComp);
	if (PossibleActionPoint != nullptr && ActionPointsInClimbableSpace.Contains(PossibleActionPoint))
	{
		if(CurrentActionPoint == PossibleActionPoint)
		{
			StopClimbing();
		}

		ActionPointsInClimbableSpace.Remove(PossibleActionPoint);
	}
}

void UTraversalMovementComponent::Jump()
{	
	if(IsFalling())
	{
		return;
	}
	
	if(GetIsClimbing())
	{
		StopClimbing();
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
	const float ClampedRaisedLegsHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, LegRaiseHalfHeight);
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, ClampedRaisedLegsHalfHeight);

	RaiseLegsCapsuleHalfHeight = OldUnscaledRadius;
	RaiseLegsCapsuleRadius = ClampedRaisedLegsHalfHeight;

	// move skeletal mesh down
	float HalfHeightAdjust = (OldUnscaledHalfHeight - ClampedRaisedLegsHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	FVector CurrentLoc = CharacterOwner->GetMesh()->GetRelativeLocation();

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
	
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCapsuleRadiusUnscaled, DefaultCapsuleHalfHeightUnscaled);

	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float OldUnscaledRadius = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();

	const float ClampedCrouchedHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, CrouchedHalfHeight);
	float HalfHeightAdjust = (OldUnscaledHalfHeight - ClampedCrouchedHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	bHasLegsRaised = false;
	OnStopLegRaise.Broadcast();
}


void UTraversalMovementComponent::StartClimbing()
{
	UActionPointComponent* TargetActionPoint = nullptr;
	float BestDistanceToPoint = 0.0f;
	float BestLatDistToPoint = 0.0f;

	// attempt to mount to an action point
	for(auto ActionPoint : ActionPointsInClimbableSpace)
	{
		FVector CharacterLoc = CharacterOwner->GetActorLocation();
		FVector PointLoc = ActionPoint->GetComponentLocation();
		FVector CharacterForward = FVector(CharacterOwner->GetActorForwardVector().X, CharacterOwner->GetActorForwardVector().Y, 0.f).GetSafeNormal();
		FVector DirToPoint = (PointLoc - CharacterLoc).GetSafeNormal();
		FVector PointForward = ActionPoint->GetForwardVector();
		
		float DistanceToPoint = FVector::Distance(CharacterLoc, PointLoc);
		float DotToPoint = FVector::DotProduct( FVector(CharacterForward.X, CharacterForward.Y, 0.f), FVector(DirToPoint.X, DirToPoint.Y, 0.f) );
		bool bLookUp = (IsFalling() && CharacterOwner->GetVelocity().Z > 0) || !IsFalling();

		// scale distance to point based on dot from vel and dir to point
		float RealDotToPoint = FVector::DotProduct(CharacterOwner->GetVelocity().GetSafeNormal(), DirToPoint);

		float LateralDistance = ((PointLoc - CharacterLoc).ProjectOnTo(CharacterOwner->GetActorRightVector())).Size();

		// check the distance plus the players velocity		
		float DistanceScale = FMath::Clamp(
			FVector(CharacterOwner->GetVelocity().X, CharacterOwner->GetVelocity().Y, 0.f).Size() * GetWorld()->GetDeltaSeconds(),
			0.f, 
			ClimbStartSettings.MaxDistanceScalar);
		float DistanceToCheck = FMath::Clamp(ClimbStartSettings.MaxDistanceThreshold * DistanceScale, ClimbStartSettings.MaxDistanceThreshold, std::numeric_limits<float>::max()) ;
		DEBUG_SCREEN_LOG("DISTANCE TO CHECK: %f", 10, DistanceToCheck);

		bool bIsCandidate = !ActionPoint->GetIsBusy() 
							&& DistanceToPoint <= DistanceToCheck
							&& PointLoc.Z <= CurrentMaxHeightToClimb 
							&& PointLoc.Z >= CurrentMinHeightToClimb
							&& LateralDistance <= ClimbStartSettings.LateralThreshold * 0.5f;

		if(bIsCandidate)
		{
			bool bIsBetter = (DistanceToPoint <= BestDistanceToPoint) && (LateralDistance < BestLatDistToPoint);

			if(TargetActionPoint == nullptr || bIsBetter)
			{
				TargetActionPoint = ActionPoint;
				BestDistanceToPoint = DistanceToPoint;
				BestLatDistToPoint = LateralDistance;
				continue;
			}
		}
		else
		{
#if WITH_EDITOR
			//DEBUG_SCREEN_ERROR("Failed To Start Climb......", 10);
			//DEBUG_SCREEN_ERROR("Distance To Point: %f", 10, DistanceToPoint);
			//DEBUG_SCREEN_ERROR("Height To Point: %f", 10, PointLoc.Z);
			//DEBUG_SCREEN_ERROR("Currrent Max Height: %f", 10, CurrentMaxHeightToClimb);
			//DEBUG_SCREEN_ERROR("Currrent Min Height: %f", 10, CurrentMinHeightToClimb);
			//DEBUG_SCREEN_ERROR("Lateral Distance of Point: %f", 10, LateralDistance);

			//DrawDebugLine(GetWorld(), CharacterOwner->GetActorLocation(), CharacterOwner->GetActorLocation() + CharacterOwner->GetActorRightVector() * LateralDistance, FColor::Red, true);
#endif // WITH_EDITOR
		}
	}

	// see if there are any close enough,
	// see if they are in front of us
	// see if its a good height
	if(TargetActionPoint != nullptr)
	{
		SetCurrentActionPoint(TargetActionPoint);
		bIsClimbing = true;
		RaiseLegs();
	}
}

void UTraversalMovementComponent::StopClimbing()
{
	bIsClimbing = false;
	DropLegs();
	SetCurrentActionPoint(nullptr);
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
}

bool UTraversalMovementComponent::SweepForGround(FVector& SweepVelocity) const
{
	FHitResult HitResult;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(CharacterOwner);
	for(auto Actor : ActionPointsInClimbableSpace)
	{
		ActorsToIgnore.Add(Actor->GetOwner());
	}

	FVector Start = CharacterOwner->GetActorLocation();
	FVector End = Start - FVector(0, 0, DefaultCapsuleHalfHeightScaled);

	bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(),
		Start, End,
		DefaultCapsuleRadiusScaled,
		DefaultCapsuleHalfHeightScaled,
		ETraceTypeQuery::TraceTypeQuery1,
		true,
		ActorsToIgnore,
		EDrawDebugTrace::Type::None,
		HitResult,
		true,
		FLinearColor::Red, FLinearColor::Green, 5
	);

	if (bHit)
	{
		//DrawDebugLine(GetWorld(), Start, End, FColor::Red);
		//DrawDebugLine(GetWorld(), Start, Start + SweepVelocity, FColor::Red);
		/*DRAW_SPHERE_RED(End, 10, 1.f);
		DRAW_SPHERE_BLUE(End - FVector(0, 0, DefaultCapsuleHalfHeightScaled), 10, 1.f);*/
		//SweepVelocity.Z = FMath::Clamp(SweepVelocity.Z, 0.f, std::numeric_limits<float>::max());
		//DRAW_SPHERE_GREEN(HitResult.ImpactPoint, 10, 1.f);
		SweepVelocity -= (End - HitResult.ImpactPoint) - FVector(0, 0, DefaultCapsuleHalfHeightScaled);
	}

	return bHit;
}

bool UTraversalMovementComponent::InitActionPointDetection()
{
	if(ActionPointDetectionCollision != nullptr)
	{
		// get any current overlapped collisions, the callbacks will miss these...
		TArray<UPrimitiveComponent*> OverlappingCollisions;
		ActionPointDetectionCollision->GetOverlappingComponents(OverlappingCollisions);
		for(auto Comp : OverlappingCollisions)
		{
			auto ActionPoint = Cast<UActionPointComponent>(Comp);
			if(ActionPoint != nullptr)
			{
				ActionPointsInClimbableSpace.AddUnique(ActionPoint);
			}
		}

		ActionPointDetectionCollision->OnComponentBeginOverlap.AddDynamic(this, &UTraversalMovementComponent::OnBeginOverlap);
		ActionPointDetectionCollision->OnComponentEndOverlap.AddDynamic(this, &UTraversalMovementComponent::OnEndOverlap);
		return true;
	}

	return false;
}

void UTraversalMovementComponent::AddClimbHorizontalInput(float NewHorizontal)
{
	Horizontal = NewHorizontal;
}

void UTraversalMovementComponent::AddClimbVerticalInput(float NewVertical)
{
	Vertical = NewVertical;
}