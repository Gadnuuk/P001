// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class P001 : ModuleRules
{
	public P001(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
        PrivateIncludePaths.AddRange(new string[]
        {
            "P001"
        });
    }
}
