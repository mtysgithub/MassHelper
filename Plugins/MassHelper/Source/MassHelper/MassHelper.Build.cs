// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassHelper : ModuleRules
{
	public MassHelper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
			}
			
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				// AI/MassAI Plugin Modules
				"MassAIBehavior",
				"MassAIDebug",
				"MassNavigation",
				"MassZoneGraphNavigation",
				
				// AI/MassCrowd Plugin Modules
				"MassCrowd",

				// Runtime/MassEntity Plugin Modules
				"MassEntity",

				// Runtime/MassGameplay Plugin Modules
				"MassActors",
				"MassCommon",
				"MassGameplayDebug",
				"MassLOD",
				"MassMovement",
				"MassReplication",
				"MassRepresentation",
				"MassSignals",
				"MassSimulation",
				"MassSmartObjects",
				"MassSpawner",
				
				"SmartObjectsModule", // work around until we find a better way to get data from smart object configs
				
				// Misc
				"AnimToTexture",
				"ContextualAnimation",
				"Core",
				"Engine",
				"MotionWarping",
				"DeveloperSettings",
				"StateTreeModule",
				"StructUtils",
				"ZoneGraph",
				"ZoneGraphDebug",
				"Json"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Slate",
				"SlateCore",
				"GameplayTasks",
				"MassSimulation",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
		
		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
				}
			);
		}
	}
}
