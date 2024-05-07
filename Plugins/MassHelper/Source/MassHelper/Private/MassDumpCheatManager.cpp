// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassHelper/Public/MassDumpCheatManager.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "MassSpawner.h"

#include "MassEntitySettings.h"
#include "MassSimulation/Public/MassSimulationSubsystem.h"
#include "MassEntity/Public/MassEntitySubsystem.h"

UMassDumpCheatManager::UMassDumpCheatManager()
{

}

void UMassDumpCheatManager::DumpStaticProcessorsDependencyByPhaseID(int PhaseID)
{ 
	UMassSimulationSubsystem* MassSimulationSubsystem = UWorld::GetSubsystem<UMassSimulationSubsystem>(this->GetWorld());
	if (ensure(MassSimulationSubsystem))
	{
		FMassProcessingPhaseManager& PhaseMannager = (FMassProcessingPhaseManager&)(MassSimulationSubsystem->GetPhaseManager());
		FMassEntityManager& EntityManager = PhaseMannager.GetEntityManagerRef();

		FMassProcessingPhaseConfig TargetPhaseConfig;
		TConstArrayView<FMassProcessingPhaseConfig> MainPhasesConfig = GET_MASS_CONFIG_VALUE(GetProcessingPhasesConfig());
		TargetPhaseConfig = MainPhasesConfig[PhaseID];

		UWorld* World = MassSimulationSubsystem->GetWorld();

		const EMassProcessingPhase Phase = EMassProcessingPhase(PhaseID);
		UMassCompositeProcessor* PhaseProcessor = NewObject<UMassCompositeProcessor>(MassSimulationSubsystem, UMassCompositeProcessor::StaticClass()
			, *FString::Printf(TEXT("GMTmp_ProcessingPhase_%s"), *UEnum::GetDisplayValueAsText(Phase).ToString()));


		FMassProcessorDependencySolver::FResult Result;
		FMassPhaseProcessorConfigurationHelper Configurator(*PhaseProcessor, TargetPhaseConfig, *MassSimulationSubsystem, EMassProcessingPhase(PhaseID));
		Configurator.bIsGameRuntime = false;
		Configurator.Configure({}, /*EntityManager=*/nullptr, &Result);

		//...
	}
}