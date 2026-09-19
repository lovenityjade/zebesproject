#pragma once
#include "CoreMinimal.h"
#include "../../../Native/sm_soundtrack.h"
#include "Async/Future.h"
#include "SMProfiles.h"
#include "../../../Native/sm_save_refill.h"
#include "../../../Native/sm_travel.h"
#include "SMTracker.h"
#include "../../../Native/sm_runs.h"
#include "../../../Native/sm_seed_rules.h"
#include "../../../Native/sm_varia_ui.h"
#include "../../../Native/sm_credits.h"
#include "../../../Native/sm_ending.h"
#include "../../../Native/sm_relic.h"
#include "InputCoreTypes.h"
#include "../../../Native/sm_generation.h"
#include "../../../Native/sm_start.h"
#include "../../../Native/sm_world_data.h"
#include "../../../Native/sm_connections.h"
#include "../../../Native/sm_areas.h"
class ASMHUD;
class SSMImGuiWidget;
struct FSMGenerationResult { FSMSeedPlan Plan; FString Error; bool Ok=false; };
struct FSMSystemMenu {
    explicit FSMSystemMenu(ASMHUD& Owner):Hud(Owner){}
    ~FSMSystemMenu();
    void Initialize(bool ShowAtBoot);
    void SetFullscreen(bool Fullscreen);
    void ToggleFullscreen();
    void ApplyRenderQuality();
    void SetOpen(bool Value);
    void ShowAchievements(int Category=1){AchievementCategory=FMath::Clamp(Category,0,2);Section=6;SetOpen(true);}
    bool AchievementIconsReady() const;
    void Tick();
    void Draw();
    void Back();
    void Persist(class FConfigFile& Settings) const;
    uint16 Buttons() const;
    bool IsOpen() const {return Open;}
    void ConfigureNativeGeneration();
private:
    ASMHUD& Hud;
    TSharedPtr<SSMImGuiWidget> Widget;
    bool Open=false, Mute=false, RowUsesTable=false, Remastered=false;
    decltype(&sm_soundtrack_configure) ConfigureSoundtrack=nullptr;
    decltype(&sm_soundtrack_status) SoundtrackStatus=nullptr;
    decltype(&sm_soundtrack_error) SoundtrackError=nullptr;
    float Volume=1.f, Deadzone=.3f, UiScale=1.f;
    FKey Keys[14],Pads[14];
    int Rebind=-1;bool RebindPad=false;
    int Section=0,SettingsPage=0,RandomPage=0;
    char Search[128]={}, ProfileName[128]={};
    int Seed=0,Skill=0,Progression=1;
    char SeedNumber[16]={};
    TArray<char> SettingsString;
    TArray<FString> SeedPatches;
    FString SeedOptions=TEXT("{}"),SeedTechniques=TEXT("{}"),SeedSkillSettings=TEXT("{}");
    char PresetPath[1024]={};
    int TechniquePage=0;
    bool NoAdvancedTechs=false,RelicHunt=false;
    bool RefillBeforeSave=true, SaveStationTravel=false;
    int MenuPreviewFrame=0;
    FString MenuPreviewName;
    decltype(&sm_set_refill_before_save) SetRefillBeforeSave=nullptr;
    decltype(&sm_travel_configure) ConfigureTravel=nullptr;
    decltype(&sm_start_configure) ConfigureStart=nullptr;
    decltype(&sm_start_configure_world) ConfigureWorld=nullptr;
    decltype(&sm_world_data_catalog_sha256) WorldCatalog=nullptr;
    decltype(&sm_doors_configure) ConfigureDoorColors=nullptr;
    decltype(&sm_doors_catalog_sha256) DoorColorsCatalog=nullptr;
    decltype(&sm_connections_configure) ConfigureConnections=nullptr;
    decltype(&sm_areas_configure) ConfigureAreas=nullptr;
    decltype(&sm_areas_catalog_sha256) AreasCatalog=nullptr;
    decltype(&sm_start_configure_initial_doors) ConfigureInitialDoors=nullptr;
    decltype(&sm_minimizer_configure) ConfigureMinimizer=nullptr;
    decltype(&sm_minimizer_catalog_sha256) MinimizerCatalog=nullptr;
    decltype(&sm_escape_clock_configure) ConfigureEscapeClock=nullptr;
    decltype(&sm_escape_routing_configure) ConfigureEscapeRouting=nullptr;
    decltype(&sm_escape_catalog_sha256) EscapeCatalog=nullptr;
    decltype(&sm_animals_configure) ConfigureAnimals=nullptr;
    bool ConfigureAnimalsSlot(int Index,const FSMSeedPlan& Plan);
    bool ConfigureEscapeSlot(int Index,const FSMSeedPlan& Plan);
    decltype(&sm_scavenger_configure) ConfigureScavenger=nullptr;
    decltype(&sm_scavenger_catalog_sha256) ScavengerCatalog=nullptr;
    decltype(&sm_objectives_configure) ConfigureObjectives=nullptr;
    decltype(&sm_objectives_catalog_sha256) ObjectivesCatalog=nullptr;
    decltype(&sm_connections_catalog_sha256) ConnectionsCatalog=nullptr;
    decltype(&sm_seed_rules_configure) SetSeedRules=nullptr;
    decltype(&sm_seed_rules_capabilities) SeedRuleCapabilities=nullptr;
    int RelicsPlaced=30,RelicsRequired=20,RelicEscapeMinutes=5;
    decltype(&sm_relic_configure) ConfigureRelic=nullptr;
    TFuture<FSMGenerationResult> Generation;
    TFuture<FSMSettingsValidation> SettingsCheck;
    FSMSettingsValidation SettingsValidation;
    FString SettingsCheckWanted, SettingsCheckRunning, SettingsCheckDone;
    double SettingsChangedAt=0;
    bool SettingsCheckInitialized=false;
    void UpdateSettingsCheck();
    bool SettingsCompatible() const;
    decltype(&sm_generation_configure) ConfigureGeneration=nullptr;
    decltype(&sm_generation_take_request) TakeGenerationRequest=nullptr;
    decltype(&sm_generation_fail) FailGeneration=nullptr;
    decltype(&sm_generation_commit) CommitGeneration=nullptr;
    decltype(&sm_slots_enable) EnableSlots=nullptr;
    decltype(&sm_slots_set) SetSlot=nullptr;
    decltype(&sm_slots_current) CurrentSlot=nullptr;
    decltype(&sm_slots_editable) EditableSlot=nullptr;
    decltype(&sm_slots_copy_sram) CopySlotSram=nullptr;
    FSMSeedPlan CommittingPlan;
    int GeneratingSlot=-1,EditingSlot=-1;
    bool OpenSlotSettings=false;
    bool UpdateNativeSlot(int Slot);
    static int SlotAction(int Action,int Slot,int Other,void* Context);
    FString GeneratingProfileId;
    FSMSeedRequest NextRequest() const;
    FString Status;
    TArray<FSMGameProfile> Profiles;
    TArray<FString> ProfileWarnings;
    FSMGameProfile Active, PendingProfile;
    FSMTracker Tracker;
    bool VanillaTracker=false,MapTracker=true,ItemTracker=true;
    int VanillaTrackerSkill=0;
    bool VariaAmmo=true,VariaHud=true,VariaReserves=true,VariaMarkers=true;
    decltype(&sm_ending_preview_launch) LaunchEnding=nullptr;
    decltype(&sm_ending_preview_close) CloseEnding=nullptr;
    decltype(&sm_ending_preview_active) EndingPreview=nullptr;
    int EndingAnimals=0;
    decltype(&sm_credits_launch) LaunchCredits=nullptr;
    decltype(&sm_credits_close) CloseCredits=nullptr;
    decltype(&sm_credits_state) CreditsState=nullptr;
    decltype(&sm_varia_ui_configure) ConfigureVariaUi=nullptr;
    decltype(&sm_varia_ui_counted_configure) ConfigureVariaCounts=nullptr;
    int VanillaRunCategory=0,NgSourceSlot=-1;
    FString NgSourcePath;
    enum class EAction {None,Vanilla,NewGamePlus,Randomized,Load,Reset,Quit};
    EAction Pending=EAction::None, Execute=EAction::None;
    bool Confirm=false;
    void Defaults();
    void RefreshProfiles();
    void ApplySettings();
    void Request(EAction Action);
    void Perform(EAction Action);
    bool Activate(const FSMGameProfile& Profile);
    bool Bind(FKey Key);
    void SelectSection(int Value);
    bool SaveSeedDraft();
#if !UE_BUILD_SHIPPING
    bool RunMenuFlowTest();
#endif
    void DrawHome();
    void DrawSaves();
    void DrawRandomizer();
    bool DrawSeedCatalog(bool Busy);
    void LoadSeedRequest(const FSMSeedRequest& Request);
    void DrawSettings();
    void DrawSystem();
    void DrawDebug();
    int AchievementCategory=1;
    void DrawAchievements();
    bool Row(const char* Label,const char* Description);
    void EndRow();
};
