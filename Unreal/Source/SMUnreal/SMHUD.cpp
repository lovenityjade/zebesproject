#include "SMHUD.h"
#include "SMLocalization.h"
#include "SMNativeLibrary.h"
#include "Misc/ScopeExit.h"
#include "SMRom.h"
#include "SMRomSetup.h"
#include "SMRandomizer.h"
#include "SMSystemMenu.h"
#include "SMSeedSettings.h"
#include "../../../Native/sm_depth.h"
#include "../../../Native/sm_relief.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundWaveProcedural.h"
#include "Components/AudioComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"

static UTexture2D* MakePresentationTexture(int W,int H,TextureFilter Filter) {
    UTexture2D* T=UTexture2D::CreateTransient(W,H,PF_B8G8R8A8);
    T->Filter=Filter;T->SRGB=false;T->NeverStream=true;T->UpdateResource();return T;
}
static void UploadPresentation(UTexture2D* Texture,const uint8* Bytes,int W,int H) {
    uint8* Copy=static_cast<uint8*>(FMemory::Malloc(W*H*4));
    FMemory::Memcpy(Copy,Bytes,W*H*4);
    auto* R=new FUpdateTextureRegion2D(0,0,0,0,W,H);
    Texture->UpdateTextureRegions(0,1,R,W*4,4,Copy,
      [](uint8* Data,const FUpdateTextureRegion2D* Region){FMemory::Free(Data);delete Region;});
}
ASMHUD::ASMHUD() { PrimaryActorTick.bCanEverTick = true; }
void ASMHUD::BeginPlay() {
    Super::BeginPlay();
    SMLocalization::Load(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SM/Presentation.ini")));
    const FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT(".."));
    FString Reason;
    if (!SMRom::Validate(SMRom::LocalPath(Root), Reason)) {
        RomSetup = MakeShared<FSMRomSetup>(SMRom::LocalPath(Root), Reason);
        RomSetup->Show(PlayerOwner);
        UE_LOG(LogTemp, Display, TEXT("SM_ROM_REQUIRED native_core_loaded=0"));
        return;
    }
    StartVerifiedGame();
}
void ASMHUD::StartVerifiedGame() {
    const FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT(".."));
    if(FParse::Param(FCommandLine::Get(),TEXT("SMRandomizerTest"))) {
        const FString TestRoot=FPaths::ProjectSavedDir()/TEXT("SMTests");
        const FString TestDir=TestRoot/TEXT("Seeds");
        IFileManager::Get().MakeDirectory(*TestRoot,true);
        bool Passed=true;FString ServiceError,Report=TEXT("{\"seeds\":[");
        for(int I=0;I<6 && Passed;I++) {
            FSMSeedRequest Request;Request.Seed=14092026+I;
            Request.Skill=I%3==0?TEXT("casual"):(I%3==1?TEXT("regular"):TEXT("veteran"));
            Request.Progression=I<3?TEXT("slow"):TEXT("medium");
            FSMSeedPlan Plan,Reloaded;
            Passed=FSMRandomizer::Generate(Root,Request,Plan,ServiceError);
            if(Passed)Passed=FSMRandomizer::Stage(Plan,TestDir,ServiceError);
            const FString SeedFile=TestDir/FString::Printf(TEXT("seed-%d-%s.json"),Request.Seed,*Plan.Fingerprint.Left(12));
            FString OnDisk;
            if(Passed)Passed=FFileHelper::LoadFileToString(OnDisk,*SeedFile) &&
                FSMRandomizer::ReadPlan(OnDisk,Reloaded,ServiceError) &&
                Reloaded.Items.Num()==100 && Reloaded.Fingerprint==Plan.Fingerprint;
            if(Passed)for(int J=0;J<100;J++)if(Reloaded.Items[J].Address!=Plan.Items[J].Address || Reloaded.Items[J].Plm!=Plan.Items[J].Plm)Passed=false;
            FFileHelper::SaveStringToFile(Passed?OnDisk:ServiceError,*(TestRoot/FString::Printf(TEXT("randomizer-ue-seed-%d.json"),Request.Seed)));
            if(I==0)FFileHelper::SaveStringToFile(Passed?OnDisk:ServiceError,*(TestRoot/TEXT("randomizer-ue.json")));
            if(I)Report+=TEXT(",");
            Report+=FString::Printf(TEXT("{\"seed\":%d,\"passed\":%s,\"sha256\":\"%s\",\"itemsReadFromDisk\":%d}"),Request.Seed,Passed?TEXT("true"):TEXT("false"),*Plan.Fingerprint,Reloaded.Items.Num());
            UE_LOG(LogTemp,Display,TEXT("SM_RANDOMIZER_UE_SEED_%s seed=%d %s"),Passed?TEXT("PASS"):TEXT("FAIL"),Request.Seed,*ServiceError);
        }
        Report+=FString::Printf(TEXT("],\"passed\":%s}"),Passed?TEXT("true"):TEXT("false"));
        FFileHelper::SaveStringToFile(Report,*(TestRoot/TEXT("randomizer-ue-verification.json")));
        UE_LOG(LogTemp,Display,TEXT("SM_RANDOMIZER_UE_%s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),*ServiceError);
        FPlatformMisc::RequestExit(false);return;
    }
    const FString RomPath = SMRom::LocalPath(Root);
    if (!SMRom::Validate(RomPath, Failure)) return;
    CoreHandle = SMNativeLibrary::Open(Root);
    if (!CoreHandle) { Failure = TEXT("Native library is missing. Reinstall the game."); return; }
#define SM_LOAD(Member, Name) Member = reinterpret_cast<decltype(Member)>(FPlatformProcess::GetDllExport(CoreHandle, TEXT(Name))); if (!Member) { Failure = TEXT("API native incomplete: " Name); return; }
    SM_LOAD(Init, "sm_init"); SM_LOAD(Step, "sm_step"); SM_LOAD(Pixels, "sm_pixels");
    SM_LOAD(Scene,"sm_scene"); SM_LOAD(FarMask,"sm_background_mask"); SM_LOAD(Layers,"sm_layers"); SM_LOAD(Emission,"sm_emission"); SM_LOAD(Lightmap,"sm_lightmap");
    SM_LOAD(SetParallax,"sm_set_parallax"); SM_LOAD(ParallaxSupported,"sm_parallax_supported");
    SM_LOAD(Audio, "sm_audio"); SM_LOAD(Error, "sm_error"); SM_LOAD(State, "sm_state");
    SM_LOAD(Frame, "sm_frame"); SM_LOAD(Opcodes, "sm_cpu_opcodes"); SM_LOAD(Shutdown, "sm_shutdown");
    SM_LOAD(Area, "sm_area"); SM_LOAD(CameraX, "sm_camera_x"); SM_LOAD(CameraY, "sm_camera_y"); SM_LOAD(Brightness, "sm_brightness"); SM_LOAD(MessageActive,"sm_message_active");
    SM_LOAD(CreditsState,"sm_credits_state");
    SM_LOAD(CinemaLights,"sm_cinema_lights");SM_LOAD(CinemaState,"sm_cinema_state");SM_LOAD(TestCinematic,"sm_test_cinematic");
    SM_LOAD(PresentationKind,"sm_presentation_kind");SM_LOAD(UiOverlay,"sm_ui_overlay");
    SM_LOAD(TestMessage,"sm_test_message");
    SM_LOAD(TestEscapeTimer,"sm_test_escape_timer");
    SM_LOAD(TestRoom,"sm_test_room");
    SM_LOAD(TestAwaken,"sm_test_awaken");
    SM_LOAD(PauseData,"sm_pause_data");SM_LOAD(SetBorder,"sm_set_border_extension");
    SM_LOAD(ClearDecorations,"sm_decor_clear");SM_LOAD(SetDecoration,"sm_decor_set");
    SM_LOAD(FxType,"sm_fx_type"); SM_LOAD(WaterY,"sm_water_y"); SM_LOAD(HeatedRoom,"sm_heated_room"); SM_LOAD(SetEngineWeather,"sm_set_engine_weather");
    SM_LOAD(SetAssistedWallJump,"sm_set_assisted_walljump");
    SM_LOAD(SetAssistedSpaceJump,"sm_set_assisted_spacejump");
    SM_LOAD(VisualState,"sm_visual_state"); SM_LOAD(SetCombatEffects,"sm_set_combat_effects");
    SM_LOAD(TestCombatEquipment,"sm_test_combat_equipment");
    SM_LOAD(TestAllEquipment,"sm_test_all_equipment");
    SM_LOAD(SetWidescreen,"sm_set_widescreen"); SM_LOAD(WideAvailable,"sm_wide_available");
    SM_LOAD(GameOverState,"sm_gameover_state");SM_LOAD(GameOverLabels,"sm_gameover_labels");
    SM_LOAD(WideScene,"sm_wide_scene"); SM_LOAD(WideLayers,"sm_wide_metadata");
    SM_LOAD(WideFar,"sm_wide_background"); SM_LOAD(WideHud,"sm_wide_overlay");
    SM_LOAD(Teleport,"sm_teleport"); SM_LOAD(TeleportCount,"sm_teleport_count"); SM_LOAD(TeleportName,"sm_teleport_name");
    SM_LOAD(Save, "sm_save"); SM_LOAD(SamusX, "sm_samus_x"); SM_LOAD(SamusY, "sm_samus_y"); SM_LOAD(Room, "sm_room");
#undef SM_LOAD
    int StartFixtureIndex=-1,BossFixtureDoor=-1,AreaFixtureDoor=-1;bool IndicatorFixture=false,DoorColorFixture=false,AreaFixture=false;
#if !UE_BUILD_SHIPPING
    FParse::Value(FCommandLine::Get(),TEXT("SMStartTest="),StartFixtureIndex);
    AreaFixture=FParse::Param(FCommandLine::Get(),TEXT("SMAreaTest"));
    FParse::Value(FCommandLine::Get(),TEXT("SMAreaConnectionTest="),AreaFixtureDoor);
    AreaFixture|=AreaFixtureDoor>=0;
    if(AreaFixture && StartFixtureIndex<0)StartFixtureIndex=0;
    DoorColorFixture=FParse::Param(FCommandLine::Get(),TEXT("SMDoorColorTest"));
    if(DoorColorFixture && StartFixtureIndex<0)StartFixtureIndex=0;
    IndicatorFixture=FParse::Param(FCommandLine::Get(),TEXT("SMIndicatorTest"));
    if(IndicatorFixture && StartFixtureIndex<0)StartFixtureIndex=1;
    FParse::Value(FCommandLine::Get(),TEXT("SMBossConnectionTest="),BossFixtureDoor);
    if(BossFixtureDoor>=0 && StartFixtureIndex<0)StartFixtureIndex=0;
#endif
    const bool LightingPreview=FParse::Param(FCommandLine::Get(),TEXT("SMLightingPreview"));
#if !UE_BUILD_SHIPPING
    RelicTest=FParse::Param(FCommandLine::Get(),TEXT("SMRelicTest"));
#endif
    PixelTest = FParse::Param(FCommandLine::Get(), TEXT("SMPixelTest"));
#if !UE_BUILD_SHIPPING
    if(IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")))){
        FParse::Value(FCommandLine::Get(),TEXT("SMSuitTest="),SuitTest);
        if(SuitTest!=1 && SuitTest!=2)SuitTest=0;
    }
    GameOverTest=FParse::Param(FCommandLine::Get(),TEXT("SMGameOverTest")) && IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")));
#endif
    CinemaTest=FParse::Param(FCommandLine::Get(),TEXT("SMCinemaTest"));
    FParse::Value(FCommandLine::Get(),TEXT("SMCinemaCase="),CinemaCase);
    FParse::Value(FCommandLine::Get(),TEXT("SMCinemaSample="),CinemaSample);
    CreditsTest=FParse::Param(FCommandLine::Get(),TEXT("SMCreditsTest"));
    AutoTest = FParse::Param(FCommandLine::Get(), TEXT("SMTest"));
    TitleTest=FParse::Param(FCommandLine::Get(),TEXT("SMTitleTest")) && IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")));
    FParse::Value(FCommandLine::Get(),TEXT("SMTitleTestCase="),TitleTestCase);
    const bool SavedRoomTest=FParse::Param(FCommandLine::Get(),TEXT("SMTestSavedRoom"));
    const bool HudPreview=FParse::Param(FCommandLine::Get(),TEXT("SMHudPreview"));
    ReliefTest=FParse::Param(FCommandLine::Get(),TEXT("SMReliefTest"));
    DepthTest=FParse::Param(FCommandLine::Get(),TEXT("SMDepthTest")) || ReliefTest;
    DepthMotion=FParse::Param(FCommandLine::Get(),TEXT("SMDepthMotion"));
    WeatherTest=FParse::Param(FCommandLine::Get(),TEXT("SMWeatherTest"));
    TeleportUiTest=FParse::Param(FCommandLine::Get(),TEXT("SMTeleportUiTest"));
    FootstepTest=FParse::Param(FCommandLine::Get(),TEXT("SMFootstepTest"));
    ElectricTest=FParse::Param(FCommandLine::Get(),TEXT("SMElectricTest"));
    CombatTest=ElectricTest || FParse::Param(FCommandLine::Get(),TEXT("SMCombatTest"));
    DisplayTest=FParse::Param(FCommandLine::Get(),TEXT("SMDisplayTest"));
    PauseTest=FParse::Param(FCommandLine::Get(),TEXT("SMPauseTest"));
    ObjectivePauseTest=FParse::Param(FCommandLine::Get(),TEXT("SMObjectivePauseTest"));
    PauseTest|=ObjectivePauseTest;
    FParse::Value(FCommandLine::Get(),TEXT("SMTestHoldState="),TestHoldState);
    WideTest=StartFixtureIndex>=0 || RelicTest || CinemaTest || CreditsTest || DisplayTest || CombatTest || PauseTest || FootstepTest || FParse::Param(FCommandLine::Get(),TEXT("SMWideTest"));
    AutoTest |= WideTest || TeleportUiTest || PixelTest || DepthTest || DepthMotion || WeatherTest;
    if(GameOverTest){AutoTest=true;WideTest=!FParse::Param(FCommandLine::Get(),TEXT("SMGameOverClassic"));}
    if(SuitTest){AutoTest=WideTest=true;}
    if(TitleTest){AutoTest=true;WideTest=!FParse::Param(FCommandLine::Get(),TEXT("SMTitleClassic"));}
    RecapCheck=FParse::Param(FCommandLine::Get(),TEXT("SMRecapCheck")) && IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")));
    SporeCheck=FParse::Param(FCommandLine::Get(),TEXT("SMSporeCheck")) && IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")));
    PlaytestCheck=SporeCheck || RecapCheck || (FParse::Param(FCommandLine::Get(),TEXT("SMPlaytestCheck")) && IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY"))));
    if(PlaytestCheck){AutoTest=WideTest=true;}
    if(AutoTest)ImageScaling=1; // Keep existing pixel-comparison fixtures at integer scale.
#if !UE_BUILD_SHIPPING
    if(AutoTest && IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")))){
        FParse::Value(FCommandLine::Get(),TEXT("SMImageScaling="),ImageScaling);
        ImageScaling=FMath::Clamp(ImageScaling,0,1);
    }
#endif
    if(AutoTest) {Depth=DepthMotion || WeatherTest;Relief=WeatherTest;EngineWeather=WeatherTest || TeleportUiTest;}
    if(WideTest){Depth=Relief=EngineWeather=true;}
    if(DepthMotion || WideTest){Intensity=.75f;Exposure=1.10f;}
    FParse::Value(FCommandLine::Get(), TEXT("SMTestFrames="), TestFrames);
    SettingsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SM/Presentation.ini"));
    SMLocalization::Load(SettingsPath);
    if(HudPreview) {
        const FString PreviewSettings=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SMTests/HUDPreview/Presentation.ini"));
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(PreviewSettings),true);
        if(!IFileManager::Get().FileExists(*PreviewSettings))IFileManager::Get().Copy(*PreviewSettings,*SettingsPath);
        SettingsPath=PreviewSettings;
    }
    if (!AutoTest) {
        Intensity=.75f;Exposure=1.10f;
        FConfigFile Settings;Settings.Read(SettingsPath);
        Settings.GetBool(TEXT("Atmosphere"), TEXT("Enabled"), Atmosphere);
        Settings.GetFloat(TEXT("Atmosphere"), TEXT("Intensity"), Intensity);
        Settings.GetFloat(TEXT("Atmosphere"), TEXT("Brightness"), Exposure);
        int Version=0;
        Settings.GetInt(TEXT("Atmosphere"),TEXT("Version"),Version);
        if(Version<3) { Intensity=.75f;Exposure=1.10f; }
        Settings.GetInt(TEXT("Atmosphere"),TEXT("BlendMode"),BlendMode);
        Settings.GetBool(TEXT("Atmosphere"),TEXT("Depth"),Depth);
        Settings.GetBool(TEXT("Atmosphere"),TEXT("Relief"),Relief);
        Settings.GetBool(TEXT("Atmosphere"),TEXT("EngineWeather"),EngineWeather);
        Settings.GetBool(TEXT("Controls"),TEXT("AssistedWallJump"),AssistedWallJump);
        Settings.GetBool(TEXT("Controls"),TEXT("AssistedSpaceJump"),AssistedSpaceJump);
        Settings.GetBool(TEXT("Display"),TEXT("Widescreen"),Widescreen);
        Settings.GetInt(TEXT("Display"),TEXT("ImageScaling"),ImageScaling);
        ImageScaling=FMath::Clamp(ImageScaling,0,1);
        // Automatic room filling is retired pending authored room decorations.
        // The custom Canvas pause was rejected. Native pause owns the UI.
        Settings.GetFloat(TEXT("Atmosphere"),TEXT("FlashStrength"),FlashStrength);
        FlashStrength=FMath::Clamp(FlashStrength,0.f,1.f);
        BlendMode=FMath::Clamp(BlendMode,0,1);
        Settings.GetBool(TEXT("Atmosphere"),TEXT("Parallax"),Parallax);
        Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
        Exposure = FMath::Clamp(Exposure, 0.75f, 1.25f);
    }
    const FString SaveDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / (AutoTest ? TEXT("SMTests") : (HudPreview?TEXT("SMTests/HUDPreview"):(LightingPreview ? TEXT("SMPreview") : TEXT("SM")))));
    IFileManager::Get().MakeDirectory(*SaveDir, true);
    if(DepthMotion)IFileManager::Get().MakeDirectory(*(SaveDir/TEXT("depth-motion")),true);
    FString SaveFile=SaveDir/(AutoTest?FString::Printf(TEXT("validation-%llu.sram"),FPlatformTime::Cycles64()):TEXT("sram.dat"));
    if(HudPreview && !IFileManager::Get().FileExists(*SaveFile)) {
        const FString Fixture=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SMTests/HUD-all-items.sram"));
        if(IFileManager::Get().Copy(*SaveFile,*Fixture)!=COPY_OK){Failure=TEXT("Sauvegarde HUD absente : lancer Scripts/test-display-regressions.py.");return;}
    }
    if(AutoTest && (SavedRoomTest || SuitTest)) {
        FString Saved=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SMPreview/sram.dat"));
        FParse::Value(FCommandLine::Get(),TEXT("SMTestSave="),Saved);
        if(IFileManager::Get().Copy(*SaveFile,*Saved)!=COPY_OK) {Failure=TEXT("Copie de la sauvegarde de validation impossible.");return;}
    }
    FSMSeedPlan StartFixture;TArray<SmSeedItem> StartItems;
    if(StartFixtureIndex>=0){
        if(StartFixtureIndex>12 || !IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")))){Failure=TEXT("Native start fixture requires the isolated test root");return;}
        FString Json,ErrorText;const FString Path=Root/(FParse::Param(FCommandLine::Get(),TEXT("SMEscapeTest"))?TEXT("escape/integration"):FParse::Param(FCommandLine::Get(),TEXT("SMMinimizerTest"))?TEXT("minimizer/public"):FParse::Param(FCommandLine::Get(),TEXT("SMFastTourianTest"))?TEXT("fast-tourian/public"):FParse::Param(FCommandLine::Get(),TEXT("SMScavengerTest"))?TEXT("scavenger/public"):FParse::Param(FCommandLine::Get(),TEXT("SMObjectivePublicTest"))?TEXT("objective-public-seeds"):FParse::Param(FCommandLine::Get(),TEXT("SMObjectiveLogicTest"))?TEXT("objective-logic-seeds"):FParse::Param(FCommandLine::Get(),TEXT("SMObjectiveTest"))?TEXT("objective-seeds"):AreaFixture?TEXT("area-seeds"):DoorColorFixture?TEXT("door-color-seeds"):BossFixtureDoor>=0?TEXT("boss-seeds"):IndicatorFixture?TEXT("indicator-seeds"):TEXT("start-seeds"))/FString::Printf(TEXT("seed-%02d.json"),StartFixtureIndex);
        if(!FFileHelper::LoadFileToString(Json,*Path) || !FSMRandomizer::ReadPlan(Json,StartFixture,ErrorText)){Failure=ErrorText;return;}
        for(const auto& I:StartFixture.Items)StartItems.Add({uint32(I.Address),uint16(I.Plm),uint16(I.Kind)});
        auto Stage=reinterpret_cast<decltype(&sm_seed_stage)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_seed_stage")));
        if(!Stage || !Stage(StartItems.GetData(),100,TCHAR_TO_UTF8(*StartFixture.Fingerprint))){Failure=TEXT("Cannot stage start fixture");return;}
        SaveFile=SaveDir/(StartFixture.Fingerprint+FString::Printf(TEXT("-start-%llu.sram"),FPlatformTime::Cycles64()));
    }
    FSMSeedPlan RelicFixture;TArray<SmSeedItem> RelicItems;
    if(RelicTest){
        if(!IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")))){Failure=TEXT("Relic visual test requires an isolated root");return;}
        FString Json,ErrorText;
        if(!FFileHelper::LoadFileToString(Json,*(Root/TEXT("full-options/verified-0.json"))) || !FSMRandomizer::ReadPlan(Json,RelicFixture,ErrorText)){Failure=ErrorText;return;}
        // A deliberate presentation fixture, not a generated/spoiler placement:
        // show the selected artwork on the accessible Morph pedestal.
        for(const auto& I:RelicFixture.Items)RelicItems.Add({uint32(I.Address),uint16(I.Location==TEXT("Morphing Ball")?0xef27:I.Plm),uint16(I.Location==TEXT("Morphing Ball")?1:I.Kind)});
        auto Stage=reinterpret_cast<decltype(&sm_seed_stage)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_seed_stage")));
        if(!Stage || !Stage(RelicItems.GetData(),100,TCHAR_TO_UTF8(*RelicFixture.Fingerprint))){Failure=TEXT("Cannot stage relic visual fixture");return;}
        SaveFile=SaveDir/(RelicFixture.Fingerprint+FString::Printf(TEXT("-relic-%llu.sram"),FPlatformTime::Cycles64()));
    }
    if (!Init(TCHAR_TO_UTF8(*RomPath), TCHAR_TO_UTF8(*SaveFile))) {
        Failure = UTF8_TO_TCHAR(Error()); return;
    }
    // The native presentation must also inherit the saved language in previews
    // and automated fixtures, which intentionally do not create the system menu.
    if(auto SetLanguage=reinterpret_cast<void(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_locale_set"))))SetLanguage(SMLocalization::Language());
    ApplyRomWindowIcon();
    if(StartFixtureIndex>=0){
        auto Enable=reinterpret_cast<decltype(&sm_slots_enable)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_slots_enable")));
        auto Set=reinterpret_cast<decltype(&sm_slots_set)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_slots_set")));
        auto Configure=reinterpret_cast<decltype(&sm_start_configure_world)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_start_configure_world")));
        auto Counts=reinterpret_cast<decltype(&sm_varia_ui_counted_configure)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_varia_ui_counted_configure")));
        if(!Enable || !Set || !Configure || !Counts){Failure=TEXT("Missing native start API");return;}
        auto Areas=reinterpret_cast<decltype(&sm_areas_configure)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_areas_configure")));
        auto Initial=reinterpret_cast<decltype(&sm_start_configure_initial_doors)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_start_configure_initial_doors")));
        if((!StartFixture.AreaDestinations.IsEmpty() && !Areas) || (!StartFixture.InitialDoors.IsEmpty() && !Initial)){Failure=TEXT("Missing area/initial door API");return;}
        auto Connections=reinterpret_cast<decltype(&sm_connections_configure)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_connections_configure")));
        if(!StartFixture.BossDestinations.IsEmpty() && !Connections){Failure=TEXT("Missing native boss API");return;}
        auto DoorColors=reinterpret_cast<decltype(&sm_doors_configure)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_doors_configure")));
        if(!StartFixture.DoorColors.IsEmpty() && !DoorColors){Failure=TEXT("Missing native door-color API");return;}
        auto Objectives=reinterpret_cast<decltype(&sm_objectives_configure)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_objectives_configure")));
        if(StartFixture.Objectives.count && !Objectives){Failure=TEXT("Missing native objective API");return;}
        auto Mini=reinterpret_cast<decltype(&sm_minimizer_configure)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_minimizer_configure")));
        if(StartFixture.Minimizer.version && !Mini){Failure=TEXT("Missing native Minimizer API");return;}
        auto EscapeClock=reinterpret_cast<decltype(&sm_escape_clock_configure)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_escape_clock_configure")));
        auto EscapeRouting=reinterpret_cast<decltype(&sm_escape_routing_configure)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_escape_routing_configure")));
        if(StartFixture.EscapeClock.version && (!EscapeClock || !EscapeRouting)){Failure=TEXT("Missing native escape API");return;}
        auto Scavenger=reinterpret_cast<decltype(&sm_scavenger_configure)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_scavenger_configure")));
        if(StartFixture.Scavenger.count && !Scavenger){Failure=TEXT("Missing native Scavenger API");return;}
        Enable(nullptr,nullptr);
        for(int I=0;I<3;I++){
            if(StartFixture.EscapeClock.version && (!EscapeClock(I,&StartFixture.EscapeClock) || !EscapeRouting(I,&StartFixture.EscapeRouting,TCHAR_TO_UTF8(*StartFixture.EscapeCatalog)))){Failure=TEXT("Rejected native escape fixture");return;}
            if(StartFixture.Minimizer.version && !Mini(I,&StartFixture.Minimizer,TCHAR_TO_UTF8(*StartFixture.MinimizerCatalog))){Failure=TEXT("Rejected native Minimizer fixture");return;}
            if(StartFixture.Scavenger.count && !Scavenger(I,&StartFixture.Scavenger,TCHAR_TO_UTF8(*StartFixture.ScavengerCatalog))){Failure=TEXT("Rejected native Scavenger fixture");return;}
            if(!StartFixture.AreaDestinations.IsEmpty() && !Areas(I,StartFixture.AreaDestinations.GetData(),StartFixture.AreaDestinations.Num(),TCHAR_TO_UTF8(*StartFixture.AreasCatalog))){Failure=TEXT("Rejected native area fixture");return;}
            if(StartFixture.Objectives.count && !Objectives(I,&StartFixture.Objectives,TCHAR_TO_UTF8(*StartFixture.ObjectivesCatalog))){Failure=TEXT("Rejected native objective fixture");return;}
            if(!StartFixture.InitialDoors.IsEmpty() && !Initial(I,StartFixture.StartSpawn,StartFixture.InitialDoors.GetData(),StartFixture.InitialDoors.Num())){Failure=TEXT("Rejected initial-door fixture");return;}
            if(!StartFixture.DoorColors.IsEmpty() && !DoorColors(I,StartFixture.DoorColors.GetData(),StartFixture.DoorColors.Num(),TCHAR_TO_UTF8(*StartFixture.DoorColorsCatalog))){Failure=TEXT("Rejected door-color fixture");return;}
            if(!StartFixture.BossDestinations.IsEmpty() && !Connections(I,StartFixture.BossDestinations.GetData(),StartFixture.BossDestinations.Num(),TCHAR_TO_UTF8(*StartFixture.ConnectionsCatalog))){Failure=TEXT("Rejected native boss fixture");return;}
            if(!Configure(I,StartFixture.StartSpawn,StartFixture.WorldPatches.GetData(),StartFixture.WorldPatches.Num(),TCHAR_TO_UTF8(*StartFixture.WorldCatalog),StartFixture.DoorIndicators.GetData(),StartFixture.DoorIndicators.Num()) ||
               !Set(I,1,1,StartItems.GetData(),100,TCHAR_TO_UTF8(*StartFixture.Fingerprint))){Failure=TEXT("Rejected native start fixture");return;}
        }
        Counts(StartFixture.HudCounts.GetData(),StartFixture.HudCounts.Num());
    }
    if(RelicTest){
        auto Enable=reinterpret_cast<decltype(&sm_slots_enable)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_slots_enable")));
        auto Set=reinterpret_cast<decltype(&sm_slots_set)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_slots_set")));
        auto Configure=reinterpret_cast<decltype(&sm_relic_configure)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_relic_configure")));
        RelicCount=reinterpret_cast<decltype(RelicCount)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_relic_count")));
        if(!Enable || !Set || !Configure || !RelicCount){Failure=TEXT("Missing relic API");return;}
        Enable(nullptr,nullptr);for(int I=0;I<3;I++){Set(I,1,1,RelicItems.GetData(),100,TCHAR_TO_UTF8(*RelicFixture.Fingerprint));Configure(I,20);}
    }
    CoreRomPath=RomPath;CoreSavePath=SaveFile;
    if(PauseTest)SettingsPath=FPaths::ChangeExtension(SaveFile,TEXT("presentation.ini"));
    ProfilePath=AutoTest?FPaths::ChangeExtension(SaveFile,TEXT("achievements.ini")):(SaveDir/TEXT("Achievements.ini"));
    FConfigFile Profile;Profile.Read(ProfilePath);
    Profile.GetInt(TEXT("Local"),TEXT("Unlocked"),AchievementBits);
    Profile.GetInt(TEXT("Local"),TEXT("EnemyKills"),AchievementTotalKills);
    SetBorder(BorderExtension);
    LoadRoomDecorations();
    SetAssistedWallJump(AutoTest?false:AssistedWallJump);
    SetAssistedSpaceJump(AutoTest?false:AssistedSpaceJump);
    Widescreen=AutoTest?WideTest:Widescreen;SetWidescreen(Widescreen);
    SetEngineWeather(EngineWeather && Atmosphere);
    SetCombatEffects(Atmosphere && (!AutoTest || WideTest));
    int Warmup = LightingPreview || HudPreview ? 8550 : 0;
    FParse::Value(FCommandLine::Get(), TEXT("SMWarmup="), Warmup);
    if ((AutoTest || LightingPreview || HudPreview) && Warmup > 0) {
        UE_LOG(LogTemp, Display, TEXT("SM_WARMUP frames=%d"), Warmup);
        for (int I = 0; I < Warmup; ++I) {
            int Button=(State()<7 || State()>18) && Frame()>180 && Frame()%120<2 ? 8 : 0;
            // Automated fixtures must navigate the redesigned mode row to Start Game.
            if((CinemaTest || GameOverTest || StartFixtureIndex>=0 || RelicTest) && Button && State()==2 && CinemaState && CinemaState(3)==3 && CinemaState(4)!=0)Button=32;
            if (!Step(Button)) {
                Failure = UTF8_TO_TCHAR(Error()); return;
            }
            if((StartFixtureIndex>=0 || LightingPreview || SavedRoomTest || HudPreview) && State()==8)break;
            if(State()==TestHoldState)break;
        }
    }
    if(RelicTest){
        if(State()!=8 || !TestRoom(0x9e9f,69*16+40,41*16+8)){Failure=TEXT("Relic fixture did not reach gameplay");return;}
        for(int I=0;I<2000;I++){if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}if(State()==8 && Room()==0x9e9f)break;}
        for(int I=0;I<120;I++)Step(0);
        RelicTestFrame=Frame();TestFrames=Frame()+700;
    }
    if(StartFixtureIndex>=0){
        const int ExpectedArea=StartFixture.StartSpawn==65534?6:StartFixture.StartSpawn>>8;
        if(State()!=8 || Area()!=ExpectedArea || Opcodes()){Failure=TEXT("Native start fixture did not reach its generated area");return;}
        TestFrames=Frame()+120;
        UE_LOG(LogTemp,Display,TEXT("SM_START_VISUAL_READY index=%d seed=%d area=%d room=%04x cpu=%llu"),StartFixtureIndex,StartFixture.Seed,Area(),Room(),Opcodes());
        if(StartFixture.Objectives.count){
            auto ObjectivesState=reinterpret_cast<decltype(&sm_objectives_state)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_objectives_state")));
            if(!ObjectivesState || ObjectivesState(0)!=StartFixture.Objectives.count || ObjectivesState(1)!=StartFixture.Objectives.required){Failure=TEXT("Objective fixture not applied to the running seed");return;}
            UE_LOG(LogTemp,Display,TEXT("SM_OBJECTIVES_VISUAL_READY seed=%d count=%d required=%d"),StartFixture.Seed,ObjectivesState(0),ObjectivesState(1));
#if !UE_BUILD_SHIPPING
            if(FParse::Param(FCommandLine::Get(),TEXT("SMObjectiveLogicTest"))){
                FSMTracker Oracle;Oracle.Initialize(CoreHandle);
                FSMGameProfile ProfileFixture;ProfileFixture.Id=TEXT("objective-oracle-fixture");
                ProfileFixture.Randomized=true;ProfileFixture.Generated=true;ProfileFixture.Plan=StartFixture;
                const double Deadline=FPlatformTime::Seconds()+20;
                do{
                    Oracle.Tick(true,true,true,Root,ProfileFixture,0);
                    if(!Oracle.ObjectivesJson.IsEmpty())break;
                    FPlatformProcess::Sleep(0.01f);
                }while(FPlatformTime::Seconds()<Deadline);
                auto Evidence=SMSeedSettings::Parse(Oracle.ObjectivesJson);
                if(!Evidence.IsValid() || Evidence->GetStringField(TEXT("progressSource"))!=TEXT("native-evaluator") ||
                   Evidence->GetIntegerField(TEXT("count"))!=StartFixture.Objectives.count ||
                   Evidence->GetIntegerField(TEXT("required"))!=StartFixture.Objectives.required){
                    Failure=TEXT("Unreal objective oracle failed: ")+Oracle.Status;return;
                }
                FFileHelper::SaveStringToFile(Oracle.ObjectivesJson,*(SaveDir/TEXT("objective-oracle.json")));
                UE_LOG(LogTemp,Display,TEXT("SM_OBJECTIVE_ORACLE_PASS seed=%d count=%d required=%d"),StartFixture.Seed,StartFixture.Objectives.count,StartFixture.Objectives.required);
            }
#endif
        }
    }
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("SMMapIconsTest"))){
        auto Explore=reinterpret_cast<decltype(&sm_test_map_explored)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_map_explored")));
        if(StartFixtureIndex<0 || !PauseTest || !Explore || !Explore(1)){Failure=TEXT("Map icon fixture requires an isolated generated start and pause test");return;}
        UE_LOG(LogTemp,Display,TEXT("SM_MAP_ICONS_VISUAL_FIXTURE seed=%d explored=all cpu=%llu"),StartFixture.Seed,Opcodes());
    }
#endif
    if(IndicatorFixture){
        if(!TestRoom(0x948c,96,104)){Failure=TEXT("Indicator room fixture rejected");return;}
        for(int I=0;I<2000;I++){if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}if(State()==8 && Room()==0x948c)break;}
        if(State()!=8 || Room()!=0x948c){Failure=TEXT("Indicator room did not load");return;}
        for(int I=0;I<120;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}
        TestFrames=Frame()+120;
        UE_LOG(LogTemp,Display,TEXT("SM_INDICATOR_VISUAL_READY indicators=%d room=%04x cpu=%llu"),StartFixture.DoorIndicators.Num(),Room(),Opcodes());
    }
    if(AreaFixtureDoor>=0){
        auto AreaDoor=reinterpret_cast<decltype(&sm_test_area_connection)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_area_connection")));
        if(AreaFixtureDoor>=32 || !AreaDoor || !TestAllEquipment || !TestAllEquipment() || !AreaDoor(AreaFixtureDoor)){Failure=TEXT("Area room fixture rejected");return;}
        for(int I=0;I<2000;I++){if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}if(State()==8)break;}
        if(State()!=8){Failure=TEXT("Area room did not load");return;}
        for(int I=0;I<440;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}
        if(Opcodes()){Failure=TEXT("Area test executed emulated CPU instructions");return;}
        TestFrames=Frame()+120;
        UE_LOG(LogTemp,Display,TEXT("SM_AREA_VISUAL_READY seed=%d source=%d destination=%d room=%04x cpu=%llu"),StartFixture.Seed,AreaFixtureDoor,StartFixture.AreaDestinations[AreaFixtureDoor],Room(),Opcodes());
    }
    if(BossFixtureDoor>=0){
        auto BossDoor=reinterpret_cast<decltype(&sm_test_boss_connection)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_boss_connection")));
        if(!BossDoor || !TestAllEquipment || !TestAllEquipment() || !BossDoor(BossFixtureDoor)){Failure=TEXT("Boss room fixture rejected");return;}
        for(int I=0;I<2000;I++){if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}if(State()==8)break;}
        if(State()!=8){Failure=TEXT("Boss room did not load");return;}
        for(int I=0;I<120;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}
        if(Opcodes()){Failure=TEXT("Boss test executed emulated CPU instructions");return;}
        TestFrames=Frame()+120;
        UE_LOG(LogTemp,Display,TEXT("SM_BOSS_VISUAL_READY seed=%d source=%d destination=%d room=%04x cpu=%llu"),StartFixture.Seed,BossFixtureDoor,StartFixture.BossDestinations[BossFixtureDoor],Room(),Opcodes());
    }
    if(DoorColorFixture){
        if(!TestAllEquipment || !TestAllEquipment() || !TestRoom(0x948c,680,104)){Failure=TEXT("Colored-door room fixture rejected");return;}
        for(int I=0;I<2000;I++){if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}if(State()==8 && Room()==0x948c)break;}
        if(State()!=8 || Room()!=0x948c){Failure=TEXT("Colored-door room did not load");return;}
        for(int I=0;I<120;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}
        auto Ram=reinterpret_cast<decltype(&sm_simulation_ram)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
        bool Found=false;
        if(Ram)for(int I=0;I<40;I++){
            const uint8* P=Ram()+0x1c37+I*2;uint16 Plm=P[0]|P[1]<<8;
            if(Plm==0xf74b)Found=true; // Seed 15092500: original KihunterRight is Plasma.
        }
        if(StartFixture.Seed==15092500 && !Found){Failure=TEXT("Generated Plasma door was not instantiated");return;}
        if(Opcodes()){Failure=TEXT("Colored-door test executed emulated CPU instructions");return;}
        TestFrames=Frame()+120;
        UE_LOG(LogTemp,Display,TEXT("SM_DOOR_COLOR_VISUAL_READY seed=%d room=%04x plasma=%d indicators=%d cpu=%llu"),StartFixture.Seed,Room(),Found,StartFixture.DoorIndicators.Num(),Opcodes());
    }
    int TestDestination=-1;
    FParse::Value(FCommandLine::Get(),TEXT("SMTestTeleport="),TestDestination);
    if(AutoTest && TestDestination>=0) {
        if(!Teleport(TestDestination)) {Failure=TEXT("Teleportation de test refusee.");return;}
        for(int I=0;I<600 && State()!=8;++I)if(!Step(0)) {Failure=UTF8_TO_TCHAR(Error());return;}
        if(State()!=8){Failure=TEXT("Chargement de destination incomplet.");return;}
        UE_LOG(LogTemp,Display,TEXT("SM_TELEPORT_READY destination=%d room=%04x fx=%d water=%d heat=%d"),TestDestination,Room(),FxType(),WaterY(),HeatedRoom());
    }
    if(AutoTest && SavedRoomTest && !DisplayTest && !PauseTest)TestFrames=Frame()+60;
    if(DisplayTest)for(int I=0;I<440;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}
    if(PauseTest || FootstepTest)for(int I=0;I<440;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}
    AchievementItems=PauseData(0);AchievementBeams=PauseData(2);AchievementPrimed=State()==8;
    if(CombatTest)for(int I=0;I<400;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}
    if(CombatTest && !TestCombatEquipment()){Failure=TEXT("Equipement de test refuse");return;}
    if(ElectricTest) {
        TestAwaken();
        if(!TestRoom(0x9f11,39,139)){Failure=TEXT("Salle de test electrique refusee");return;}
        for(int I=0;I<440;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}
    }
    CinemaTexture=UTexture2D::CreateTransient(32,3,PF_A32B32G32R32F);
    CinemaTexture->Filter=TF_Nearest;CinemaTexture->SRGB=false;CinemaTexture->NeverStream=true;CinemaTexture->UpdateResource();
    EffectsTexture=UTexture2D::CreateTransient(64,2,PF_A32B32G32R32F);
    EffectsTexture->Filter=TF_Nearest;EffectsTexture->SRGB=false;EffectsTexture->NeverStream=true;EffectsTexture->UpdateResource();
    WideGameTexture=MakePresentationTexture(400,240,TF_Nearest);
    WideSceneTexture=MakePresentationTexture(400,240,TF_Nearest);
    WideMaskTexture=MakePresentationTexture(400,240,TF_Nearest);
    WideHudTexture=MakePresentationTexture(400,240,TF_Nearest);
    UiTexture=MakePresentationTexture(256,240,TF_Nearest);
    GameTexture=MakePresentationTexture(256,240,TF_Nearest);
    SceneTexture=MakePresentationTexture(256,240,TF_Nearest);
    EmissionTexture=MakePresentationTexture(256,240,TF_Nearest);
    EnvironmentMaskTexture=MakePresentationTexture(256,240,TF_Nearest);
    LightTexture=MakePresentationTexture(64,60,TF_Bilinear);
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SM/M_Present.M_Present"))) {
        PresentMaterial = UMaterialInstanceDynamic::Create(Material, this);
        PresentMaterial->SetTextureParameterValue(TEXT("EnvironmentMask"), EnvironmentMaskTexture);
        PresentMaterial->SetTextureParameterValue(TEXT("EffectsTexture"),EffectsTexture);
        PresentMaterial->SetTextureParameterValue(TEXT("CinemaTexture"),CinemaTexture);
        PresentMaterial->SetTextureParameterValue(TEXT("GameTexture"), GameTexture);
        PresentMaterial->SetTextureParameterValue(TEXT("SceneTexture"), SceneTexture);
        PresentMaterial->SetTextureParameterValue(TEXT("EmissionTexture"), EmissionTexture);
        PresentMaterial->SetTextureParameterValue(TEXT("LightTexture"), LightTexture);
    }
    LoadGameOverPresentation();
    LoadTitlePresentation();
    if(SuitTest && !PrepareSuitCapture()){Failure=TEXT("Suit capture fixture failed; see log.");return;}
    if(GameOverTest){
        auto Test=reinterpret_cast<int(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_gameover")));
        if(!Test || !Test()){Failure=TEXT("Game Over fixture requires loaded gameplay.");return;}
        for(int I=0;I<2400 && GameOverState(2)<4;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return;}
        if(GameOverState(2)!=4){Failure=TEXT("Game Over fixture did not finish its music cue and fade.");UE_LOG(LogTemp,Error,TEXT("SM_GAMEOVER_FIXTURE_NOT_READY phase=%d"),GameOverState(2));return;}
        TestFrames=0; // Dedicated captures below run for 120 rendered frames.
    }
    AudioWave = NewObject<USoundWaveProcedural>(this);
    AudioWave->SetSampleRate(44100);
    AudioWave->NumChannels = 2;
    AudioWave->Duration = INDEFINITELY_LOOPING_DURATION;
    AudioWave->bLooping = false;
    AudioWave->SoundGroup = SOUNDGROUP_Music;
    AudioComponent = NewObject<UAudioComponent>(this);
    AudioComponent->bIsUISound = true;
    AudioComponent->bAllowSpatialization = false;
    AudioComponent->SetSound(AudioWave);
    AudioComponent->RegisterComponent();
    AudioComponent->Play();
    PlayerOwner->SetInputMode(FInputModeGameOnly());
    PlayerOwner->bShowMouseCursor = false;
    Ready = true;
    if(!AutoTest && !HudPreview && !LightingPreview) {
        SystemMenu=MakeShared<FSMSystemMenu>(*this);SystemMenu->Initialize(false);
    }
    UE_LOG(LogTemp, Display, TEXT("SM_NATIVE_READY engine=Unreal mode=C-only rom_sha1=%s"), SMRom::Sha1);
}
uint16 ASMHUD::ReadButtons() const {
    if(SystemMenu)return SystemMenu->Buttons();
    uint16 Buttons = 0;
    auto Press = [this, &Buttons](FKey Key, FKey Pad, uint16 Mask) {
        if (PlayerOwner->IsInputKeyDown(Key) || PlayerOwner->IsInputKeyDown(Pad)) Buttons |= Mask;
    };
    Press(EKeys::Z, EKeys::Gamepad_FaceButton_Bottom, 256);
    Press(EKeys::X, EKeys::Gamepad_FaceButton_Left, 1);
    Press(EKeys::RightShift, EKeys::Gamepad_Special_Left, 4);
    Press(EKeys::Enter, EKeys::Gamepad_Special_Right, 8);
    Press(EKeys::Up, EKeys::Gamepad_DPad_Up, 16);
    Press(EKeys::Down, EKeys::Gamepad_DPad_Down, 32);
    Press(EKeys::Left, EKeys::Gamepad_DPad_Left, 64);
    Press(EKeys::Right, EKeys::Gamepad_DPad_Right, 128);
    Press(EKeys::C, EKeys::Gamepad_FaceButton_Right, 2);
    Press(EKeys::S, EKeys::Gamepad_FaceButton_Top, 512);
    Press(EKeys::A, EKeys::Gamepad_LeftShoulder, 1024);
    Press(EKeys::D, EKeys::Gamepad_RightShoulder, 2048);
    Press(EKeys::Q, EKeys::Gamepad_LeftTrigger, 0x1000);
    Press(EKeys::E, EKeys::Gamepad_RightTrigger, 0x2000);
    const float X = PlayerOwner->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
    const float Y = PlayerOwner->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
    if (X < -0.3f) Buttons |= 64; if (X > 0.3f) Buttons |= 128;
    if (Y < -0.3f) Buttons |= 32; if (Y > 0.3f) Buttons |= 16;
    return Buttons;
}
void ASMHUD::Tick(float DeltaSeconds) {
    if(SuitTest)DeltaSeconds=736.f/44100.f;
    Super::Tick(DeltaSeconds);
    if (RomSetup) {
        if (!RomSetup->Completed) {
#if !UE_BUILD_SHIPPING
            const FString TestRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT(".."));
            if (FParse::Param(FCommandLine::Get(), TEXT("SMRomSetupTest")) &&
                IFileManager::Get().FileExists(*(TestRoot / TEXT("ISOLATED_TEST_DIRECTORY")))) {
                ++RomSetupTestTicks;
                if (RomSetupTestTicks == 60) FScreenshotRequest::RequestScreenshot(TestRoot / TEXT("rom-required.png"), true, false);
                if (RomSetupTestTicks == 90) {
                    const bool Blocked = !CoreHandle && !Ready && !GameTexture;
                    UE_LOG(LogTemp, Display, TEXT("SM_ROM_GATE_TEST %s"), Blocked ? TEXT("PASS") : TEXT("FAIL"));
                    FPlatformMisc::RequestExitWithStatus(false, Blocked ? 0 : 1);
                }
            }
#endif
            return;
        }
        RomSetup.Reset();
        PlayerOwner->FlushPressedKeys();
        PlayerOwner->bShowMouseCursor = false;
        PlayerOwner->SetInputMode(FInputModeGameOnly());
        StartVerifiedGame();
        return;
    }
    if(Ready && TitleTest)TickTitleTest(DeltaSeconds);
    if(Ready && TickStartupWarnings(DeltaSeconds))return;
    if(Ready && TickDialogue(DeltaSeconds))return;
    if(Ready && PlaytestCheck)TickPlaytestCheck();
    if(Ready && TickRunRecap(DeltaSeconds))return;
    if(SystemMenu){
        SystemMenu->Tick();
        if(SystemMenu->IsOpen())return;
    }
    // Native pause inputs, animation and map scrolling continue in Step().
    if(Ready && TeleportUiTest)TickTeleportUiTest();
    if (PlayerOwner && (PlayerOwner->WasInputKeyJustPressed(EKeys::Escape) ||
        (SystemMenu && (PlayerOwner->WasInputKeyJustPressed(EKeys::F4) || PlayerOwner->WasInputKeyJustPressed(EKeys::Gamepad_RightThumbstick))))) {
        if(TeleportMenu) {TeleportMenu=false;AudioComponent->SetPaused(Paused);return;}
        if(SystemMenu){SystemMenu->SetOpen(true);return;}
        PlayerOwner->ConsoleCommand(TEXT("quit"));
    }
    if (!Ready) return;
    if(CinemaTest){TickCinemaTest();return;}
    if(CreditsTest){TickCreditsTest();return;}
    if(PlayerOwner->WasInputKeyJustPressed(EKeys::F8) &&
       (PlayerOwner->IsInputKeyDown(EKeys::LeftControl)||PlayerOwner->IsInputKeyDown(EKeys::RightControl))) {
        const int Count=LoadRoomDecorations();
        Notice=SMLocalization::Format(TEXT("Reloaded painted scenery: {0} tiles."),{Count});NoticeUntil=FPlatformTime::Seconds()+3;
        return;
    }
    if(PlayerOwner->WasInputKeyJustPressed(EKeys::F10)) {
        if(TeleportMenu) {TeleportMenu=false;AudioComponent->SetPaused(Paused);return;}
        if(State()==8 && !MessageActive()) {TeleportMenu=true;AudioComponent->SetPaused(true);}
    }
    if(TeleportMenu) {
        if(PlayerOwner->WasInputKeyJustPressed(EKeys::Up) || PlayerOwner->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Up))
            TeleportSelection=(TeleportSelection+TeleportCount()-1)%TeleportCount();
        if(PlayerOwner->WasInputKeyJustPressed(EKeys::Down) || PlayerOwner->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Down))
            TeleportSelection=(TeleportSelection+1)%TeleportCount();
        if(PlayerOwner->WasInputKeyJustPressed(EKeys::Enter) || PlayerOwner->WasInputKeyJustPressed(EKeys::Z) || PlayerOwner->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom)) {
            if(Teleport(TeleportSelection)) {
                TeleportMenu=false;Paused=false;Accumulator=0;PresentationRoom=-1;
                AudioWave->ResetAudio();AudioComponent->SetPaused(false);
                Notice=SMLocalization::Format(TEXT("Destination: {0}"),{FString(UTF8_TO_TCHAR(TeleportName(TeleportSelection)))});
                NoticeUntil=FPlatformTime::Seconds()+4;
                UE_LOG(LogTemp,Display,TEXT("SM_TELEPORT_REQUEST destination=%d"),TeleportSelection);
            } else {Notice=SMLocalization::Text(FString(TEXT("Wait for the current action to finish, then try again.")));NoticeUntil=FPlatformTime::Seconds()+3;}
        }
        return;
    }
    if(DepthMotion)++MotionTicks;
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::P)) { Paused = !Paused; AudioComponent->SetPaused(Paused); }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F1)) ShowHelp = !ShowHelp;
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F2)) { Atmosphere = !Atmosphere; NotifySettings(); }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F3)) { Intensity = Intensity >= .99f ? .25f : FMath::Min(1.f,(FMath::FloorToInt(Intensity*4)+1)*.25f); NotifySettings(); }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F4)) { Exposure = Exposure<.99f ? 1.f : (Exposure<1.09f ? 1.10f : (Exposure<1.14f ? 1.15f : .85f)); NotifySettings(); }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F12)) {
        const bool Space=PlayerOwner->IsInputKeyDown(EKeys::LeftShift) || PlayerOwner->IsInputKeyDown(EKeys::RightShift) ||
            PlayerOwner->IsInputKeyDown(EKeys::LeftControl) || PlayerOwner->IsInputKeyDown(EKeys::RightControl);
        if(Space) {
            AssistedSpaceJump=!AssistedSpaceJump;SetAssistedSpaceJump(AssistedSpaceJump);
        } else {AssistedWallJump=!AssistedWallJump;SetAssistedWallJump(AssistedWallJump);}
        NotifySettings();
        Notice=SMLocalization::Format(TEXT("{0}: {1}"),{SMLocalization::Text(FString(Space?TEXT("Space Jump"):TEXT("Wall jump"))),
            SMLocalization::Text(FString((Space?AssistedSpaceJump:AssistedWallJump)?TEXT("Assisted"):TEXT("Original")))});
    }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F9)) { EngineWeather=!EngineWeather; NotifySettings(); }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F8)) { Relief=!Relief; RefreshScenePresentation(); NotifySettings(); }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F7)) { Depth=!Depth; RefreshScenePresentation(); NotifySettings(); }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F6)) { BlendMode=1-BlendMode; NotifySettings(); }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F5)) { Parallax=!Parallax; NotifySettings(); }
    if (PlayerOwner->WasInputKeyJustPressed(EKeys::F11)) {
        if(PlayerOwner->IsInputKeyDown(EKeys::LeftControl)||PlayerOwner->IsInputKeyDown(EKeys::RightControl)) {
            Widescreen=!Widescreen;SetWidescreen(Widescreen);NotifySettings();
        } else if(SystemMenu)SystemMenu->ToggleFullscreen();
    }
    if(WeatherTest && CapturedAt) {
        ++FreezeTicks;
        WeatherTime=FreezeTicks/30.f;
        if(FreezeTicks==20)WeatherTestCase=1;
        if(FreezeTicks==50)WeatherTestCase=2;
        if(FreezeTicks==80)WeatherTestCase=3;
        if(FreezeTicks==110)WeatherTestCase=4;
        if(FreezeTicks==140)WeatherTestCase=5;
        if(FreezeTicks==170)WeatherTestCase=6;
        const TCHAR* Name=nullptr;
        switch(FreezeTicks) {
            case 10:Name=TEXT("baseline");break;
            case 30:Name=TEXT("rain");break;
            case 40:Name=TEXT("rain-moving");break;
            case 60:Name=TEXT("fog");break;
            case 90:Name=TEXT("water");break;
            case 120:Name=TEXT("heat");break;
            case 150:Name=TEXT("native");break;
            case 180:Name=TEXT("gui-bypass");break;
        }
        if(Name)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("weather-%s.png"),Name),false,false);
        if(FreezeTicks==195)PlayerOwner->ConsoleCommand(TEXT("quit"));
        return;
    }
    if(DepthTest && CapturedAt) {
        ++FreezeTicks;
        if(FreezeTicks==10 || FreezeTicks==30) {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/
                (ReliefTest ? (FreezeTicks==10?TEXT("relief-flat.png"):TEXT("relief-layered.png")) :
                 (FreezeTicks==10?TEXT("depth-flat.png"):TEXT("depth-layered.png"))),false,false);
        }
        if(FreezeTicks==20) {if(ReliefTest)Relief=true;else Depth=true;RefreshScenePresentation();}
        if(FreezeTicks==45)PlayerOwner->ConsoleCommand(TEXT("quit"));
        return;
    }
    if (PixelTest && CapturedAt) {
        ++FreezeTicks;
        if (FreezeTicks == 10 || FreezeTicks == 30 || FreezeTicks == 50) {
            const FString Name = FreezeTicks == 10 ? TEXT("pixel-original.png") : (FreezeTicks == 30 ? TEXT("pixel-atmosphere.png") : TEXT("pixel-multiply.png"));
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("SMTests") / Name, false, false);
            UE_LOG(LogTemp, Display, TEXT("SM_PIXEL_CAPTURE %s frame=%d"), *Name, Frame());
        }
        if (FreezeTicks == 20) Atmosphere = true;
        if (FreezeTicks == 40) BlendMode=1;
        if (FreezeTicks == 65) PlayerOwner->ConsoleCommand(TEXT("quit"));
        return;
    }
    if (Paused) return;
    WeatherTime+=DeltaSeconds; // Atmospheric motion continues through native fades and item messages.
    SetEngineWeather(EngineWeather && Atmosphere);
    SetCombatEffects(Atmosphere && (!AutoTest || WideTest));
    Accumulator += SuitTest?736.0/44100.0:FMath::Min<double>(DeltaSeconds, 0.1);
    const double StepSeconds = 736.0 / 44100.0;
    bool Advanced = false;
    while (Accumulator >= StepSeconds) {
        uint16 Buttons = ReadButtons();
        if (AutoTest) Buttons = ((State()<7 || State()>18) && Frame()>180 && Frame()%120<2 ? 8 : 0);
        if(TitleTest)Buttons=TitleTestInput?8:0;
        if(TitleInputFence){Buttons=0;if(!ReadButtons() && !PlayerOwner->IsInputKeyDown(EKeys::AnyKey) && !PlayerOwner->IsInputKeyDown(EKeys::SpaceBar) && !PlayerOwner->IsInputKeyDown(EKeys::Escape) && !PlayerOwner->IsInputKeyDown(EKeys::LeftMouseButton) && !PlayerOwner->IsInputKeyDown(EKeys::RightMouseButton))TitleInputFence=false;}
        if(PlaytestCheck && !RecapCheck && !SporeCheck && PlaytestTicks>=110 && PlaytestTicks<150){
            Buttons=64;for(int I=0;I<12;I++)if(VisualState(48,0)&(0x8000>>I))Buttons|=1<<I;
        }
        if(RelicTest && Frame()>RelicTestFrame+100 && !RelicCount())Buttons=64;
        if(RelicTest && RelicCollectedFrame && Frame()>RelicCollectedFrame+400 && MessageActive())Buttons=256;
        if(FootstepTest) {
            static const int Destinations[]={0,5,3,7,1,6};
            const int T=FootstepTicks++;
            if(T%300==0 && T/300<6) {
                if(!Teleport(Destinations[T/300])){Failure=TEXT("Footstep test teleport failed");Ready=false;break;}
                for(int I=0;I<440;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());Ready=false;break;}
                // Equip the private rainy-arrival fixture after loading its
                // native rain state; later rooms use the normal equipped state.
                if(T==0 && !TestAllEquipment()){Failure=TEXT("Footstep test equipment failed");Ready=false;break;}
            }
            const int Walk=T%300;
            Buttons=Walk<110?128:(Walk<240?64:0);
        }
        if(PauseTest) {
            const int T=PauseTestTicks;
            Buttons=T<9 || (T>=560 && T<569)?8:
                ((T>=130 && T<180)?128:((T>=220 && T<229)?2048:((T>=390 && T<399)?1024:0)));
            if(ObjectivePauseTest)Buttons=T<9 || (T>=620 && T<629)?8:
                ((T>=130 && T<139)||(T>=420 && T<429)||(T>=510 && T<519)?1024:
                 (T>=220 && T<229)||(T>=310 && T<319)?2048:0);
        }
        if(State()==TestHoldState)Buttons=0;
        if(DisplayTest) {
            const int T=DisplayTicks++;Buttons=0;
            if(T==30 && !TestMessage(9)){Failure=TEXT("Message de test refuse");Ready=false;break;}
            if(T==1540 && !TestEscapeTimer()){Failure=TEXT("Compte a rebours de test refuse");Ready=false;break;}
            if(T>=415 && T<460)Buttons=2;
            if(T>=480 && T<1050) {
                const int Move=T-480;
                Buttons=128 | (Move%20<10?512:0) | (Move>=70 && Move<100?256:0);
            }
            if((T>=1100 && T<1108) || (T>=1400 && T<1408))Buttons=8;
            if(T>=1200 && T<1208)Buttons=2048;
            DisplayMessages+=MessageActive()!=0;
            DisplayDoors+=State()>=9 && State()<=11;
            DisplayPauseSeen|=State()==14 || State()==15;
        }
        if(CombatTest && State()==8) {
            const int T=CombatTicks++;Buttons=0;
            if(T>=120 && T<150)Buttons=128;
            if(T>=150 && T<220)Buttons=512;
            if((T>=240&&T<242)||(T>=250&&T<252))Buttons=32;
            if(T>=260&&T<262)Buttons=4;
            if(T>=270&&T<273)Buttons=512;
            if(T>=650&&T<652)Buttons=2;
            if(T>=700&&T<703)Buttons=512;
            if(ElectricTest) {
                Buttons=0;
                if(T<8)Buttons=128;
                if((T>=20&&T<22)||(T>=30&&T<32)||(T>=40&&T<42)||(T>=50&&T<52))Buttons=4;
                if(T>=70&&T<170)Buttons=512;
                if(T>=190&&T<192)Buttons=2;
                if(T>=210&&T<240)Buttons=512;
                if(T>=240&&T<250)Buttons=128;
                if(T>=250&&T<285)Buttons=128|256;
                if(T>=340&&T<520)Buttons=64|512;
                if(T==550) {
                    if(!Teleport(0)){Failure=TEXT("Test electrique : teleportation refusee");Ready=false;break;}
                    for(int I=0;I<440;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());Ready=false;break;}
                }
                if(T>=960&&T<968)Buttons=128;
                if((T>=1000&&T<1002)||(T>=1010&&T<1012)||(T>=1020&&T<1022)||(T>=1030&&T<1032))Buttons=4;
                if(T>=1050&&T<1130)Buttons=16|(T%16<8?512:0);
                if(T>=1150&&T<1160)Buttons=128;
                if(T>=1160&&T<1190)Buttons=128|256;
            }
        }
        if(DepthMotion)Buttons=MotionTicks<60?(128|1):(MotionTicks<180?(64|1):0);
        if(PresentationRoom!=Room()) { PresentationRoom=Room();AnchorX=CameraX();AnchorY=CameraY(); }
        const bool Shift=Atmosphere && Parallax && ParallaxSupported() && State()==8;
        const int LookX=Depth?FMath::Clamp((SamusX()-CameraX()-128)/32,-2,2):0;
        const int LookY=Depth?FMath::Clamp((SamusY()-CameraY()-112)/48,-1,1):0;
        SetParallax(FMath::Clamp(-(CameraX()-AnchorX)/(Depth?8:12)-LookX,-8,8),
                    FMath::Clamp(-(CameraY()-AnchorY)/(Depth?12:18)-LookY,-6,6),Shift);
        if(SuitTest)Buttons=0;
        if (!Step(Buttons)) { Failure = UTF8_TO_TCHAR(Error()); Ready = false; UE_LOG(LogTemp, Error, TEXT("SM_NATIVE_ERROR %s"), *Failure); break; }
        UpdateTitleTimeline();
        AdvanceSuitTransformation(float(StepSeconds));
        if(PresentationKind()==1 && State()==8 && !MessageActive())VisualEffects.Advance(VisualState,Room(),FxType(),WaterY(),float(StepSeconds));
        if(PresentationKind()==1 && State()==8 && !SuitTest && !VisualState(49,0)) {
            TrackAchievements();
        }
        const int CreditMode=CreditsState?CreditsState(0):0;
        if(CreditMode!=LastCreditsMode){AudioWave->ResetAudio();LastCreditsMode=CreditMode;}
        const bool GameOverAudio=State()==26;
        if(GameOverAudio!=LastGameOverAudio){AudioWave->ResetAudio();LastGameOverAudio=GameOverAudio;}
        AudioWave->QueueAudio(reinterpret_cast<const uint8*>(Audio()), 736 * 2 * sizeof(int16));
        if(SuitTest)SuitCaptureAudio.Append(reinterpret_cast<const uint8*>(Audio()),736*4);
        Accumulator -= StepSeconds;
        Advanced = true;
        if (State() != LastState) {
            UE_LOG(LogTemp, Display, TEXT("SM_STATE frame=%d state=%d room=%04x samus=%d,%d cpu_opcodes=%llu"), Frame(), State(), Room(), SamusX(), SamusY(), Opcodes());
            LastState = State();
        }
        if (!(CreditsState && CreditsState(0)==2) && Frame() % 600 == 0) Save();
    }
    if(SuitTest)CaptureSuitFrame();
    if (AudioWave->GetAvailableAudioByteCount() > 44100 * 4 / 2) AudioWave->ResetAudio();
    GameOverTime=GameOverState && GameOverState(0)?GameOverTime+DeltaSeconds:0.f;
    if(GameOverTest){
        ++GameOverTestTicks;
        GameOverTime=GameOverTestTicks<80?.9f:1.73f;
        if(GameOverTestTicks==60 || GameOverTestTicks==100){
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("gameover-%s-%s.png"),Widescreen?TEXT("wide"):TEXT("classic"),GameOverTestTicks==60?TEXT("lit"):TEXT("flicker")),false,false);
            UE_LOG(LogTemp,Display,TEXT("SM_GAMEOVER_CAPTURE wide=%d phase=%d selection=%d time=%.2f"),Widescreen,GameOverState(2),GameOverState(1),GameOverTime);
        }
        if(GameOverTestTicks==120)PlayerOwner->ConsoleCommand(TEXT("quit"));
    }
    if (Advanced) {
        UploadPresentation(GameTexture,Pixels(),256,240);
        RefreshScenePresentation();
        UpdateEnvironmentMask();
        UpdateEffectsTexture();
        UploadPresentation(EmissionTexture,Emission(),256,240);
        UploadPresentation(LightTexture,Lightmap(),64,60);
    }
    if(FootstepTest) {
        for(int I=0;I<6;I++)if(VisualEffects.FootstepCount[I] && !FootstepCaptured[I]) {
            FootstepCaptured[I]=true;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("footstep-%d.png"),I),false,false);
            UE_LOG(LogTemp,Display,TEXT("SM_FOOTSTEP_CAPTURE style=%d room=%04x count=%d"),I,Room(),VisualEffects.FootstepCount[I]);
            break;
        }
        if(FootstepTicks>=1800) {
            const int* C=VisualEffects.FootstepCount;
            const bool Passed=C[0]>0 && C[1]>0 && C[2]>0 && C[4]>0 && C[5]>0 && !Opcodes();
            const FString Report=FString::Printf(TEXT("{\"passed\":%s,\"footsteps\":[%d,%d,%d,%d,%d,%d],\"cpuOpcodes\":%llu}"),Passed?TEXT("true"):TEXT("false"),C[0],C[1],C[2],C[3],C[4],C[5],Opcodes());
            FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("SMTests/footstep-verification.json")));
            UE_LOG(LogTemp,Display,TEXT("SM_FOOTSTEP_%s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),*Report);
            PlayerOwner->ConsoleCommand(TEXT("quit"));
        }
    }
    if(Ready && PauseTest)TickPauseTest();
    if(DisplayTest) {
        static const int CaptureAt[]={10,60,230,470,550,650,720,820,1000,1160,1250,1340,1530,1580,1820};
        if(DisplayTicks>1550)for(int I=32*256;I<224*256;I++)if(UiOverlay()[I*4+3]){DisplayTimerSeen=true;break;}
        if(DisplayCapture<UE_ARRAY_COUNT(CaptureAt) && DisplayTicks>=CaptureAt[DisplayCapture]) {
            const FString Base=FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("display-%02d"),DisplayCapture++);
            TArray<uint8> Ui;Ui.Append(WideHud(),400*240*4);FFileHelper::SaveArrayToFile(Ui,*(Base+TEXT("-ui.bgra")));
            FScreenshotRequest::RequestScreenshot(Base+TEXT(".png"),false,false);
            UE_LOG(LogTemp,Display,TEXT("SM_DISPLAY_CAPTURE tick=%d state=%d message=%d width=%d weatherTime=%.3f"),DisplayTicks,State(),MessageActive(),UseWide()?400:256,WeatherTime);
        }
        if(DisplayTicks>=1900) {
            const bool Passed=UseWide() && DisplayPauseSeen && DisplayTimerSeen && DisplayMessages>350 && DisplayDoors>0 && State()==8 && Room()!=0x93d5 && Opcodes()==0;
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"width\":400,\"messageFrames\":%d,\"doorFrames\":%d,\"pauseScreen\":%s,\"timerOverlay\":%s,\"finalRoom\":\"%04x\",\"cpuOpcodes\":%llu}"),Passed?TEXT("true"):TEXT("false"),DisplayMessages,DisplayDoors,DisplayPauseSeen?TEXT("true"):TEXT("false"),DisplayTimerSeen?TEXT("true"):TEXT("false"),Room(),Opcodes()),*(FPaths::ProjectSavedDir()/TEXT("SMTests/display-verification.json")));
            UE_LOG(LogTemp,Display,TEXT("SM_DISPLAY_%s"),Passed?TEXT("PASS"):TEXT("FAIL"));
            PlayerOwner->ConsoleCommand(TEXT("quit"));
        }
    }
    if(ElectricTest) {
        const TCHAR* Name=nullptr;
        if(!CapturedGrapple && CombatTicks>950 && VisualState(34,0) &&
            FMath::Abs(VisualState(31,0)-VisualState(33,0))>24){CapturedGrapple=true;Name=TEXT("electric-grapple.png");}
        if(!CapturedScrew && CombatTicks>1150 && VisualState(35,0)){CapturedScrew=true;Name=TEXT("electric-screw.png");}
        if(!CapturedEnemy && VisualEffects.EnemyCount){CapturedEnemy=true;Name=TEXT("electric-enemy.png");}
        if(Name)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/Name,false,false);
        if(!Name && CombatTicks>=960&&CombatTicks<=1240&&CombatTicks/4>ElectricSnapshots) {
            ElectricSnapshots=CombatTicks/4;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("electric-seq-%03d.png"),ElectricSnapshots),false,false);
        }
        if(CombatTicks>=1300) {
            FConfigFile DiskProfile;DiskProfile.Read(ProfilePath);
            int SavedAchievements=0;DiskProfile.GetInt(TEXT("Local"),TEXT("Unlocked"),SavedAchievements);
            const bool Passed=CapturedGrapple&&CapturedScrew&&CapturedEnemy&&!Opcodes()&&(SavedAchievements&24)==24;
            const FString Report=FString::Printf(TEXT("{\"passed\":%s,\"grapple\":%d,\"screw\":%d,\"enemyDeaths\":%d,\"achievements\":%d,\"cpuOpcodes\":%llu}"),Passed?TEXT("true"):TEXT("false"),VisualEffects.GrappleCount,VisualEffects.ScrewCount,VisualEffects.EnemyCount,SavedAchievements,Opcodes());
            FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("SMTests/electric-verification.json")));
            UE_LOG(LogTemp,Display,TEXT("SM_ELECTRIC_%s %s"),Passed?TEXT("PASS"):TEXT("FAIL"),*Report);PlayerOwner->ConsoleCommand(TEXT("quit"));
        }
    }
    if(CombatTest && !ElectricTest) {
        const TCHAR* Shot=nullptr;
        if(!CapturedSplash && VisualEffects.SplashCount){CapturedSplash=true;Shot=TEXT("splash");}
        if(!CapturedMuzzle && VisualEffects.MuzzleCount){CapturedMuzzle=true;Shot=TEXT("muzzle");}
        if(!CapturedCharge && VisualState(4,0)>=60){CapturedCharge=true;Shot=TEXT("charge");}
        if(!CapturedExplosion && VisualEffects.ExplosionCount){CapturedExplosion=true;Shot=TEXT("explosion");}
        if(Shot)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("combat-%s.png"),Shot),false,false);
        if((VisualState(13,0)&0x8000) && CombatTicks-LastBombCapture>=12 && CombatSnapshots<32) {
            LastBombCapture=CombatTicks;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("powerbomb-%02d.png"),CombatSnapshots++),false,false);
        }
        if(CombatTicks>=850) {
            const bool Passed=VisualEffects.SplashCount>0 && VisualEffects.MuzzleCount>0 && CapturedCharge && VisualEffects.ExplosionCount>0 && VisualEffects.PowerBombCount>0 && Opcodes()==0;
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"splashes\":%d,\"muzzles\":%d,\"charge\":%s,\"explosions\":%d,\"powerbombs\":%d,\"cpu_opcodes\":%llu}"),Passed?TEXT("true"):TEXT("false"),VisualEffects.SplashCount,VisualEffects.MuzzleCount,CapturedCharge?TEXT("true"):TEXT("false"),VisualEffects.ExplosionCount,VisualEffects.PowerBombCount,Opcodes()),*(FPaths::ProjectSavedDir()/TEXT("SMTests/combat-verification.json")));
            UE_LOG(LogTemp,Display,TEXT("SM_COMBAT_%s splash=%d muzzle=%d charge=%d explosion=%d powerbomb=%d"),Passed?TEXT("PASS"):TEXT("FAIL"),VisualEffects.SplashCount,VisualEffects.MuzzleCount,CapturedCharge,VisualEffects.ExplosionCount,VisualEffects.PowerBombCount);
            PlayerOwner->ConsoleCommand(TEXT("quit"));
        }
    }
    if(DepthMotion && MotionTicks%60==0)UE_LOG(LogTemp,Display,TEXT("SM_DEPTH_CAMERA player=%d,%d camera=%d,%d"),SamusX(),SamusY(),CameraX(),CameraY());
    if(DepthMotion && MotionTicks>=4 && MotionTicks<=240 && MotionTicks%4==0)
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests/depth-motion")/FString::Printf(TEXT("frame-%04d.png"),MotionTicks/4-1),false,false);
    if(DepthMotion && MotionTicks==248) {
        UE_LOG(LogTemp,Display,TEXT("SM_DEPTH_MOTION_DONE frames=60 room=%04x cpu_opcodes=%llu"),Room(),Opcodes());
        PlayerOwner->ConsoleCommand(TEXT("quit"));
    }
    if(WeatherTest && TestFrames>0 && Frame()>=TestFrames && !CapturedAt) {
        CapturedAt=Frame();Atmosphere=true;BlendMode=0;Intensity=.75f;Exposure=1.10f;
        RefreshScenePresentation();UpdateEnvironmentMask();
        TArray<uint8> Raw;Raw.Append(ReliefPixels.GetData(),256*240*4);
        FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("SMTests/weather-base.bgra")));
        Raw.Reset();Raw.Append(Pixels(),256*240*4);
        FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("SMTests/weather-original.bgra")));
        FFileHelper::SaveArrayToFile(EnvironmentMaskPixels,*(FPaths::ProjectSavedDir()/TEXT("SMTests/weather-mask.bgra")));
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"frame\":%d,\"room\":\"%04x\",\"state\":%d,\"fx\":%d,\"water_y\":%d,\"heated\":%d,\"cpu_opcodes\":%llu}"),Frame(),Room(),State(),FxType(),WaterY(),HeatedRoom(),Opcodes()),*(FPaths::ProjectSavedDir()/TEXT("SMTests/weather-state.json")));
        UE_LOG(LogTemp,Display,TEXT("SM_WEATHER_FROZEN room=%04x fx=%d water=%d heat=%d cpu_opcodes=%llu"),Room(),FxType(),WaterY(),HeatedRoom(),Opcodes());
    }
    if(DepthTest && TestFrames>0 && Frame()>=TestFrames && !CapturedAt) {
        CapturedAt=Frame();Atmosphere=true;BlendMode=0;Intensity=.75f;Exposure=1.10f;Depth=ReliefTest;Relief=false;
        RefreshScenePresentation();
        TArray<uint8> Raw;Raw.Append(ReliefTest?DepthPixels.GetData():Scene(),256*240*4);
        FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/(ReliefTest?TEXT("SMTests/relief-flat.bgra"):TEXT("SMTests/depth-flat.bgra"))));
        if(ReliefTest) {
            Raw.Reset();Raw.Append(Layers(),256*240*4);
            FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("SMTests/relief-layers.bgra")));
        }
        Raw.Reset();Raw.Append(FarMask(),256*240);
        FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("SMTests/depth-mask.bin")));
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"frame\":%d,\"room\":\"%04x\",\"state\":%d,\"cpu_opcodes\":%llu}"),Frame(),Room(),State(),Opcodes()),*(FPaths::ProjectSavedDir()/TEXT("SMTests/depth-state.json")));
        UE_LOG(LogTemp,Display,TEXT("SM_DEPTH_FROZEN frame=%d room=%04x cpu_opcodes=%llu"),Frame(),Room(),Opcodes());
    }
    if (PixelTest && TestFrames > 0 && Frame() >= TestFrames && !CapturedAt) {
        CapturedAt = Frame();
        Atmosphere = false;
        TArray<uint8> Raw; Raw.Append(Pixels(), 256 * 240 * 4);
        FFileHelper::SaveArrayToFile(Raw, *(FPaths::ProjectSavedDir() / TEXT("SMTests/reference.bgra")));
        Raw.Reset();Raw.Append(Emission(),256*240*4);
        FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("SMTests/emission.bgra")));
        Raw.Reset();Raw.Append(Lightmap(),64*60*4);
        FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("SMTests/lightmap.bgra")));
        UE_LOG(LogTemp, Display, TEXT("SM_PIXEL_FROZEN frame=%d state=%d room=%04x cpu_opcodes=%llu"), Frame(), State(), Room(), Opcodes());
    }
    if(RelicTest){
        if(!RelicCollectedFrame && RelicCount())RelicCollectedFrame=Frame();
        if(Frame()>=RelicTestFrame+60 && Frame()<=RelicTestFrame+62)
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests/relic-unreal-idle.png"),false,false);
        if(RelicCollectedFrame && Frame()>=RelicCollectedFrame+40 && Frame()<=RelicCollectedFrame+42)
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests/relic-unreal-message.png"),false,false);
        if(RelicCollectedFrame && !MessageActive() && !RelicDismissedFrame)RelicDismissedFrame=Frame();
        if(RelicDismissedFrame && Frame()>=RelicDismissedFrame+5 && Frame()<=RelicDismissedFrame+7)
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests/relic-unreal-pickup.png"),false,false);
        if(Frame()>=TestFrames && !CapturedAt){
            const bool Passed=RelicCount()==1 && RelicDismissedFrame>RelicCollectedFrame+360 && VisualEffects.RelicBursts==1 && VisualEffects.RelicGlowFrames>30 && !Opcodes();
            UE_LOG(LogTemp,Display,TEXT("SM_RELIC_VISUAL_TEST %s collected=%d bursts=%d glowFrames=%d cpu=%llu"),Passed?TEXT("PASS"):TEXT("FAIL"),RelicCount(),VisualEffects.RelicBursts,VisualEffects.RelicGlowFrames,Opcodes());
        }
    }
    if (!PlaytestCheck && !FootstepTest && !PauseTest && !CombatTest && !PixelTest && !DepthTest && !DepthMotion && !WeatherTest && !TeleportUiTest && AutoTest && TestFrames > 0 && Frame() >= TestFrames && !CapturedAt) {
        if(TestHoldState>=0) {
            TArray<uint8> Raw;Raw.Append(WideHud(),400*240*4);FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("SMTests/story-ui.bgra")));
            Raw.Reset();Raw.Append(WideScene(),400*240*4);FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("SMTests/story-scene.bgra")));
            UE_LOG(LogTemp,Display,TEXT("SM_STORY_CAPTURE state=%d kind=%d width=%d"),State(),PresentationKind(),UseWide()?400:256);
        }
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("SMTests/Unreal-native.png"), false, false);
        CapturedAt = Frame();
        UE_LOG(LogTemp, Display, TEXT("SM_TEST_CAPTURE frame=%d cpu_opcodes=%llu"), Frame(), Opcodes());
    }
    if (!CombatTest && !PixelTest && !DepthTest && !DepthMotion && !WeatherTest && !TeleportUiTest && CapturedAt && Frame() > CapturedAt + 15) PlayerOwner->ConsoleCommand(TEXT("quit"));
}
void ASMHUD::TickTeleportUiTest() {
    const int T=++TeleportUiTicks;
    auto Key=[this](FKey K,EInputEvent E) {PlayerOwner->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,E,FPlatformTime::Cycles64()));};
    auto Require=[this](bool OK,const TCHAR* Label) {
        if(!OK) {UE_LOG(LogTemp,Error,TEXT("SM_TELEPORT_UI_FAIL %s"),Label);PlayerOwner->ConsoleCommand(TEXT("quit"));}
    };
    if(T==10 || T==60)Key(EKeys::F10,IE_Pressed);
    if(T==11 || T==61)Key(EKeys::F10,IE_Released);
    if(T==20) {Require(TeleportMenu,TEXT("open"));TeleportUiFrame=Frame();FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests/teleport-menu.png"),false,false);}
    if(T==30 || T==70)Key(EKeys::Down,IE_Pressed);
    if(T==31 || T==71)Key(EKeys::Down,IE_Released);
    if(T==40) {Require(TeleportSelection==1,TEXT("navigate"));Require(Frame()==TeleportUiFrame,TEXT("pause"));}
    if(T==42)Key(EKeys::Escape,IE_Pressed);
    if(T==43)Key(EKeys::Escape,IE_Released);
    if(T==50)Require(!TeleportMenu,TEXT("cancel"));
    if(T==80)Key(EKeys::Enter,IE_Pressed);
    if(T==81)Key(EKeys::Enter,IE_Released);
    if(T==250) {
        Require(!TeleportMenu && State()==8 && Room()==0x9ad9,TEXT("arrival"));
        Require(Opcodes()==0,TEXT("native-only"));
        if(State()==8 && Room()==0x9ad9)UE_LOG(LogTemp,Display,TEXT("SM_TELEPORT_UI_PASS open navigate pause cancel confirm arrival room=9ad9 cpu_opcodes=%llu"),Opcodes());
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests/teleport-arrival.png"),false,false);
    }
    if(T==265)PlayerOwner->ConsoleCommand(TEXT("quit"));
}
void ASMHUD::RefreshScenePresentation() {
    if(!SceneTexture || !Scene) return;
    const uint8* Composed=Scene();
    const bool Gameplay=PresentationKind()==1;
    if(Depth && Gameplay) {
        DepthPixels.SetNumUninitialized(256*240*4);
        sm_compose_depth(Composed,Layers(),FarMask(),DepthPixels.GetData());
        Composed=DepthPixels.GetData();
        if(DepthTest && !ReliefTest && CapturedAt)FFileHelper::SaveArrayToFile(DepthPixels,*(FPaths::ProjectSavedDir()/TEXT("SMTests/depth-layered.bgra")));
    }
    if(Relief && Gameplay) {
        ReliefPixels.SetNumUninitialized(256*240*4);
        sm_compose_relief(Composed,Layers(),FarMask(),ReliefPixels.GetData());
        Composed=ReliefPixels.GetData();
        if(ReliefTest && CapturedAt)FFileHelper::SaveArrayToFile(ReliefPixels,*(FPaths::ProjectSavedDir()/TEXT("SMTests/relief-layered.bgra")));
    }
    UploadPresentation(SceneTexture,Composed,256,240);
    UploadPresentation(UiTexture,UiOverlay(),256,240);
    RefreshWide();
}
void ASMHUD::UpdateEffectsTexture() {
    VisualEffects.WriteTexture(EffectsPixels,CameraX()-(UseWide()?72:0),CameraY());
    uint8* Copy=(uint8*)FMemory::Malloc(EffectsPixels.Num()*sizeof(FVector4f));
    FMemory::Memcpy(Copy,EffectsPixels.GetData(),EffectsPixels.Num()*sizeof(FVector4f));
    auto* Region=new FUpdateTextureRegion2D(0,0,0,0,64,2);
    EffectsTexture->UpdateTextureRegions(0,1,Region,64*sizeof(FVector4f),sizeof(FVector4f),Copy,
      [](uint8* Data,const FUpdateTextureRegion2D* R){FMemory::Free(Data);delete R;});
}
bool ASMHUD::UseWide() const {
    return Widescreen;
}
void ASMHUD::RefreshWide() {
    if(!UseWide())return;
    const uint8* Composed=WideScene();
    UploadPresentation(WideGameTexture,Composed,400,240);
    if(Depth && PresentationKind()==1) {
        WideDepthPixels.SetNumUninitialized(400*240*4);
        sm_compose_depth_size(Composed,WideLayers(),WideFar(),WideDepthPixels.GetData(),400,0);
        Composed=WideDepthPixels.GetData();
    }
    if(Relief && PresentationKind()==1) {
        WideReliefPixels.SetNumUninitialized(400*240*4);
        sm_compose_relief_size(Composed,WideLayers(),WideFar(),WideReliefPixels.GetData(),400,0);
        Composed=WideReliefPixels.GetData();
    }
    UploadPresentation(WideSceneTexture,Composed,400,240);
    UploadPresentation(WideHudTexture,WideHud(),400,240);
    WideMaskPixels.SetNumZeroed(400*240*4);FMemory::Memzero(WideMaskPixels.GetData(),WideMaskPixels.Num());
    const uint8* Meta=WideLayers();const uint8* Far=WideFar();
    for(int I=0;I<400*224;I++) {
        if(!Meta[I*4+3])continue;
        const int Layer=Meta[I*4];const bool Sprite=Layer==4 || Layer==6;
        WideMaskPixels[I*4]=!Sprite && (Far[I] || Layer==1 || Layer==5)?255:0;
        WideMaskPixels[I*4+1]=Sprite?255:0;
        WideMaskPixels[I*4+2]=Layer==0 && !Far[I]?255:0;
        WideMaskPixels[I*4+3]=255;
    }
    UploadPresentation(WideMaskTexture,WideMaskPixels.GetData(),400,240);
}
void ASMHUD::UpdateEnvironmentMask() {
    EnvironmentMaskPixels.SetNumZeroed(256*240*4);
    FMemory::Memzero(EnvironmentMaskPixels.GetData(),EnvironmentMaskPixels.Num());
    const uint8* Meta=Layers();const uint8* Far=FarMask();
    for(int Y=32;Y<224;++Y)for(int X=0;X<256;++X) {
        const int I=Y*256+X,Layer=Meta[I*4];
        if(!Meta[I*4+3])continue;
        const bool Sprite=Layer==4 || Layer==6;
        EnvironmentMaskPixels[I*4]=!Sprite && (Far[I] || Layer==1 || Layer==5) ? 255:0;
        EnvironmentMaskPixels[I*4+1]=Sprite?255:0;
        EnvironmentMaskPixels[I*4+2]=Layer==0 && !Far[I]?255:0;
        EnvironmentMaskPixels[I*4+3]=255;
    }
    UploadPresentation(EnvironmentMaskTexture,EnvironmentMaskPixels.GetData(),256,240);
}
void ASMHUD::DrawHUD() {
    Super::DrawHUD();
    if (!Canvas) return;
    DrawRect(FLinearColor::Black, 0, 0, Canvas->SizeX, Canvas->SizeY);
    if (!Failure.IsEmpty()) { DrawText(SMLocalization::Text(Failure), FLinearColor::Red, 40, 40); return; }
    ON_SCOPE_EXIT {if(!DialogueVisible)DrawBuildVersion();};
    if(StartupWarning>=0){DrawStartupWarning();return;}
    if(DrawTitlePresentation())return;
    if(DrawRunRecap())return;
    if(DrawGameOver())return;
    if (!GameTexture) return;
    const float SourceWidth=UseWide()?400.f:256.f;
    const float FitScale=FMath::Min(Canvas->SizeX / SourceWidth, Canvas->SizeY / 224.f);
    if(FitScale<=0)return;
    // Tiny windows must downscale instead of cropping the game. Integer mode
    // retains exact pixel blocks whenever at least a 1x image fits.
    const float Scale=ImageScaling==1 && FitScale>=1.f?FMath::FloorToFloat(FitScale):FitScale;
    const float W = SourceWidth * Scale, H = 224 * Scale;
    const float X = FMath::FloorToFloat((Canvas->SizeX - W) * 0.5f), Y = FMath::FloorToFloat((Canvas->SizeY - H) * 0.5f);
    if (PixelTest && FreezeTicks == 10) {
        UE_LOG(LogTemp, Display, TEXT("SM_PIXEL_GEOMETRY viewport=%dx%d origin=%d,%d scale=%g size=%dx%d"), Canvas->SizeX, Canvas->SizeY, int(X), int(Y), Scale, int(W), int(H));
    }
    if (PresentMaterial) {
        UpdatePresentation();
        DrawMaterial(PresentMaterial, X, Y, W, H, 0, 0, 1, 224.f/240.f);
    } else {
        DrawTexture(GameTexture, X, Y, W, H, 0, 0, 1, 224.f/240.f, FLinearColor::White, BLEND_Opaque);
    }
    DrawSuitTransformation(X,Y,Scale);
    DrawDialogue(X,Y,Scale);
    if (!AutoTest && (ShowHelp || FPlatformTime::Seconds() < NoticeUntil)) {
        const FString Text = ShowHelp ?
            SMLocalization::Text(FString(TEXT("F1 Help | F2 Effects | F3 Opacity | F4 System menu | F5 Parallax | F6 Lighten/Multiply | F7 Depth | F8 Relief | F9 Weather | F10 Teleport")))+TEXT("\n")+
            SMLocalization::Text(FString(TEXT("F11 Full screen | Ctrl+F11 Wide/4:3 | F12 Wall Jump assist | Shift+F12 Space Jump assist")))+TEXT("\n")+
            SMLocalization::Text(FString(TEXT("Arrows: move | Z: jump | X: run | S: shoot | C: cancel item | A/D: aim")))+TEXT("\n")+
            SMLocalization::Text(FString(TEXT("Right Shift: select item | Enter: Start | P: pause | Escape: system menu"))) : SMLocalization::Text(Notice);
        DrawRect(FLinearColor(0,0,0,.85f), 12, 12, 1250, ShowHelp ? 90 : 27);
        DrawText(Text, FLinearColor(.8f,.95f,1.f), 20, 18);
    }
    if(TeleportMenu) {
        const float MX=FMath::Max(16.f,(Canvas->SizeX-660.f)*.5f),MY=FMath::Max(16.f,(Canvas->SizeY-390.f)*.5f);
        DrawRect(FLinearColor(.012f,.025f,.04f,.97f),MX,MY,660,390);
        DrawText(SMLocalization::Text(FString(TEXT("TELEPORTATION"))),FLinearColor(.35f,.85f,1.f),MX+24,MY+18,nullptr,1.6f);
        DrawText(SMLocalization::Text(FString(TEXT("Choose a destination"))),FLinearColor(.7f,.8f,.85f),MX+24,MY+52);
        for(int I=0;I<TeleportCount();++I) {
            const float Row=MY+85+I*31;
            if(I==TeleportSelection)DrawRect(FLinearColor(.04f,.22f,.30f,1.f),MX+16,Row-4,628,29);
            DrawText(FString::Printf(TEXT("%s %s"),I==TeleportSelection?TEXT(">"):TEXT(" "),UTF8_TO_TCHAR(TeleportName(I))),I==TeleportSelection?FLinearColor::White:FLinearColor(.65f,.75f,.8f),MX+26,Row,nullptr,1.15f);
        }
        DrawText(SMLocalization::Text(FString(TEXT("Up/Down: select   Enter/Z: travel   Escape/F10: close"))),FLinearColor(.65f,.8f,.85f),MX+24,MY+350);
    }
    if (Paused && !TeleportMenu) DrawText(TEXT("PAUSE"), FLinearColor::White, 20, 20, nullptr, 2);
}
void ASMHUD::PersistSettings() {
    if (AutoTest && !PauseTest) return;
    FConfigFile Settings;Settings.Read(SettingsPath);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(SettingsPath),true);
    Settings.SetBool(TEXT("Controls"),TEXT("AssistedWallJump"),AssistedWallJump);
    Settings.SetBool(TEXT("Controls"),TEXT("AssistedSpaceJump"),AssistedSpaceJump);
    Settings.SetBool(TEXT("Display"),TEXT("Widescreen"),Widescreen);
    Settings.SetInt64(TEXT("Display"),TEXT("ImageScaling"),ImageScaling);
    Settings.SetBool(TEXT("Display"),TEXT("BorderExtension"),BorderExtension);
    Settings.SetFloat(TEXT("Atmosphere"),TEXT("FlashStrength"),FlashStrength);
    Settings.SetBool(TEXT("Atmosphere"),TEXT("EngineWeather"),EngineWeather);
    Settings.SetBool(TEXT("Atmosphere"),TEXT("Relief"),Relief);
    Settings.SetBool(TEXT("Atmosphere"),TEXT("Depth"),Depth);
    Settings.SetInt64(TEXT("Atmosphere"),TEXT("BlendMode"),BlendMode);
    Settings.SetInt64(TEXT("Atmosphere"),TEXT("Version"),3);
    Settings.SetBool(TEXT("Atmosphere"),TEXT("Parallax"),Parallax);
    Settings.SetBool(TEXT("Atmosphere"), TEXT("Enabled"), Atmosphere);
    Settings.SetFloat(TEXT("Atmosphere"), TEXT("Intensity"), Intensity);
    Settings.SetFloat(TEXT("Atmosphere"), TEXT("Brightness"), Exposure);
    if(SystemMenu)SystemMenu->Persist(Settings);
    if(!Settings.Write(SettingsPath))UE_LOG(LogTemp,Warning,TEXT("SM_SETTINGS_WRITE_FAILED %s"),*SettingsPath);
}
void ASMHUD::NotifySettings() {
    auto OnOff=[](bool Value){return SMLocalization::Text(FString(Value?TEXT("On"):TEXT("Off")));};
    Notice=SMLocalization::Format(TEXT("Gaussian layer: {0} | {1} | Opacity: {2}% | Brightness: {3}% | Depth: {4} | Relief: {5} | Weather: {6}"),
        {OnOff(Atmosphere),SMLocalization::Text(FString(BlendMode?TEXT("Multiply"):TEXT("Lighten"))),FMath::RoundToInt(Intensity*100),FMath::RoundToInt(Exposure*100),OnOff(Depth),OnOff(Relief),OnOff(EngineWeather)});
    NoticeUntil = FPlatformTime::Seconds() + 3;
    PersistSettings();
    UE_LOG(LogTemp, Display, TEXT("SM_PRESENTATION %s"), *Notice);
}
void ASMHUD::UpdateCinemaTexture() {
    if(!CinemaTexture||!CinemaLights)return;
    constexpr int Bytes=32*3*4*sizeof(float);
    auto* Copy=(uint8*)FMemory::Malloc(Bytes);FMemory::Memcpy(Copy,CinemaLights(),Bytes);
    auto* Region=new FUpdateTextureRegion2D(0,0,0,0,32,3);
    CinemaTexture->UpdateTextureRegions(0,1,Region,32*4*sizeof(float),4*sizeof(float),Copy,
      [](uint8* Data,const FUpdateTextureRegion2D* R){FMemory::Free(Data);delete R;});
}
void ASMHUD::UpdatePresentation() {
    UpdateCinemaTexture();
    PresentMaterial->SetVectorParameterValue(TEXT("CinemaState"),FLinearColor(CinemaState && !(CreditsState && CreditsState(0)) && !(CinemaTest && CinemaTestTicks<40)?CinemaState(1):0,Frame()/60.f,CinemaState?CinemaState(0):0,0));
    const bool Wide=UseWide();
    const bool World=PresentationKind()==1;
    PresentMaterial->SetVectorParameterValue(TEXT("CreditsState"),FLinearColor(CreditsState?CreditsState(0):0,CreditsState?CreditsState(1)/60.f:0,0,0));
    PresentMaterial->SetScalarParameterValue(TEXT("CombatEffects"),World && State()==8 && !MessageActive() && (!AutoTest || WideTest)?1.f:0.f);
    PresentMaterial->SetVectorParameterValue(TEXT("ChargeState"),VisualEffects.Charge(CameraX()-(Wide?72:0),CameraY()));
    PresentMaterial->SetVectorParameterValue(TEXT("PowerBombState"),VisualEffects.PowerBomb(CameraX()-(Wide?72:0),CameraY()));
    PresentMaterial->SetVectorParameterValue(TEXT("PowerBombPhase"),VisualEffects.PowerBombPhase());
    const float VX=CameraX()-(Wide?72:0),VY=CameraY();
    PresentMaterial->SetVectorParameterValue(TEXT("GrappleState"),VisualEffects.Grapple(VX,VY));
    PresentMaterial->SetVectorParameterValue(TEXT("GrappleEndState"),VisualEffects.GrappleEnd(VX,VY));
    PresentMaterial->SetVectorParameterValue(TEXT("ScrewState"),VisualEffects.Screw(VX,VY));
    PresentMaterial->SetScalarParameterValue(TEXT("FlashStrength"),FlashStrength);
    PresentMaterial->SetScalarParameterValue(TEXT("ViewWidth"),Wide?400.f:256.f);
    PresentMaterial->SetScalarParameterValue(TEXT("HudTop"),Wide || !World?0.f:32.f);
    PresentMaterial->SetTextureParameterValue(TEXT("GameTexture"),Wide?WideGameTexture:GameTexture);
    PresentMaterial->SetTextureParameterValue(TEXT("SceneTexture"),Wide?WideSceneTexture:SceneTexture);
    PresentMaterial->SetTextureParameterValue(TEXT("EnvironmentMask"),Wide?WideMaskTexture:EnvironmentMaskTexture);
    PresentMaterial->SetTextureParameterValue(TEXT("GuiTexture"),Wide?WideHudTexture:UiTexture);
    const bool Gameplay = PresentationKind()!=0;
    PresentMaterial->SetScalarParameterValue(TEXT("Enabled"), Atmosphere ? 1.f : 0.f);
    PresentMaterial->SetScalarParameterValue(TEXT("Gameplay"), Gameplay && !(WeatherTest && WeatherTestCase==6) ? 1.f : 0.f);
    FLinearColor Weather(FxType()==10?1.f:0.f,FxType()==12?1.f:(FxType()==44?.45f:0.f),WaterY()!=32767?1.f:0.f,HeatedRoom()?1.f:0.f);
    float Surface=WaterY()==32767?32767.f:float(WaterY()-CameraY());
    if(WeatherTest && WeatherTestCase!=5) {
        Weather=FLinearColor(WeatherTestCase==1?1.f:0.f,WeatherTestCase==2?1.f:0.f,WeatherTestCase==3?1.f:0.f,WeatherTestCase==4?1.f:0.f);
        Surface=100.f;
    }
    PresentMaterial->SetScalarParameterValue(TEXT("EngineWeather"),EngineWeather && World?1.f:0.f);
    PresentMaterial->SetScalarParameterValue(TEXT("SporeAtmosphere"),World && Room()==0x9dc7?1.f:0.f);
    PresentMaterial->SetVectorParameterValue(TEXT("WeatherState"),Weather);
    PresentMaterial->SetVectorParameterValue(TEXT("WeatherView"),FLinearColor(CameraX()-(Wide?72:0),CameraY(),Surface,WeatherTime));
    PresentMaterial->SetScalarParameterValue(TEXT("Intensity"), Intensity);
    PresentMaterial->SetScalarParameterValue(TEXT("Brightness"), Exposure);
    PresentMaterial->SetScalarParameterValue(TEXT("BlendMode"), BlendMode);
    PresentMaterial->SetScalarParameterValue(TEXT("SceneFade"), CreditsState && CreditsState(0)?1.f:Brightness() / 15.f);
}
void ASMHUD::EndPlay(const EEndPlayReason::Type Reason) {
    DialogueVisible=false;DialoguePortrait=nullptr;
    ReleaseRomWindowIcon();
    RomSetup.Reset();
    SystemMenu.Reset();
    if (AudioComponent) AudioComponent->Stop();
    if (Shutdown) Shutdown();
    if (CoreHandle) FPlatformProcess::FreeDllHandle(CoreHandle);
    Ready = false;
    Super::EndPlay(Reason);
}

