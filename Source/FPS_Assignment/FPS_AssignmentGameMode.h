// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPS_AssignmentGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AFPS_AssignmentGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPS_AssignmentGameMode();
};



