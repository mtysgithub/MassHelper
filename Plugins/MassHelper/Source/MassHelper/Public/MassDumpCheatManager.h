// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "MassHelper/Public/Processor/MassProcessorDependencyPrinter.h"
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
	void DumpStaticProcessorExecutesGroupTreeByPhaseID(int PhaseID);

	UFUNCTION(exec)
	void DumpStaticProcessorDependencyByPhaseID(int PhaseID);

	UFUNCTION(exec)
	void DumpRuntimeProcessorDependencyByPhaseID(int PhaseID);

private:
	void DoPrint(int PhaseID, FString& ToSaveFileName, EPrintMode PrintMode);
};