void ASMHUD::TickCreditsTest(){
    auto Launch=reinterpret_cast<int(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_credits_launch")));
    auto Close=reinterpret_cast<void(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_credits_close")));
    const int T=++CreditsTestTicks;
    if(T==1){
        for(int I=0;I<440;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());Ready=false;return;}
        CreditsTestFrame=Frame();
        if(!Launch || !Launch()){UE_LOG(LogTemp,Error,TEXT("SM_CREDITS_TEST_LAUNCH_FAIL"));PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
        for(int I=0;I<256;I++)Step(0);
        Atmosphere=false;Intensity=1.1f;BlendMode=0;Exposure=1.0f;EngineWeather=false;
    }
    if(T==20)Atmosphere=true;
    if(T==40)for(int I=0;I<90;I++)Step(0);
    if(T==60)for(int I=0;I<44;I++)Step(128);
    if(T==80)for(int I=0;I<180;I++)Step(128);
    if(T==100)for(int I=0;I<440;I++)Step(128);
    const TCHAR* Name=T==10?TEXT("credits-original"):T==30?TEXT("credits-glow"):T==50?TEXT("credits-twinkle"):T==70?TEXT("credits-decompilation"):T==90?TEXT("credits-upstream"):T==110?TEXT("credits-statistics"):nullptr;
    UploadPresentation(GameTexture,Pixels(),256,240);RefreshScenePresentation();
    if(Name){
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/(FString(Name)+TEXT(".png")),false,false);
        TArray<uint8> Raw;Raw.Append(WideHud(),400*240*4);FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("SMTests")/(FString(Name)+TEXT("-ui.bgra"))));
        UE_LOG(LogTemp,Display,TEXT("SM_CREDITS_CAPTURE %s roll_frame=%d"),Name,CreditsState(1));
    }
    if(T==125){
        Close();const bool Passed=CreditsState(0)==0 && Frame()==CreditsTestFrame && Opcodes()==0;
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"nativeFramePreserved\":%s,\"cpuOpcodes\":%llu}"),Passed?TEXT("true"):TEXT("false"),Frame()==CreditsTestFrame?TEXT("true"):TEXT("false"),Opcodes()),*(FPaths::ProjectSavedDir()/TEXT("SMTests/credits-unreal-verification.json")));
        UE_LOG(LogTemp,Display,TEXT("SM_CREDITS_UNREAL_%s"),Passed?TEXT("PASS"):TEXT("FAIL"));PlayerOwner->ConsoleCommand(TEXT("quit"));
    }
}

