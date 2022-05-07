// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TraversalMovement.h"

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "ActionPointComponent.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class P001_API UActionPointComponent : public USphereComponent
{
	GENERATED_BODY()
	
public:
	UActionPointComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite);
	USceneComponent* OffsetCenterMass;

	UFUNCTION(BlueprintPure)
	TEnumAsByte<EActionPointType> GetActionPointType() {return ActionPointType;}

	UFUNCTION(BlueprintPure)
	bool GetIsBusy(){return bIsBusy;}
	
	UFUNCTION(BlueprintCallable)
	void SetIsBusy(bool bValue){bIsBusy = bValue;}

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EActionPointType> ActionPointType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsBusy;

	virtual void BeginPlay() override;
};
