//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

using UnrealBuildTool;
using System.IO;
using System;

public class SpatialMapping : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public SpatialMapping(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.AddRange(
            new string[] {
                "SpatialMapping/Public",

                // ... add public include paths required here ...
              }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "SpatialMapping/Private"

				// ... add other private include paths required here ...
			}
            );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                 "Stereolabs",
                 "ZED"

				// ... add other public dependencies that you statically link with here ...
			}
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "RenderCore",
                "Engine"
            }
            );
    }
}


