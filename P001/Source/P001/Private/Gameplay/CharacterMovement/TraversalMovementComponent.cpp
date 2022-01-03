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
#include "Kismet/GameplayStatics.h"

UTraversalMovementComponent::UTraversalMovementComponent()
{
	ClimbTransitionInputTime = 0.5f;
	LegRaiseHalfHeight = 50.f;
	CollisionCheckDrawMode = EDrawDebugTrace::Type::ForDuration;
	CollisionCheckDrawTime = 0.1f;
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
		if(CurrentActionPoint != nullptr)
		{	
			// use start settings here because it denotes our idle
			FVector PlayerOffsetFromWall = ClimbStartSettings.PlayerOffsetFromWall;

			FVector PlayerOffset = CurrentActionPoint->GetForwardVector() * PlayerOffsetFromWall.X
				+ CurrentActionPoint->GetRightVector() * PlayerOffsetFromWall.Y
				+ CurrentActionPoint->GetUpVector() * PlayerOffsetFromWall.Z;

			FVector TargetLocation = CurrentActionPoint->GetComponentLocation() + PlayerOffset;
			FQuat TargetRotation = (-CurrentActionPoint->GetForwardVector()).ToOrientationQuat();

			if(!bIsTransitioning)
			{
				// glue the player to the current point
				CharacterOwner->SetActorLocation(TargetLocation);
				CharacterOwner->SetActorRotation(TargetRotation);

				CurrentTransitionTime = 0.0f;

				// look for the next location the player can move to
				if(Horizontal != 0 || Vertical != 0)
				{
					CurrentClimbTransitionInputTime += DeltaTime;
					if(CurrentClimbTransitionInputTime >= (bFreezeClimbTransitions ? 0 : ClimbTransitionInputTime))
					{
						CurrentClimbTransitionInputTime = ClimbTransitionInputTime;

						//search radius should scale down with input - useful for joystick
						float MaxSearchRadius = ClimbTransitionSettings.MaxDistanceThreshold;
						FVector InputVector = (CharacterOwner->GetActorRightVector() * Horizontal
											+ CharacterOwner->GetActorUpVector() * Vertical)
											.GetSafeNormal();

						// Determine what action points are appropriate to climb to
						TMap<UActionPointComponent*, FTraverseActionPointData> ActioPointsInRangeAndDirection;

						TArray<UActionPointComponent*> ActionPointsToCheck = ActionPointsInClimbableSpace;
						ActionPointsToCheck.Remove(CurrentActionPoint);
						
						DrawDebugCylinder(GetWorld(), CurrentActionPoint->GetComponentLocation(), CurrentActionPoint->GetComponentLocation() + -CharacterOwner->GetActorForwardVector() * ClimbTransitionSettings.WallStepDepth, MaxSearchRadius, 30, FColor::Blue, false, -1.0f, (uint8)0U, 5);

						for (auto Comp : ActionPointsToCheck)
						{
							FVector ReferencePoint = CurrentActionPoint->GetComponentLocation();
							FVector ComponentLocation = Comp->GetComponentLocation();

							FVector RelativeCompLoc = ComponentLocation - ReferencePoint;
							FVector LateralDirection = FVector::CrossProduct(InputVector, -CharacterOwner->GetActorForwardVector());
					
							float DistanceToPoint = FVector::Distance(ComponentLocation, ReferencePoint);
							float LateralDistance = RelativeCompLoc.ProjectOnToNormal(LateralDirection).Size();
							float LateralThreshold = ClimbTransitionSettings.LateralThreshold;
							float Dot = FVector::DotProduct(RelativeCompLoc.GetSafeNormal(), InputVector);

							//TODO: Set MaxSearchRadius to the absolute value of the dot of the direction and the right vector
							// the player can reach horizontally further than they can vertically

							//TODO: Set LateralThreshold to the absolute value of a lerp(height, radius, dot(direction, input))
							// the player can reach horizontally further than they can vertically

							if(DistanceToPoint <= MaxSearchRadius && LateralDistance <= LateralThreshold && Dot > 0)
							{	
								// Cast to Point to see if it's possible
								FVector DepthCheckOffset = (CharacterOwner->GetActorForwardVector() * -ClimbTransitionSettings.WallStepDepth);
								FVector Start = CharacterOwner->GetActorLocation() + DepthCheckOffset;
								FVector End = ComponentLocation + PlayerOffset + DepthCheckOffset;

								FHitResult HitResult;
								TArray<AActor*> ActorsToIgnore;
								ActorsToIgnore.Add(CharacterOwner);
								for (auto Point : ActionPointsInClimbableSpace)
								{
									ActorsToIgnore.Add(Comp->GetOwner());
								}

								bool bCanReach = !UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(),
									Start, End,
									CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(),
									CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
									ETraceTypeQuery::TraceTypeQuery1,
									true,
									ActorsToIgnore,
									CollisionCheckDrawMode,
									HitResult,
									true,
									FLinearColor::Green, FLinearColor::Red, 0.1f
								);
								
								if(bCanReach)
								{
									FTraverseActionPointData Data;
									Data.Distance = DistanceToPoint;
									Data.LateralDistance = LateralDistance;
									Data.Score =  (LateralThreshold - Data.LateralDistance) + ( 1 - Data.Distance / MaxSearchRadius );
									Data.Direction = RelativeCompLoc;
									ActioPointsInRangeAndDirection.Add(Comp, Data);

									// debug it
									if(bLogPossibleClimbPoints)
									{
										FVector LateralEnd = ReferencePoint + (LateralDirection * 100);
										FVector InputEnd = ReferencePoint + InputVector * 100;
									
										DrawDebugLine(GetWorld(), ReferencePoint, InputEnd, FColor::Red);
										DRAW_SPHERE_YELLOW(ComponentLocation, 10, 0.1f);

										DEBUG_SCREEN_WARNING("----------------------", 0.2f);
										DEBUG_SCREEN_WARNING("Direction %s, ", 0.2f, *Data.Direction.ToString());
										DEBUG_SCREEN_WARNING("Score %f, ", 0.2f, Data.Score); 
										DEBUG_SCREEN_WARNING("Lateral Distance %f, ", 0.2f, Data.LateralDistance);
										DEBUG_SCREEN_WARNING("Distance %f, ", 0.2f, Data.Distance);
										DEBUG_SCREEN_WARNING("%s, ", 0.2f, *Comp->GetOwner()->GetName());
									}
								}
							}
						}

						// among them choose the best among them...
						TPair<UActionPointComponent*, FTraverseActionPointData> BestActionPoint;
						bool bHasOne = false;
						for(auto Pair : ActioPointsInRangeAndDirection)
						{
							if(!bHasOne)
							{
								BestActionPoint = Pair;
								bHasOne = true;
								continue;
							}

							if(Pair.Value.Score > BestActionPoint.Value.Score)
							{
								BestActionPoint = Pair;
							}

						}

						if(bHasOne)
						{
							DRAW_SPHERE_BLUE(BestActionPoint.Key->GetComponentLocation(), 20, 0.1f);

							if(!bFreezeClimbTransitions)
							{
								FVector ReferencePoint = CurrentActionPoint->GetComponentLocation();
								FVector ComponentLocation = BestActionPoint.Key->GetComponentLocation();
								FVector RelativeCompLoc = ComponentLocation - ReferencePoint;
								
								float DistanceToPoint = FVector::Distance(BestActionPoint.Key->GetComponentLocation(), ReferencePoint) / MaxSearchRadius;
								float Left = FVector::DotProduct(-CurrentActionPoint->GetRightVector(), RelativeCompLoc.GetSafeNormal());
								float Up = FVector::DotProduct(CurrentActionPoint->GetUpVector(), RelativeCompLoc.GetSafeNormal());

								FVector NormalizedClimbDirection = FVector(Left, 0.f, Up) * DistanceToPoint;

								if(bLogClimbTransitionAnimationValues)
								{
									DEBUG_SCREEN_ERROR("Distance: %f", 10, DistanceToPoint);
									DEBUG_SCREEN_ERROR("Right   : %f", 10, NormalizedClimbDirection.X);
									DEBUG_SCREEN_ERROR("Up      : %f", 10, NormalizedClimbDirection.Z);
								}

								StartClimbTransition(BestActionPoint.Key, NormalizedClimbDirection, DistanceToPoint);
							}
						
							CurrentClimbTransitionInputTime = 0;
						}
					}
				}
				else
				{
					CurrentClimbTransitionInputTime = 0;	
				}
			}
			else
			{
				CurrentClimbTransitionInputTime = 0;

				CurrentTransitionTime += DeltaTime;
				if(CurrentTransitionTime > ClimbTransitionTime)
				{
					CurrentTransitionTime = ClimbTransitionTime;
				}

				float LerpDelta = CurrentTransitionTime / ClimbTransitionTime;

				// move for transition
				PlayerOffset = CurrentActionPointTransitioningTo->GetForwardVector() * PlayerOffsetFromWall.X
							 + CurrentActionPointTransitioningTo->GetRightVector() * PlayerOffsetFromWall.Y
							 + CurrentActionPointTransitioningTo->GetUpVector() * PlayerOffsetFromWall.Z;
				
				TargetLocation = CurrentActionPointTransitioningTo->GetComponentLocation() + PlayerOffset;
				TargetLocation = FMath::Lerp(CharacterOwner->GetActorLocation(), TargetLocation, LerpDelta);
				//CharacterOwner->SetActorLocation(TargetLocation);

				if(CurrentTransitionTime == ClimbTransitionTime)
				{
					EndClimbTransition();
				}
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
		CharacterOwner->GetRootComponent()->ComponentVelocity = FVector(0.f);
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

void UTraversalMovementComponent::StartClimbTransition(UActionPointComponent* NewActionPoint, FVector ClimbDirection, float TransitionTime)
{
	CurrentActionPointTransitioningTo = NewActionPoint;
	bIsTransitioning = true;
	ClimbTransitionTime = TransitionTime;
	CurrentClimbTransitionInputTime = 0.f;

	OnWallClimbStartTransition.Broadcast(ClimbDirection.X, ClimbDirection.Z);
}

void UTraversalMovementComponent::EndClimbTransition()
{
	if(CurrentActionPointTransitioningTo != nullptr)
	{
		SetCurrentActionPoint(CurrentActionPointTransitioningTo);
		CurrentActionPointTransitioningTo = nullptr;
		bIsTransitioning = false;

		OnWallClimbEndTransition.Broadcast();
	}
}

void UTraversalMovementComponent::FreezeClimb(bool bFreeze)
{
	bFreezeClimbTransitions = bFreeze;
}

void UTraversalMovementComponent::ShowArrow(bool bShow)
{
	bShowArrow = bShow;
}
