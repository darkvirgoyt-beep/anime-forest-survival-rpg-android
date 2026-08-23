using UnrealBuildTool;
using System.Collections.Generic;

public class ForestSliceClientTarget : TargetRules
{
    public ForestSliceClientTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Client;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        ExtraModuleNames.Add("ForestSlice");
    }
}
