#include "SMRandomizer.h"
#include "SMNativeLibrary.h"
#include "SMSeedSettings.h"
#include "Algo/Find.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"

#include "SMNativeObjectives.inl"
#include "SMNativeScavenger.inl"
#include "SMNativeMinimizer.inl"
#include "SMNativeEscape.inl"
#include "SMReadEscape.inl"

static bool ReadObjectiveContract(const TSharedPtr<FJsonObject>& Data,bool AreaLayout,FSMSeedPlan& Out,FString& Error){
    auto Fail=[&](const TCHAR* Message){Error=Message;return false;};
    auto Number=[&](const TCHAR* Key,uint16& Dest,int32 Low,int32 High){double N=0;
        if(!Data->TryGetNumberField(Key,N) || N<Low || N>High || N!=FMath::FloorToDouble(N))return false;
        Dest=uint16(N);return true;
    };
    double Schema=0;bool Layout=false;
    if(!Data->TryGetNumberField(TEXT("schema"),Schema) || (Schema!=1 && Schema!=2 && Schema!=3 && Schema!=4 && Schema!=5) ||
       !Data->TryGetStringField(TEXT("catalogSha256"),Out.ObjectivesCatalog) || Out.ObjectivesCatalog!=NativeObjectiveCatalog ||
       !Data->TryGetBoolField(TEXT("areaLayout"),Layout) || Layout!=AreaLayout)
        return Fail(TEXT("Unsupported native objective catalog or world layout"));
    auto& Plan=Out.Objectives;Plan.version=uint32(Schema);Plan.size=sizeof(Plan);
    const TArray<TSharedPtr<FJsonValue>> *Goals=nullptr,*Names=nullptr;
    if(!Data->TryGetArrayField(TEXT("goals"),Goals) || Goals->Num()<1 || Goals->Num()>18 ||
       !Data->TryGetArrayField(TEXT("names"),Names) || Names->Num()!=Goals->Num())return Fail(TEXT("Invalid native objective list"));
    Plan.count=Goals->Num();TSet<int32> Seen;
    for(int32 I=0;I<Goals->Num();I++){double Id=0;FString Name;
        if(!(*Goals)[I]->TryGetNumber(Id) || Id<0 || Id>=UE_ARRAY_COUNT(NativeObjectiveSpecs) || Id!=FMath::FloorToDouble(Id) ||
           (!NativeObjectiveSpecs[int32(Id)].Supported && !(Id==16 && Schema>=2 && Out.Scavenger.count)) || Seen.Contains(int32(Id)) ||
           !(*Names)[I]->TryGetString(Name) || Name!=NativeObjectiveSpecs[int32(Id)].Name)return Fail(TEXT("Native objective identities disagree"));
        Seen.Add(int32(Id));Plan.goals[I]=uint8(Id);
    }
    if((Schema<3 && (Schema==2)!=Seen.Contains(16)) || Seen.Contains(16)!=bool(Out.Scavenger.count))return Fail(TEXT("Missing Scavenger objective dependency"));
    if(!Number(TEXT("required"),Plan.required,1,Plan.count) || !Number(TEXT("flags"),Plan.flags,0,Schema==5?31:Schema>=3?15:7) ||
       !Number(TEXT("itemMask"),Plan.item_mask,0,65535) || (Plan.item_mask&~0xf32f) ||
       !Number(TEXT("beamMask"),Plan.beam_mask,0,65535) || (Plan.beam_mask&~0x100f))return Fail(TEXT("Invalid objective quota, flags or equipment mask"));
    FString Tourian;
    if((Schema<4 && (Schema==3)!=bool(Plan.flags&SM_OBJECTIVE_FAST_TOURIAN)) || (Schema>=3 && (Plan.flags&SM_OBJECTIVE_FAST_TOURIAN) && (!Data->TryGetStringField(TEXT("tourian"),Tourian) || Tourian!=TEXT("Fast"))))return Fail(TEXT("Missing Fast Tourian dependency"));
    if(Schema<5 && (Schema==4)!=bool(Out.Minimizer.version))return Fail(TEXT("Missing Minimizer objective dependency"));
    if((Schema==5)!=bool(Out.EscapeClock.version) || (Schema==5 && (bool(Plan.flags&SM_OBJECTIVE_DISABLED_TOURIAN)!=bool(Out.EscapeClock.flags&1) || (Plan.flags&24)==24)))return Fail(TEXT("Missing escape objective dependency"));
    if(Plan.flags&SM_OBJECTIVE_BT_SLEEP)for(int32 I=0;I<Plan.count;I++)
        if(FString(NativeObjectiveSpecs[Plan.goals[I]].Name)==TEXT("activate chozo robots"))return Fail(TEXT("Bomb Torizo cannot sleep with the Chozo robots objective"));
    for(const auto& Key:{TEXT("itemCounted"),TEXT("areaCounted")}){
        const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
        if(!Data->TryGetArrayField(Key,Values) || Values->Num()!=100)return Fail(TEXT("Missing objective item membership"));
        const bool Area=FCString::Strcmp(Key,TEXT("areaCounted"))==0;
        for(int32 I=0;I<100;I++){double Value=0;
            if(!(*Values)[I]->TryGetNumber(Value) || (Value!=0 && Value!=1))return Fail(TEXT("Invalid objective item membership"));
            if(Out.Minimizer.version && Value && !Out.Minimizer.checks[I])return Fail(TEXT("Objective includes an excluded check"));
            (Area?Plan.area_counted:Plan.item_counted)[I]=uint8(Value);
            if(Area && (Out.HudCounts.Num()!=100 || Out.HudCounts[I]!=Value))return Fail(TEXT("Objective region counts disagree with the HUD"));
        }
    }
    static const uint16 EnemyTotals[]={62,19,22,20,5,17};
    for(const auto& Key:{TEXT("enemyTotals"),TEXT("mapTotals")}){
        const bool Map=FCString::Strcmp(Key,TEXT("mapTotals"))==0;const int32 Count=Map?12:6;
        const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
        if(!Data->TryGetArrayField(Key,Values) || Values->Num()!=Count)return Fail(TEXT("Incomplete objective world totals"));
        for(int32 I=0;I<Count;I++){double Value=0;uint16 Expected=Map?(NativeObjectiveMapTotals[AreaLayout?1:0][I]-((Plan.flags&SM_OBJECTIVE_FAST_TOURIAN) && !AreaLayout && I==1?1:0)):EnemyTotals[I];
            if(Out.Minimizer.version){
                Expected=0;
                if(Map)Expected=(Out.Minimizer.regions&(1u<<I))?NativeObjectiveMapTotals[1][I]:NativeMinimizerBossTiles[I];
                else for(int32 R=1;R<=10;R++)if(Out.Minimizer.regions&(1u<<R))Expected+=NativeMinimizerEnemies[I][R];
            }
            if(Map && Schema==5 && (!Out.Minimizer.version || (Out.Minimizer.regions&(1u<<I)))){
                static const uint16 Offsets[]={0,0,4,0,1,0,1,0,0,1,0,0};Expected-=Offsets[I];
            }
            if(!(*Values)[I]->TryGetNumber(Value) || Value!=Expected)return Fail(TEXT("Unsupported filtered objective world"));
            (Map?Plan.map_totals:Plan.enemy_totals)[I]=uint16(Value);
        }
    }
    return true;
}

