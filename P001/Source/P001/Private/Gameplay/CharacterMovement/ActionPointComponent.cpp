// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/CharacterMovement/ActionPointComponent.h"

UActionPointComponent::UActionPointComponent()
{
	SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

void UActionPointComponent::BeginPlay()
{
	Super::BeginPlay();
}