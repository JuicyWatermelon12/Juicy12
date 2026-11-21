// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BanditBaller : ModuleRules
{
	public BanditBaller(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"CommonUI",
			"NetCore",
			"Niagara",
			"EngineCameras",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Editor-only: builds StateTree assets from code (gs.Editor.BuildEnemyStateTree).
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "StateTreeEditorModule", "PropertyBindingUtils", "AssetRegistry" });
		}

		PublicIncludePaths.AddRange(new string[] {
			"BanditBaller",
			"BanditBaller/Core",
			"BanditBaller/Combat",
			"BanditBaller/Abilities",
			"BanditBaller/Economy",
			"BanditBaller/Market",
			"BanditBaller/Casino",
			"BanditBaller/AI",
			"BanditBaller/UI",
			"BanditBaller/Variant_Platforming",
			"BanditBaller/Variant_Platforming/Animation",
			"BanditBaller/Variant_Combat",
			"BanditBaller/Variant_Combat/AI",
			"BanditBaller/Variant_Combat/Animation",
			"BanditBaller/Variant_Combat/Gameplay",
			"BanditBaller/Variant_Combat/Interfaces",
			"BanditBaller/Variant_Combat/UI",
			"BanditBaller/Variant_SideScrolling",
			"BanditBaller/Variant_SideScrolling/AI",
			"BanditBaller/Variant_SideScrolling/Gameplay",
			"BanditBaller/Variant_SideScrolling/Interfaces",
			"BanditBaller/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
