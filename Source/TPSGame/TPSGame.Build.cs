// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPSGame : ModuleRules
{
	public TPSGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"NavigationSystem",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"TPSGame",
			"TPSGame/Variant_Platforming",
			"TPSGame/Variant_Platforming/Animation",
			"TPSGame/Variant_Combat",
			"TPSGame/Variant_Combat/AI",
			"TPSGame/Variant_Combat/Animation",
			"TPSGame/Variant_Combat/Gameplay",
			"TPSGame/Variant_Combat/Interfaces",
			"TPSGame/Variant_Combat/UI",
			"TPSGame/Variant_SideScrolling",
			"TPSGame/Variant_SideScrolling/AI",
			"TPSGame/Variant_SideScrolling/Gameplay",
			"TPSGame/Variant_SideScrolling/Interfaces",
			"TPSGame/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
