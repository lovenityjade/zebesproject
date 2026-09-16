#include "SMSystemMenu.h"
#include "SMSeedSettings.h"
#include "SMRom.h"
#include "UnrealClient.h"
#include "SMHUD.h"
#include "SMImGuiWidget.h"
#include "imgui.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

namespace {
const uint16 Masks[14]={256,1,4,8,16,32,64,128,2,512,1024,2048,0x1000,0x2000};
const char* Actions[14]={"Jump","Run","Select weapon","Native pause / map","Move up","Move down","Move left","Move right","Cancel weapon","Fire","Aim up","Aim down","Previous weapon","Next weapon"};
const ImVec4 Gold(.831f,.682f,.333f,1),Green(.39f,.72f,.59f,1);
void Description(const char* Text){ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));ImGui::TextWrapped("%s",Text);ImGui::PopStyleColor();}
bool Reserved(FKey K){
    const FString Name=K.GetFName().ToString();
    return K==EKeys::Escape || K==EKeys::P || K==EKeys::Gamepad_RightThumbstick ||
        (Name.StartsWith(TEXT("F")) && Name.Len()>1 && FChar::IsDigit(Name[1]));
}
void Heading(const char* Title,const char* Detail){ImGui::TextColored(Gold,"%s",Title);Description(Detail);ImGui::Spacing();ImGui::Separator();ImGui::Spacing();}
}
FSMSystemMenu::~FSMSystemMenu(){
    // The worker owns no HUD/Slate pointers. Keep its library loaded until it finishes.
    if(Generation.IsValid())Generation.Wait();
    if(SettingsCheck.IsValid())SettingsCheck.Wait();
    if(Widget){Widget->DrawMenu=nullptr;Widget->Back=nullptr;Widget->CaptureBinding=nullptr;if(GEngine && GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Widget.ToSharedRef());Widget.Reset();}
}
void FSMSystemMenu::Defaults(){
    const FKey K[]={EKeys::Z,EKeys::X,EKeys::RightShift,EKeys::Enter,EKeys::Up,EKeys::Down,EKeys::Left,EKeys::Right,EKeys::C,EKeys::S,EKeys::A,EKeys::D,EKeys::Q,EKeys::E};
    const FKey P[]={EKeys::Gamepad_FaceButton_Bottom,EKeys::Gamepad_FaceButton_Left,EKeys::Gamepad_Special_Left,EKeys::Gamepad_Special_Right,EKeys::Gamepad_DPad_Up,EKeys::Gamepad_DPad_Down,EKeys::Gamepad_DPad_Left,EKeys::Gamepad_DPad_Right,EKeys::Gamepad_FaceButton_Right,EKeys::Gamepad_FaceButton_Top,EKeys::Gamepad_LeftShoulder,EKeys::Gamepad_RightShoulder,EKeys::Gamepad_LeftTrigger,EKeys::Gamepad_RightTrigger};
    for(int I=0;I<14;I++){Keys[I]=K[I];Pads[I]=P[I];}Deadzone=.3f;
}
void FSMSystemMenu::Initialize(bool ShowAtBoot){
#define LOAD_GENERATION(Member,Symbol) Member=reinterpret_cast<decltype(Member)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT(Symbol)))
    LOAD_GENERATION(ConfigureSoundtrack,"sm_soundtrack_configure");
    LOAD_GENERATION(SoundtrackStatus,"sm_soundtrack_status");
    LOAD_GENERATION(SoundtrackError,"sm_soundtrack_error");
    LOAD_GENERATION(ConfigureGeneration,"sm_generation_configure");
    LOAD_GENERATION(TakeGenerationRequest,"sm_generation_take_request");
    LOAD_GENERATION(FailGeneration,"sm_generation_fail");
    LOAD_GENERATION(CommitGeneration,"sm_generation_commit");
    LOAD_GENERATION(EnableSlots,"sm_slots_enable");
    LOAD_GENERATION(SetSlot,"sm_slots_set");
    LOAD_GENERATION(ConfigureRelic,"sm_relic_configure");
    LOAD_GENERATION(SetRefillBeforeSave,"sm_set_refill_before_save");
    LOAD_GENERATION(SetSeedRules,"sm_seed_rules_configure");
    LOAD_GENERATION(SeedRuleCapabilities,"sm_seed_rules_capabilities");
    LOAD_GENERATION(CurrentSlot,"sm_slots_current");
    LOAD_GENERATION(EditableSlot,"sm_slots_editable");
    LOAD_GENERATION(CopySlotSram,"sm_slots_copy_sram");
    LOAD_GENERATION(ConfigureAnimals,"sm_animals_configure");
    LOAD_GENERATION(ConfigureEscapeClock,"sm_escape_clock_configure");LOAD_GENERATION(ConfigureEscapeRouting,"sm_escape_routing_configure");LOAD_GENERATION(EscapeCatalog,"sm_escape_catalog_sha256");
    LOAD_GENERATION(ConfigureMinimizer,"sm_minimizer_configure");LOAD_GENERATION(MinimizerCatalog,"sm_minimizer_catalog_sha256");
    LOAD_GENERATION(ConfigureScavenger,"sm_scavenger_configure");LOAD_GENERATION(ScavengerCatalog,"sm_scavenger_catalog_sha256");
    LOAD_GENERATION(ConfigureObjectives,"sm_objectives_configure");LOAD_GENERATION(ObjectivesCatalog,"sm_objectives_catalog_sha256");LOAD_GENERATION(ConfigureAreas,"sm_areas_configure");LOAD_GENERATION(AreasCatalog,"sm_areas_catalog_sha256");LOAD_GENERATION(ConfigureInitialDoors,"sm_start_configure_initial_doors");LOAD_GENERATION(ConfigureStart,"sm_start_configure");LOAD_GENERATION(ConfigureWorld,"sm_start_configure_world");LOAD_GENERATION(WorldCatalog,"sm_world_data_catalog_sha256");LOAD_GENERATION(ConfigureDoorColors,"sm_doors_configure");LOAD_GENERATION(DoorColorsCatalog,"sm_doors_catalog_sha256");LOAD_GENERATION(ConfigureConnections,"sm_connections_configure");LOAD_GENERATION(ConnectionsCatalog,"sm_connections_catalog_sha256");LOAD_GENERATION(LaunchCredits,"sm_credits_launch");LOAD_GENERATION(CloseCredits,"sm_credits_close");LOAD_GENERATION(CreditsState,"sm_credits_state");
    LOAD_GENERATION(ConfigureVariaUi,"sm_varia_ui_configure");LOAD_GENERATION(ConfigureVariaCounts,"sm_varia_ui_counted_configure");
#undef LOAD_GENERATION
    Tracker.Initialize(Hud.CoreHandle);
    Defaults();FConfigFile Config;Config.Read(Hud.SettingsPath);
    Config.GetBool(TEXT("VariaUI"),TEXT("MaximumAmmo"),VariaAmmo);Config.GetBool(TEXT("VariaUI"),TEXT("Hud"),VariaHud);
    Config.GetBool(TEXT("VariaUI"),TEXT("Reserves"),VariaReserves);Config.GetBool(TEXT("VariaUI"),TEXT("MapMarkers"),VariaMarkers);
    Config.GetBool(TEXT("Tracker"),TEXT("VanillaEnabled"),VanillaTracker);
    Config.GetBool(TEXT("Tracker"),TEXT("Map"),MapTracker);Config.GetBool(TEXT("Tracker"),TEXT("Items"),ItemTracker);
    Config.GetInt(TEXT("Tracker"),TEXT("VanillaSkill"),VanillaTrackerSkill);VanillaTrackerSkill=FMath::Clamp(VanillaTrackerSkill,0,2);
    Config.GetFloat(TEXT("Audio"),TEXT("MasterVolume"),Volume);Volume=FMath::Clamp(Volume,0.f,1.f);
    Config.GetBool(TEXT("Audio"),TEXT("Remastered"),Remastered);
    Config.GetBool(TEXT("Menu"),TEXT("ShowHelp"),Hud.ShowHelp);
    Config.GetBool(TEXT("Audio"),TEXT("Muted"),Mute);Config.GetFloat(TEXT("Input"),TEXT("StickDeadzone"),Deadzone);Deadzone=FMath::Clamp(Deadzone,.1f,.8f);
    Config.GetFloat(TEXT("Menu"),TEXT("Scale"),UiScale);UiScale=FMath::Clamp(UiScale,.8f,1.4f);
    Config.GetBool(TEXT("QualityOfLife"),TEXT("RefillBeforeSave"),RefillBeforeSave);
    Config.GetBool(TEXT("Randomizer"),TEXT("NoAdvancedTechs"),NoAdvancedTechs);
    Config.GetBool(TEXT("Randomizer"),TEXT("RelicHunt"),RelicHunt);
    Config.GetInt(TEXT("Randomizer"),TEXT("RelicsPlaced"),RelicsPlaced);RelicsPlaced=FMath::Clamp(RelicsPlaced,1,60);
    Config.GetInt(TEXT("Randomizer"),TEXT("RelicEscapeMinutes"),RelicEscapeMinutes);if(!SMSeedSettings::ValidEscapeMinutes(RelicEscapeMinutes))RelicEscapeMinutes=5;
    Config.GetInt(TEXT("Randomizer"),TEXT("RelicsRequired"),RelicsRequired);RelicsRequired=FMath::Clamp(RelicsRequired,1,RelicsPlaced);
    Config.GetInt(TEXT("Randomizer"),TEXT("Seed"),Seed);Seed=FMath::Max(0,Seed);
    if(Seed)FCStringAnsi::Snprintf(SeedNumber,sizeof(SeedNumber),"%d",Seed);
    Config.GetArray(TEXT("Randomizer"),TEXT("Patches"),SeedPatches);
    Config.GetInt(TEXT("Randomizer"),TEXT("Skill"),Skill);Skill=FMath::Clamp(Skill,0,7);
    Config.GetInt(TEXT("Randomizer"),TEXT("Progression"),Progression);Progression=FMath::Clamp(Progression,0,8);
    Config.GetString(TEXT("Randomizer"),TEXT("Options"),SeedOptions);
    Config.GetString(TEXT("Randomizer"),TEXT("Techniques"),SeedTechniques);
    Config.GetString(TEXT("Randomizer"),TEXT("SkillSettings"),SeedSkillSettings);
    for(FString* Value:{&SeedOptions,&SeedTechniques,&SeedSkillSettings})if(!SMSeedSettings::Parse(*Value).IsValid())*Value=TEXT("{}");
    for(int I=0;I<14;I++)for(int P=0;P<2;P++){
        FString Name;Config.GetString(TEXT("Input"),*FString::Printf(TEXT("%s%d"),P?TEXT("Pad"):TEXT("Key"),I),Name);
        FKey K{FName(*Name)};if(K.IsValid() && !K.IsAnalog() && !K.IsMouseButton() && K.IsGamepadKey()==bool(P) && !Reserved(K))(P?Pads[I]:Keys[I])=K;
    }
    for(int I=12;I<14;I++)for(int J=0;J<I;J++){
        if(Keys[I]==Keys[J])Keys[I]=EKeys::Invalid;
        if(Pads[I]==Pads[J])Pads[I]=EKeys::Invalid;
    }
    Active.Id=TEXT("legacy-vanilla");Active.Name=TEXT("Original save");Active.Legacy=true;Active.SramPath=Hud.CoreSavePath;Active.Directory=FPaths::GetPath(Hud.CoreSavePath);
    RefreshProfiles();
    Widget=SNew(SSMImGuiWidget);Widget->DrawMenu=[this]{Draw();};Widget->Back=[this]{Back();};Widget->CaptureBinding=[this](FKey K){return Bind(K);};
    Widget->SetVisibility(EVisibility::Collapsed);
    GEngine->GameViewport->AddViewportWidgetContent(Widget.ToSharedRef(),100);
    Hud.AudioComponent->SetVolumeMultiplier(Mute?0.f:Volume);
    FString LastBank;FSMGameProfile Boot;FString BootError;FGuid BankGuid;
    if(Config.GetString(TEXT("Menu"),TEXT("ActiveBank"),LastBank) && FGuid::ParseExact(LastBank,EGuidFormats::Digits,BankGuid) && FSMProfiles::Read(FSMProfiles::Root()/LastBank,Boot,BootError))Activate(Boot);
    else Activate(Active);
    SetOpen(ShowAtBoot);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("SMMenuFlowTest")) &&
       IFileManager::Get().FileExists(*(FPaths::ProjectDir()/TEXT("../ISOLATED_TEST_DIRECTORY")))){
        const bool Passed=RunMenuFlowTest();
        UE_LOG(LogTemp,Display,TEXT("SM_MENU_FLOW_TEST %s"),Passed?TEXT("PASS"):TEXT("FAIL"));
        FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
    }
    FString Preview;
    if(FParse::Value(FCommandLine::Get(),TEXT("SMMenuPreview="),Preview) &&
       IFileManager::Get().FileExists(*(FPaths::ProjectDir()/TEXT("../ISOLATED_TEST_DIRECTORY")))){
        const TMap<FString,int> ExtraPages={{TEXT("effects"),5},{TEXT("trackers"),6},{TEXT("input"),2},{TEXT("interface"),4}};
        if(const int* Page=ExtraPages.Find(Preview)){Section=3;SettingsPage=*Page;MenuPreviewFrame=1;MenuPreviewName=Preview;SetOpen(true);}
        if(Preview==TEXT("home")){Section=0;MenuPreviewFrame=1;MenuPreviewName=Preview;SetOpen(true);}
        if(Preview==TEXT("search")){MenuPreviewFrame=1;MenuPreviewName=Preview;SetOpen(true);FCStringAnsi::Strcpy(Search,"Space Jump");}
        if(Preview==TEXT("audio")){Section=3;SettingsPage=1;MenuPreviewFrame=1;MenuPreviewName=Preview;SetOpen(true);}
        if(Preview==TEXT("graphics")){Section=3;SettingsPage=0;MenuPreviewFrame=1;MenuPreviewName=Preview;SetOpen(true);}
        if(Preview==TEXT("refill")){Section=3;SettingsPage=3;MenuPreviewFrame=1;MenuPreviewName=Preview;SetOpen(true);}
        if(Preview.StartsWith(TEXT("randomizer"))){Section=2;RandomPage=FMath::Clamp(FCString::Atoi(*Preview.Mid(10)),0,8);MenuPreviewFrame=1;MenuPreviewName=Preview;SetOpen(true);if(Preview.Contains(TEXT("-goals")))FCStringAnsi::Strcpy(Search,"Selected objectives");}
    }
