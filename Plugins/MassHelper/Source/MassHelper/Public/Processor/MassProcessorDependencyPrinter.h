// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "MassEntity/Public/MassProcessingPhaseManager.h"
#include "MassEntity/Public/MassProcessorDependencySolver.h"

struct MASSHELPER_API FMassPhaseProcessorDependencyPrinter : public FMassPhaseProcessorConfigurationHelper
{
public:
	FMassPhaseProcessorDependencyPrinter(UMassCompositeProcessor& InOutPhaseProcessor, const FMassProcessingPhaseConfig& InPhaseConfig, UObject& InProcessorOuter, EMassProcessingPhase InPhase) :
		FMassPhaseProcessorConfigurationHelper(InOutPhaseProcessor, InPhaseConfig, InProcessorOuter, InPhase)
	{

	}

	virtual void Print(FString& ToString, TArrayView<UMassProcessor*> DynamicProcessors, const TSharedPtr<FMassEntityManager>& EntityManager,
		FMassProcessorDependencySolver::FResult* OutOptionalResult);
};

struct MASSHELPER_API FMassProcessorDependencySolverPrinterImpl : public FMassProcessorDependencySolver
{
public:
	FMassProcessorDependencySolverPrinterImpl(TArrayView<UMassProcessor*> InProcessors, const bool bIsGameRuntime = true) :
		FMassProcessorDependencySolver(InProcessors, bIsGameRuntime)
	{

	}

	virtual void Print(FString& OutputString);
};
