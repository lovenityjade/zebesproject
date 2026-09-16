using UnrealBuildTool;
public class SMUnrealEditorTarget : TargetRules {
    public SMUnrealEditorTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("SMUnreal");
    }
}
