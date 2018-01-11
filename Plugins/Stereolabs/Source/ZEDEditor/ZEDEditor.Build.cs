//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

namespace UnrealBuildTool.Rules
{
	public class ZEDEditor : ModuleRules
	{
        public ZEDEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PrivateIncludePaths.Add("ZEDEditor/Private");
            PublicIncludePaths.Add("ZEDEditor/Public");

            PrivateDependencyModuleNames.AddRange(new string[]
                {"Slate",
                    "SlateCore" });

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Stereolabs",
                     "ZED"
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