// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


DECLARE_LOG_CATEGORY_EXTERN(GameDebugLog, Log, All);

/** Log Macros for this module. */
#define DEBUG_LOG(x, ...) UE_LOG(GameDebugLog, Log, TEXT(x), ##__VA_ARGS__ );
#define DEBUG_WARNING(x, ...) UE_LOG(GameDebugLog, Warning, TEXT(x), ##__VA_ARGS__ );
#define DEBUG_ERROR(x, ...) UE_LOG(GameDebugLog, Error, TEXT(x), ##__VA_ARGS__ );

// logs that will only appear in the editor.
#if WITH_EDITOR
#define DEBUG_EDITOR_LOG(x, ...) UE_LOG(GameDebugLog, Log, TEXT(x), ##__VA_ARGS__ );
#define DEBUG_EDITOR_WARNING(x, ...)  UE_LOG(GameDebugLog, Warning, TEXT(x), ##__VA_ARGS__ );
#define DEBUG_EDITOR_ERROR(x, ...) SR_ERROR(x, ...)  UE_LOG(GameDebugLog, Error, TEXT(x), ##__VA_ARGS__ );
#else
#define DEBUG_EDITOR_LOG(x, ...)
#define DEBUG_EDITOR_WARNING(x, ...)
#define DEBUG_EDITOR_ERROR(x, ...)
#endif

/** ScreenLog Macros for this module. */
#define DEBUG_SCREEN_LOG(x, t, ...) GEngine->AddOnScreenDebugMessage( -1, t, FColor::Blue, FString("[DEBUG_LOG] ").Append( FString::Printf( TEXT( x ), ##__VA_ARGS__ ) ) );
#define DEBUG_SCREEN_WARNING(x, t, ...) GEngine->AddOnScreenDebugMessage( -1, t, FColor::Yellow, FString("[DEBUG_WARNING] ").Append( FString::Printf( TEXT( x ), ##__VA_ARGS__ ) ) );
#define DEBUG_SCREEN_ERROR(x, t, ...) GEngine->AddOnScreenDebugMessage( -1, t, FColor::Red, FString("[DEBUG_ERROR] ").Append( FString::Printf( TEXT( x ), ##__VA_ARGS__ ) ) );

#define DRAW_SPHERE_BLUE(Loc, Rad, LifeTime) DrawDebugSphere(GetWorld(), Loc, Rad, 20, FColor::Blue, false, LifeTime, 0, 1);	
#define DRAW_SPHERE_YELLOW(Loc, Rad, LifeTime) DrawDebugSphere(GetWorld(), Loc, Rad, 20, FColor::Yellow, false, LifeTime, 0, 1);	
#define DRAW_SPHERE_RED(Loc, Rad, LifeTime) DrawDebugSphere(GetWorld(), Loc, Rad, 20, FColor::Red, false, LifeTime, 0, 1);	
#define DRAW_SPHERE_GREEN(Loc, Rad, LifeTime) DrawDebugSphere(GetWorld(), Loc, Rad, 20, FColor::Green, false, LifeTime, 0, 1);	

#define NEW_OBJECT(T, C) NewObject<T>( (UObject*)GetTransientPackage(), C )
#define CONSTRUCT_OBJECT(T) NewObject<T>( (UObject*)GetTransientPackage(), T::StaticClass() )

#define FORMAT(x, ...) FString::Printf(TEXT(x), ##__VA_ARGS__ );