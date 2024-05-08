// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassHelper/Public/MassDumpCheatManager.h"
#include "MassHelper/Public/Processor/MassProcessorDependencyPrinter.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "MassSpawner.h"

#include "MassEntitySettings.h"
#include "MassSimulation/Public/MassSimulationSubsystem.h"
#include "MassEntity/Public/MassEntitySubsystem.h"
#include <Misc/FileHelper.h>

UMassDumpCheatManager::UMassDumpCheatManager()
{

}

void UMassDumpCheatManager::DumpStaticProcessorsDependencyByPhaseID(int PhaseID)
{ 
	bool bRuntime = false;
	FString ToSaveFileName = FString::Printf(TEXT("Mass_ProcessorsDependency_Phase%d_"), PhaseID) + ((bRuntime) ? ("Runtime.json") : ("Static.json"));
	DoDumpProcessorsDependency(PhaseID, ToSaveFileName);
}

void UMassDumpCheatManager::DumpRuntimeProcessorsDependencyByPhaseID(int PhaseID)
{
	bool bRuntime = true;
	FString ToSaveFileName = FString::Printf(TEXT("Mass_ProcessorsDependency_Phase%d_"), PhaseID) + ((bRuntime) ? ("Runtime.json") : ("Static.json"));
	DoDumpProcessorsDependency(PhaseID, ToSaveFileName);
}

void UMassDumpCheatManager::DoDumpProcessorsDependency(int PhaseID, FString& ToSaveFileName)
{
	UMassSimulationSubsystem* MassSimulationSubsystem = UWorld::GetSubsystem<UMassSimulationSubsystem>(this->GetWorld());
	check(MassSimulationSubsystem);
	if (MassSimulationSubsystem)
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


		FMassProcessorDependencySolverPrinterImpl::FResult Result;
		FMassPhaseProcessorDependencyPrinter Configurator(*PhaseProcessor, TargetPhaseConfig, *MassSimulationSubsystem, EMassProcessingPhase(PhaseID));
		Configurator.bIsGameRuntime = false;

		FString DependencyString;
		Configurator.Print(DependencyString, {}, /*EntityManager=*/nullptr, &Result);

		UE_LOG(LogMass, Log, TEXT("%s"), *DependencyString);
		FString SavePath = FPaths::ProjectSavedDir() / ToSaveFileName;
		FFileHelper::SaveStringToFile(DependencyString, *SavePath);
	}
}
