// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/CharacterMovement/ActionPointComponent.h"

UActionPointComponent::UActionPointComponent()
{
	SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

void UActionPointComponent::BeginPlay()
{
	Super::BeginPlay();
}