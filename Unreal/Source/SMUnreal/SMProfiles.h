#pragma once
#include "CoreMinimal.h"
#include "SMRandomizer.h"

// A bank owns one SRAM; each native slot owns independent mode, request and seed.
// Historical single-seed profiles are imported without modifying their originals.
struct FSMGameProfile {
    FString Id, Name, Directory, SramPath, CreatedUtc;
    bool Randomized = false;
    bool Legacy = false;
    bool Generated = true;
    FSMSeedRequest Request;
    FSMSeedPlan Plan;
    TArray<FSMGameProfile> Slots;
};
class FSMProfiles {
public:
    static FString Root();
    static bool CreateBank(const FString& Name,const FSMSeedRequest* Request,const FSMGameProfile* Import,FSMGameProfile& Out,FString& Error);
    static bool ReplaceSlot(FSMGameProfile& Bank,int Slot,const FSMSeedRequest* Request,const FSMGameProfile* Copy,FString& Error,const TArray<uint8>* Sram=nullptr);
    static bool CompleteBankGeneration(FSMGameProfile& Bank,int Slot,const FSMSeedPlan& Plan,const TArray<uint8>& Sram,FString& Error);
    static bool ResolveSeed(FSMGameProfile& Slot,FSMSeedRequest& Out,FString& Error);
    static bool SaveRequest(const FSMGameProfile& Slot,const FSMSeedRequest& Request,FString& Error);
    static void List(TArray<FSMGameProfile>& Out, TArray<FString>& Warnings);
    static bool Read(FString Directory, FSMGameProfile& Out, FString& Error,bool Recover=true);
    static bool Create(const FString& Name, const FSMSeedPlan* Seed, FSMGameProfile& Out, FString& Error, const FSMSeedRequest* PendingRequest=nullptr);
    static bool CompleteGeneration(const FSMGameProfile& Draft,const FSMSeedPlan& Seed,FSMGameProfile& Out,FString& Error);
};
#if !UE_BUILD_SHIPPING
// Explicit command-line self-test, restricted to a caller-created isolated root.
bool SMRunProfileSelfTest(const FString& IsolatedRoot,FString& Error);
bool SMRunGameplayProfileSelfTest(const FString& IsolatedRoot,FString& Error,int Mode=0);
#endif
