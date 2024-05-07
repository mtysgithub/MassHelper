// Copyright Epic Games, Inc. All Rights Reserved.
#include "MassHelper/Public/Processor/MassProcessorDependencyPrinter.h"
#include "Json/Public/Dom/JsonObject.h"
#include "Json/Public/Serialization/JsonSerializer.h"

namespace UE::MassHelper::Private
{
	FString NameViewToString(TConstArrayView<FName> View)
	{
		if (View.Num() == 0)
		{
			return TEXT("[]");
		}
		FString ReturnVal = FString::Printf(TEXT("[%s"), *View[0].ToString());
		for (int i = 1; i < View.Num(); ++i)
		{
			ReturnVal += FString::Printf(TEXT(", %s"), *View[i].ToString());
		}
		return ReturnVal + TEXT("]");
	}
}

void FMassPhaseProcessorDependencyPrinter::Print(FString& OutputString, TArrayView<UMassProcessor*> DynamicProcessors, const TSharedPtr<FMassEntityManager>& EntityManager, FMassProcessorDependencySolver::FResult* OutOptionalResult)
{
	FMassRuntimePipeline TmpPipeline;
	TmpPipeline.CreateFromArray(PhaseConfig.ProcessorCDOs, ProcessorOuter);
	for (UMassProcessor* Processor : DynamicProcessors)
	{
		checkf(Processor != nullptr, TEXT("Dynamic processor provided to MASS is null."));
		if (Processor->GetProcessingPhase() == Phase)
		{
			TmpPipeline.AppendProcessor(*Processor);
		}
	}

	TArray<FMassProcessorOrderInfo> SortedProcessors;
	FMassProcessorDependencySolverPrinterImpl Solver(TmpPipeline.GetMutableProcessors(), bIsGameRuntime);

	Solver.ResolveDependencies(SortedProcessors, EntityManager, OutOptionalResult);

	for (const FMassProcessorOrderInfo& ProcessorOrderInfo : SortedProcessors)
	{
		TmpPipeline.RemoveProcessor(*ProcessorOrderInfo.Processor);
	}

	if (TmpPipeline.Num())
	{
		UE_VLOG_UELOG(&PhaseProcessor, LogMass, Verbose, TEXT("Discarding processors due to not having anything to do (no relevant Archetypes):"));
		for (const UMassProcessor* Processor : TmpPipeline.GetProcessors())
		{
			UE_VLOG_UELOG(&PhaseProcessor, LogMass, Verbose, TEXT("\t%s"), *Processor->GetProcessorName());
		}
	}

	Solver.Print(OutputString);
}

void FMassProcessorDependencySolverPrinterImpl::Print(FString& OutputString)
{
    using UE::MassHelper::Private::NameViewToString;

    TSharedPtr<FJsonObject> RootJson = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> NodesArray;

    for (int i = 0; i < AllNodes.Num(); ++i)
    {
        auto& Node = AllNodes[i];

        TArray<FName>& ExecuteBeforeNodes = Node.ExecuteBefore;
        TArray<FName>& ExecuteAfterNodes = Node.ExecuteAfter;
        FString NodeName = Node.Name.ToString();

        FString ExecuteBeforeStr = NameViewToString(ExecuteBeforeNodes);
        FString ExecuteAfterStr = NameViewToString(ExecuteAfterNodes);

        //FString TmpStr = FString::Printf(TEXT("%s | %s | %s"),
        //    *ExecuteBeforeStr,
        //    *NodeName,
        //    *ExecuteAfterStr);

        //UE_LOG(LogMass, Log, TEXT("%s"), *TmpStr);

        // Create a JSON object for this node
        TSharedPtr<FJsonObject> NodeJson = MakeShareable(new FJsonObject);

        // Add node name
        NodeJson->SetStringField(TEXT("NodeName"), NodeName);
        NodeJson->SetStringField(TEXT("Color"), Node.IsGroup() ? ("B") : ("R"));

        // Convert ExecuteBeforeNodes and ExecuteAfterNodes to JSON arrays
        TArray<TSharedPtr<FJsonValue>> ExecuteBeforeJsonArray;
        for (const FName& BeforeNode : ExecuteBeforeNodes)
        {
            ExecuteBeforeJsonArray.Add(MakeShareable(new FJsonValueString(BeforeNode.ToString())));
        }
        NodeJson->SetArrayField(TEXT("ExecuteBeforeNodes"), ExecuteBeforeJsonArray);

        TArray<TSharedPtr<FJsonValue>> ExecuteAfterJsonArray;
        for (const FName& AfterNode : ExecuteAfterNodes)
        {
            ExecuteAfterJsonArray.Add(MakeShareable(new FJsonValueString(AfterNode.ToString())));
        }
        NodeJson->SetArrayField(TEXT("ExecuteAfterNodes"), ExecuteAfterJsonArray);

        // Add this node's JSON object to the array of nodes
        NodesArray.Add(MakeShareable(new FJsonValueObject(NodeJson)));
    }

    // Add the array of nodes to the root JSON object
    RootJson->SetArrayField(TEXT("Nodes"), NodesArray);

    // Convert the root JSON object to a string
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootJson.ToSharedRef(), Writer);

    // Optionally, add the JSON string to the ToString variable if you want to return it
    OutputString.Append(TEXT("\nJSON Representation:\n"));
    OutputString.Append(OutputString);
}
