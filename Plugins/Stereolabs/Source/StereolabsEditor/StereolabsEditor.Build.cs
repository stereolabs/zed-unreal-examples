namespace UnrealBuildTool.Rules
{
	public class StereolabsEditor : ModuleRules
	{
        public StereolabsEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PrivateIncludePaths.Add("StereolabsEditor/Private");
            PublicIncludePaths.Add("StereolabsEditor/Public");

            PrivateDependencyModuleNames.AddRange(new string[]
                {"Slate",
                    "SlateCore" });

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Stereolabs",
                     "ZED",
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