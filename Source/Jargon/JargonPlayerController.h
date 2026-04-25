// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/JargonExplorationPlayerController.h"
#include "JargonPlayerController.generated.h"

/**
 * Legacy compatibility wrapper kept so existing template-derived assets keep working
 * while exploration ownership moves to the explicit Jargon exploration classes.
 */
UCLASS(abstract)
class AJargonPlayerController : public AJargonExplorationPlayerController
{
	GENERATED_BODY()
};
