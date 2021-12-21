// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/CharacterMovement/ActionPointComponent.h"

UActionPointComponent::UActionPointComponent()
{
	//OnComponentBeginOverlap->AddDynamic(this, &UActionPointComponent::)
}

void UActionPointComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UActionPointComponent::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

}