FString FSMRandomizer::RequestJson(const FSMSeedRequest& Request) {
    TSharedRef<FJsonObject> Input=MakeShared<FJsonObject>();
    Input->SetNumberField(TEXT("seed"),Request.Seed);
    Input->SetStringField(TEXT("skill"),Request.Skill);
    Input->SetStringField(TEXT("progression"),Request.Progression);
    TArray<TSharedPtr<FJsonValue>> Options;
    for(const FString& Option:Request.Patches) Options.Add(MakeShared<FJsonValueString>(Option));
    Input->SetArrayField(TEXT("patches"),Options);
    Input->SetBoolField(TEXT("noAdvancedTechs"),Request.NoAdvancedTechs);
    auto Relic=MakeShared<FJsonObject>();Relic->SetBoolField(TEXT("enabled"),Request.RelicHunt);
    Relic->SetNumberField(TEXT("placed"),Request.RelicsPlaced);Relic->SetNumberField(TEXT("required"),Request.RelicsRequired);Relic->SetNumberField(TEXT("escapeMinutes"),Request.RelicEscapeMinutes);
    Input->SetObjectField(TEXT("relicHunt"),Relic);
    if(!SMSeedSettings::ValidBasics(Request) || !SMSeedSettings::Write(Request,Input)){return FString();}
    FString RequestJson;
    FJsonSerializer::Serialize(Input,TJsonWriterFactory<>::Create(&RequestJson));
    return RequestJson;
}
FSMSettingsValidation FSMRandomizer::ValidateSettings(const FString& Root,const FSMSeedRequest& Request) {
    FSMSettingsValidation Result;FString Error;
    auto Fail=[&](const FString& Message){Result.Ok=false;Result.Issues.Add({Message,0,-1});return Result;};
    if(!SMSeedSettings::ValidateDraft(Request,Error))return Fail(Error);
    const FString Json=RequestJson(Request);
    void* Handle=SMNativeLibrary::Open(Root);
    if(!Handle)return Fail(TEXT("Native settings validator unavailable. Rebuild the native module."));
    using ValidateFn=int(*)(const char*,const char*,char*,int);
    auto Validate=reinterpret_cast<ValidateFn>(FPlatformProcess::GetDllExport(Handle,TEXT("sm_randomizer_validate")));
    TArray<char> Buffer;Buffer.SetNumZeroed(262144);
    const int Size=Validate?Validate(TCHAR_TO_UTF8(*(Root/TEXT("Randomizer"))),TCHAR_TO_UTF8(*Json),Buffer.GetData(),Buffer.Num()):0;
    FPlatformProcess::FreeDllHandle(Handle);
    if(Size<=0 || Size>Buffer.Num())return Fail(TEXT("Settings check unavailable. Update the native module before creating a game."));
    auto Envelope=SMSeedSettings::Parse(UTF8_TO_TCHAR(Buffer.GetData()));
    const TArray<TSharedPtr<FJsonValue>>* Issues=nullptr;
    if(!Envelope || !Envelope->TryGetBoolField(TEXT("ok"),Result.Ok) || !Envelope->TryGetArrayField(TEXT("issues"),Issues))return Fail(TEXT("Invalid settings-check response."));
    for(const auto& Value:*Issues){
        auto I=Value->AsObject();FSMSettingsIssue Issue;double Other=-1;
        if(!I || !I->TryGetStringField(TEXT("message"),Issue.Message))return Fail(TEXT("Invalid settings-check issue."));
        Issue.Page=FMath::Clamp(I->GetIntegerField(TEXT("page")),0,8);
        if(I->TryGetNumberField(TEXT("otherPage"),Other))Issue.OtherPage=FMath::Clamp(int32(Other),0,8);
        Result.Issues.Add(Issue);
    }
    Envelope->TryGetStringArrayField(TEXT("notes"),Result.Notes);
    Result.Ok=Result.Ok && Result.Issues.IsEmpty();
    return Result;
}
bool FSMRandomizer::Generate(const FString& Root,const FSMSeedRequest& Draft,FSMSeedPlan& Out,FString& Error) {
    FSMSeedRequest Request=Draft;if(Request.Seed==0)Request.Seed=SMSeedSettings::RandomSeed();
    Out=FSMSeedPlan(); Error.Reset();
    const FString RequestJson=FSMRandomizer::RequestJson(Request);
    if(RequestJson.IsEmpty()){Error=TEXT("Invalid seed settings");return false;}
    void* Handle=SMNativeLibrary::Open(Root);
    if(!Handle) { Error=TEXT("Native randomizer module unavailable"); return false; }
    using GenerateFn=int(*)(const char*,const char*,char*,int);
    GenerateFn Generate=(GenerateFn)FPlatformProcess::GetDllExport(Handle,TEXT("sm_randomizer_generate"));
    TArray<char> Buffer; Buffer.SetNumZeroed(2097152);
    const int Size=Generate?Generate(TCHAR_TO_UTF8(*(Root/TEXT("Randomizer"))),TCHAR_TO_UTF8(*RequestJson),Buffer.GetData(),Buffer.Num()):0;
    FPlatformProcess::FreeDllHandle(Handle);
    if(Size<=0 || Size>Buffer.Num()) { Error=TEXT("Randomizer runtime failed; see native log"); return false; }
    return ReadPlan(UTF8_TO_TCHAR(Buffer.GetData()),Out,Error);
}
bool FSMRandomizer::ReadPlan(const FString& Json,FSMSeedPlan& Out,FString& Error) {
    Out=FSMSeedPlan(); Error.Reset();
    TSharedPtr<FJsonObject> Envelope;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Envelope) || !Envelope.IsValid()) {
        Error=TEXT("Invalid seed JSON"); return false;
    }
    bool Ok=false;
    if(!Envelope->TryGetBoolField(TEXT("ok"),Ok) || !Ok) {
        if(!Envelope->TryGetStringField(TEXT("error"),Error)) Error=TEXT("Generation failed");
        return false;
    }
    const TSharedPtr<FJsonObject>* Manifest;
    if(!Envelope->TryGetObjectField(TEXT("manifest"),Manifest)) {Error=TEXT("Missing manifest");return false;}
    double Schema=0,Seed=0;
    const TArray<TSharedPtr<FJsonValue>>* Items;
    if(!(*Manifest)->TryGetNumberField(TEXT("schema"),Schema) || Schema!=1 ||
       !(*Manifest)->TryGetNumberField(TEXT("seed"),Seed) || Seed<1 || Seed>2147483647 || Seed!=FMath::FloorToDouble(Seed) ||
       !(*Manifest)->TryGetArrayField(TEXT("placements"),Items) || Items->Num()!=100 ||
       !(*Manifest)->TryGetStringField(TEXT("sha256"),Out.Fingerprint) || Out.Fingerprint.Len()!=64) {
        Error=TEXT("Unsupported or incomplete seed manifest"); return false;
    }
    for(TCHAR C:Out.Fingerprint) if(!FChar::IsHexDigit(C)) {Error=TEXT("Invalid seed fingerprint");return false;}
    TArray<FString> RequiredBehaviors;(*Manifest)->TryGetStringArrayField(TEXT("requiredNativeBehavior"),RequiredBehaviors);
    if(RequiredBehaviors.Contains(TEXT("hud-counts-v1")))Out.HudCounts.Init(0,100);
    TSet<int32> Addresses;
    for(const TSharedPtr<FJsonValue>& Value:*Items) {
        const TSharedPtr<FJsonObject>* Item;
        FSMSeedItem Entry; double Address=0,Plm=0;
        if(!Value->TryGetObject(Item) ||
           !(*Item)->TryGetNumberField(TEXT("address"),Address) || Address!=FMath::FloorToDouble(Address) ||
           !(*Item)->TryGetNumberField(TEXT("plm"),Plm) || Plm!=FMath::FloorToDouble(Plm) ||
           !(*Item)->TryGetStringField(TEXT("location"),Entry.Location) ||
           !(*Item)->TryGetStringField(TEXT("item"),Entry.Item) ||
           !(*Item)->TryGetStringField(TEXT("visibility"),Entry.Visibility)) {Error=TEXT("Invalid placement");return false;}
        if(Address<0 || Address>=3145727 || Plm<0 || Plm>65535) {Error=TEXT("Placement out of bounds");return false;}
        Entry.Address=(int32)Address;Entry.Plm=(int32)Plm;
        double Kind=0;(*Item)->TryGetNumberField(TEXT("kind"),Kind);
        if(Kind<0 || Kind>2 || Kind!=FMath::FloorToDouble(Kind) || (Kind && (Entry.Plm-0xeed7)%84!=80) ||
           (Kind==1 && Entry.Item!=TEXT("ChozoRelic")) || (Kind==2 && Entry.Item!=TEXT("Nothing") && Entry.Item!=TEXT("NoEnergy")) ||
           (Kind==0 && (Entry.Item==TEXT("ChozoRelic") || Entry.Item==TEXT("Nothing") || Entry.Item==TEXT("NoEnergy")))) {Error=TEXT("Invalid native pickup kind");return false;}
        Entry.Kind=int32(Kind);
        static const int32 Allowed[] = {
#include "SMItemAddresses.inl"
        };
        bool AllowedAddress=false;
        for(int32 A:Allowed) if(A==Entry.Address) {AllowedAddress=true;break;}
        if(!AllowedAddress || Addresses.Contains(Entry.Address) || Entry.Plm<0xeed7 || Entry.Plm>0xefcf || (Entry.Plm-0xeed7)%4) {
            Error=TEXT("Placement outside native item tables");return false;
        }
        if(!Out.HudCounts.IsEmpty()){
            bool Counted=false;
            if(!(*Item)->TryGetBoolField(TEXT("hudCounted"),Counted) || (Entry.Kind==2 && Counted) || (Entry.Kind==1 && !Counted)){
                Error=TEXT("Missing or inconsistent HUD count contract");return false;
            }
            for(int32 I=0;I<100;I++)if(Allowed[I]==Entry.Address)Out.HudCounts[I]=Counted;
        }
        Addresses.Add(Entry.Address);Out.Items.Add(MoveTemp(Entry));
    }
    const TSharedPtr<FJsonObject>* Rules;
    int32 RelicCount=0;for(const auto& I:Out.Items)RelicCount+=I.Kind==1;
    if((*Manifest)->TryGetObjectField(TEXT("rules"),Rules)) {
        const TSharedPtr<FJsonObject>* Options=nullptr;FString Refill;
        if((*Rules)->TryGetObjectField(TEXT("options"),Options)){
            if((*Options)->TryGetStringField(TEXT("refill_before_save"),Refill))Out.RefillBeforeSave=Refill==TEXT("on");
            FString Value;
            if(RequiredBehaviors.Contains(TEXT("seed-interface-v1"))){
                if((*Options)->TryGetStringField(TEXT("revealMap"),Value) && Value==TEXT("off"))Out.NativeRules|=4;
                if((*Options)->TryGetStringField(TEXT("hud"),Value) && Value==TEXT("off"))Out.NativeRules|=8;
                if((*Options)->TryGetStringField(TEXT("better_reserves"),Value) && Value==TEXT("off"))Out.NativeRules|=16;
            }
            if(RequiredBehaviors.Contains(TEXT("fast-elevators-v1")) && (*Options)->TryGetStringField(TEXT("elevators_speed"),Value) && Value==TEXT("on"))Out.NativeRules|=32;
            // Suit behavior belongs to the effective seed settings. Reading
            // old Vanilla manifests keeps their original protections and ID.
            if((*Options)->TryGetStringField(TEXT("gravityBehaviour"),Value)){
                if(Value==TEXT("Balanced"))Out.NativeRules|=512;
                else if(Value==TEXT("Progressive"))Out.NativeRules|=1024;
                else if(Value!=TEXT("Vanilla")){Error=TEXT("Invalid suit behavior");return false;}
            }else if(RequiredBehaviors.Contains(TEXT("native-suits-v1"))){Error=TEXT("Missing suit behavior");return false;}
            struct FGameplayRule {const TCHAR* Option;const TCHAR* Behavior;uint32 Flag;};
            static const FGameplayRule GameplayRules[]={
                {TEXT("spinjumprestart"),TEXT("respin-v1"),2048},
                {TEXT("Infinite_Space_Jump"),TEXT("infinite-spacejump-v1"),4096},
                {TEXT("itemsounds"),TEXT("item-sounds-v1"),8192},
                {TEXT("relaxed_round_robin_cf"),TEXT("round-robin-cf-v1"),64},
                {TEXT("rando_speed"),TEXT("momentum-landing-v1"),128},
                {TEXT("nerfedCharge"),TEXT("nerfed-charge-v1"),256}};
            for(const auto& Rule:GameplayRules){
                const bool Enabled=(*Options)->TryGetStringField(Rule.Option,Value) && Value==TEXT("on");
                if(Enabled!=RequiredBehaviors.Contains(Rule.Behavior)){Error=TEXT("Gameplay option and native behavior disagree");return false;}
                if(Enabled)Out.NativeRules|=Rule.Flag;
            }
            if((*Options)->TryGetStringField(TEXT("fast_doors"),Value) && Value==TEXT("on"))Out.NativeRules|=1;
            if((*Options)->TryGetStringField(TEXT("energyQty"),Value) && Value==TEXT("ultra sparse"))Out.NativeRules|=2;
        }
        (*Rules)->TryGetBoolField(TEXT("noAdvancedTechs"),Out.NoAdvancedTechs);
        const TSharedPtr<FJsonObject>* Relic;
        if((*Rules)->TryGetObjectField(TEXT("relicHunt"),Relic)) {
            bool Enabled=false;double Placed=0,Required=0;
            if(!(*Relic)->TryGetBoolField(TEXT("enabled"),Enabled) || !(*Relic)->TryGetNumberField(TEXT("placed"),Placed) ||
               !(*Relic)->TryGetNumberField(TEXT("required"),Required) || Required<1 || Required>Placed || Placed>60 ||
               Placed!=FMath::FloorToDouble(Placed) || Required!=FMath::FloorToDouble(Required) || RelicCount!=(Enabled?Placed:0)) {
                Error=TEXT("Relic count and completion quota disagree");return false;
            }
            Out.RelicsRequired=Enabled?int32(Required):0;if(!SMSeedSettings::ReadEscapeMinutes(*Relic,Out.RelicEscapeMinutes)){Error=TEXT("Invalid tablet escape duration");return false;}
        }
    }
    if(RequiredBehaviors.Contains(TEXT("native-door-indicators-v1")) && !RequiredBehaviors.Contains(TEXT("native-start-v1"))){Error=TEXT("Door indicators require the native world contract");return false;}
    const bool InitialDoors=RequiredBehaviors.Contains(TEXT("native-initial-doors-v1"));
    const bool Minimizer=RequiredBehaviors.Contains(TEXT("native-minimizer-v1"));
    const bool AreaConnections=RequiredBehaviors.Contains(TEXT("native-area-connections-v1"));
    if((InitialDoors || AreaConnections || Minimizer) && !RequiredBehaviors.Contains(TEXT("native-start-v1"))){Error=TEXT("Area/initial doors require the native world contract");return false;}
    if(RequiredBehaviors.Contains(TEXT("native-start-v1"))){
        const TSharedPtr<FJsonObject>* Context=nullptr;const TSharedPtr<FJsonObject>* World=nullptr;
        const TArray<TSharedPtr<FJsonValue>>* Patches=nullptr;double Version=0,Spawn=0,ContextSpawn=0;
        if(!(*Manifest)->TryGetObjectField(TEXT("nativeContext"),Context) || !(*Context)->TryGetObjectField(TEXT("world"),World) ||
           !(*World)->TryGetNumberField(TEXT("schema"),Version) || Version!=1 ||
           !(*World)->TryGetStringField(TEXT("catalogSha256"),Out.WorldCatalog) || Out.WorldCatalog.Len()!=64 ||
           !(*World)->TryGetNumberField(TEXT("startSpawn"),Spawn) || Spawn<0 || Spawn>65534 || Spawn!=FMath::FloorToDouble(Spawn) ||
           !(*Context)->TryGetNumberField(TEXT("spawn"),ContextSpawn) || ContextSpawn!=Spawn ||
           !(*World)->TryGetArrayField(TEXT("dataPatchIds"),Patches) || Patches->Num()>64){Error=TEXT("Invalid native start/world contract");return false;}
        for(TCHAR C:Out.WorldCatalog)if(!FChar::IsHexDigit(C)){Error=TEXT("Invalid world catalog fingerprint");return false;}
        Out.StartSpawn=int32(Spawn);TSet<int32> Seen;
        if((*World)->HasField(TEXT("initialDoors"))!=InitialDoors || (*World)->HasField(TEXT("areaPatches"))!=(AreaConnections || Minimizer)){Error=TEXT("World capabilities and door data disagree");return false;}
        if(InitialDoors){
            const TSharedPtr<FJsonObject>* Initial=nullptr;const TArray<TSharedPtr<FJsonValue>>* Doors=nullptr;double InitialSchema=0;
            if(!(*World)->TryGetObjectField(TEXT("initialDoors"),Initial) || !(*Initial)->TryGetNumberField(TEXT("schema"),InitialSchema) || InitialSchema!=1 ||
               !(*Initial)->TryGetArrayField(TEXT("opened"),Doors) || Doors->Num()>16){Error=TEXT("Invalid initial-door contract");return false;}
            #include "SMNativeInitialDoors.inl"
            const auto* Spec=Algo::FindByPredicate(NativeInitialDoorSpecs,[&](const auto& Entry){return Entry.Spawn==Out.StartSpawn;});
            if(!Spec || Doors->Num()!=Spec->Count || (Spec->AreaOnly && !AreaConnections && !Minimizer)){Error=TEXT("Initial doors do not match this start");return false;}
            for(int32 I=0;I<Doors->Num();I++){double Door=0;
                if(!(*Doors)[I]->TryGetNumber(Door) || Door!=Spec->Doors[I]){Error=TEXT("Initial door bits disagree with VARIA");return false;}
                Out.InitialDoors.Add(uint8(Door));
            }
        }
        for(const auto& Value:*Patches){double Id;
            if(!Value->TryGetNumber(Id) || Id<0 || Id>=64 || Id!=FMath::FloorToDouble(Id) || Seen.Contains(int32(Id))){Error=TEXT("Invalid ordered world patches");return false;}
            Seen.Add(int32(Id));Out.WorldPatches.Add(uint8(Id));
        }
        if((*World)->HasField(TEXT("indicators"))!=RequiredBehaviors.Contains(TEXT("native-door-indicators-v1"))){Error=TEXT("Door indicator capability and data disagree");return false;}
        if(RequiredBehaviors.Contains(TEXT("native-door-indicators-v1"))){
            const TArray<TSharedPtr<FJsonValue>>* Indicators=nullptr;
            if(!(*World)->TryGetArrayField(TEXT("indicators"),Indicators) || Indicators->Num()>SM_INDICATOR_MAX){Error=TEXT("Missing native door indicators");return false;}
            TSet<int32> Locations;
            for(const auto& Value:*Indicators){
                const TSharedPtr<FJsonObject>* Entry=nullptr;double Location=0,Plm=0;
                if(!Value->TryGetObject(Entry) || !(*Entry)->TryGetNumberField(TEXT("locationId"),Location) ||
                   !(*Entry)->TryGetNumberField(TEXT("plm"),Plm) || Location<0 || Location>=SM_INDICATOR_MAX || Location!=FMath::FloorToDouble(Location) ||
                   Plm!=FMath::FloorToDouble(Plm) || !((Plm>=0xfbb0 && Plm<=0xfc0a && (int32(Plm)-0xfbb0)%6==0) || (Plm>=0xf60b && Plm<=0xf665 && (int32(Plm)-0xf60b)%6==0)) || Locations.Contains(int32(Location))){Error=TEXT("Invalid native door indicator");return false;}
                Locations.Add(int32(Location));Out.DoorIndicators.Add({uint16(Location),uint16(Plm)});
            }
        }
    }
    const bool BossConnections=RequiredBehaviors.Contains(TEXT("native-boss-connections-v1"));
    const TSharedPtr<FJsonObject>* Context=nullptr;const TSharedPtr<FJsonObject>* Topology=nullptr;
    FString TopologyMode;
    if((*Manifest)->TryGetObjectField(TEXT("nativeContext"),Context) && (*Context)->TryGetObjectField(TEXT("topology"),Topology))(*Topology)->TryGetStringField(TEXT("mode"),TopologyMode);
    if(BossConnections!=(TopologyMode==TEXT("boss") || TopologyMode==TEXT("area-boss")) || AreaConnections!=(TopologyMode==TEXT("area") || TopologyMode==TEXT("area-boss")) ||
       (!TopologyMode.IsEmpty() && TopologyMode!=TEXT("vanilla") && TopologyMode!=TEXT("boss") && TopologyMode!=TEXT("area") && TopologyMode!=TEXT("area-boss") && TopologyMode!=TEXT("minimizer")) || Minimizer!=(TopologyMode==TEXT("minimizer"))){Error=TEXT("Routing capability and topology disagree");return false;}
    if(Topology && ((*Topology)->HasField(TEXT("nativeAreas"))!=AreaConnections || (*Topology)->HasField(TEXT("native"))!=BossConnections || (*Topology)->HasField(TEXT("nativeMinimizer"))!=Minimizer)){Error=TEXT("Unexpected native routing table");return false;}
    if(Minimizer){
        const TSharedPtr<FJsonObject>* Native=nullptr;double MiniSchema=0;
        const TArray<TSharedPtr<FJsonValue>> *Targets=nullptr,*Regions=nullptr,*Checks=nullptr,*Names=nullptr,*Pairs=nullptr,*Empty=nullptr;
        auto FailMini=[&](){Error=TEXT("Invalid native Minimizer routing or membership");return false;};
        if(!InitialDoors || !(*Topology)->TryGetObjectField(TEXT("nativeMinimizer"),Native) ||
           !(*Native)->TryGetNumberField(TEXT("schema"),MiniSchema) || MiniSchema!=1 ||
           !(*Native)->TryGetStringField(TEXT("catalogSha256"),Out.MinimizerCatalog) || Out.MinimizerCatalog!=NativeMinimizerCatalog ||
           !(*Native)->TryGetArrayField(TEXT("destinations"),Targets) || Targets->Num()!=40 ||
           !(*Native)->TryGetArrayField(TEXT("regions"),Regions) || Regions->IsEmpty() ||
           !(*Native)->TryGetArrayField(TEXT("checks"),Checks) || Checks->Num()!=100 ||
           !(*Native)->TryGetArrayField(TEXT("locations"),Names) ||
           !(*Topology)->TryGetArrayField(TEXT("mixedPairs"),Pairs))return FailMini();
        for(const TCHAR* Key:{TEXT("areaPairs"),TEXT("bossPairs")})if(!(*Topology)->TryGetArrayField(Key,Empty) || !Empty->IsEmpty())return FailMini();
        auto& P=Out.Minimizer;P.version=1;P.size=sizeof(P);int32 Previous=0;
        for(const auto& V:*Regions){double R=0;if(!V->TryGetNumber(R) || R<=Previous || R>10 || R!=FMath::FloorToDouble(R))return FailMini();Previous=int32(R);P.regions|=1u<<Previous;}
        for(int32 I=0;I<40;I++){double D=0;if(!(*Targets)[I]->TryGetNumber(D) || D<0 || D>=40 || D!=FMath::FloorToDouble(D))return FailMini();P.destinations[I]=uint8(D);}
        TSet<FString> ExpectedPairs,ActualPairs;
        auto PairKey=[](FString A,FString B){if(A>B)Swap(A,B);return A+TEXT("|")+B;};
        for(int32 I=0;I<40;I++){
            if(P.destinations[P.destinations[I]]!=I)return FailMini();
            ExpectedPairs.Add(PairKey(NativeMinimizerEndpoints[I],NativeMinimizerEndpoints[P.destinations[I]]));
        }
        for(const auto& V:*Pairs){const TArray<TSharedPtr<FJsonValue>>* Pair=nullptr;FString A,B;
            if(!V->TryGetArray(Pair) || Pair->Num()!=2 || !(*Pair)[0]->TryGetString(A) || !(*Pair)[1]->TryGetString(B))return FailMini();
            FString Key=PairKey(A,B);if(!ExpectedPairs.Contains(Key) || ActualPairs.Contains(Key))return FailMini();ActualPairs.Add(Key);
        }
        if(ActualPairs.Num()!=ExpectedPairs.Num())return FailMini();
        int32 N=0;
        for(int32 I=0;I<100;I++){double On=0;FString Name;
            if(!(*Checks)[I]->TryGetNumber(On) || (On!=0 && On!=1))return FailMini();P.checks[I]=uint8(On);
            if(On){const auto& L=NativeMinimizerLocations[I];
                if(!(P.regions&(1u<<L.Region)) && !L.PostBoss)return FailMini();
                if(N>=Names->Num() || !(*Names)[N++]->TryGetString(Name) || Name!=L.Name)return FailMini();
            }else if(Out.Items[I].Kind!=2)return FailMini();
        }
        if(N!=Names->Num())return FailMini();
    }
    if(AreaConnections){
        const TSharedPtr<FJsonObject>* Native=nullptr;const TArray<TSharedPtr<FJsonValue>> *Targets=nullptr,*Pairs=nullptr;
        if(!InitialDoors || !(*Topology)->TryGetObjectField(TEXT("nativeAreas"),Native) ||
           !(*Native)->TryGetStringField(TEXT("catalogSha256"),Out.AreasCatalog) || Out.AreasCatalog.Len()!=64 ||
           !(*Native)->TryGetArrayField(TEXT("destinations"),Targets) || Targets->Num()!=32 ||
           !(*Topology)->TryGetArrayField(TEXT("areaPairs"),Pairs) || Pairs->Num()!=16){Error=TEXT("Invalid native area connection contract");return false;}
        for(TCHAR C:Out.AreasCatalog)if(!FChar::IsHexDigit(C)){Error=TEXT("Invalid area catalog fingerprint");return false;}
        TSet<int32> Seen;
        for(const auto& Value:*Targets){double Target=0;
            if(!Value->TryGetNumber(Target) || Target<0 || Target>=32 || Target!=FMath::FloorToDouble(Target) || Seen.Contains(int32(Target))){Error=TEXT("Invalid area destination table");return false;}
            Seen.Add(int32(Target));Out.AreaDestinations.Add(uint8(Target));
        }
        #include "SMNativeAreaNames.inl"
        for(int32 I=0;I<32;I++)if(Out.AreaDestinations[I]==I || Out.AreaDestinations[Out.AreaDestinations[I]]!=I){Error=TEXT("Invalid reciprocal area routing");return false;}
        Seen.Reset();
        for(const auto& Value:*Pairs){const TArray<TSharedPtr<FJsonValue>>* Pair=nullptr;FString A,B;
            if(!Value->TryGetArray(Pair) || Pair->Num()!=2 || !(*Pair)[0]->TryGetString(A) || !(*Pair)[1]->TryGetString(B)){Error=TEXT("Malformed area graph pair");return false;}
            int32 I=NativeAreaNames.IndexOfByKey(A),J=NativeAreaNames.IndexOfByKey(B);
            if(I==INDEX_NONE || J==INDEX_NONE || Seen.Contains(I) || Seen.Contains(J) || Out.AreaDestinations[I]!=J){Error=TEXT("Area graph disagrees with native doors");return false;}
            Seen.Add(I);Seen.Add(J);
        }
    }
    if(BossConnections){
        const TSharedPtr<FJsonObject>* Native=nullptr;const TArray<TSharedPtr<FJsonValue>> *Targets=nullptr,*Pairs=nullptr;
        if(!RequiredBehaviors.Contains(TEXT("native-start-v1")) || !(*Topology)->TryGetObjectField(TEXT("native"),Native) ||
           !(*Native)->TryGetStringField(TEXT("catalogSha256"),Out.ConnectionsCatalog) || Out.ConnectionsCatalog.Len()!=64 ||
           !(*Native)->TryGetArrayField(TEXT("destinations"),Targets) || Targets->Num()!=8 ||
           !(*Topology)->TryGetArrayField(TEXT("bossPairs"),Pairs) || Pairs->Num()!=4){Error=TEXT("Invalid native boss connection contract");return false;}
        for(TCHAR C:Out.ConnectionsCatalog)if(!FChar::IsHexDigit(C)){Error=TEXT("Invalid boss catalog fingerprint");return false;}
        TSet<int32> Seen;
        for(const auto& Value:*Targets){double Target=0;
            if(!Value->TryGetNumber(Target) || Target<0 || Target>=8 || Target!=FMath::FloorToDouble(Target) || Seen.Contains(int32(Target))){Error=TEXT("Invalid boss destination table");return false;}
            Seen.Add(int32(Target));Out.BossDestinations.Add(uint8(Target));
        }
        // Immutable v1 order matches the native catalog. Every portal connects
        // an outside room to one arena, and the return connection is reciprocal.
        const TArray<FString> Names={TEXT("DraygonRoomIn"),TEXT("DraygonRoomOut"),TEXT("KraidRoomIn"),TEXT("KraidRoomOut"),TEXT("PhantoonRoomIn"),TEXT("PhantoonRoomOut"),TEXT("RidleyRoomIn"),TEXT("RidleyRoomOut")};
        for(int32 I=0;I<8;I++)if(Out.BossDestinations[Out.BossDestinations[I]]!=I || (Out.BossDestinations[I]%2)==I%2){Error=TEXT("Invalid reciprocal boss routing");return false;}
        Seen.Reset();
        for(const auto& Value:*Pairs){const TArray<TSharedPtr<FJsonValue>>* Pair=nullptr;FString A,B;
            if(!Value->TryGetArray(Pair) || Pair->Num()!=2 || !(*Pair)[0]->TryGetString(A) || !(*Pair)[1]->TryGetString(B)){Error=TEXT("Malformed boss graph pair");return false;}
            int32 I=Names.IndexOfByKey(A),J=Names.IndexOfByKey(B);
            if(I==INDEX_NONE || J==INDEX_NONE || Seen.Contains(I) || Seen.Contains(J) || Out.BossDestinations[I]!=J){Error=TEXT("Boss graph disagrees with native doors");return false;}
            Seen.Add(I);Seen.Add(J);
        }
    }
    const bool ColoredDoors=RequiredBehaviors.Contains(TEXT("native-door-colors-v1"));
    for(const auto& Indicator:Out.DoorIndicators)if(Indicator.plm>=0xf60b && Indicator.plm<=0xf665 && !ColoredDoors){Error=TEXT("Beam indicators require native colored doors");return false;}
    const TSharedPtr<FJsonObject>* DoorContract=nullptr;
    if(Context && (*Context)->HasField(TEXT("doorColors"))!=ColoredDoors){Error=TEXT("Door-color capability and data disagree");return false;}
    if(ColoredDoors){
        const TArray<TSharedPtr<FJsonValue>>* Colors=nullptr;const TSharedPtr<FJsonObject>* Doors=nullptr;double DoorSchema=0;
        if(!Context || !RequiredBehaviors.Contains(TEXT("native-start-v1")) || !(*Context)->TryGetObjectField(TEXT("doorColors"),DoorContract) ||
           !(*DoorContract)->TryGetNumberField(TEXT("schema"),DoorSchema) || DoorSchema!=1 ||
           !(*DoorContract)->TryGetStringField(TEXT("catalogSha256"),Out.DoorColorsCatalog) || Out.DoorColorsCatalog.Len()!=64 ||
           !(*DoorContract)->TryGetArrayField(TEXT("colors"),Colors) || Colors->Num()!=SM_DOOR_COLOR_COUNT ||
           !(*DoorContract)->TryGetObjectField(TEXT("doors"),Doors) || (*Doors)->Values.Num()!=SM_DOOR_COLOR_COUNT){Error=TEXT("Invalid native door-color contract");return false;}
        for(TCHAR C:Out.DoorColorsCatalog)if(!FChar::IsHexDigit(C)){Error=TEXT("Invalid door-color catalog fingerprint");return false;}
        #include "SMNativeDoorCatalog.inl"
        static_assert(UE_ARRAY_COUNT(NativeDoorSpecs)==SM_DOOR_COLOR_COUNT,"Door catalog size mismatch");
        const TArray<FString> Names={TEXT("blue"),TEXT("red"),TEXT("green"),TEXT("yellow"),TEXT("grey"),TEXT("wave"),TEXT("spazer"),TEXT("plasma"),TEXT("ice")};
        for(int32 I=0;I<SM_DOOR_COLOR_COUNT;I++){
            const auto& Spec=NativeDoorSpecs[I];double Value=0,Address=0,Facing=0,Bit=0;FString Color;bool Hidden=true;const TSharedPtr<FJsonObject>* Door=nullptr;
            if(!(*Colors)[I]->TryGetNumber(Value) || Value<0 || Value>=9 || Value!=FMath::FloorToDouble(Value) ||
               !(*Doors)->TryGetObjectField(Spec.Name,Door) || !(*Door)->TryGetStringField(TEXT("color"),Color) || !Names.Contains(Color) ||
               !(*Door)->TryGetNumberField(TEXT("address"),Address) || Address!=Spec.Address ||
               !(*Door)->TryGetNumberField(TEXT("facing"),Facing) || Facing!=Spec.Facing ||
               !(*Door)->TryGetNumberField(TEXT("openedBit"),Bit) || Bit!=Spec.OpenedBit ||
               !(*Door)->TryGetBoolField(TEXT("hidden"),Hidden) || Hidden ||
               Value!=(!Spec.Randomizable || Color==TEXT("blue")?0:Names.IndexOfByKey(Color))){Error=TEXT("Native and tracker door requirements disagree");return false;}
            Out.DoorColors.Add(uint8(Value));
        }
    }
    const bool HasAnimals=RequiredBehaviors.Contains(TEXT("native-animals-v1"));
    const TSharedPtr<FJsonObject>* AnimalsContract=nullptr;
    if(bool(Context && (*Context)->TryGetObjectField(TEXT("animals"),AnimalsContract))!=HasAnimals){Error=TEXT("Animals capability and data disagree");return false;}
    if(HasAnimals){
        #include "SMNativeAnimals.inl"
        const TSharedPtr<FJsonObject> *AnimalsRules=nullptr,*Options=nullptr;
        double Version=0,Mode=0;FString Patch,Enabled,Logic,Escape;
        if(!RequiredBehaviors.Contains(TEXT("native-start-v1")) ||
           !(*AnimalsContract)->TryGetNumberField(TEXT("schema"),Version) || Version!=1 ||
           !(*AnimalsContract)->TryGetNumberField(TEXT("mode"),Mode) || Mode<1 || Mode>10 || Mode!=FMath::FloorToDouble(Mode) ||
           !(*AnimalsContract)->TryGetStringField(TEXT("patch"),Patch) || Patch!=NativeAnimalNames[int32(Mode)] ||
           !(*AnimalsContract)->TryGetStringField(TEXT("catalogSha256"),Out.AnimalsCatalog) || Out.AnimalsCatalog!=NativeAnimalsCatalog ||
           !(*Manifest)->TryGetObjectField(TEXT("rules"),AnimalsRules) || !(*AnimalsRules)->TryGetObjectField(TEXT("options"),Options) ||
           !(*Options)->TryGetStringField(TEXT("animals"),Enabled) || Enabled!=TEXT("on") ||
           !(*Options)->TryGetStringField(TEXT("logic"),Logic) || Logic!=TEXT("vanilla") ||
           !(*Options)->TryGetStringField(TEXT("escapeRando"),Escape) || Escape!=TEXT("off")){
            Error=TEXT("Invalid Animals Surprise contract");return false;
        }
        Out.AnimalsMode=int32(Mode);
    }else{
        const TSharedPtr<FJsonObject> *AnimalsRules=nullptr,*Options=nullptr;FString Enabled;
        if((*Manifest)->TryGetObjectField(TEXT("rules"),AnimalsRules) && (*AnimalsRules)->TryGetObjectField(TEXT("options"),Options) &&
           (*Options)->TryGetStringField(TEXT("animals"),Enabled) && Enabled==TEXT("on")){Error=TEXT("Animals enabled without a native plan");return false;}
    }
    const bool HasScavenger=RequiredBehaviors.Contains(TEXT("native-scavenger-v1"));
    const TSharedPtr<FJsonObject>* ScavengerContract=nullptr;
    if(bool(Context && (*Context)->HasField(TEXT("scavenger")))!=HasScavenger){Error=TEXT("Scavenger capability and data disagree");return false;}
    if(HasScavenger){
        double HuntSchema=0;FString Split;const TArray<TSharedPtr<FJsonValue>> *Words=nullptr,*Names=nullptr;
        if(!(*Context)->TryGetObjectField(TEXT("scavenger"),ScavengerContract) ||
           !(*Context)->TryGetStringField(TEXT("split"),Split) || Split!=TEXT("Scavenger") ||
           !(*ScavengerContract)->TryGetNumberField(TEXT("schema"),HuntSchema) || HuntSchema!=1 ||
           !(*ScavengerContract)->TryGetStringField(TEXT("catalogSha256"),Out.ScavengerCatalog) || Out.ScavengerCatalog!=NativeScavengerCatalog ||
           !(*ScavengerContract)->TryGetArrayField(TEXT("words"),Words) || Words->Num()<1 || Words->Num()>17 ||
           !(*ScavengerContract)->TryGetArrayField(TEXT("locations"),Names) || Names->Num()!=Words->Num()){
            Error=TEXT("Invalid native Scavenger contract");return false;
        }
        Out.Scavenger.version=1;Out.Scavenger.size=sizeof(Out.Scavenger);Out.Scavenger.count=Words->Num();TSet<int32> Seen;
        for(int32 I=0;I<Words->Num();I++){
            double Word=0;FString Name;
            if(!(*Words)[I]->TryGetNumber(Word) || Word<0 || Word>65535 || Word!=FMath::FloorToDouble(Word) || !(*Names)[I]->TryGetString(Name)){
                Error=TEXT("Invalid Scavenger location");return false;
            }
            int32 Hud=int32(Word)&255;
            if(Hud>=UE_ARRAY_COUNT(NativeScavengerSpecs) || Word!=NativeScavengerSpecs[Hud].Word || Name!=NativeScavengerSpecs[Hud].Name || Seen.Contains(Hud)){
                Error=TEXT("Scavenger location identities disagree");return false;
            }
            Seen.Add(Hud);Out.Scavenger.order[I]=uint16(Word);
        }
    }
    const bool HasObjectives=RequiredBehaviors.Contains(TEXT("native-objectives-v1"));
    const bool HasEscape=RequiredBehaviors.Contains(TEXT("native-escape-v1"));
    const TSharedPtr<FJsonObject>* EscapeContract=nullptr;
    const bool ContainsEscape=Topology && (*Topology)->TryGetObjectField(TEXT("nativeEscape"),EscapeContract);
    if(HasEscape!=ContainsEscape || (HasEscape && (!HasObjectives || !ReadEscapeContract(*EscapeContract,Out,Error)))){if(Error.IsEmpty())Error=TEXT("Missing escape dependency");return false;}
    if(HasEscape){
        const TArray<TSharedPtr<FJsonValue>>* Pairs=nullptr;const TArray<TSharedPtr<FJsonValue>>* Pair=nullptr;
        if(!(*Topology)->TryGetArrayField(TEXT("escapePairs"),Pairs) || Pairs->Num()!=1 || !(*Pairs)[0]->TryGetArray(Pair) || Pair->Num()!=2){Error=TEXT("Missing escape graph edge");return false;}
        int32 Target=-1;for(int32 I=0;I<Out.EscapeRouting.count;I++)if(Out.EscapeRouting.pairs[I][0]==0)Target=Out.EscapeRouting.pairs[I][1];
        FString A,B;if(!(*Pair)[0]->TryGetString(A) || !(*Pair)[1]->TryGetString(B) || A!=NativeEscapeEndpoints[0] || Target<0 || B!=NativeEscapeEndpoints[Target]){Error=TEXT("Escape graph disagrees with native doors");return false;}
    }
    if(RequiredBehaviors.Contains(TEXT("native-fast-tourian-v1")) && !HasObjectives){Error=TEXT("Fast Tourian requires native objectives");return false;}
    const TSharedPtr<FJsonObject>* ObjectiveContract=nullptr;
    if(bool(Context && (*Context)->HasField(TEXT("objectives")))!=HasObjectives){Error=TEXT("Objective capability and data disagree");return false;}
    if(HasObjectives){
        FString Tourian;double Required=0;TArray<FString> GoalNames;
        if(Out.RelicsRequired || !RequiredBehaviors.Contains(TEXT("native-start-v1")) ||
           !(*Context)->TryGetObjectField(TEXT("objectives"),ObjectiveContract) ||
           !(*Context)->TryGetStringField(TEXT("tourian"),Tourian) || (Tourian!=TEXT("Vanilla") && Tourian!=TEXT("Fast") && Tourian!=TEXT("Disabled")) ||
           !ReadObjectiveContract(*ObjectiveContract,AreaConnections || Minimizer,Out,Error)){
            if(Error.IsEmpty())Error=TEXT("Incompatible native objective completion mode");return false;
        }
        if((Tourian==TEXT("Fast"))!=bool(Out.Objectives.flags&SM_OBJECTIVE_FAST_TOURIAN) || (Tourian==TEXT("Fast"))!=RequiredBehaviors.Contains(TEXT("native-fast-tourian-v1"))){Error=TEXT("Tourian capability and objective contract disagree");return false;}
        if(!(*Context)->TryGetStringArrayField(TEXT("goals"),GoalNames) || GoalNames.Num()!=Out.Objectives.count ||
           !(*Context)->TryGetNumberField(TEXT("goalsRequired"),Required) || Required!=Out.Objectives.required){Error=TEXT("Effective objective summary disagrees");return false;}
        for(int32 I=0;I<GoalNames.Num();I++)if(GoalNames[I]!=NativeObjectiveSpecs[Out.Objectives.goals[I]].Name){Error=TEXT("Effective objective names disagree");return false;}
    }
    if(HasEscape){
        const auto* Expected=(*Topology)->Values.Find(TEXT("nativeEscape"));const auto* Embedded=(*ObjectiveContract)->Values.Find(TEXT("escape"));
        FString Mode;(*Context)->TryGetStringField(TEXT("tourian"),Mode);
        if(!Embedded || !FJsonValue::CompareEqual(*Expected->Get(),*Embedded->Get()) || (Mode==TEXT("Disabled"))!=bool(Out.EscapeClock.flags&1)){Error=TEXT("Escape objective contract disagrees");return false;}
    }
    if(Minimizer){
        const auto* Expected=(*Topology)->Values.Find(TEXT("nativeMinimizer"));
        const auto* Embedded=ObjectiveContract?(*ObjectiveContract)->Values.Find(TEXT("minimizer")):nullptr;
        if(!HasObjectives || !Embedded || !FJsonValue::CompareEqual(*Expected->Get(),*Embedded->Get())){Error=TEXT("Minimizer objective membership disagrees");return false;}
    }
    if(HasScavenger){
        const auto* Embedded=ObjectiveContract?(*ObjectiveContract)->Values.Find(TEXT("scavenger")):nullptr;
        const auto* Expected=(*Context)->Values.Find(TEXT("scavenger"));
        if(!HasObjectives || !Embedded || !FJsonValue::CompareEqual(*Expected->Get(),*Embedded->Get())){Error=TEXT("Scavenger objective order disagrees");return false;}
    }
    if(RelicCount && !Out.RelicsRequired){Error=TEXT("Relic seed has no completion quota");return false;}
    (*Manifest)->TryGetStringArrayField(TEXT("pendingPatches"),Out.PendingPatches);
    (*Manifest)->TryGetStringArrayField(TEXT("requiredNativeBehavior"),Out.RequiredNativeBehavior);
    const TSharedPtr<FJsonObject>* Tracker;
    if((*Manifest)->TryGetObjectField(TEXT("tracker"),Tracker)) {
        double TrackerSchema=0,TrackerSeed=0;FString Fingerprint;
        const TArray<TSharedPtr<FJsonValue>>* Locations;
        if(!(*Tracker)->TryGetNumberField(TEXT("schema"),TrackerSchema) || TrackerSchema!=1 ||
           !(*Tracker)->TryGetNumberField(TEXT("seed"),TrackerSeed) || TrackerSeed!=Seed ||
           !(*Tracker)->TryGetStringField(TEXT("seedFingerprint"),Fingerprint) || Fingerprint!=Out.Fingerprint ||
           !(*Tracker)->TryGetArrayField(TEXT("locations"),Locations) || Locations->Num()!=100) {
            Error=TEXT("Invalid tracker contract");return false;
        }
        const TSharedPtr<FJsonObject>* GoalSettings=nullptr;
        const bool TrackerHasObjectives=(*Tracker)->TryGetObjectField(TEXT("settings"),GoalSettings) && (*GoalSettings)->HasField(TEXT("nativeObjectives"));
        const bool TrackerHasScavenger=GoalSettings && (*GoalSettings)->HasField(TEXT("scavenger"));
        if(TrackerHasScavenger!=HasScavenger){Error=TEXT("Tracker Scavenger capability disagrees");return false;}
        if(HasScavenger){
            const auto* Expected=(*Context)->Values.Find(TEXT("scavenger"));const auto* Actual=(*GoalSettings)->Values.Find(TEXT("scavenger"));
            if(!FJsonValue::CompareEqual(*Expected->Get(),*Actual->Get())){Error=TEXT("Tracker Scavenger order disagrees");return false;}
        }
        if(TrackerHasObjectives!=HasObjectives){Error=TEXT("Tracker objective capability disagrees");return false;}
        if(HasObjectives){
            const auto* Expected=(*Context)->Values.Find(TEXT("objectives"));const auto* Actual=(*GoalSettings)->Values.Find(TEXT("nativeObjectives"));
            if(!Expected || !Actual || !FJsonValue::CompareEqual(*Expected->Get(),*Actual->Get())){Error=TEXT("Tracker and native objective rules disagree");return false;}
        }
        if(BossConnections || AreaConnections){
            const TSharedPtr<FJsonObject>* TrackerTopology=nullptr;
            if(!(*Tracker)->TryGetObjectField(TEXT("topology"),TrackerTopology)){Error=TEXT("Boss tracker has no world connections");return false;}
            for(const TCHAR* Key:{TEXT("mode"),TEXT("bossPairs"),TEXT("areaPairs"),TEXT("escapePairs"),TEXT("native"),TEXT("nativeAreas"),TEXT("nativeMinimizer"),TEXT("mixedPairs")}){
                const auto* Expected=(*Topology)->Values.Find(Key);const auto* Actual=(*TrackerTopology)->Values.Find(Key);
                if(!Expected && !Actual)continue;
                if(!Expected || !Actual || !FJsonValue::CompareEqual(*Expected->Get(),*Actual->Get())){Error=TEXT("Tracker topology disagrees with native world");return false;}
            }
        }
        if(InitialDoors){
            const TSharedPtr<FJsonObject>* Settings=nullptr;const TArray<TSharedPtr<FJsonValue>>* Doors=nullptr;
            if(!(*Tracker)->TryGetObjectField(TEXT("settings"),Settings) || !(*Settings)->TryGetArrayField(TEXT("initialDoors"),Doors) || Doors->Num()!=Out.InitialDoors.Num()){Error=TEXT("Tracker has no matching initial doors");return false;}
            for(int32 I=0;I<Doors->Num();I++){double Door=0;if(!(*Doors)[I]->TryGetNumber(Door) || Door!=Out.InitialDoors[I]){Error=TEXT("Tracker initial door bits disagree");return false;}}
        }
        if(ColoredDoors){
            const TSharedPtr<FJsonObject> *TrackerTopology=nullptr,*TrackerDoors=nullptr;
            if(!(*Tracker)->TryGetObjectField(TEXT("topology"),TrackerTopology) || !(*TrackerTopology)->TryGetObjectField(TEXT("doorColors"),TrackerDoors)){Error=TEXT("Missing colored-door tracker contract");return false;}
            for(const TCHAR* Key:{TEXT("schema"),TEXT("catalogSha256"),TEXT("colors")}){
                const auto* Expected=(*DoorContract)->Values.Find(Key);const auto* Actual=(*TrackerDoors)->Values.Find(Key);
                if(!Expected || !Actual || !FJsonValue::CompareEqual(*Expected->Get(),*Actual->Get())){Error=TEXT("Tracker door-color contract disagrees");return false;}
            }
            const auto* Expected=(*DoorContract)->Values.Find(TEXT("doors"));const auto* Actual=(*TrackerTopology)->Values.Find(TEXT("doors"));
            if(!Expected || !Actual || !FJsonValue::CompareEqual(*Expected->Get(),*Actual->Get())){Error=TEXT("Tracker door requirements disagree");return false;}
        }
        if(HasEscape){
            const TSharedPtr<FJsonObject>* TrackerTopology=nullptr;
            if(!(*Tracker)->TryGetObjectField(TEXT("topology"),TrackerTopology)){Error=TEXT("Missing escape tracker topology");return false;}
            for(const TCHAR* Key:{TEXT("nativeEscape"),TEXT("escapePairs")}){
                const auto* Expected=(*Topology)->Values.Find(Key);const auto* Actual=(*TrackerTopology)->Values.Find(Key);
                if(!Expected || !Actual || !FJsonValue::CompareEqual(*Expected->Get(),*Actual->Get())){Error=TEXT("Escape tracker disagrees");return false;}
            }
        }
        TSet<int32> CollectionBits;
        for(int32 I=0;I<100;I++) {
            const TSharedPtr<FJsonObject>* Location;double Address=0,Plm=0,Bit=0,Index=0;FString Item;
            if(!(*Locations)[I]->TryGetObject(Location) ||
               !(*Location)->TryGetNumberField(TEXT("index"),Index) || Index!=I ||
               !(*Location)->TryGetNumberField(TEXT("address"),Address) || Address!=Out.Items[I].Address ||
               !(*Location)->TryGetNumberField(TEXT("plm"),Plm) || Plm!=Out.Items[I].Plm ||
               !(*Location)->TryGetStringField(TEXT("item"),Item) || Item!=Out.Items[I].Item ||
               !(*Location)->TryGetNumberField(TEXT("collectionBit"),Bit) || Bit<0 || Bit>255 || Bit!=FMath::FloorToDouble(Bit) || CollectionBits.Contains(int32(Bit))) {
                Error=TEXT("Tracker locations disagree with the native seed");return false;
            }
            double Kind=0;(*Location)->TryGetNumberField(TEXT("kind"),Kind);
            if(Kind!=Out.Items[I].Kind){Error=TEXT("Tracker pickup kind disagrees with native seed");return false;}
            CollectionBits.Add(int32(Bit));
        }
        FJsonSerializer::Serialize(Tracker->ToSharedRef(),TJsonWriterFactory<>::Create(&Out.TrackerJson));
    }
    if((InitialDoors || AreaConnections || HasObjectives) && Out.TrackerJson.IsEmpty()){Error=TEXT("Native world plan requires its tracker contract");return false;}
    Out.Seed=(int32)Seed;Out.Json=Json;
    return true;
}
bool FSMRandomizer::Stage(const FSMSeedPlan& Plan,const FString& Directory,FString& Error) {
    FSMSeedPlan Verified;
    if(!ReadPlan(Plan.Json,Verified,Error)) return false;
    IFileManager::Get().MakeDirectory(*Directory,true);
    const FString Path=Directory/FString::Printf(TEXT("seed-%d-%s.json"),Verified.Seed,*Verified.Fingerprint.Left(12));
    if(!FFileHelper::SaveStringToFile(Verified.Json,*(Path+TEXT(".tmp")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) ||
       !IFileManager::Get().Move(*Path,*(Path+TEXT(".tmp")),true,true)) {Error=TEXT("Cannot stage seed");return false;}
    return true;
}
