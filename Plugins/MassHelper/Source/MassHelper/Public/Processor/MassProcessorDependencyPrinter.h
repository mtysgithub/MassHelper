// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "MassEntity/Public/MassProcessingPhaseManager.h"
#include "MassEntity/Public/MassProcessorDependencySolver.h"

#include "Json/Public/Dom/JsonObject.h"
#include "Json/Public/Serialization/JsonSerializer.h"

enum EPrintMode
{
	ExecutesGroupTree,
	CompletelyDependency,
};

struct MASSHELPER_API FMassPhaseProcessorDependencyPrinter : public FMassPhaseProcessorConfigurationHelper
{
public:


public:
	FMassPhaseProcessorDependencyPrinter(UMassCompositeProcessor& InOutPhaseProcessor, const FMassProcessingPhaseConfig& InPhaseConfig, UObject& InProcessorOuter, EMassProcessingPhase InPhase) :
		FMassPhaseProcessorConfigurationHelper(InOutPhaseProcessor, InPhaseConfig, InProcessorOuter, InPhase)
	{

	}

	void Print(EPrintMode PrintMode, FString& OutputString, TArrayView<UMassProcessor*> DynamicProcessors, const TSharedPtr<FMassEntityManager>& EntityManager,
		FMassProcessorDependencySolver::FResult* OutOptionalResult);

protected:
	virtual void PrintExecutesGroupTree(FString& OutputString, TArrayView<UMassProcessor*> DynamicProcessors, const TSharedPtr<FMassEntityManager>& EntityManager,
		FMassProcessorDependencySolver::FResult* OutOptionalResult);

	virtual void PrintCompletelyDependency(FString& OutputString, TArrayView<UMassProcessor*> DynamicProcessors, const TSharedPtr<FMassEntityManager>& EntityManager,
		FMassProcessorDependencySolver::FResult* OutOptionalResult);
};

struct MASSHELPER_API FMassProcessorDependencySolverPrinterImpl : public FMassProcessorDependencySolver
{
public:
	FMassProcessorDependencySolverPrinterImpl(TArrayView<UMassProcessor*> InProcessors, const bool bIsGameRuntime = true) :
		FMassProcessorDependencySolver(InProcessors, bIsGameRuntime)
	{

	}

	void ResolveExecutesGroupTree(TSharedPtr<FMassEntityManager> EntityManager, FMassProcessorDependencySolver::FResult* InOutOptionalResult);

	void PrintExecutesGroupTree(FString& OutputString);
	void PrintCompletelyDependency(FString& OutputString);

    template <typename T>
	void PrintCommon(FString Mode, TArray<T>& Nodes, FString& OutString)
	{
        using UE::MassHelper::Private::NameViewToString;

        TSharedPtr<FJsonObject> RootJson = MakeShareable(new FJsonObject);
        TArray<TSharedPtr<FJsonValue>> NodesArray;

        for (int i = 0; i < Nodes.Num(); ++i)
        {
            T& Node = Nodes[i];

            FString NodeName = Node.Name.ToString();
            // Create a JSON object for this node
            TSharedPtr<FJsonObject> NodeJson = MakeShareable(new FJsonObject);
            // Add node name
            NodeJson->SetStringField(TEXT("NodeName"), NodeName);
            NodeJson->SetStringField(TEXT("Color"), Node.IsGroup() ? ("blue") : ("red"));

            /*------------------------------------------------------------------------------------------------*/

            TArray<FName> OriginalDependencies;
            for (int ParentIndex = 0; ParentIndex < Node.OriginalDependencies.Num(); ++ParentIndex)
            {
                auto& ParentNode = Nodes[Node.OriginalDependencies[ParentIndex]];
                OriginalDependencies.Add(ParentNode.Name);
            }
            TArray<TSharedPtr<FJsonValue>> OriginalDependenciesJsonArray;
            for (const FName& ParentNode : OriginalDependencies)
            {
                OriginalDependenciesJsonArray.Add(MakeShareable(new FJsonValueString(ParentNode.ToString())));
            }
            NodeJson->SetArrayField(TEXT("OriginalDependencies"), OriginalDependenciesJsonArray);

            TArray<FName> SubNodeIndices;
            for (int SubNodeIndex = 0; SubNodeIndex < Node.SubNodeIndices.Num(); ++SubNodeIndex)
            {
                auto& SubNode = Nodes[Node.SubNodeIndices[SubNodeIndex]];
                SubNodeIndices.Add(SubNode.Name);
            }
            TArray<TSharedPtr<FJsonValue>> SubNodeIndicesJsonArray;
            for (const FName& SubNode : SubNodeIndices)
            {
                SubNodeIndicesJsonArray.Add(MakeShareable(new FJsonValueString(SubNode.ToString())));
            }
            NodeJson->SetArrayField(TEXT("SubNodeIndices"), SubNodeIndicesJsonArray);

            /*------------------------------------------------------------------------------------------------*/

            TArray<FName>& ExecuteBeforeNodes = Node.ExecuteBefore;
            // Convert ExecuteBeforeNodes and ExecuteAfterNodes to JSON arrays
            TArray<TSharedPtr<FJsonValue>> ExecuteBeforeJsonArray;
            for (const FName& BeforeNode : ExecuteBeforeNodes)
            {
                ExecuteBeforeJsonArray.Add(MakeShareable(new FJsonValueString(BeforeNode.ToString())));
            }
            NodeJson->SetArrayField(TEXT("ExecuteBeforeNodes"), ExecuteBeforeJsonArray);


            TArray<FName>& ExecuteAfterNodes = Node.ExecuteAfter;
            TArray<TSharedPtr<FJsonValue>> ExecuteAfterJsonArray;
            for (const FName& AfterNode : ExecuteAfterNodes)
            {
                ExecuteAfterJsonArray.Add(MakeShareable(new FJsonValueString(AfterNode.ToString())));
            }
            NodeJson->SetArrayField(TEXT("ExecuteAfterNodes"), ExecuteAfterJsonArray);

            /*------------------------------------------------------------------------------------------------*/

            // Add this node's JSON object to the array of nodes
            NodesArray.Add(MakeShareable(new FJsonValueObject(NodeJson)));
        }

        // Add the array of nodes to the root JSON object
        RootJson->SetStringField(TEXT("PrintMode"), *Mode);
        RootJson->SetArrayField(TEXT("Nodes"), NodesArray);

        // Convert the root JSON object to a string
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutString);
        FJsonSerializer::Serialize(RootJson.ToSharedRef(), Writer);
	}

#pragma region PrintExecutesGroupTree
protected:
	struct FForPrintExecutesGroupTreeNode
	{
		FForPrintExecutesGroupTreeNode(const FName InName, UMassProcessor* InProcessor, const int32 InNodeIndex = INDEX_NONE)
			: Name(InName), Processor(InProcessor), NodeIndex(InNodeIndex)
		{}

		bool IsGroup() const { return Processor == nullptr; }

		FName Name = TEXT("");
		UMassProcessor* Processor = nullptr;
		TArray<int32> OriginalDependencies;
		TArray<int32> SubNodeIndices;
		TArray<FName> ExecuteBefore;
		TArray<FName> ExecuteAfter;
		FMassExecutionRequirements Requirements;
		int32 NodeIndex = INDEX_NONE;
		TArray<FMassArchetypeHandle> ValidArchetypes;
	};

	int32 CreateForPrintGroupTreeNodes(UMassProcessor& Processor);

	TArray<FForPrintExecutesGroupTreeNode> AllForPrintGroupTreeNodes;
	TMap<FName, int32> ForPrintGroupTreeNodeIndexMap;
#pragma endregion

};