void ASMHUD::TickCinemaTest(){
    const int T=++CinemaTestTicks;
    if(T==1){
        bool Ok=false;
        // The boot coroutine initializes RAM before the first title state. A
        // menu fixture entered at frame zero is overwritten by that startup.
        if(CinemaCase>=5){
            for(int I=0;I<900 && State()==0;I++)if(!Step(0))break;
            if(State()!=1){UE_LOG(LogTemp,Error,TEXT("SM_CINEMA_BOOT_FIXTURE_FAILED"));PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
        }
        if(CinemaCase==3){
            int Animals=-1;FParse::Value(FCommandLine::Get(),TEXT("SMEndingAnimals="),Animals);
            auto End=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_ending_preview_launch")));
            Ok=End && End(Animals);
        }
        else Ok=TestCinematic && TestCinematic(CinemaCase>=5?5:CinemaCase);
        if(!Ok){UE_LOG(LogTemp,Error,TEXT("SM_CINEMA_FIXTURE_FAILED"));PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
        if(CinemaCase>=5){
            auto Reach=[&](int Target,int Sub){
                for(int I=0;I<2400;I++){if(State()==Target && CinemaState(3)==Sub)return true;if(!Step(0))return false;}return false;
            };
            auto Press=[&](int Button){return Step(Button) && Step(0);};
            Ok=Reach(4,4);
            if(Ok && CinemaCase==9)Ok=Press(32); // pending randomized B
            if(Ok && CinemaCase>=6)Ok=Press(8) && Reach(2,3);
            if(Ok && (CinemaCase==7 || CinemaCase==8 || CinemaCase==10))Ok=Press(128);
            if(Ok && (CinemaCase==8 || CinemaCase==10))Ok=Press(128);
            if(Ok && CinemaCase==10)Ok=Press(32) && Press(128) && Press(128) && Press(128);
            if(!Ok){UE_LOG(LogTemp,Error,TEXT("SM_CINEMA_MENU_FIXTURE_FAILED"));PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
        }
        for(int I=0;I<CinemaSample;I++)if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());Ready=false;return;}
        Atmosphere=false;Intensity=1.1f;BlendMode=0;Exposure=1;EngineWeather=false;
    }
    if(T==20)Atmosphere=true;
    if(T==60)for(int I=0;I<6;I++)Step(0);
    UploadPresentation(GameTexture,Pixels(),256,240);RefreshScenePresentation();
    if(T==10||T==30||T==50||T==70){
        FString Name=FString::Printf(TEXT("cinema-%d-%d-%s"),CinemaCase,CinemaSample,T==10?TEXT("original"):T==30?TEXT("gaussian"):T==50?TEXT("effects"):TEXT("motion"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/(Name+TEXT(".png")),false,false);
        TArray<uint8> Raw;Raw.Append(WideHud(),400*240*4);FFileHelper::SaveArrayToFile(Raw,*(FPaths::ProjectSavedDir()/TEXT("SMTests")/(Name+TEXT("-ui.bgra"))));
        UE_LOG(LogTemp,Display,TEXT("SM_CINEMA_CAPTURE %s phase=%d sources=%d state=%d opcodes=%llu"),*Name,CinemaState(0),CinemaState(1),State(),Opcodes());
    }
    if(T==85){UE_LOG(LogTemp,Display,TEXT("SM_CINEMA_UNREAL_%s"),Opcodes()==0?TEXT("PASS"):TEXT("FAIL"));PlayerOwner->ConsoleCommand(TEXT("quit"));}
}
