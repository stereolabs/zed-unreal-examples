//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

using UnrealBuildTool;
using System.IO;
using System;

public class Devices : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public Devices(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.AddRange(
            new string[] {
                "Devices/Public"

                // ... add public include paths required here ...
              }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "Devices/Private"

				// ... add other private include paths required here ...
			}
            );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "ZED",
                "Stereolabs",
                "MixedReality"

				// ... add other public dependencies that you statically link with here ...
			}
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "RenderCore",
                "Engine",
                "HeadMountedDisplay"
            }
            );
    }
}


