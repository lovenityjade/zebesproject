using UnrealBuildTool;
public class SMUnrealTarget : TargetRules {
    public SMUnrealTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("SMUnreal");
    }
}
