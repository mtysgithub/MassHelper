// Copyright Epic Games, Inc. All Rights Reserved.
#include "MassHelper/Public/Processor/MassProcessorDependencyPrinter.h"

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

void FMassPhaseProcessorDependencyPrinter::Print(EPrintMode PrintMode, FString& OutputString, TArrayView<UMassProcessor*> DynamicProcessors, const TSharedPtr<FMassEntityManager>& EntityManager, FMassProcessorDependencySolver::FResult* OutOptionalResult)
{
    switch (PrintMode)
    {
    case EPrintMode::ExecutesGroupTree:
        PrintExecutesGroupTree(OutputString, DynamicProcessors, EntityManager, OutOptionalResult);
        break;
    case EPrintMode::CompletelyDependency:
        PrintCompletelyDependency(OutputString, DynamicProcessors, EntityManager, OutOptionalResult);
        break;
    default:
        break;
    }
}

void FMassPhaseProcessorDependencyPrinter::PrintExecutesGroupTree(FString& OutputString, TArrayView<UMassProcessor*> DynamicProcessors, const TSharedPtr<FMassEntityManager>& EntityManager, FMassProcessorDependencySolver::FResult* OutOptionalResult)
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

    FMassProcessorDependencySolverPrinterImpl Solver(TmpPipeline.GetMutableProcessors(), bIsGameRuntime);
    Solver.ResolveExecutesGroupTree(EntityManager, OutOptionalResult);
    Solver.PrintExecutesGroupTree(OutputString);
}

void FMassPhaseProcessorDependencyPrinter::PrintCompletelyDependency(FString& OutputString, TArrayView<UMassProcessor*> DynamicProcessors, const TSharedPtr<FMassEntityManager>& EntityManager, FMassProcessorDependencySolver::FResult* OutOptionalResult)
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

	Solver.PrintCompletelyDependency(OutputString);
}

void FMassProcessorDependencySolverPrinterImpl::ResolveExecutesGroupTree(TSharedPtr<FMassEntityManager> EntityManager, FMassProcessorDependencySolver::FResult* InOutOptionalResult)
{
    if (Processors.Num() == 0)
    {
        return;
    }

    FScopedCategoryAndVerbosityOverride LogOverride(TEXT("LogMass"), ELogVerbosity::Log);

    if (InOutOptionalResult)
    {
        DependencyGraphFileName = InOutOptionalResult->DependencyGraphFileName;
    }

    bAnyCyclesDetected = false;

    UE_LOG(LogMass, Log, TEXT("Gathering dependencies data:"));

    AllForPrintGroupTreeNodes.Reset();
    ForPrintGroupTreeNodeIndexMap.Reset();
    // as the very first node we add a "root" node that represents the "top level group" and also simplifies the rest
    // of the lookup code - if a processor declares it's in group None or depends on Node it we don't need to check that 
    // explicitly. 
    AllForPrintGroupTreeNodes.Add(FForPrintExecutesGroupTreeNode(FName(), nullptr, 0));
    ForPrintGroupTreeNodeIndexMap.Add(FName(), 0);

    const bool bCreateVirtualArchetypes = (!EntityManager);
    if (bCreateVirtualArchetypes)
    {
        // create FMassEntityManager instance that we'll use to sort out processors' overlaps
        // the idea for this is that for every processor we have we create an archetype matching given processor's requirements. 
        // Once that's done we have a collection of "virtual" archetypes our processors expect. Then we ask every processor 
        // to cache the archetypes they'd accept, using processors' owned queries. The idea is that some of the nodes will 
        // end up with more than just the virtual archetype created for that specific node. The practice proved the idea correct. 
        EntityManager = MakeShareable(new FMassEntityManager());
    }

    // gather the processors information first
    for (UMassProcessor* Processor : Processors)
    {
        if (Processor == nullptr)
        {
            UE_LOG(LogMass, Warning, TEXT("%s nullptr found in Processors collection being processed"), ANSI_TO_TCHAR(__FUNCTION__));
            continue;
        }

        const int32 ProcessorNodeIndex = CreateForPrintGroupTreeNodes(*Processor);

        if (bCreateVirtualArchetypes)
        {
            // this line is a part of a nice trick we're doing here utilizing EntityManager's archetype creation based on 
            // what each processor expects, and EntityQuery's capability to cache archetypes matching its requirements (used below)
            EntityManager->CreateArchetype(AllForPrintGroupTreeNodes[ProcessorNodeIndex].Requirements.AsCompositionDescriptor());
        }
    }

    UE_LOG(LogMass, Verbose, TEXT("Pruning processors..."));

    int32 PrunedProcessorsCount = 0;
    for (FForPrintExecutesGroupTreeNode& Node : AllForPrintGroupTreeNodes)
    {
        if (Node.IsGroup() == false)
        {
            // for each processor-representing node we cache information on which archetypes among the once we've created 
            // above (see the EntityManager.CreateArchetype call in the previous loop) match this processor. 
            Node.Processor->GetArchetypesMatchingOwnedQueries(*EntityManager.Get(), Node.ValidArchetypes);

            // prune the archetype-less processors
            if (Node.ValidArchetypes.Num() == 0 && Node.Processor->ShouldAllowQueryBasedPruning(bGameRuntime))
            {
                CA_ASSUME(Node.Processor);
                UE_LOG(LogMass, Verbose, TEXT("\t%s"), *Node.Processor->GetName());

                if (InOutOptionalResult)
                {
                    InOutOptionalResult->PrunedProcessorClasses.Add(Node.Processor->GetClass());
                }

                // clearing out the processor will result in the rest of the algorithm to treat it as a group - we still 
                // want to preserve the configured ExecuteBefore and ExecuteAfter dependencies
                Node.Processor = nullptr;
                ++PrunedProcessorsCount;
            }
        }
    }

    UE_LOG(LogMass, Verbose, TEXT("Number of processors pruned: %d"), PrunedProcessorsCount);

    check(AllForPrintGroupTreeNodes.Num());

    //TODO
    //Check bAnyCyclesDetected
}

