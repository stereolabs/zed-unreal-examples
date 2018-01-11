//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

namespace UnrealBuildTool.Rules
{
	public class SpatialMappingEditor : ModuleRules
	{
        public SpatialMappingEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PrivateIncludePaths.Add("SpatialMappingEditor/Private");
            PublicIncludePaths.Add("SpatialMappingEditor/Public");

            PrivateDependencyModuleNames.AddRange(new string[]
                {"Slate",
                    "SlateCore" });

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Stereolabs",
                     "SpatialMapping"
                }
                );

            PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Slate",
                    "SlateCore",
                    "Engine",
                    "UnrealEd",
                    "HeadMountedDisplay",
                    "DesktopPlatform",
                    "InputCore"
                }
				);
		}
	}
}