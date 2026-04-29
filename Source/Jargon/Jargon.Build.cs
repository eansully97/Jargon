// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Jargon : ModuleRules
{
	public Jargon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"AssetRegistry",
			"ApplicationCore"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd"
			});
		}

		PublicIncludePaths.AddRange(new string[] {
			"Jargon",
			"Jargon/Variant_Strategy",
			"Jargon/Variant_Strategy/UI",
			"Jargon/Variant_TwinStick",
			"Jargon/Variant_TwinStick/AI",
			"Jargon/Variant_TwinStick/Gameplay",
			"Jargon/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
