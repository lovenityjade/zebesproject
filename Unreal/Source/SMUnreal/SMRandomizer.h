#pragma once
#include "CoreMinimal.h"
#include "../../../Native/sm_indicators.h"
#include "../../../Native/sm_doors.h"
#include "../../../Native/sm_objectives.h"
#include "../../../Native/sm_scavenger.h"
#include "../../../Native/sm_minimizer.h"
#include "../../../Native/sm_escape.h"
#include "../../../Native/sm_animals.h"

struct FSMSeedRequest {
    int32 Seed = 0; // Draft: 0 chooses a number when Generate Game is requested.
    FString Skill = TEXT("casual");
    FString Progression = TEXT("medium");
    TArray<FString> Patches;
    // Value-owned JSON objects prevent shared mutable drafts between A/B/C.
    FString OptionsJson=TEXT("{}"),TechniquesJson=TEXT("{}"),SkillSettingsJson=TEXT("{}");
    bool NoAdvancedTechs = false, RelicHunt = false;
    int32 RelicsPlaced = 30, RelicsRequired = 20, RelicEscapeMinutes = 5;
};
struct FSMSeedItem {
    FString Location, Item, Visibility;
    int32 Address = 0, Plm = 0, Kind = 0;
};
struct FSMSettingsIssue {
    FString Message;
    int32 Page=0, OtherPage=-1;
    FString MessageKey;
    TArray<FString> MessageArgs;
};
struct FSMSettingsValidation {
    bool Ok=false;
    TArray<FSMSettingsIssue> Issues;
    TArray<FString> Notes;
};
struct FSMSeedPlan {
    FString Json, Fingerprint, TrackerJson;
    int32 Seed = 0, RelicsRequired = 0, RelicEscapeMinutes = 5;
    bool NoAdvancedTechs = false;
    bool RefillBeforeSave = false;
    uint32 NativeRules=0;
    TArray<FSMSeedItem> Items;
    // Native address order, empty for historical plans (their original all-check HUD).
    TArray<uint8> HudCounts;
    int32 StartSpawn=0;
    FString WorldCatalog, ConnectionsCatalog;
    FString AreasCatalog;
    TArray<uint8> AreaDestinations, InitialDoors;
    TArray<uint8> BossDestinations, DoorColors;
    FString DoorColorsCatalog;
    FString ObjectivesCatalog;
    SmObjectivePlan Objectives{};
    FString MinimizerCatalog;
    SmMinimizerPlan Minimizer{};
    int32 AnimalsMode=0;
    FString AnimalsCatalog;
    FString EscapeCatalog;
    SmEscapeClockPlan EscapeClock{};
    SmEscapeRoutingPlan EscapeRouting{};
    FString ScavengerCatalog;
    SmScavengerPlan Scavenger{};
    TArray<uint8> WorldPatches;
    TArray<SmDoorIndicator> DoorIndicators;
    TArray<FString> PendingPatches, RequiredNativeBehavior;
};
/* Compiled generation service. FSMProfiles stores immutable plans; the system
 * menu activates them only after the native core has shut down. */
class FSMRandomizer {
public:
    static FString RequestJson(const FSMSeedRequest& Request);
    static FSMSettingsValidation ValidateSettings(const FString& Root,const FSMSeedRequest& Request);
    static bool Generate(const FString& ProjectRoot, const FSMSeedRequest& Request,
                         FSMSeedPlan& Out, FString& Error);
    static bool ReadPlan(const FString& Json, FSMSeedPlan& Out, FString& Error);
    static bool Stage(const FSMSeedPlan& Plan, const FString& Directory, FString& Error);
};
