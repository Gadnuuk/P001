// Copyright Epic Games, Inc. All Rights Reserved.

#include "P001Character.h"

#include "Gameplay/CharacterMovement/TraversalMovementComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// AP001Character

AP001Character::AP001Character()
{
}

AP001Character::AP001Character(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTraversalMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;


	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	ActionPointCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Action Point Collision"));
	ActionPointCollision->SetupAttachment(GetRootComponent());
	ActionPointCollision->SetSphereRadius(500.f);	
	// traversal options set
	GetTraversalMovementComponent()->ActionPointDetectionCollision = ActionPointCollision;
	GetTraversalMovementComponent()->bCanEverRaiseLegs = true;

	GetTraversalMovementComponent()->ClimbStartSettings.PlayerOffsetFromWall = FVector(28.169436f, 0.f, -86.333076f);
	GetTraversalMovementComponent()->ClimbStartSettings.MaxDistanceThreshold = 200.f;
	GetTraversalMovementComponent()->ClimbStartSettings.MaxDotThreshold = 0.5f;
	GetTraversalMovementComponent()->ClimbStartSettings.MaxHeightThreshold = 75.0f;
	GetTraversalMovementComponent()->ClimbStartSettings.MinHeightThreshold = -50.0f;
	GetTraversalMovementComponent()->ClimbStartSettings.FallingHeightScalar = 1.5f;
	GetTraversalMovementComponent()->ClimbStartSettings.LateralThreshold = 100.f;
	GetTraversalMovementComponent()->ClimbStartSettings.MaxDistanceScalar = 1.5f;
	
	GetTraversalMovementComponent()->ClimbTransitionSettings.PlayerOffsetFromWall = FVector(50.f, 0, -50.f);
	GetTraversalMovementComponent()->ClimbTransitionSettings.MaxDistanceThreshold = 200.f;
	GetTraversalMovementComponent()->ClimbTransitionSettings.MinDistanceThreshold = GetCapsuleComponent()->GetScaledCapsuleRadius();
	GetTraversalMovementComponent()->ClimbTransitionSettings.MaxDotThreshold = 0.5f;
	GetTraversalMovementComponent()->ClimbTransitionSettings.MaxHeightThreshold = 75.0f;
	GetTraversalMovementComponent()->ClimbTransitionSettings.MinHeightThreshold = -50.0f;
	GetTraversalMovementComponent()->ClimbTransitionSettings.FallingHeightScalar = 1.5f;
	GetTraversalMovementComponent()->ClimbTransitionSettings.LateralThreshold = 100.f;
	GetTraversalMovementComponent()->ClimbTransitionSettings.WallStepDepth = 30.f;
	GetTraversalMovementComponent()->ClimbTransitionSettings.MaxDistanceScalar = 1.5f;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AP001Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AP001Character::StartJump); // ACharacter::Jump handled by animation event.
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AP001Character::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AP001Character::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AP001Character::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AP001Character::LookUpAtRate);
}

class UTraversalMovementComponent* AP001Character::GetTraversalMovementComponent()
{
	return Cast<UTraversalMovementComponent>(GetCharacterMovement());
}

void AP001Character::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AP001Character::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AP001Character::StartJump()
{	
	GetTraversalMovementComponent()->Jump();
}

void AP001Character::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f) && (!GetTraversalMovementComponent()->GetIsClimbing()))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
	else if ((Controller != nullptr))
	{
		// apply move vector to traversal move component
		GetTraversalMovementComponent()->AddClimbVerticalInput(Value);
	}
}

void AP001Character::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) && (!GetTraversalMovementComponent()->GetIsClimbing()))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);

	}
	else if((Controller != nullptr))
	{
		// apply move vector to traversal move component
		GetTraversalMovementComponent()->AddClimbHorizontalInput(Value);
	}
}
