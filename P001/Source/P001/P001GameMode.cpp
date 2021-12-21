// Copyright Epic Games, Inc. All Rights Reserved.

#include "P001GameMode.h"
#include "P001Character.h"
#include "UObject/ConstructorHelpers.h"

AP001GameMode::AP001GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/P001/Blueprints/Pawns/Player/BP_Player"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
