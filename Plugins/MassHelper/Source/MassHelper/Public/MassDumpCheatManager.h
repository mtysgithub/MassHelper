// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "MassDumpCheatManager.generated.h"

/**
 * Extension of the CheatManager class that enables custom console commands and debug functions for development use.
 */
UCLASS()
class MASSHELPER_API UMassDumpCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UMassDumpCheatManager();

	UFUNCTION(exec)
	virtual void DumpStaticProcessorsDependencyByPhaseID(int PhaseID);

	UFUNCTION(exec)
	virtual void DumpRuntimeProcessorsDependencyByPhaseID(int PhaseID);

private:
	void DoDumpProcessorsDependency(int PhaseID, FString& ToSaveFileName);
};
