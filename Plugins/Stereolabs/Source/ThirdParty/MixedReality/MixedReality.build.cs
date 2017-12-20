using System;
using System.IO;
using UnrealBuildTool;

public class MixedReality : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public string ProjectBinariesPathDirectory
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../../../../Binaries/Win64/")); }
    }

    public MixedReality(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;
 
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string PostFix = "64";
            string[] LibrariesName =  {
                                        "sl_mr_core"
                                      };

            PublicIncludePaths.Add(ModulePath + "/include");
            PublicLibraryPaths.Add(ModulePath + "/lib");

            foreach (string Library in LibrariesName)
            {
                string DLLName = Library + PostFix + ".dll";
                string LibName = Library + PostFix + ".lib";

                PublicAdditionalLibraries.Add(LibName);
                PublicDelayLoadDLLs.Add(DLLName);

                string DLLPath = Path.Combine(ModulePath + "/bin/", DLLName);
               
                if (Target.Type != TargetRules.TargetType.Editor)
                {
                    if (!Directory.Exists(ProjectBinariesPathDirectory))
                    {
                        Directory.CreateDirectory(ProjectBinariesPathDirectory);
                    }
                    File.Copy(DLLPath, Path.Combine(ProjectBinariesPathDirectory, DLLName), true);

                    RuntimeDependencies.Add(ProjectBinariesPathDirectory + DLLName, StagedFileType.NonUFS);
                }
                else
                {
                    if (Directory.Exists(ProjectBinariesPathDirectory) && File.Exists(Path.Combine(ProjectBinariesPathDirectory, DLLName)))
                    {
                        File.Delete(Path.Combine(ProjectBinariesPathDirectory, DLLName));
                    }
                }
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            string Err = string.Format("Attempt to build against ZED MIXED REALITY SDK on unsupported platform {0}", Target.Platform);
            System.Console.WriteLine(Err);
            throw new BuildException(Err);
        }
    }
}