void FMassProcessorDependencySolverPrinterImpl::PrintExecutesGroupTree(FString& OutputString)
{
    PrintCommon(FString("ExecutesGroupTree"), AllForPrintGroupTreeNodes, OutputString);
}

void FMassProcessorDependencySolverPrinterImpl::PrintCompletelyDependency(FString& OutputString)
{
    PrintCommon(FString("CompletelyDependency"), AllNodes, OutputString);
}

int32 FMassProcessorDependencySolverPrinterImpl::CreateForPrintGroupTreeNodes(UMassProcessor& Processor)
{
    check(Processor.GetClass());
    FName ProcName = Processor.GetClass()->GetFName();

    if (const int32* NodeIndexPtr = ForPrintGroupTreeNodeIndexMap.Find(ProcName))
    {
        // we can accept another instance of this processor class if the processor itself supports that.
        // Note that the first instance is added with the class while the subsequent instances are added with 
        // the instance name. This is done on purpose. The class name is used as dependency, so if at least the first 
        // processor is placed with the class name, like other processors, it can still be used to influence execution
        // order via ExecuteBefore and ExecuteAfter.
        if (Processor.ShouldAllowMultipleInstances())
        {
            ProcName = Processor.GetFName();
        }
        else
        {
            UE_LOG(LogMass, Warning, TEXT("%s Processor %s already registered. Duplicates are not supported.")
                , ANSI_TO_TCHAR(__FUNCTION__), *ProcName.ToString());
            return *NodeIndexPtr;
        }
    }

    const FMassProcessorExecutionOrder& ExecutionOrder = Processor.GetExecutionOrder();

    // first figure out the groups so that the group nodes come before the processor nodes, this is required for child
    // nodes to inherit group's dependencies like in scenarios where some processor required to ExecuteBefore a given group
    int32 ParentGroupNodeIndex = INDEX_NONE;
    if (ExecutionOrder.ExecuteInGroup.IsNone() == false)
    {
        TArray<FString> AllGroupNames;
        CreateSubGroupNames(ExecutionOrder.ExecuteInGroup, AllGroupNames);

        check(AllGroupNames.Num() > 0);

        for (const FString& GroupName : AllGroupNames)
        {
            const FName GroupFName(GroupName);
            int32* LocalGroupIndex = ForPrintGroupTreeNodeIndexMap.Find(GroupFName);
            // group name hasn't been encountered yet - create it
            if (LocalGroupIndex == nullptr)
            {
                int32 NewGroupNodeIndex = AllForPrintGroupTreeNodes.Num();
                ForPrintGroupTreeNodeIndexMap.Add(GroupFName, NewGroupNodeIndex);
                FForPrintExecutesGroupTreeNode& GroupNode = AllForPrintGroupTreeNodes.Add_GetRef({ GroupFName, nullptr, NewGroupNodeIndex });
                // just ignore depending on the dummy "root" node
                if (ParentGroupNodeIndex != INDEX_NONE)
                {
                    GroupNode.OriginalDependencies.Add(ParentGroupNodeIndex);
                    AllForPrintGroupTreeNodes[ParentGroupNodeIndex].SubNodeIndices.Add(NewGroupNodeIndex);
                }

                ParentGroupNodeIndex = NewGroupNodeIndex;
            }
            else
            {
                ParentGroupNodeIndex = *LocalGroupIndex;
            }

        }
    }

    const int32 NodeIndex = AllForPrintGroupTreeNodes.Num();
    ForPrintGroupTreeNodeIndexMap.Add(ProcName, NodeIndex);
    FForPrintExecutesGroupTreeNode& ProcessorNode = AllForPrintGroupTreeNodes.Add_GetRef({ ProcName, &Processor, NodeIndex });

    ProcessorNode.ExecuteAfter.Append(ExecutionOrder.ExecuteAfter);
    ProcessorNode.ExecuteBefore.Append(ExecutionOrder.ExecuteBefore);
    Processor.ExportRequirements(ProcessorNode.Requirements);
    ProcessorNode.Requirements.CountResourcesUsed();

    if (ParentGroupNodeIndex > 0)
    {
        AllForPrintGroupTreeNodes[ParentGroupNodeIndex].SubNodeIndices.Add(NodeIndex);
    }

    return NodeIndex;
}
