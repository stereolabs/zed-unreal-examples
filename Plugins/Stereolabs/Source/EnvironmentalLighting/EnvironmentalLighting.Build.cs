//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

using UnrealBuildTool;
using System.IO;
using System;

public class EnvironmentalLighting : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public EnvironmentalLighting(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.AddRange(
            new string[] {
                "EnvironmentalLighting/Public"

                // ... add public include paths required here ...
              }
        );


        PrivateIncludePaths.AddRange(
            new string[] {
                "EnvironmentalLighting/Private"

				// ... add other private include paths required here ...
			}
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
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
                "Engine"
            }
        );
    }
}