#endif
}
void FSMSystemMenu::SetOpen(bool Value){
    if(Value)SelectSection(Section);
    Open=Value;Rebind=-1;Hud.Accumulator=0;Hud.AudioWave->ResetAudio();Hud.AudioComponent->SetPaused(Open || Hud.Paused);
    Hud.PlayerOwner->FlushPressedKeys();Hud.PlayerOwner->bShowMouseCursor=Open;
    Widget->SetVisibility(Open?EVisibility::Visible:EVisibility::Collapsed);
    if(Open){Widget->ClearInput();Hud.TeleportMenu=false;FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Widget);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);Hud.PlayerOwner->SetInputMode(Mode);FSlateApplication::Get().SetAllUserFocus(Widget.ToSharedRef());}
    else {Hud.PlayerOwner->SetInputMode(FInputModeGameOnly());FSlateApplication::Get().SetAllUserFocusToGameViewport();}
}
void FSMSystemMenu::Back(){
    if(Rebind>=0){Rebind=-1;return;}
    if(Confirm){Confirm=false;Pending=EAction::None;return;}
    if(Search[0]){Search[0]=0;return;}
    if(Section){Section=0;return;}SetOpen(false);
}
void FSMSystemMenu::RefreshProfiles(){FSMProfiles::List(Profiles,ProfileWarnings);}
void FSMSystemMenu::Persist(FConfigFile& C) const {
    C.SetString(TEXT("Randomizer"),TEXT("Options"),*SeedOptions);
    C.SetString(TEXT("Randomizer"),TEXT("Techniques"),*SeedTechniques);
    C.SetString(TEXT("Randomizer"),TEXT("SkillSettings"),*SeedSkillSettings);
    C.SetBool(TEXT("QualityOfLife"),TEXT("RefillBeforeSave"),RefillBeforeSave);
    C.SetBool(TEXT("VariaUI"),TEXT("MaximumAmmo"),VariaAmmo);C.SetBool(TEXT("VariaUI"),TEXT("Hud"),VariaHud);
    C.SetBool(TEXT("VariaUI"),TEXT("Reserves"),VariaReserves);C.SetBool(TEXT("VariaUI"),TEXT("MapMarkers"),VariaMarkers);
    C.SetBool(TEXT("Tracker"),TEXT("VanillaEnabled"),VanillaTracker);C.SetBool(TEXT("Tracker"),TEXT("Map"),MapTracker);
    C.SetBool(TEXT("Tracker"),TEXT("Items"),ItemTracker);C.SetInt64(TEXT("Tracker"),TEXT("VanillaSkill"),VanillaTrackerSkill);
    if(Active.Slots.Num()==3)C.SetString(TEXT("Menu"),TEXT("ActiveBank"),*Active.Id);
    C.SetFloat(TEXT("Audio"),TEXT("MasterVolume"),Volume);C.SetBool(TEXT("Audio"),TEXT("Muted"),Mute);
    C.SetBool(TEXT("Audio"),TEXT("Remastered"),Remastered);
    C.SetBool(TEXT("Menu"),TEXT("ShowHelp"),Hud.ShowHelp);
    C.SetFloat(TEXT("Input"),TEXT("StickDeadzone"),Deadzone);C.SetFloat(TEXT("Menu"),TEXT("Scale"),UiScale);
    C.SetBool(TEXT("Randomizer"),TEXT("NoAdvancedTechs"),NoAdvancedTechs);C.SetBool(TEXT("Randomizer"),TEXT("RelicHunt"),RelicHunt);
    C.SetInt64(TEXT("Randomizer"),TEXT("RelicsPlaced"),RelicsPlaced);C.SetInt64(TEXT("Randomizer"),TEXT("RelicsRequired"),RelicsRequired);C.SetInt64(TEXT("Randomizer"),TEXT("RelicEscapeMinutes"),RelicEscapeMinutes);
    if(Seed>=0)C.SetInt64(TEXT("Randomizer"),TEXT("Seed"),Seed);C.SetArray(TEXT("Randomizer"),TEXT("Patches"),SeedPatches);C.SetInt64(TEXT("Randomizer"),TEXT("Skill"),Skill);C.SetInt64(TEXT("Randomizer"),TEXT("Progression"),Progression);
    for(int I=0;I<14;I++){C.SetString(TEXT("Input"),*FString::Printf(TEXT("Key%d"),I),*Keys[I].GetFName().ToString());C.SetString(TEXT("Input"),*FString::Printf(TEXT("Pad%d"),I),*Pads[I].GetFName().ToString());}
}
uint16 FSMSystemMenu::Buttons() const {
    if(Open)return 0;uint16 Result=0;
    for(int I=0;I<14;I++)if(Hud.PlayerOwner->IsInputKeyDown(Keys[I]) || Hud.PlayerOwner->IsInputKeyDown(Pads[I]))Result|=Masks[I];
    float X=Hud.PlayerOwner->GetInputAnalogKeyState(EKeys::Gamepad_LeftX),Y=Hud.PlayerOwner->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
    if(X<-Deadzone)Result|=64;if(X>Deadzone)Result|=128;if(Y<-Deadzone)Result|=32;if(Y>Deadzone)Result|=16;
    return Result;
}
bool FSMSystemMenu::Bind(FKey K){
    if(Rebind<0)return false;
    if(K==EKeys::Escape){Rebind=-1;return true;}
    if(K.IsGamepadKey()!=RebindPad || K.IsAnalog() || K.IsMouseButton())return true;
    if(Reserved(K)) {Status=TEXT("That key is reserved for a system shortcut.");return true;}
    FKey* Bindings=RebindPad?Pads:Keys;
    for(int I=0;I<14;I++)if(I!=Rebind && Bindings[I]==K)Bindings[I]=Bindings[Rebind];
    Bindings[Rebind]=K;Rebind=-1;Status=TEXT("Binding saved. Conflicting bindings are swapped.");Hud.PersistSettings();return true;
}
void FSMSystemMenu::ApplySettings(){
    if(!Hud.Ready)return;
    if(ConfigureSoundtrack){
        const FString MusicPath=SMRom::DataRoot(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("..")))/TEXT("Soundtracks/Remastered");
        ConfigureSoundtrack(Remastered,TCHAR_TO_UTF8(*MusicPath));
    }
    if(ConfigureVariaCounts)ConfigureVariaCounts(Active.Randomized && Active.Generated?Active.Plan.HudCounts.GetData():nullptr,Active.Randomized && Active.Generated?Active.Plan.HudCounts.Num():0);
    if(SetSeedRules)SetSeedRules(Active.Randomized && Active.Generated?Active.Plan.NativeRules:0);
    if(SetRefillBeforeSave)SetRefillBeforeSave(RefillBeforeSave || (Active.Randomized && Active.Generated && Active.Plan.RefillBeforeSave));
    Hud.SetWidescreen(Hud.Widescreen);Hud.SetAssistedWallJump(Hud.AssistedWallJump);Hud.SetAssistedSpaceJump(Hud.AssistedSpaceJump);
    Hud.SetEngineWeather(Hud.EngineWeather && Hud.Atmosphere);Hud.SetCombatEffects(Hud.Atmosphere);
    Hud.RefreshScenePresentation();Hud.UpdatePresentation();Hud.AudioComponent->SetVolumeMultiplier(Mute?0.f:Volume);Hud.PersistSettings();
}
void FSMSystemMenu::Request(EAction Action){
    if(Generation.IsValid()){Status=TEXT("Please wait for generation in the native game menu to finish.");return;}
    if(Action==EAction::Randomized && !SettingsCompatible()){Status=TEXT("Resolve the highlighted settings conflicts before creating a randomized game.");return;}
    Pending=Action;
    // Changing sessions discards progress since the last actual save station.
    if(Hud.State()>=6){Confirm=true;}else{Execute=Action;Pending=EAction::None;}
}
FSMSeedRequest FSMSystemMenu::NextRequest() const {
    FSMSeedRequest Request;Request.Seed=Seed;Request.Patches=SeedPatches;
    const TCHAR* Skills[]={TEXT("casual"),TEXT("regular"),TEXT("veteran"),TEXT("newbie"),TEXT("expert"),TEXT("master"),TEXT("samus"),TEXT("solution")};
    const TCHAR* Speeds[]={TEXT("slow"),TEXT("medium"),TEXT("fast"),TEXT("slowest"),TEXT("fastest"),TEXT("basic"),TEXT("VARIAble"),TEXT("speedrun"),TEXT("random")};
    Request.Skill=Skills[Skill];Request.Progression=Speeds[Progression];
    Request.OptionsJson=SeedOptions;Request.TechniquesJson=SeedTechniques;Request.SkillSettingsJson=SeedSkillSettings;
    Request.NoAdvancedTechs=NoAdvancedTechs;Request.RelicHunt=RelicHunt;
    Request.RelicsPlaced=RelicsPlaced;Request.RelicsRequired=RelicsRequired;Request.RelicEscapeMinutes=RelicEscapeMinutes;return Request;
}
bool FSMSystemMenu::ConfigureAnimalsSlot(int Index,const FSMSeedPlan& Plan){
    return ConfigureAnimals?ConfigureAnimals(Index,Plan.AnimalsMode,Plan.AnimalsMode?TCHAR_TO_UTF8(*Plan.AnimalsCatalog):nullptr)!=0:Plan.AnimalsMode==0;
}
bool FSMSystemMenu::ConfigureEscapeSlot(int Index,const FSMSeedPlan& Plan){
    if(!ConfigureEscapeClock || !ConfigureEscapeRouting || !EscapeCatalog)return !Plan.EscapeClock.version;
    const FString Catalog=Plan.EscapeClock.version?Plan.EscapeCatalog:UTF8_TO_TCHAR(EscapeCatalog());
    return ConfigureEscapeClock(Index,Plan.EscapeClock.version?&Plan.EscapeClock:nullptr) && ConfigureEscapeRouting(Index,Plan.EscapeRouting.version?&Plan.EscapeRouting:nullptr,TCHAR_TO_UTF8(*Catalog));
}
bool FSMSystemMenu::UpdateNativeSlot(int Index){
    if(!SetSlot || !Active.Slots.IsValidIndex(Index))return false;
    const auto& Plan=Active.Slots[Index].Plan;
    if((Plan.NativeRules || Plan.RequiredNativeBehavior.Contains(TEXT("seed-interface-v1"))) && (!SetSeedRules || !SeedRuleCapabilities || (Plan.NativeRules & ~SeedRuleCapabilities())))return false;
    if(Plan.RequiredNativeBehavior.Contains(TEXT("hud-counts-v1")) && !ConfigureVariaCounts)return false;
    const auto& P=Active.Slots[Index];TArray<SmSeedItem> Items;
    if(Plan.RequiredNativeBehavior.Contains(TEXT("native-area-connections-v1")) && (!ConfigureAreas || !AreasCatalog))return false;
    if(ConfigureAreas && AreasCatalog){
        const bool Areas=P.Randomized && P.Generated && !Plan.AreaDestinations.IsEmpty();
        const FString Catalog=Areas?Plan.AreasCatalog:UTF8_TO_TCHAR(AreasCatalog());
        if(!ConfigureAreas(Index,Areas?Plan.AreaDestinations.GetData():nullptr,Areas?Plan.AreaDestinations.Num():0,TCHAR_TO_UTF8(*Catalog)))return false;
    }
    if(Plan.Minimizer.version && (!ConfigureMinimizer || !MinimizerCatalog))return false;
    if(ConfigureMinimizer && MinimizerCatalog){
        const bool Has=P.Randomized && P.Generated && Plan.Minimizer.version;
        const FString Catalog=Has?Plan.MinimizerCatalog:UTF8_TO_TCHAR(MinimizerCatalog());
        if(!ConfigureMinimizer(Index,Has?&Plan.Minimizer:nullptr,TCHAR_TO_UTF8(*Catalog)))return false;
    }
    if(!ConfigureAnimalsSlot(Index,P.Randomized && P.Generated?Plan:FSMSeedPlan()) || !ConfigureEscapeSlot(Index,P.Randomized && P.Generated?Plan:FSMSeedPlan()))return false;
    if(Plan.Scavenger.count && (!ConfigureScavenger || !ScavengerCatalog))return false;
    if(ConfigureScavenger && ScavengerCatalog){
        const FString Catalog=Plan.Scavenger.count?Plan.ScavengerCatalog:UTF8_TO_TCHAR(ScavengerCatalog());
        if(!ConfigureScavenger(Index,Plan.Scavenger.count?&Plan.Scavenger:nullptr,TCHAR_TO_UTF8(*Catalog)))return false;
    }
    if(Plan.RequiredNativeBehavior.Contains(TEXT("native-objectives-v1")) && (!ConfigureObjectives || !ObjectivesCatalog))return false;
    if(ConfigureObjectives && ObjectivesCatalog){
        const bool HasObjectives=P.Randomized && P.Generated && Plan.Objectives.count;
        const FString Catalog=HasObjectives?Plan.ObjectivesCatalog:UTF8_TO_TCHAR(ObjectivesCatalog());
        if(!ConfigureObjectives(Index,HasObjectives?&Plan.Objectives:nullptr,TCHAR_TO_UTF8(*Catalog)))return false;
    }
    if(Plan.RequiredNativeBehavior.Contains(TEXT("native-door-colors-v1")) && (!ConfigureDoorColors || !DoorColorsCatalog))return false;
    if(ConfigureDoorColors && DoorColorsCatalog){
        const bool Colored=P.Randomized && P.Generated && !Plan.DoorColors.IsEmpty();
        const FString Catalog=Colored?Plan.DoorColorsCatalog:UTF8_TO_TCHAR(DoorColorsCatalog());
        if(!ConfigureDoorColors(Index,Colored?Plan.DoorColors.GetData():nullptr,Colored?Plan.DoorColors.Num():0,TCHAR_TO_UTF8(*Catalog)))return false;
    }
    if(Plan.RequiredNativeBehavior.Contains(TEXT("native-start-v1")) && (!ConfigureStart || !WorldCatalog))return false;
    if(Plan.RequiredNativeBehavior.Contains(TEXT("native-door-indicators-v1")) && !ConfigureWorld)return false;
    if(ConfigureStart && WorldCatalog){
        const FString Catalog=P.Randomized && P.Generated && !Plan.WorldCatalog.IsEmpty()?Plan.WorldCatalog:UTF8_TO_TCHAR(WorldCatalog());
        const bool Generated=P.Randomized && P.Generated;
        if(ConfigureWorld){
            if(!ConfigureWorld(Index,Generated?Plan.StartSpawn:0,Generated?Plan.WorldPatches.GetData():nullptr,Generated?Plan.WorldPatches.Num():0,TCHAR_TO_UTF8(*Catalog),Generated?Plan.DoorIndicators.GetData():nullptr,Generated?Plan.DoorIndicators.Num():0))return false;
        } else if(!ConfigureStart(Index,Generated?Plan.StartSpawn:0,Generated?Plan.WorldPatches.GetData():nullptr,Generated?Plan.WorldPatches.Num():0,TCHAR_TO_UTF8(*Catalog)))return false;
    }
    if(Plan.RequiredNativeBehavior.Contains(TEXT("native-boss-connections-v1")) && (!ConfigureConnections || !ConnectionsCatalog))return false;
    if(ConfigureConnections && ConnectionsCatalog){
        const bool Boss=P.Randomized && P.Generated && !Plan.BossDestinations.IsEmpty();
        const FString Catalog=Boss?Plan.ConnectionsCatalog:UTF8_TO_TCHAR(ConnectionsCatalog());
        if(!ConfigureConnections(Index,Boss?Plan.BossDestinations.GetData():nullptr,Boss?Plan.BossDestinations.Num():0,TCHAR_TO_UTF8(*Catalog)))return false;
    }
    if(Plan.RequiredNativeBehavior.Contains(TEXT("native-initial-doors-v1")) && !ConfigureInitialDoors)return false;
    if(ConfigureInitialDoors){
        const bool Generated=P.Randomized && P.Generated;
        if(!ConfigureInitialDoors(Index,Generated?Plan.StartSpawn:0,Generated?Plan.InitialDoors.GetData():nullptr,Generated?Plan.InitialDoors.Num():0))return false;
    }
    for(const auto& I:P.Plan.Items)Items.Add({uint32(I.Address),uint16(I.Plm),uint16(I.Kind)});
    if(!SetSlot(Index,P.Randomized,P.Generated,Items.GetData(),Items.Num(),TCHAR_TO_UTF8(*P.Plan.Fingerprint)))return false;
    if(ConfigureRelic)ConfigureRelic(Index,P.Randomized && P.Generated?P.Plan.RelicsRequired:0);
    auto RelicTimer=reinterpret_cast<decltype(&sm_relic_escape_configure)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_relic_escape_configure")));
    if(RelicTimer)RelicTimer(Index,P.Plan.RelicEscapeMinutes);
    return true;
}
int FSMSystemMenu::SlotAction(int Action,int Slot,int Other,void* Context){
    auto& Self=*static_cast<FSMSystemMenu*>(Context);
    if(!Self.Active.Slots.IsValidIndex(Slot) || Self.Generation.IsValid())return 0;
    FString Error;FSMSeedRequest Request=Self.NextRequest();
    if(Action==1){
        const auto& P=Self.Active.Slots[Slot];if(!P.Randomized || P.Generated)return 0;
        Self.LoadSeedRequest(P.Request);
        Self.EditingSlot=Slot;Self.OpenSlotSettings=true;return 1;
    }
    TArray<uint8> Sram;
    if(Action>=2){
        Sram.SetNumUninitialized(8192);if(!Self.CopySlotSram || Self.CopySlotSram(Sram.GetData(),Sram.Num())!=8192)return 0;
        if(Action==4){
            if(Self.CommittingPlan.Json.IsEmpty() || !FSMProfiles::CompleteBankGeneration(Self.Active,Slot,Self.CommittingPlan,Sram,Error)){Self.Status=Error;return 0;}
            return 1;
        }
        if(Action==2 && (Other<0 || Other>2))return 0;
        const int Offsets[]={0x10,0x66c,0xcc8};
        if(Action==2){
            FMemory::Memcpy(Sram.GetData()+Offsets[Other],Sram.GetData()+Offsets[Slot],1628);
            for(int Offset:{0,8,0x1ff0,0x1ff8})FMemory::Memcpy(Sram.GetData()+Offset+2*Other,Sram.GetData()+Offset+2*Slot,2);
        }else if(Action==3){
            FMemory::Memzero(Sram.GetData()+Offsets[Slot],1628);
            for(int Offset:{0,8,0x1ff0,0x1ff8})FMemory::Memzero(Sram.GetData()+Offset+2*Slot,2);
        }else return 0;
    }
    int Target=Action==2?Other:Slot;
    FSMGameProfile Copy=Self.Active.Slots[Slot];
    if(Action==0 && (!Self.EditableSlot || !Self.EditableSlot()))return 0;
    if(!FSMProfiles::ReplaceSlot(Self.Active,Target,Action==0 && Other?&Request:nullptr,Action==2?&Copy:nullptr,Error,Sram.IsEmpty()?nullptr:&Sram)){
        Self.Status=Error;
        if(IFileManager::Get().FileExists(*(Self.Active.Directory/TEXT("bank.transaction.json")))){Self.Hud.Ready=false;Self.Hud.Failure=TEXT("Save transaction interrupted. Reload this save bank to recover.");}
        return 0;
    }
    Self.UpdateNativeSlot(Target);Self.RefreshProfiles();return 1;
}
void FSMSystemMenu::ConfigureNativeGeneration(){
    if(!Hud.Ready)return;
    if(Active.Slots.Num()==3 && EnableSlots){EnableSlots(&SlotAction,this);for(int I=0;I<3;I++)if(!UpdateNativeSlot(I)){Hud.Ready=false;Hud.Failure=TEXT("A save-slot seed is incompatible with this ROM.");break;}}
    else if(ConfigureGeneration)ConfigureGeneration(Active.Randomized,Active.Generated);
}
void FSMSystemMenu::Tick(){
    if(ConfigureVariaUi)ConfigureVariaUi((VariaAmmo?1:0)|(VariaHud?2:0)|(VariaReserves?4:0)|(VariaMarkers?8:0));
    if(OpenSlotSettings){OpenSlotSettings=false;Section=2;RandomPage=0;Search[0]=0;SetOpen(true);}
    if(CurrentSlot && Active.Slots.IsValidIndex(CurrentSlot())){
        const auto& Slot=Active.Slots[CurrentSlot()];Active.Randomized=Slot.Randomized;Active.Generated=Slot.Generated;Active.Plan=Slot.Plan;Active.Request=Slot.Request;
        Tracker.Tick(Hud.Ready && (Slot.Randomized || VanillaTracker),MapTracker,ItemTracker,
            FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("..")),Slot,VanillaTrackerSkill);
    }else{
        Tracker.Tick(false,MapTracker,ItemTracker,FString(),Active,VanillaTrackerSkill);
    }
    if(ConfigureVariaCounts)ConfigureVariaCounts(Active.Randomized && Active.Generated?Active.Plan.HudCounts.GetData():nullptr,Active.Randomized && Active.Generated?Active.Plan.HudCounts.Num():0);
    if(SetSeedRules)SetSeedRules(Active.Randomized && Active.Generated?Active.Plan.NativeRules:0);
    if(SetRefillBeforeSave)SetRefillBeforeSave(RefillBeforeSave || (Active.Randomized && Active.Generated && Active.Plan.RefillBeforeSave));
    if(TakeGenerationRequest && TakeGenerationRequest()) {
        GeneratingSlot=CurrentSlot?CurrentSlot():-1;
        if(!ConfigureMinimizer || !MinimizerCatalog || !ConfigureScavenger || !ScavengerCatalog || !ConfigureObjectives || !ObjectivesCatalog || !ConfigureAreas || !AreasCatalog || !ConfigureInitialDoors || !ConfigureDoorColors || !DoorColorsCatalog || !ConfigureConnections || !ConnectionsCatalog || !ConfigureWorld || !WorldCatalog || !ConfigureVariaCounts || !SetSeedRules || !SeedRuleCapabilities || (SeedRuleCapabilities()&63)!=63){Status=TEXT("Update the native module before generating with these settings.");if(FailGeneration)FailGeneration(0);}
        else if(!Active.Slots.IsValidIndex(GeneratingSlot) || !Active.Slots[GeneratingSlot].Randomized || Active.Slots[GeneratingSlot].Generated || Generation.IsValid()) {
            if(FailGeneration)FailGeneration(0);
        }else{
            FSMSeedRequest Request;FString Error;
            if(!FSMProfiles::ResolveSeed(Active.Slots[GeneratingSlot],Request,Error)){Status=Error;FailGeneration(0);return;}
            if(EditingSlot==GeneratingSlot)LoadSeedRequest(Request);
            const FString Root=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT(".."));
            GeneratingProfileId=Active.Id;Status=TEXT("Generating from the native new-game menu...");
            Generation=Async(EAsyncExecution::Thread,[Root,Request]{FSMGenerationResult Result;Result.Ok=FSMRandomizer::Generate(Root,Request,Result.Plan,Result.Error);return Result;});
        }
    }
    if(Generation.IsValid() && Generation.IsReady()){
        FSMGenerationResult Result=Generation.Get();Generation={};
        if(GeneratingProfileId!=Active.Id || !Active.Slots.IsValidIndex(GeneratingSlot)){Status=TEXT("Discarded generation from a previous session.");return;}
        FString Error;
        if(!Result.Ok){Status=Result.Error;FailGeneration(0);}
        else{
            CommittingPlan=Result.Plan;
            TArray<SmSeedItem> Items;for(const auto& Item:Result.Plan.Items)Items.Add({uint32(Item.Address),uint16(Item.Plm),uint16(Item.Kind)});
            const FString AreaCatalog=Result.Plan.AreasCatalog.IsEmpty()?UTF8_TO_TCHAR(AreasCatalog()):Result.Plan.AreasCatalog;
            const FString RoutingCatalog=Result.Plan.ConnectionsCatalog.IsEmpty()?UTF8_TO_TCHAR(ConnectionsCatalog()):Result.Plan.ConnectionsCatalog;
            const FString ColorCatalog=Result.Plan.DoorColorsCatalog.IsEmpty()?UTF8_TO_TCHAR(DoorColorsCatalog()):Result.Plan.DoorColorsCatalog;
            const FString MiniCatalog=Result.Plan.MinimizerCatalog.IsEmpty()?UTF8_TO_TCHAR(MinimizerCatalog()):Result.Plan.MinimizerCatalog;
            const FString HuntCatalog=Result.Plan.ScavengerCatalog.IsEmpty()?UTF8_TO_TCHAR(ScavengerCatalog()):Result.Plan.ScavengerCatalog;
            const FString GoalCatalog=Result.Plan.ObjectivesCatalog.IsEmpty()?UTF8_TO_TCHAR(ObjectivesCatalog()):Result.Plan.ObjectivesCatalog;
            if(ConfigureAnimalsSlot(GeneratingSlot,Result.Plan) && ConfigureEscapeSlot(GeneratingSlot,Result.Plan) && ConfigureMinimizer && ConfigureMinimizer(GeneratingSlot,Result.Plan.Minimizer.version?&Result.Plan.Minimizer:nullptr,TCHAR_TO_UTF8(*MiniCatalog)) && ConfigureScavenger && ConfigureScavenger(GeneratingSlot,Result.Plan.Scavenger.count?&Result.Plan.Scavenger:nullptr,TCHAR_TO_UTF8(*HuntCatalog)) && ConfigureAreas && ConfigureAreas(GeneratingSlot,Result.Plan.AreaDestinations.GetData(),Result.Plan.AreaDestinations.Num(),TCHAR_TO_UTF8(*AreaCatalog)) && ConfigureObjectives && ConfigureObjectives(GeneratingSlot,Result.Plan.Objectives.count?&Result.Plan.Objectives:nullptr,TCHAR_TO_UTF8(*GoalCatalog)) && ConfigureInitialDoors && ConfigureInitialDoors(GeneratingSlot,Result.Plan.StartSpawn,Result.Plan.InitialDoors.GetData(),Result.Plan.InitialDoors.Num()) && ConfigureDoorColors && ConfigureDoorColors(GeneratingSlot,Result.Plan.DoorColors.GetData(),Result.Plan.DoorColors.Num(),TCHAR_TO_UTF8(*ColorCatalog)) && ConfigureConnections && ConfigureConnections(GeneratingSlot,Result.Plan.BossDestinations.GetData(),Result.Plan.BossDestinations.Num(),TCHAR_TO_UTF8(*RoutingCatalog)) && ConfigureWorld && ConfigureWorld(GeneratingSlot,Result.Plan.StartSpawn,Result.Plan.WorldPatches.GetData(),Result.Plan.WorldPatches.Num(),TCHAR_TO_UTF8(*Result.Plan.WorldCatalog),Result.Plan.DoorIndicators.GetData(),Result.Plan.DoorIndicators.Num()) && CommitGeneration(Items.GetData(),Items.Num(),TCHAR_TO_UTF8(*Result.Plan.Fingerprint),TCHAR_TO_UTF8(*Active.SramPath))){
                if(ConfigureRelic)ConfigureRelic(GeneratingSlot,Result.Plan.RelicsRequired);
                auto RelicTimer=reinterpret_cast<decltype(&sm_relic_escape_configure)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_relic_escape_configure")));if(RelicTimer)RelicTimer(GeneratingSlot,Result.Plan.RelicEscapeMinutes);
                Status=TEXT("Game generated and saved. Select Start Game in the native menu.");
                UE_LOG(LogTemp,Display,TEXT("SM_NATIVE_GENERATION_READY profile=%s slot=%d seed=%d autosaved=1 tracker=1"),*Active.Id,GeneratingSlot,Result.Plan.Seed);
            }else{Status=TEXT("Could not save the generated slot. Generate Game can retry.");FailGeneration(1);
                if(IFileManager::Get().FileExists(*(Active.Directory/TEXT("bank.transaction.json")))){Hud.Ready=false;Hud.Failure=TEXT("Save transaction interrupted. Reload this save bank to recover.");}
            }
            CommittingPlan={};RefreshProfiles();
        }
        if(!Result.Ok || !Error.IsEmpty())UE_LOG(LogTemp,Warning,TEXT("SM_NATIVE_GENERATION_FAILED %s"),*Status);
    }
    if(Execute!=EAction::None){const EAction Action=Execute;Execute=EAction::None;Perform(Action);}
}
bool FSMSystemMenu::Activate(const FSMGameProfile& Profile){
    if(!ConfigureGeneration || !TakeGenerationRequest || !CommitGeneration || !FailGeneration || !EnableSlots || !SetSlot || !CurrentSlot || !EditableSlot || !CopySlotSram){Status=TEXT("Rebuild the native library to enable the new-game menu.");return false;}
    if(!Hud.Save()){Status=TEXT("Cannot flush the current SRAM. Session change cancelled.");return false;}
    // Shutdown's final flush must happen before journal replay, otherwise it
    // could overwrite the recovered bank with the previous in-memory SRAM.
    if(IFileManager::Get().FileExists(*(Profile.Directory/TEXT("bank.transaction.json")))){
        Hud.AudioComponent->SetPaused(true);Hud.AudioWave->ResetAudio();Hud.Shutdown();Hud.Ready=false;
    }
    FSMGameProfile Checked=Profile;FString Error;
    if(!Profile.Legacy && !FSMProfiles::Read(Profile.Directory,Checked,Error)){Status=Error;if(!Hud.Ready)Hud.Failure=Error;return false;}
    if(IFileManager::Get().FileExists(*Checked.SramPath) && IFileManager::Get().FileSize(*Checked.SramPath)!=8192){Status=TEXT("The selected SRAM has an invalid size. It was not loaded or modified.");return false;}
    if(Checked.Slots.IsEmpty()){
        FSMGameProfile Imported;
        if(!FSMProfiles::CreateBank(Checked.Name,nullptr,&Checked,Imported,Error)){Status=Error;return false;}
        Checked=MoveTemp(Imported);
    }
    EditingSlot=-1;
    struct FSeedItem {uint32 Address;uint16 Plm,Kind;};
    using FStage=int(*)(const FSeedItem*,int,const char*);using FClear=int(*)();
    auto Stage=reinterpret_cast<FStage>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_seed_stage")));
    auto Clear=reinterpret_cast<FClear>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_seed_clear")));
    if(!Stage || !Clear){Status=TEXT("Native seed activation API is unavailable.");return false;}
    auto Prepare=[&](const FSMGameProfile& P){
        if(!Clear())return false;if(P.Slots.Num()==3 || !P.Randomized || !P.Generated)return true;
        TArray<FSeedItem> Items;for(const FSMSeedItem& I:P.Plan.Items)Items.Add({uint32(I.Address),uint16(I.Plm),uint16(I.Kind)});
        return Stage(Items.GetData(),Items.Num(),TCHAR_TO_UTF8(*P.Plan.Fingerprint))!=0;
    };
    Hud.AudioComponent->SetPaused(true);Hud.AudioWave->ResetAudio();Hud.Shutdown();
    bool Ok=Prepare(Checked) && Hud.Init(TCHAR_TO_UTF8(*Hud.CoreRomPath),TCHAR_TO_UTF8(*Checked.SramPath));
    if(!Ok){
        Status=TEXT("Cannot load profile: ")+FString(UTF8_TO_TCHAR(Hud.Error()));
        Hud.Shutdown();Hud.Ready=Prepare(Active) && Hud.Init(TCHAR_TO_UTF8(*Hud.CoreRomPath),TCHAR_TO_UTF8(*Active.SramPath));
        if(!Hud.Ready)Hud.Failure=Status+TEXT(" Previous session could not be reopened; saves remain on disk.");
    }else{
        Hud.RouteVisible=Hud.RouteAutoShown=false;Active=MoveTemp(Checked);Hud.CoreSavePath=Active.SramPath;Hud.ProfilePath=Active.Directory/TEXT("Achievements.ini");Hud.Ready=true;
        FConfigFile Achievements;Achievements.Read(Hud.ProfilePath);Hud.AchievementBits=Hud.AchievementTotalKills=0;
        Achievements.GetInt(TEXT("Local"),TEXT("Unlocked"),Hud.AchievementBits);Achievements.GetInt(TEXT("Local"),TEXT("EnemyKills"),Hud.AchievementTotalKills);
        Status=TEXT("Profile loaded: ")+Active.Name;
        UE_LOG(LogTemp,Display,TEXT("SM_SESSION_ACTIVATED mode=%s profile=%s seed=%d fingerprint=%s"),Active.Randomized?TEXT("randomized"):TEXT("vanilla"),*Active.Id,Active.Plan.Seed,*Active.Plan.Fingerprint);
    }
    if(Hud.Ready){
        Hud.Paused=false;Hud.TeleportMenu=false;Hud.Accumulator=0;Hud.PresentationRoom=Hud.LastState=-1;Hud.VisualEffects.Reset();
        Hud.AchievementPrimed=false;Hud.AchievementKills=Hud.AchievementPower=Hud.AchievementGrapple=Hud.AchievementScrew=0;
        ConfigureNativeGeneration();Hud.SetBorder(false);Hud.LoadRoomDecorations();ApplySettings();
    }
    if(Ok){Hud.PersistSettings();RefreshProfiles();SetOpen(false);}return Ok;
}
void FSMSystemMenu::Perform(EAction Action){
    if(Action==EAction::Quit){if(Generation.IsValid()){Status=TEXT("Please wait for seed generation to finish before exiting.");return;}if(!Hud.Save()){Status=TEXT("Cannot flush SRAM; exit cancelled.");return;}Hud.PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
    if(Action==EAction::Reset){Activate(Active);return;}
    if(Action==EAction::Load){Activate(PendingProfile);return;}
    FSMGameProfile Profile;FString Error;
    if(Action==EAction::Randomized && !SettingsCompatible()){Status=TEXT("Settings changed. Wait for the compatibility check before creating a game.");return;}
    const FSMSeedRequest Request=NextRequest();
    if(FSMProfiles::CreateBank(UTF8_TO_TCHAR(ProfileName),Action==EAction::Randomized?&Request:nullptr,nullptr,Profile,Error)){
        RefreshProfiles();
        if(Activate(Profile) && Action!=EAction::Randomized){
            auto Configure=reinterpret_cast<decltype(&sm_run_configure)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_run_configure")));
            auto Import=reinterpret_cast<decltype(&sm_run_import_ngplus)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_run_import_ngplus")));
            bool Ok=Configure!=nullptr;
            for(int I=0;I<3 && Ok;I++){
                Ok=Configure(I,VanillaRunCategory)!=0;
                if(Ok && Action==EAction::NewGamePlus)Ok=Import && Import(TCHAR_TO_UTF8(*NgSourcePath),NgSourceSlot,I);
            }
            if(!Ok)Status=TEXT("Could not configure the new run. The source save was preserved.");
        }
    }else Status=Error;
}
bool FSMSystemMenu::Row(const char* Label,const char* Detail){
    if(Search[0] && !FString(UTF8_TO_TCHAR(Label)).Contains(UTF8_TO_TCHAR(Search)) && !FString(UTF8_TO_TCHAR(Detail)).Contains(UTF8_TO_TCHAR(Search)))return false;
    ImGui::PushID(Label);
    RowUsesTable=ImGui::GetContentRegionAvail().x>ImGui::GetFontSize()*34;
    if(RowUsesTable){
        if(!ImGui::BeginTable("row",2,ImGuiTableFlags_SizingStretchProp)){ImGui::PopID();return false;}ImGui::TableSetupColumn("label",0,.62f);ImGui::TableSetupColumn("control",0,.38f);ImGui::TableNextColumn();
        ImGui::TextWrapped("%s",Label);Description(Detail);ImGui::TableNextColumn();
    }else{ImGui::TextWrapped("%s",Label);Description(Detail);}
    ImGui::SetNextItemWidth(-1);return true;
}
void FSMSystemMenu::EndRow(){if(RowUsesTable)ImGui::EndTable();ImGui::Spacing();ImGui::Separator();ImGui::Spacing();ImGui::PopID();}
void FSMSystemMenu::SelectSection(int Value){
    // An editable native slot owns its draft. Opening Settings with Escape must
    // behave exactly like the native Randomizer Options entry, never edit a
    // different slot's request left over from an earlier session.
    if(Value==2){
        const int Slot=CurrentSlot?CurrentSlot():-1;
        const bool Editable=Active.Slots.IsValidIndex(Slot) && EditableSlot && EditableSlot() &&
            Active.Slots[Slot].Randomized && !Active.Slots[Slot].Generated;
        if(Editable && EditingSlot!=Slot)LoadSeedRequest(Active.Slots[Slot].Request);
        EditingSlot=Editable?Slot:-1;
    }
    Section=Value;Search[0]=0;Rebind=-1;
}
void FSMSystemMenu::DrawHome(){
    Heading("WELCOME TO ZEBES","Play from the original game menus. Use this menu whenever you want to adjust your experience.");
    if(ImGui::Button("Return to game",ImVec2(-1,ImGui::GetFontSize()*2)))SetOpen(false);
    ImGui::Spacing();
    Heading("START A GAME","Choose SAMUS A, B or C in the game's file-select screen.");
    ImGui::TextWrapped("Vanilla: choose Vanilla Mode, then Start Game.");
    ImGui::TextWrapped("Randomized: choose Randomizer Mode, review Randomizer Options, then Generate Game. Start Game unlocks when generation finishes.");
    Description("Each slot keeps its own mode and seed. Your randomizer settings are remembered for the next new slot; existing games keep their original rules.");
    ImGui::Spacing();
    if(ImGui::Button("Game settings",ImVec2(-1,0))){SettingsPage=0;SelectSection(3);}
    if(ImGui::Button("Prepare randomizer settings",ImVec2(-1,0))){RandomPage=0;SelectSection(2);}
    if(ImGui::Button("Save library / New Game+",ImVec2(-1,0)))SelectSection(1);
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    ImGui::TextDisabled("CURRENT SAVE BANK");ImGui::TextWrapped("%s",TCHAR_TO_UTF8(*Active.Name));
    const int Slot=CurrentSlot?CurrentSlot():-1;
    if(Active.Slots.IsValidIndex(Slot)){
        const auto& Save=Active.Slots[Slot];
        ImGui::Text("SAMUS %c  /  %s",'A'+Slot,Save.Randomized?"RANDOMIZED":"VANILLA");
        if(Save.Randomized){if(Save.Generated)ImGui::Text("Seed %d",Save.Plan.Seed);else Description("Waiting for Generate Game in the native menu.");}
    }
    ImGui::Spacing();Description("Open settings: Escape / F4 / right stick click. Open the original map and equipment screen: Start.");
}
void FSMSystemMenu::DrawSaves(){
    if(!Search[0]){
    Heading("SAVE LIBRARY","Each save bank contains three independent A/B/C slots. Choose Vanilla or Randomized inside the game, separately for each new slot.");
    Description("You can play immediately using the current bank. Create an additional bank only when you need more slots, a speedrun or New Game+.");
    ImGui::SetNextItemWidth(FMath::Min(420.f,ImGui::GetContentRegionAvail().x));
    ImGui::InputTextWithHint("##profile-name","New save bank name (optional)",ProfileName,sizeof(ProfileName));
    ImGui::SetNextItemWidth(FMath::Min(300.f,ImGui::GetContentRegionAvail().x));ImGui::Combo("Run category",&VanillaRunCategory,"Casual\0Speedrun - No QoL\0Speedrun - QoL\0");
    if(VanillaRunCategory)Description("Ranked by native in-game time. Glitches allowed. QoL permits only Wall Jump / Space Jump assists. Save refills and trackers are disabled during speedruns.");
    if(ImGui::Button(VanillaRunCategory?"Create Vanilla speedrun bank":"Create additional save bank"))Request(EAction::Vanilla);
    ImGui::Spacing();Description("Save at a station in the game. Flushing SRAM does not save your current position. Loading a profile returns to its title and file-select screen.");
    if(ImGui::Button("Refresh library"))RefreshProfiles();
    }
    for(const FString& Warning:ProfileWarnings)ImGui::TextWrapped("Profile unavailable: %s",TCHAR_TO_UTF8(*Warning));
    for(const FSMGameProfile& P:Profiles){
        const FString Mode=P.Slots.Num()==3?TEXT("A / B / C"):P.Randomized?TEXT("Randomized"):TEXT("Vanilla");
        if(Search[0] && !(P.Name+Mode+FString::FromInt(P.Plan.Seed)).Contains(UTF8_TO_TCHAR(Search)))continue;
        ImGui::PushID(TCHAR_TO_UTF8(*P.Id));ImGui::Separator();
        auto Completed=reinterpret_cast<decltype(&sm_run_has_completion)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_run_has_completion")));
        if(Completed)for(int I=0;I<3;I++)if(Completed(TCHAR_TO_UTF8(*P.SramPath),I)){
            const FString Label=FString::Printf(TEXT("New Game+ from SAMUS %c"),'A'+I);
            if(ImGui::Button(TCHAR_TO_UTF8(*Label))){NgSourcePath=P.SramPath;NgSourceSlot=I;Request(EAction::NewGamePlus);}
            Description("Creates a separate bank with the first completed game's equipment and ammo capacities. Enemy endurance x2; enemy damage x1.5.");
        }
        ImGui::TextColored(P.Randomized?Gold:Green,"%s%s",TCHAR_TO_UTF8(*Mode.ToUpper()),P.Id==Active.Id?"  /  CURRENT":"");
        ImGui::TextWrapped("%s",TCHAR_TO_UTF8(*P.Name));
        if(P.Randomized)Description(TCHAR_TO_UTF8(*(P.Generated?FString::Printf(TEXT("Seed %d  /  %s"),P.Plan.Seed,*P.Plan.Fingerprint.Left(12)):(P.Request.Seed?FString::Printf(TEXT("Seed %d  /  Awaiting Generate Game"),P.Request.Seed):FString(TEXT("Random seed  /  Awaiting Generate Game"))))));
        for(int I=0;I<P.Slots.Num();I++){
            const auto& Slot=P.Slots[I];ImGui::Text("%c  /  %s  /  %s",'A'+I,Slot.Randomized?"Randomized":"Vanilla",Slot.Randomized?(Slot.Generated?TCHAR_TO_UTF8(*FString::Printf(TEXT("Seed %d"),Slot.Plan.Seed)):"Awaiting generation"):"Ready");
            if(Slot.Randomized){
                ImGui::PushID(I);ImGui::SameLine();
                if(ImGui::SmallButton("Copy settings")){
                    FString Text,Error;if(SMSeedSettings::ExportString(Slot.Request,Text,Error)){ImGui::SetClipboardText(TCHAR_TO_UTF8(*Text));Status=FString::Printf(TEXT("Samus %c settings copied. Seed: %s."),'A'+I,Slot.Request.Seed?*FString::FromInt(Slot.Request.Seed):TEXT("random at generation"));}else Status=Error;
                }ImGui::PopID();
            }
        }
        if(P.Legacy)Description("Your existing save file, kept at its original location.");
        if(ImGui::Button("Load save bank")){PendingProfile=P;Request(EAction::Load);}ImGui::PopID();
    }
}
bool FSMSystemMenu::SettingsCompatible() const {
    const FString Current=FSMRandomizer::RequestJson(NextRequest());
    return !Current.IsEmpty() && SettingsCheckInitialized && SettingsCheckDone==Current && SettingsValidation.Ok;
}
void FSMSystemMenu::UpdateSettingsCheck(){
    const auto Request=NextRequest();
    const FString Current=FSMRandomizer::RequestJson(Request);
    if(!SettingsCheckInitialized || Current!=SettingsCheckWanted){
        SettingsCheckInitialized=true;SettingsCheckWanted=Current;SettingsChangedAt=FPlatformTime::Seconds();
    }
    if(SettingsCheck.IsValid() && SettingsCheck.IsReady()){
        auto Result=SettingsCheck.Get();SettingsCheck={};
        if(SettingsCheckRunning==Current){SettingsValidation=MoveTemp(Result);SettingsCheckDone=Current;}
    }
    if(!Generation.IsValid() && !SettingsCheck.IsValid() && SettingsCheckDone!=Current && FPlatformTime::Seconds()-SettingsChangedAt>=.25){
        SettingsCheckRunning=Current;
        const FString Root=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT(".."));
        SettingsCheck=Async(EAsyncExecution::Thread,[Root,Request]{return FSMRandomizer::ValidateSettings(Root,Request);});
    }
}
void FSMSystemMenu::DrawRandomizer(){
    if(!Search[0]){
        const char* Titles[]={"SEED & SHARING","LOGIC & DIFFICULTY","ITEM PROGRESSION","ITEMS & AMMUNITION","WORLD & ESCAPE","GOALS & VICTORY","GAMEPLAY PATCHES","ADVANCED / TECHNIQUES","ADVANCED / COMBAT & HEAT"};
        const char* Details[]={"Start with a preset, share a configuration or leave the seed blank for a surprise.","Choose what the generator and tracker expect you to be able to do.","Decide how progression items are spread through the world.","Configure the equipment pool, energy and ammunition.","Choose starting locations, world connections, doors and escape rules.","Choose what completes your adventure, including Chozo Tablet Hunt.","Optional changes to the original game's behavior and routes.","Fine-tune individual techniques. Presets already provide values for all of them.","Fine-tune boss fights, damage tolerance and heat runs. These affect both generation and tracking."};
        Heading(Titles[RandomPage],Details[RandomPage]);
    }
    UpdateSettingsCheck();
    if(SettingsCheckDone!=SettingsCheckWanted || SettingsCheckWanted.IsEmpty()){
        ImGui::TextColored(Gold,"Checking settings...");
        if(Seed<0)ImGui::TextWrapped("Seed number: leave it blank or enter 1 to 2147483647.");
    }else if(!SettingsValidation.Ok){
        ImGui::TextColored(ImVec4(1,.45f,.35f,1),"Resolve these conflicts before Generate Game:");
        const char* Categories[]={"Seed & presets","Logic & difficulty","Progression","Items & ammo","World & escape","Goals & victory","Gameplay patches","Techniques","Combat & heat"};
        for(int I=0;I<SettingsValidation.Issues.Num();I++){
            const auto& Issue=SettingsValidation.Issues[I];ImGui::PushID(I);
            ImGui::TextWrapped("%s",TCHAR_TO_UTF8(*Issue.Message));
            if(ImGui::SmallButton(Categories[Issue.Page])){RandomPage=Issue.Page;Search[0]=0;}
            if(Issue.OtherPage>=0){ImGui::SameLine();if(ImGui::SmallButton(Categories[Issue.OtherPage])){RandomPage=Issue.OtherPage;Search[0]=0;}}
            ImGui::PopID();
        }
    }else{
        ImGui::TextColored(Green,"Settings compatible");
        if(ImGui::IsItemHovered())ImGui::SetTooltip("Generation will also verify a complete playable route.");
    }
    if(SettingsCheckDone==SettingsCheckWanted)for(const auto& Note:SettingsValidation.Notes)Description(TCHAR_TO_UTF8(*Note));
    ImGui::Separator();
    const bool Editing=Active.Slots.IsValidIndex(EditingSlot) && CurrentSlot && CurrentSlot()==EditingSlot && EditableSlot && EditableSlot() && Active.Slots[EditingSlot].Randomized;
    if(Editing)ImGui::TextColored(Gold,"EDITING SAMUS %c  /  SAVED AUTOMATICALLY",'A'+EditingSlot);
    else Description("New-slot defaults / saved automatically. Existing seeds are unchanged.");
    const bool Busy=Generation.IsValid();bool Changed=false;
    const bool All=Search[0]!=0;
    if(RandomPage==0 || All){
        if(Row("Seed number","Leave empty for a random number when you select Generate Game. Enter a number to reproduce a seed with the same settings.")){
            ImGui::BeginDisabled(Busy);
            if(ImGui::InputTextWithHint("##seed","Random when blank",SeedNumber,sizeof(SeedNumber),ImGuiInputTextFlags_CharsDecimal)){
                if(!SMSeedSettings::ParseSeedNumber(UTF8_TO_TCHAR(SeedNumber),Seed))Seed=-1;
                Changed=true;
            }
            if(Seed<0)ImGui::TextWrapped("Enter 1 to 2147483647, or leave the field empty.");
            ImGui::EndDisabled();EndRow();
        }
        if(!All){
            Description(Editing?"Return to the native menu and select Generate Game when your settings are ready.":"Return to the game, choose an empty A/B/C slot and select Randomizer Mode to use these settings.");
            if(Busy)ImGui::TextColored(Gold,"Generation is running in the native game menu.");
        }
    }
    Changed|=DrawSeedCatalog(Busy);
    if(RandomPage==5 || All){
        if(Row("Chozo Tablet Hunt","Collect the required quota to trigger a timed escape to your ship. Mother Brain is not required. Tablets replace surplus ammo packs; all checks must remain reachable.")){ImGui::BeginDisabled(Busy);Changed|=ImGui::Checkbox("##relichunt",&RelicHunt);ImGui::EndDisabled();EndRow();}
        if(RelicHunt){
            if(Row("Tablets in the world","The generator preserves enough ammo packs for progression. Very high totals can fail pool validation.")){ImGui::BeginDisabled(Busy);Changed|=ImGui::SliderInt("##placed",&RelicsPlaced,1,60);RelicsRequired=FMath::Min(RelicsRequired,RelicsPlaced);ImGui::EndDisabled();EndRow();}
            if(Row("Tablets required","Collect this many tablets to start the escape. Space Jump is granted and incoming damage doubles during the escape.")){ImGui::BeginDisabled(Busy);Changed|=ImGui::SliderInt("##required",&RelicsRequired,1,RelicsPlaced);ImGui::EndDisabled();EndRow();}
            if(Row("Tablet escape timer","Collect the required quota, then reach and board Samus's ship before the native timer expires.")){
                const int Values[]={3,5,6,7,10};int Choice=0;for(int I=0;I<5;I++)if(Values[I]==RelicEscapeMinutes)Choice=I;
                ImGui::BeginDisabled(Busy);if(ImGui::Combo("##tablet-timer",&Choice,"3 minutes\0" "5 minutes\0" "6 minutes\0" "7 minutes\0" "10 minutes\0")){RelicEscapeMinutes=Values[Choice];Changed=true;}ImGui::EndDisabled();EndRow();
            }
        }
    }
    if(Changed)SaveSeedDraft();
}
bool FSMSystemMenu::SaveSeedDraft(){
    const bool Editing=Active.Slots.IsValidIndex(EditingSlot) && CurrentSlot && CurrentSlot()==EditingSlot &&
        EditableSlot && EditableSlot() && Active.Slots[EditingSlot].Randomized && !Active.Slots[EditingSlot].Generated;
    if(Editing){
        FString Error;const auto Draft=NextRequest();
        if(!FSMProfiles::SaveRequest(Active.Slots[EditingSlot],Draft,Error)){
            Status=TEXT("Settings were not saved: ")+Error;return false;
        }
        Active.Slots[EditingSlot].Request=Draft;
    }
    Hud.PersistSettings();return true;
}

