#include "SMTracker.h"
#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformProcess.h"

FSMTracker::~FSMTracker(){if(Work.IsValid())Work.Wait();}
void FSMTracker::Initialize(void* Handle){
#define LOAD(Member,Name) Member=reinterpret_cast<decltype(Member)>(FPlatformProcess::GetDllExport(Handle,TEXT(Name)))
    LOAD(Snapshot,"sm_tracker_snapshot");LOAD(Configure,"sm_tracker_configure");
    LOAD(PublishBosses,"sm_tracker_publish_bosses");
    LOAD(Publish,"sm_tracker_publish");LOAD(Evaluate,"sm_tracker_evaluate");
    LOAD(ObjectiveSnapshot,"sm_objectives_snapshot");LOAD(PublishObjectives,"sm_tracker_publish_objectives");
#undef LOAD
}
static bool Same(const SmTrackerSnapshot& A,const SmTrackerSnapshot& B){
    return A.session==B.session && A.slot==B.slot && A.randomized==B.randomized &&
        !FMemory::Memcmp(A.seed_fingerprint,B.seed_fingerprint,65) &&
        A.acquired_items==B.acquired_items && A.acquired_beams==B.acquired_beams &&
        !FMemory::Memcmp(&A.max_health,&B.max_health,10) &&
        !FMemory::Memcmp(A.collected,B.collected,100) && !FMemory::Memcmp(A.boss_bits,B.boss_bits,8) &&
        !FMemory::Memcmp(A.opened_doors,B.opened_doors,64) && !FMemory::Memcmp(A.events,B.events,8);
}
void FSMTracker::Tick(bool Enabled,bool Map,bool Items,const FString& Root,const FSMGameProfile& Slot,int VanillaSkill){
    if(!Configure || !Snapshot || !Publish || !PublishBosses || !Evaluate){Status=TEXT("Tracker module unavailable");return;}
    SmTrackerSnapshot Now{};
    SmObjectiveSnapshot NowObjectives{};
    const bool WithObjectives=ObjectiveSnapshot && PublishObjectives;
    const bool ObjectivesValid=WithObjectives && ObjectiveSnapshot(&NowObjectives,sizeof(NowObjectives));
    const bool Valid=Snapshot(&Now,sizeof(Now)) && Now.world_valid &&
        (!WithObjectives || ObjectivesValid) &&
        (!Slot.Plan.Objectives.count || (ObjectivesValid && NowObjectives.state[0]==Slot.Plan.Objectives.count)) &&
        bool(Now.randomized)==Slot.Randomized &&
        (!Slot.Randomized || Slot.Plan.Fingerprint==UTF8_TO_TCHAR(Now.seed_fingerprint));
    // A pending randomized slot must never fall back to vanilla rules.
    Enabled=Enabled && (!Slot.Randomized || (Slot.Generated && !Slot.Plan.TrackerJson.IsEmpty()));
    const FString NextContext=Slot.Id+TEXT(":")+Slot.Plan.Fingerprint+FString::FromInt(VanillaSkill);
    if(Context!=NextContext){Context=NextContext;HaveLast=false;ObjectivesJson.Empty();Configure(false,Map,Items);}
    Configure(Enabled,Map,Items);
    if(!Enabled || !Valid){HaveLast=false;ObjectivesJson.Empty();Status=Enabled?TEXT("Waiting for gameplay"):TEXT("Disabled");}
    if(Work.IsValid() && Work.IsReady()){
        auto Result=Work.Get();Work={};
        if(Enabled && Valid && Context==SubmittedContext && Same(Now,Submitted) &&
           (!WithObjectives || !FMemory::Memcmp(&NowObjectives,&SubmittedObjectives,sizeof(NowObjectives)))){
            const bool Published=Result.States.Num()==100 && Result.BossStates.Num()==SM_TRACKER_BOSS_COUNT && (WithObjectives?
                PublishObjectives(&Submitted,&SubmittedObjectives,Result.States.GetData(),100):Publish(&Submitted,Result.States.GetData(),100)) &&
                PublishBosses(&Submitted,Result.BossStates.GetData(),Result.BossStates.Num());
            if(Published){Status=TEXT("Live — native inventory / effective seed rules");ObjectivesJson=Result.ObjectivesJson;}
            else {HaveLast=false;Status=Result.Error.IsEmpty()?TEXT("Waiting for a consistent snapshot"):Result.Error;UE_LOG(LogTemp,Warning,TEXT("SM_TRACKER_QUERY_FAILED %s"),*Status);}
        }
    }
    if(!Enabled || !Valid || !Map || Work.IsValid() || (HaveLast && Same(Last,Now) &&
       (!WithObjectives || !FMemory::Memcmp(&LastObjectives,&NowObjectives,sizeof(NowObjectives)))))return;
    Last=Submitted=Now;HaveLast=true;SubmittedContext=Context;
    LastObjectives=SubmittedObjectives=NowObjectives;ObjectivesJson.Empty();
    auto Request=MakeShared<FJsonObject>();Request->SetBoolField(TEXT("randomized"),Slot.Randomized);
    if(Slot.Randomized){
        TSharedPtr<FJsonObject> Tracker;
        if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Slot.Plan.TrackerJson),Tracker) || !Tracker.IsValid()){
            Status=TEXT("Missing effective seed rules");return;
        }
        const TSharedPtr<FJsonObject>* Settings;const TSharedPtr<FJsonObject>* Topology;
        if(!Tracker->TryGetObjectField(TEXT("settings"),Settings) || !Tracker->TryGetObjectField(TEXT("topology"),Topology)){
            Status=TEXT("Incomplete effective seed rules");return;
        }
        Request->SetObjectField(TEXT("settings"),*Settings);Request->SetObjectField(TEXT("topology"),*Topology);
    }else{
        static const TCHAR* Skills[]={TEXT("casual"),TEXT("regular"),TEXT("veteran")};
        Request->SetStringField(TEXT("skill"),Skills[FMath::Clamp(VanillaSkill,0,2)]);
    }
    if(WithObjectives){
        auto NativeObjectives=MakeShared<FJsonObject>();
        auto Numbers=[](const int32* Values,int Count){TArray<TSharedPtr<FJsonValue>> Array;for(int I=0;I<Count;I++)Array.Add(MakeShared<FJsonValueNumber>(Values[I]));return Array;};
        NativeObjectives->SetNumberField(TEXT("version"),NowObjectives.version);
        NativeObjectives->SetArrayField(TEXT("state"),Numbers(NowObjectives.state,8));
        NativeObjectives->SetArrayField(TEXT("goals"),Numbers(NowObjectives.goals,18));
        TArray<TSharedPtr<FJsonValue>> Values;
        for(int I=0;I<18;I++)Values.Add(MakeShared<FJsonValueArray>(Numbers(NowObjectives.values[I],5)));
        NativeObjectives->SetArrayField(TEXT("values"),Values);
        Request->SetObjectField(TEXT("objectiveState"),NativeObjectives);
    }
    auto Inventory=MakeShared<FJsonObject>();
    Inventory->SetNumberField(TEXT("items"),Now.acquired_items);Inventory->SetNumberField(TEXT("beams"),Now.acquired_beams);
    Inventory->SetNumberField(TEXT("health"),Now.max_health);Inventory->SetNumberField(TEXT("reserve"),Now.max_reserve);
    Inventory->SetNumberField(TEXT("missiles"),Now.max_missiles);Inventory->SetNumberField(TEXT("supers"),Now.max_supers);
    Inventory->SetNumberField(TEXT("powerBombs"),Now.max_power_bombs);
    auto Bytes=[&](const TCHAR* Name,const uint8* Values,int Count){TArray<TSharedPtr<FJsonValue>> Array;for(int I=0;I<Count;I++)Array.Add(MakeShared<FJsonValueNumber>(Values[I]));Inventory->SetArrayField(Name,Array);};
    Bytes(TEXT("bosses"),Now.boss_bits,8);Bytes(TEXT("doors"),Now.opened_doors,64);Bytes(TEXT("collected"),Now.collected,100);
    Request->SetObjectField(TEXT("inventory"),Inventory);
    FString Json;FJsonSerializer::Serialize(Request,TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json));
    Status=TEXT("Updating reachability...");
    const auto Fn=Evaluate;
    Work=Async(EAsyncExecution::Thread,[Fn,Root,Json]{
        FSMTrackerResult Result;TArray<char> Buffer;Buffer.SetNumZeroed(262144);
        const int Size=Fn(TCHAR_TO_UTF8(*(Root/TEXT("Randomizer"))),TCHAR_TO_UTF8(*Json),Buffer.GetData(),Buffer.Num());
        if(Size<=0 || Size>Buffer.Num()){Result.Error=TEXT("Tracker interpreter failed");return Result;}
        TSharedPtr<FJsonObject> Output;
        if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(Buffer.GetData())),Output) || !Output.IsValid()){
            Result.Error=TEXT("Invalid tracker response");return Result;
        }
        bool Ok=false;const TArray<TSharedPtr<FJsonValue>>* Checks;
        if(!Output->TryGetBoolField(TEXT("ok"),Ok) || !Ok || !Output->TryGetArrayField(TEXT("checks"),Checks) || Checks->Num()!=100){
            if(!Output->TryGetStringField(TEXT("error"),Result.Error))Result.Error=TEXT("Incomplete tracker response");return Result;
        }
        for(int I=0;I<100;I++){
            const TSharedPtr<FJsonObject>* Check;double Index,State;
            if(!(*Checks)[I]->TryGetObject(Check) || !(*Check)->TryGetNumberField(TEXT("index"),Index) || Index!=I ||
                !(*Check)->TryGetNumberField(TEXT("state"),State) || !(State==0 || State==1 || State==2 || State==4 || State==8)){
                Result.States.Reset();Result.Error=TEXT("Invalid check identity/state");return Result;
            }
            Result.States.Add(uint8(State));
        }
        static const TCHAR* BossNames[]={TEXT("Kraid"),TEXT("Phantoon"),TEXT("Draygon"),TEXT("Ridley"),TEXT("Mother Brain"),
            TEXT("Spore Spawn"),TEXT("Crocomire"),TEXT("Botwoon"),TEXT("Golden Torizo"),TEXT("Bomb Torizo")};
        const TArray<TSharedPtr<FJsonValue>>* Bosses;
        if(!Output->TryGetArrayField(TEXT("bosses"),Bosses) || Bosses->Num()!=SM_TRACKER_BOSS_COUNT){
            Result.Error=TEXT("Missing encounter states");return Result;
        }
        for(int I=0;I<SM_TRACKER_BOSS_COUNT;I++){
            const TSharedPtr<FJsonObject>* Boss;double Index,State;FString Name;
            if(!(*Bosses)[I]->TryGetObject(Boss) || !(*Boss)->TryGetNumberField(TEXT("index"),Index) || Index!=I ||
                !(*Boss)->TryGetStringField(TEXT("name"),Name) || Name!=BossNames[I] ||
                !(*Boss)->TryGetNumberField(TEXT("state"),State) || !(State==0 || State==1 || State==2 || State==4 || State==8 || State==255)){
                Result.BossStates.Reset();Result.Error=TEXT("Invalid encounter identity/state");return Result;
            }
            Result.BossStates.Add(uint8(State));
        }
        const TSharedPtr<FJsonObject>* Objectives;
        if(Output->TryGetObjectField(TEXT("objectives"),Objectives))
            FJsonSerializer::Serialize((*Objectives).ToSharedRef(),TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Result.ObjectivesJson));
        return Result;
    });
}
