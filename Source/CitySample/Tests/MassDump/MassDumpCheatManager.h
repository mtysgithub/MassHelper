// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "MassDumpCheatManager.generated.h"

/**
 * Extension of the CheatManager class that enables custom console commands and debug functions for development use.
 */
UCLASS()
class CITYSAMPLE_API UMassDumpCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UMassDumpCheatManager();

	UFUNCTION(exec)
	virtual void DumpPhaseProcessorByID(int PhaseID);
};