void FSMSystemMenu::SetFullscreen(bool Fullscreen){
    auto* Video=GEngine->GetGameUserSettings();
    Video->SetFullscreenMode(Fullscreen?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);
    Video->ApplyResolutionSettings(false);Video->ConfirmVideoMode();Video->SaveSettings();
}
void FSMSystemMenu::ToggleFullscreen(){SetFullscreen(GEngine->GetGameUserSettings()->GetFullscreenMode()==EWindowMode::Windowed);}
void FSMSystemMenu::DrawSettings(){
    if(!Search[0]){
        const char* Titles[]={"DISPLAY","AUDIO","CONTROLS","GAMEPLAY & COMFORT","INTERFACE","ATMOSPHERE & EFFECTS","MAP & TRACKERS"};
        const char* Details[]={"Window, screen proportions and frame presentation.","Music, sound effects and soundtrack selection.","Keyboard and controller bindings. Select a button to reassign it.","Optional assists and comfort settings. Speedrun rules take precedence.","Menu readability, shortcut help and native VARIA interface options.","Lighting, depth and weather over the original pixel art.","Tracking integrated into the original map and minimap."};
        Heading(Titles[SettingsPage],Details[SettingsPage]);
        Description("Changes are applied and saved automatically.");ImGui::Spacing();
    }
    bool Changed=false;const bool All=Search[0]!=0;
    auto Toggle=[&](const char* Label,const char* Detail,bool& Value){if(Row(Label,Detail)){Changed|=ImGui::Checkbox("##value",&Value);EndRow();}};
    auto Slider=[&](const char* Label,const char* Detail,float& Value,float Min,float Max,const char* Format){if(Row(Label,Detail)){Changed|=ImGui::SliderFloat("##value",&Value,Min,Max,Format);EndRow();}};
    if(SettingsPage==0 || All){
        Toggle("Widescreen","Extend the gameplay view horizontally.",Hud.Widescreen);
        if(Row("Window mode","Choose a windowed or borderless fullscreen presentation.")){
            UGameUserSettings* Video=GEngine->GetGameUserSettings();int Mode=Video->GetFullscreenMode()==EWindowMode::Windowed?0:1;
            if(ImGui::Combo("##mode",&Mode,"Windowed\0Borderless fullscreen\0"))SetFullscreen(Mode!=0);EndRow();
        }
        if(Row("Image scaling","Fit to screen uses the largest image that preserves its proportions. Integer scaling keeps uniform pixel blocks with black margins; very small windows fall back to fit. Neither mode stretches or crops the game.")){
            Changed|=ImGui::Combo("##scaling",&Hud.ImageScaling,"Fit to screen\0Integer scaling\0");EndRow();
        }
        if(Row("Vertical sync","Synchronize presentation with the display.")){
            auto* Video=GEngine->GetGameUserSettings();bool Enabled=Video->IsVSyncEnabled();if(ImGui::Checkbox("##vsync",&Enabled)){Video->SetVSyncEnabled(Enabled);Video->ApplyNonResolutionSettings();Video->SaveSettings();}EndRow();
        }
    }
    if(SettingsPage==5 || All){
        Toggle("Atmosphere & lighting","Enable the presentation layer and native combat effects.",Hud.Atmosphere);
        if(Row("Soft Gaussian blend","Blend a lightly blurred copy over the scene. The HUD is excluded.")){Changed|=ImGui::Combo("##blend",&Hud.BlendMode,"Lighten\0Multiply\0");EndRow();}
        Slider("Blend opacity","Strength of the soft Gaussian layer.",Hud.Intensity,0,1,"%.2f");
        Slider("Scene brightness","Brightness of the softened scene. 1.00 is neutral; 1.10 adds a gentle lift.",Hud.Exposure,.75f,1.25f,"%.2f");
        Toggle("Background parallax","Offset supported background layers as Samus moves.",Hud.Parallax);
        Toggle("Background depth","Add restrained depth to the original scenery.",Hud.Depth);
        Toggle("Surface relief","Light and shade edges to emphasize structure.",Hud.Relief);
        Toggle("Engine weather","Rain, fog, underwater and heat effects by environment.",Hud.EngineWeather);
    }
    if(SettingsPage==1 || All){
        Slider("Master volume","Overall volume of music and sound effects.",Volume,0,1,"%.2f");Toggle("Mute audio","Silence game audio without changing its volume setting.",Mute);
        if(Row("Soundtrack","Original SNES music or the selected Remastered arrangements. Sound effects stay original. Missing tracks use Original automatically.")){
            int Selection=Remastered?1:0;
            ImGui::BeginDisabled(!ConfigureSoundtrack);
            if(ImGui::Combo("##soundtrack",&Selection,"Original\0Remastered\0")){Remastered=Selection==1;Changed=true;Hud.AudioWave->ResetAudio();}
            ImGui::EndDisabled();
            if(Remastered && SoundtrackError && SoundtrackError()[0])ImGui::TextWrapped("%s",SoundtrackError());
            EndRow();
        }
    }
    if(SettingsPage==2 || All){
        Slider("Left stick deadzone","Ignore small analog-stick movements around its center.",Deadzone,.1f,.8f,"%.2f");
        for(int I=0;I<14;I++)if(Row(Actions[I],"")){
            for(int P=0;P<2;P++){
                ImGui::PushID(P);const FString Label=Rebind==I && RebindPad==bool(P)?TEXT("Press a button..."):(P?TEXT("Pad: "):TEXT("Key: "))+(P?Pads[I]:Keys[I]).GetDisplayName().ToString();
                if(ImGui::Button(TCHAR_TO_UTF8(*Label),ImVec2(-1,0))){Rebind=I;RebindPad=P!=0;}ImGui::PopID();
            }EndRow();
        }
        if(!All && ImGui::Button("Restore default bindings")){Defaults();Changed=true;}
        if(!All)Description("Select a binding, then press a key or button. Escape cancels; duplicate bindings swap. Menu: Escape / F4 / right stick click. Native pause: Start.");
    }
    auto RunState=reinterpret_cast<decltype(&sm_run_state)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_run_state")));
    const int RunCategory=RunState?RunState(0):0;
    if(SettingsPage==3 || All){
        if(RunCategory)Description("Speedrun rules override these preferences: save refills and trackers are off. No QoL also forces both jump assists off.");
        ImGui::BeginDisabled(RunCategory!=0);
        Toggle("Refill energy and ammo when saving","Fully restore energy, reserves, Missiles, Super Missiles and Power Bombs when you confirm a save at a station. Applies to Vanilla and Randomized games; off by default.",RefillBeforeSave);
        ImGui::EndDisabled();
        ImGui::BeginDisabled(RunCategory==1);
        Toggle("Assisted wall jump","Enable the forgiving wall jump. Disable to retain original timing.",Hud.AssistedWallJump);
        Toggle("Assisted Space Jump","Relax the original repeated-jump rhythm.",Hud.AssistedSpaceJump);
        ImGui::EndDisabled();
        Slider("Combat flash strength","Reduce added combat flashes, including power bombs. Does not modify original ROM flashes.",Hud.FlashStrength,0,1,"%.2f");
    }
    if(SettingsPage==4 || All){
        Toggle("VARIA maximum ammo","Randomized slots only. Maximum capacity above the compact ammo icon, current ammo below. Supports three-digit Supers and Power Bombs.",VariaAmmo);
        Toggle("VARIA area & objective HUD","Randomized slots only. Graph-area name, uncollected locations, compact energy tanks and five-second objective notifications.",VariaHud);
        Toggle("VARIA improved reserves","Randomized slots only. Filled tank pickups, no wasted reserve energy, cancelable manual transfer, safer auto-refill and empty/partial/full HUD indicator.",VariaReserves);
        Toggle("VARIA map item markers","Randomized slots only. Native collection dots when the logic map tracker is disabled; tracker colors take precedence when enabled.",VariaMarkers);
        Slider("Menu text scale","Resize the system interface independently of the game pixels.",UiScale,.8f,1.4f,"%.2f");
        if(Row("Native pause & map","The original game assets, equipment screen and map remain available with Start.")){ImGui::TextUnformatted("Original Super Metroid UI");EndRow();}
        Toggle("Gameplay shortcut help","Show the existing in-game shortcut overlay.",Hud.ShowHelp);
    }
    if(SettingsPage==6 || All){
        Toggle("Trackers in Vanilla","Off by default. Enables tracking with original item placements and unpatched vanilla rules; does not randomize the save.",VanillaTracker);
        Toggle("Map tracker","Item squares and boss diamonds on the native pause map and minimap. Both use the effective rules of the seed.",MapTracker);
        Toggle("Item tracker","Original pack icons and acquired capacities in the native map header. Dim equipment is missing; dim boss portraits are defeated.",ItemTracker);
        if(Row("Vanilla tracker techniques","Only used for Vanilla tracking. Randomized saves always use their saved skill configuration.")){
            Changed|=ImGui::Combo("##vanillaskill",&VanillaTrackerSkill,"Casual\0Regular\0Veteran\0");EndRow();
        }
        if(Row("Tracker legend","Squares: items. Diamonds: boss fights. Green: reachable and beatable with a safe return after victory. Mixed item checks share a square. White outlines: pending.")){
            ImGui::TextColored(ImVec4(32/255.f,1,32/255.f,1),"Green: accessible");
            ImGui::TextColored(ImVec4(207/255.f,16/255.f,16/255.f,1),"Red: blocked");
            ImGui::TextColored(ImVec4(1,1,32/255.f,1),"Yellow: advanced route / uncertain return");
            ImGui::TextColored(ImVec4(48/255.f,64/255.f,1,1),"Blue: inspect only (when supported)");
            ImGui::TextDisabled("Gray: collected / defeated");ImGui::TextWrapped("%s",TCHAR_TO_UTF8(*Tracker.Status));EndRow();
        }
    }
    if(Changed)ApplySettings();
}
void FSMSystemMenu::DrawDebug(){
    if(!Search[0])Heading("DEBUG","The original F1-F12 tools, with their current values. Changes also update Settings.");
    if(Row("Credit roll preview","Original assets, Zebes atmosphere and this slot's tracked statistics. Start returns to the untouched session; hold Right to fast-forward.")){
        bool Preview=CreditsState && CreditsState(0)==2;
        bool Available=Hud.Ready && LaunchCredits && (Hud.State()==8 || Hud.State()==15);
        ImGui::BeginDisabled(!Available);
        if(ImGui::Button(Preview?"Return from Credit Roll":"Launch Credit Roll",ImVec2(-1,0))){
            if(Preview)CloseCredits();
            else if(!LaunchCredits())Status=TEXT("Finish the current message or transition before previewing credits.");
            Hud.Paused=false;SetOpen(false);Hud.Accumulator=0;Hud.RefreshScenePresentation();
        }
        ImGui::EndDisabled();EndRow();
    }
    bool Changed=false;
    auto Toggle=[&](const char* Label,const char* Detail,bool& Value){
        if(Row(Label,Detail)){Changed|=ImGui::Checkbox("##enabled",&Value);EndRow();}
    };
    ImGui::BeginDisabled(!Hud.Ready);
    Toggle("F1 / Shortcut help","Show the gameplay shortcut overlay after resuming.",Hud.ShowHelp);
    Toggle("F2 / Atmosphere","Enable the Gaussian presentation layer, lighting and associated effects.",Hud.Atmosphere);
    if(Row("F3 / Blend opacity","Cycle the original presets: 25%, 50%, 75%, 100%.")){
        ImGui::Text("Current: %.0f%%",Hud.Intensity*100);
        if(ImGui::Button("Next opacity preset",ImVec2(-1,0))){
            Hud.Intensity=Hud.Intensity>=.99f?.25f:FMath::Min(1.f,(FMath::FloorToInt(Hud.Intensity*4)+1)*.25f);
            Changed=true;
        }
        EndRow();
    }
    if(Row("F4 / Scene brightness","Original F4 tool: 85%, 100%, 110%, 115%. The F4 key now opens the system menu.")){
        ImGui::Text("Current: %.0f%%",Hud.Exposure*100);
        if(ImGui::Button("Next brightness preset",ImVec2(-1,0))){
            Hud.Exposure=Hud.Exposure<.99f?1.f:(Hud.Exposure<1.09f?1.10f:(Hud.Exposure<1.14f?1.15f:.85f));
            Changed=true;
        }
        EndRow();
    }
    Toggle("F5 / Background parallax","Offset supported background layers as the camera moves.",Hud.Parallax);
    if(Row("F6 / Gaussian blend mode","Switch between Lighten and Multiply over the scene, excluding the HUD.")){
        Changed|=ImGui::Combo("##blend",&Hud.BlendMode,"Lighten\0Multiply\0");EndRow();
    }
    Toggle("F7 / Background depth","Enable the layered 2.5D scenery presentation.",Hud.Depth);
    Toggle("F8 / Surface relief","Enable illuminated and shaded edges on scenery.",Hud.Relief);
    if(Row("Ctrl+F8 / Reload painted scenery","Reload the room decorations saved by the external editor. Collisions remain unchanged.")){
        if(ImGui::Button("Reload decorations",ImVec2(-1,0))){
            const int Count=Hud.LoadRoomDecorations();Hud.RefreshScenePresentation();
            Status=FString::Printf(TEXT("Reloaded painted scenery: %d tiles."),Count);
        }
        EndRow();
    }
    Toggle("F9 / Engine weather","Enable the Unreal rain, fog, underwater and heat effects.",Hud.EngineWeather);
    if(Row("F10 / Teleportation","Open the existing destination selector. Available during gameplay, outside item messages and transitions.")){
        const bool Available=Hud.Ready && Hud.State()==8 && !Hud.MessageActive();
        ImGui::BeginDisabled(!Available);
        if(ImGui::Button("Open teleport destinations",ImVec2(-1,0))){
            SetOpen(false);Hud.TeleportMenu=true;Hud.AudioComponent->SetPaused(true);
        }
        ImGui::EndDisabled();
        if(!Available)Description("Resume gameplay to enable teleportation.");
        EndRow();
    }
    if(Row("F11 / Fullscreen","Toggle windowed or borderless fullscreen and remember the choice.")){
        if(ImGui::Button("Toggle fullscreen",ImVec2(-1,0)))ToggleFullscreen();
        EndRow();
    }
    Toggle("Ctrl+F11 / Widescreen","Switch between the wide view and the native game width.",Hud.Widescreen);
    Toggle("F12 / Assisted wall jump","Enabled: forgiving wall jumps. Disabled: original timing.",Hud.AssistedWallJump);
    Toggle("Shift+F12 / Assisted Space Jump","Enabled: forgiving repeated jumps. Disabled: original rhythm. Ctrl+F12 is also supported in gameplay.",Hud.AssistedSpaceJump);
    ImGui::EndDisabled();
    if(Changed)ApplySettings();
}
void FSMSystemMenu::DrawSystem(){
    Heading("SYSTEM","Session controls and local achievements.");
    auto RunState=reinterpret_cast<decltype(&sm_run_state)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_run_state")));
    auto RunTime=reinterpret_cast<decltype(&sm_run_time)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_run_time")));
    if(RunState && RunTime && RunState(0)){
        const uint64 Frames=RunTime(0),Seconds=RunTime(1)/1000;
        ImGui::Text("SPEEDRUN  /  %s  /  %s",RunState(1)?"NEW GAME+":"VANILLA",RunState(0)==1?"NO QOL":"QOL");
        ImGui::Text("In-game: %02llu:%02llu:%02llu   Real: %02llu:%02llu:%02llu",Frames/216000,Frames/3600%60,Frames/60%60,Seconds/3600,Seconds/60%60,Seconds%60);
        if(!RunState(2))Description("Unranked: this run has incomplete history or used a debug action.");
    }
    if(ImGui::Button("Resume game"))SetOpen(false);
    if(ImGui::Button("Reset to title"))Request(EAction::Reset);
    ImGui::BeginDisabled(Generation.IsValid());if(ImGui::Button("Exit game"))Request(EAction::Quit);ImGui::EndDisabled();
    ImGui::Spacing();Description("Reset and Exit preserve SRAM. Progress since the last save station is not saved.");
    if(ImGui::Button("Achievements"))Section=6;
    auto RouteState=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_route_state")));
    ImGui::BeginDisabled(!RouteState || RouteState(0)<1);
    if(ImGui::Button("Run recap")){SetOpen(false);Hud.OpenRunRecap();}
    ImGui::EndDisabled();
    Description("Replay your recorded path on the original area maps. Visits and backtracking stay in chronological order.");
}
void FSMSystemMenu::DrawAchievements(){
    Heading("ACHIEVEMENTS","Your discoveries and milestones across this save bank.");
    const char* Names[]={"A NEW DISCOVERY","HUNTER","POWER UNLEASHED","REACH FURTHER","CHARGED ARMOR"};
    const char* Descriptions[]={"Acquire a new equipment or beam upgrade.","Defeat 10 enemies.","Detonate a Power Bomb.","Use the Grapple Beam.","Use the Screw Attack."};
    int Count=0;for(int I=0;I<5;I++)if(Hud.AchievementBits&(1<<I))Count++;
    ImGui::Text("%d / 5 unlocked",Count);ImGui::ProgressBar(Count/5.f,ImVec2(-1,0));ImGui::Spacing();
    for(int I=0;I<5;I++) {
        const bool Unlocked=(Hud.AchievementBits&(1<<I))!=0;
        ImGui::PushID(I);ImGui::BeginChild("milestone",ImVec2(0,ImGui::GetFontSize()*5.5f),ImGuiChildFlags_Borders);
        ImGui::TextColored(Unlocked?Gold:ImVec4(.5f,.55f,.6f,1),"%02d  %s",I+1,Names[I]);
        ImGui::TextWrapped("%s",Descriptions[I]);
        if(I==1 && !Unlocked)ImGui::Text("Progress: %d / 10",FMath::Min(10,Hud.AchievementTotalKills));
        else ImGui::TextDisabled("%s",Unlocked?"UNLOCKED":"LOCKED");
        ImGui::EndChild();ImGui::PopID();
    }
}
void FSMSystemMenu::Draw(){
    // Slate's game DPI curve scales the entire viewport down on small windows.
    // Keep text at the user's requested screen size; layout remains in Slate
    // coordinates so hit testing and the responsive navigation still agree.
    ImGui::GetIO().FontGlobalScale=UiScale/Widget->GetPixelScale();
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("The Zebes Project settings",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored(Gold,"THE ZEBES PROJECT");
    ImGui::TextDisabled("SETTINGS & EXTRAS");
    const float ReturnWidth=ImGui::CalcTextSize("Return to game").x+ImGui::GetStyle().FramePadding.x*2;
    ImGui::SetNextItemWidth(FMath::Max(80.f,ImGui::GetContentRegionAvail().x-ReturnWidth-ImGui::GetStyle().ItemSpacing.x));
    ImGui::InputTextWithHint("##search","Search all settings and saves...",Search,sizeof(Search));
    ImGui::SameLine();if(ImGui::Button("Return to game"))SetOpen(false);
    ImGui::Separator();
    const float Footer=ImGui::GetFontSize()*3.8f;
    ImGui::BeginChild("content",ImVec2(0,-Footer),ImGuiChildFlags_None);
    const char* Sections[]={"Overview","Save library","Randomizer","Settings","Session","Debug tools","Achievements"};
    const int Order[]={0,3,2,1,6,4,5};
    const char* RandomPages[]={"Seed & sharing","Logic & difficulty","Progression","Items & ammo","World & escape","Goals & victory","Gameplay patches","Techniques","Combat & heat"};
    const char* SettingPages[]={"Display","Audio","Controls","Gameplay & comfort","Interface","Atmosphere & effects","Map & trackers"};
    const int SettingOrder[]={0,5,1,2,3,6,4};
    const bool Wide=ImGui::GetContentRegionAvail().x>ImGui::GetFontSize()*42;
    if(Wide){
        ImGui::BeginChild("navigation",ImVec2(ImGui::GetFontSize()*12.5f,0),ImGuiChildFlags_Borders);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(ImGui::GetStyle().ItemSpacing.x,ImGui::GetFontSize()*.15f));
        for(int I:Order){
            if(I==1 || I==5){ImGui::Spacing();ImGui::Separator();ImGui::Spacing();}
            if(ImGui::Selectable(Sections[I],Section==I && !Search[0],0,ImVec2(0,ImGui::GetFontSize()*1.35f)))SelectSection(I);
            if(I==Section && !Search[0] && (I==2 || I==3)){
                ImGui::Indent(ImGui::GetFontSize()*.55f);
                const int Count=I==2?UE_ARRAY_COUNT(RandomPages):UE_ARRAY_COUNT(SettingPages);
                int& Page=I==2?RandomPage:SettingsPage;
                for(int N=0;N<Count;N++){
                    const int P=I==2?N:SettingOrder[N];
                    if(I==2 && P==7){ImGui::Spacing();ImGui::TextDisabled("ADVANCED");}
                    ImGui::PushID(P);
                    if(ImGui::Selectable(I==2?RandomPages[P]:SettingPages[P],Page==P,0,ImVec2(0,ImGui::GetFontSize()*1.3f)))Page=P;
                    ImGui::PopID();
                }
                ImGui::Unindent(ImGui::GetFontSize()*.55f);ImGui::Spacing();
            }
        }
        ImGui::PopStyleVar();ImGui::EndChild();ImGui::SameLine();
    }else{
        int Selected=Section;ImGui::SetNextItemWidth(-1);
        if(ImGui::Combo("##section",&Selected,Sections,UE_ARRAY_COUNT(Sections)))SelectSection(Selected);
        if(!Search[0] && (Section==2 || Section==3)){
            ImGui::SetNextItemWidth(-1);
            if(Section==2)ImGui::Combo("##category",&RandomPage,RandomPages,UE_ARRAY_COUNT(RandomPages));
            else ImGui::Combo("##category",&SettingsPage,SettingPages,UE_ARRAY_COUNT(SettingPages));
        }
    }
    // Each page keeps its own scroll position. Search is a separate page.
    ImGui::PushID(Search[0]?-1:Section);
    ImGui::PushID(Search[0]?0:Section==2?RandomPage:Section==3?SettingsPage:0);
    ImGui::BeginChild("page",ImVec2(0,0),ImGuiChildFlags_Borders);
    if(Search[0]){
        Heading("SEARCH RESULTS","Matching settings and saved games.");
        ImGui::PushID("settings");DrawSettings();ImGui::PopID();
        ImGui::PushID("randomizer");DrawRandomizer();ImGui::PopID();
        ImGui::PushID("saves");DrawSaves();ImGui::PopID();
        ImGui::PushID("debug");DrawDebug();ImGui::PopID();
    }else switch(Section){case 0:DrawHome();break;case 1:DrawSaves();break;case 2:DrawRandomizer();break;case 3:DrawSettings();break;case 4:DrawSystem();break;case 5:DrawDebug();break;case 6:DrawAchievements();break;}
    ImGui::EndChild();ImGui::PopID();ImGui::PopID();ImGui::EndChild();
    ImGui::Separator();
    // Generation diagnostics may include several blocked checks and VARIA's
    // explanation. Keep the entire message readable instead of clipping it
    // below a footer sized for a single status line.
    ImGui::BeginChild("status",ImVec2(0,0),ImGuiChildFlags_None);
    if(!Status.IsEmpty())ImGui::TextWrapped("%s",TCHAR_TO_UTF8(*Status));
    else Description("Tab / arrows to navigate. Enter or A to select. Escape / Back to return. Start belongs to the original pause screen.");
    ImGui::EndChild();
    if(Confirm)ImGui::OpenPopup("Leave this session?");
    if(ImGui::BeginPopupModal("Leave this session?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
        ImGui::TextUnformatted("Progress since your last save station will be lost.");Description("The existing save file and profile will be kept.");
        if(ImGui::Button("Continue")){Execute=Pending;Pending=EAction::None;Confirm=false;ImGui::CloseCurrentPopup();}
        ImGui::SameLine();if(ImGui::Button("Cancel") || !Confirm){Pending=EAction::None;Confirm=false;ImGui::CloseCurrentPopup();}
        ImGui::EndPopup();
    }
    ImGui::End();
#if !UE_BUILD_SHIPPING
    if(MenuPreviewFrame){
        if(++MenuPreviewFrame==45){
            UE_LOG(LogTemp,Display,TEXT("SM_MENU_PREVIEW page=%s section=%d search=%s"),*MenuPreviewName,Section,UTF8_TO_TCHAR(Search));
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/(MenuPreviewName+TEXT("-menu.png")),true,false);
            UE_LOG(LogTemp,Display,TEXT("SM_REFILL_MENU configured=%d native=%d"),RefillBeforeSave,reinterpret_cast<int(*)()>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_refill_before_save")))());
        }
        if(MenuPreviewFrame==75)Hud.PlayerOwner->ConsoleCommand(TEXT("quit"));
    }
#endif
}
