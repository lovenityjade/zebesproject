using UnrealBuildTool;
public class SMUnreal : ModuleRules {
    public SMUnreal(ReadOnlyTargetRules Target) : base(Target) {
        string VersionFile = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "../../../VERSION"));
        string Version = System.IO.File.ReadAllText(VersionFile).Trim();
        if (!System.Text.RegularExpressions.Regex.IsMatch(Version, @"^ALPHA-[0-9]+\.[0-9]+$")) throw new BuildException("Invalid VERSION label");
        ExternalDependencies.Add(VersionFile);
        PublicDefinitions.Add("SM_BUILD_VERSION=\"" + Version + "\"");
        RuntimeDependencies.Add("$(ProjectDir)/Content/Splash/Icon.bmp", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Splash/EdIcon.bmp", StagedFileType.NonUFS);
        if (Target.Platform == UnrealTargetPlatform.Linux) AddEngineThirdPartyPrivateStaticDependencies(Target,"SDL3");
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        RuntimeDependencies.Add("$(ProjectDir)/Content/UI/Montserrat-Regular.ttf", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/UI/OFL.txt", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/GameOver/GameOver-16x9.png", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/GameOver/GameOver-4x3.png", StagedFileType.NonUFS);
        // Reviewed release audio is staged separately with exact hashes and credits.
        RuntimeDependencies.Add("$(ProjectDir)/Content/BossRush/*.png", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Finale/*.png", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Story/Portraits/*.png", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Title/...", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Achievements/...", StagedFileType.NonUFS);
        PublicSystemIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "../../ThirdParty/ImGui"));
        PublicDependencyModuleNames.AddRange(new[] {"Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "RHI", "Json", "Slate", "SlateCore", "ApplicationCore"});
    }
}
