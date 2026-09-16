#pragma once
#include "CoreMinimal.h"
#include "Async/Future.h"
#include "SMProfiles.h"
#include "../../../Native/sm_tracker.h"
struct FSMTrackerResult { TArray<uint8> States, BossStates; FString Error, ObjectivesJson; };
struct FSMTracker {
    ~FSMTracker();
    void Initialize(void* Handle);
    void Tick(bool Enabled,bool Map,bool Items,const FString& Root,const FSMGameProfile& Slot,int VanillaSkill);
    FString Status;
    FString ObjectivesJson;
private:
    decltype(&sm_tracker_snapshot) Snapshot=nullptr;
    decltype(&sm_tracker_configure) Configure=nullptr;
    decltype(&sm_tracker_publish) Publish=nullptr;
    decltype(&sm_tracker_publish_bosses) PublishBosses=nullptr;
    decltype(&sm_objectives_snapshot) ObjectiveSnapshot=nullptr;
    decltype(&sm_tracker_publish_objectives) PublishObjectives=nullptr;
    using EvaluateFn=int(*)(const char*,const char*,char*,int);
    EvaluateFn Evaluate=nullptr;
    TFuture<FSMTrackerResult> Work;
    SmTrackerSnapshot Submitted{}, Last{};
    SmObjectiveSnapshot SubmittedObjectives{}, LastObjectives{};
    FString Context, SubmittedContext;
    bool HaveLast=false;
};
