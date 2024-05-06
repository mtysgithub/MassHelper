// Copyright Epic Games, Inc. All Rights Reserved.

#include "AITestsCommon.h"
#include "MassEntityManager.h"
#include "MassProcessingTypes.h"
#include "MassEntityTestSuite/Public/MassEntityTestTypes.h"
#include "MassExecutor.h"

UE_DISABLE_OPTIMIZATION_SHIP

namespace FMassEntityTest
{
#if WITH_MASSENTITY_DEBUG
struct FEntityTest_Profile_ArchetypeCreation : FEntityTestBase
{
	virtual bool InstantTest() override
	{
		AITEST_TRUE("Floats archetype should have been created", FloatsArchetype.IsValid());
		AITEST_TRUE("Ints archetype should have been created", IntsArchetype.IsValid());
		AITEST_TRUE("Floats archetype should have been created", FloatsIntsArchetype.IsValid());

		TArray<const UScriptStruct*> FragmentsList;

		EntityManager->DebugGetArchetypeFragmentTypes(FloatsArchetype, FragmentsList);
		AITEST_EQUAL("Floats archetype should contain just a single fragment", FragmentsList.Num(), 1);
		AITEST_EQUAL("Floats archetype\'s lone fragment should be of Float fragment type", FragmentsList[0], FTestFragment_Float::StaticStruct());

		FragmentsList.Reset();
		EntityManager->DebugGetArchetypeFragmentTypes(IntsArchetype, FragmentsList);
		AITEST_EQUAL("Ints archetype should contain just a single fragment", FragmentsList.Num(), 1);
		AITEST_EQUAL("Ints archetype\'s lone fragment should be of Ints fragment type", FragmentsList[0], FTestFragment_Int::StaticStruct());

		FragmentsList.Reset();
		EntityManager->DebugGetArchetypeFragmentTypes(FloatsIntsArchetype, FragmentsList);
		AITEST_EQUAL("FloatsInts archetype should contain exactly two fragments", FragmentsList.Num(), 2);
		AITEST_TRUE("FloatsInts archetype\'s should contain both expected fragment types", FragmentsList.Find(FTestFragment_Int::StaticStruct()) != INDEX_NONE && FragmentsList.Find(FTestFragment_Float::StaticStruct()) != INDEX_NONE);

		/*----------------------------------------------------------------*/

		TArray<FMassEntityHandle> ToProfileEntitys;
		FragmentsList.Reset();
		EntityManager->DebugGetArchetypeFragmentTypes(FloatsArchetype, FragmentsList);
		EntityManager->DebugGetArchetypeFragmentTypes(IntsArchetype, FragmentsList);
		EntityManager->DebugGetArchetypeFragmentTypes(FloatsIntsArchetype, FragmentsList);
		AITEST_EQUAL("All archetype should contain exactly two fragments", FragmentsList.Num(), 4);

		auto PendingToModifyArchetype_1 = EntityManager->CreateArchetype(MakeArrayView(FragmentsList.GetData(), 1));
		auto PendingToModifyArchetype_2 = EntityManager->CreateArchetype(MakeArrayView(FragmentsList.GetData(), 2));
		auto PendingToModifyArchetype_3 = EntityManager->CreateArchetype(MakeArrayView(FragmentsList.GetData(), 3));
		auto PendingToModifyArchetype_4 = EntityManager->CreateArchetype(MakeArrayView(FragmentsList.GetData(), 4));

		EntityManager->BatchCreateEntities(PendingToModifyArchetype_1, 1000000, ToProfileEntitys);

		TArray<FMassArchetypeHandle> ArchetypeArr = { PendingToModifyArchetype_1, PendingToModifyArchetype_1, PendingToModifyArchetype_2, PendingToModifyArchetype_3 };

		for (int i = 1; i <= 3; ++i)
		{
			EntityManager->AddFragmentToEntity(ToProfileEntitys[0], FragmentsList[i]);
 			auto ToCheckArchetype = EntityManager->GetArchetypeForEntity(ToProfileEntitys[0]);
			AITEST_EQUAL("ToCheckArchetype should equal ArchetypeArr[i]", ToCheckArchetype, ArchetypeArr[i]);
		}

		return true;
	}
};
IMPLEMENT_AI_INSTANT_TEST(FEntityTest_Profile_ArchetypeCreation, "MassProfile.Entity.AchetypesCreation");

#endif // WITH_MASSENTITY_DEBUG

} // FMassEntityTestTest

UE_ENABLE_OPTIMIZATION_SHIP

#undef LOCTEXT_NAMESPACE
