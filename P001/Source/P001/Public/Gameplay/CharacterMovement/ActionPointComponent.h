// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "ActionPointComponent.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class P001_API UActionPointComponent : public USphereComponent
{
	GENERATED_BODY()
	
	UActionPointComponent();

protected:
	virtual void BeginPlay() override;

	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult);

};
