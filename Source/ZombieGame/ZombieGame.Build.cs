// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ZombieGame : ModuleRules
{
	public ZombieGame(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ZombieGame",
			"ZombieGame/Variant_Strategy",
			"ZombieGame/Variant_Strategy/UI",
			"ZombieGame/Variant_TwinStick",
			"ZombieGame/Variant_TwinStick/AI",
			"ZombieGame/Variant_TwinStick/Gameplay",
			"ZombieGame/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
