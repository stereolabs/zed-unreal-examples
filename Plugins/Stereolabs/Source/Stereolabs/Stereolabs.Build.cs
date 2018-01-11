//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

using UnrealBuildTool;
using System.IO;
using System;

public class Stereolabs : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public Stereolabs(ReadOnlyTargetRules Target) : base(Target)
    {
        string CudaSDKPath = System.Environment.GetEnvironmentVariable("CUDA_PATH_V9_1", EnvironmentVariableTarget.Machine);
        string ZEDSDKPath  = System.Environment.GetEnvironmentVariable("ZED_SDK_ROOT_DIR", EnvironmentVariableTarget.Machine);

        PublicIncludePaths.AddRange(
            new string[] {
                "Stereolabs/Public"

				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "Stereolabs/Private"

				// ... add other private include paths required here ...
			}
            );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "MixedReality",

                "HeadMountedDisplay"

				// ... add other public dependencies that you statically link with here ...
			}
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",

                "Engine",

                "InputCore",

                "RenderCore",
                "ShaderCore",

                "RHI",
                "D3D11RHI",
                "OpenGLDrv",

                "UMG",
                "SlateCore"
            }
            );

        // Paths for low-level directx and opengl access
        string engine_path = Path.GetFullPath(Target.RelativeEnginePath);
        PrivateIncludePaths.AddRange(
          new string[]
          {
                engine_path + "Source/Runtime/Windows/D3D11RHI/Private/",
                engine_path + "Source/Runtime/Windows/D3D11RHI/Private/Windows",
                engine_path + "Source/Runtime/OpenGLDrv/Private/",
                engine_path + "Source/Runtime/OpenGLDrv/Private/Windows",
                engine_path + "Source/ThirdParty/OpenGL"
              // ... add other private include paths required here ...
          }
          );


        LoadZEDSDK(Target, ZEDSDKPath);
        LoadCUDA(Target, CudaSDKPath);
    }

    public void LoadZEDSDK(ReadOnlyTargetRules Target, string DirPath)
    {
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            if(!Directory.Exists(DirPath))
            {
                string Err = string.Format("ZED SDK missing");
                System.Console.WriteLine(Err);
                throw new BuildException(Err);
            }

            // Check SDK version
            string DefinesHeaderFilePath = Path.Combine(DirPath, "include\\sl_zed\\defines.hpp");
            string Major = "2";
            string Minor = "3";

            // Find SDK major and minor version and compare
            foreach (var line in File.ReadLines(DefinesHeaderFilePath))
            {
                if (!string.IsNullOrEmpty(line))
                {
                    if(line.Contains("#define ZED_SDK_MAJOR_VERSION"))
                    {
                        string SDKMajor = line.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries)[2];
                        if(!SDKMajor.Equals(Major))
                        {
                            string Err = string.Format("ZED SDK Major Version mismatch : found {0} expected {1}", SDKMajor, Major);
                            System.Console.WriteLine(Err);
                            throw new BuildException(Err);
                        }
                    }
                    else if (line.Contains("#define ZED_SDK_MINOR_VERSION"))
                    {
                        string SDKMinor = line.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries)[2];
                        if (!SDKMinor.Equals(Minor))
                        {
                            string Err = string.Format("ZED SDK Minor Version mismatch : found {0} expected {1}", SDKMinor, Minor);
                            System.Console.WriteLine(Err);
                            throw new BuildException(Err);
                        }

                        break;
                    }
                }
            }

            // Set the paths to the SDK
            string[] LibrariesNames = Directory.GetFiles(Path.Combine(DirPath, "lib"));

            PublicIncludePaths.Add(Path.Combine(DirPath, "include"));
            PublicLibraryPaths.Add(Path.Combine(DirPath, "lib"));

            foreach (string Library in LibrariesNames)
            {
                PublicAdditionalLibraries.Add(Library);
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            string Err = string.Format("Attempt to build against ZED SDK on unsupported platform {0}", Target.Platform);
            System.Console.WriteLine(Err);
            throw new BuildException(Err);
        }
    }

    public void LoadCUDA(ReadOnlyTargetRules Target, string DirPath)
    {
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            if (!Directory.Exists(DirPath))
            {
                string Err = string.Format("CUDA SDK missing");
                System.Console.WriteLine(Err);
                throw new BuildException(Err);
            }

            string[] LibrariesName =  {
                                        "cuda",
                                        "cudart"
                                      };

            PublicIncludePaths.Add(Path.Combine(DirPath, "include"));
            PublicLibraryPaths.Add(Path.Combine(DirPath, "lib\\x64"));

            foreach (string Library in LibrariesName)
            {
                PublicAdditionalLibraries.Add(Library + ".lib");
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            string Err = string.Format("Attempt to build against CUDA on unsupported platform {0}", Target.Platform);
            System.Console.WriteLine(Err);
            throw new BuildException(Err);
        }
    }
}
