#include "SMTracker.h"
#include "SMNativeLibrary.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "../../../Native/sm_seed.h"

#if !UE_BUILD_SHIPPING
/* Exercise the actual Unreal async controller without a window or user saves.
 * RAM fixtures validate the bridge; native frame/render tests live in Scripts. */
bool SMRunTrackerSelfTest(const FString& Root,FString& Error){
    if(!IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")))){Error=TEXT("Missing isolated marker");return false;}
    void* Handle=SMNativeLibrary::Open(Root);
    if(!Handle){Error=TEXT("Missing native module");return false;}
#define FN(Name) auto Name=reinterpret_cast<decltype(&sm_##Name)>(FPlatformProcess::GetDllExport(Handle,TEXT("sm_" #Name)))
    FN(init);FN(shutdown);FN(seed_stage);FN(seed_clear);FN(simulation_ram);
#undef FN
    if(!init || !shutdown || !seed_stage || !seed_clear || !simulation_ram){Error=TEXT("Missing ABI");FPlatformProcess::FreeDllHandle(Handle);return false;}
    bool Ok=true;
    {
        FSMTracker Tracker;Tracker.Initialize(Handle);
        for(int Mode=0;Mode<3 && Ok;Mode++){
            FSMGameProfile Profile;Profile.Id=FString::Printf(TEXT("tracker-fixture-%d"),Mode);Profile.Randomized=Mode!=0;
            if(Mode){
                FString Json;
                Ok=FFileHelper::LoadFileToString(Json,*(Root/TEXT("results")/FString::Printf(TEXT("seed-%d.json"),14092025+Mode))) && FSMRandomizer::ReadPlan(Json,Profile.Plan,Error);
                if(!Ok)break;
                TArray<SmSeedItem> Items;for(const auto& I:Profile.Plan.Items)Items.Add({uint32(I.Address),uint16(I.Plm),uint16(I.Kind)});
                Ok=seed_stage(Items.GetData(),100,TCHAR_TO_UTF8(*Profile.Plan.Fingerprint))!=0;if(!Ok)break;
            }
            const FString Directory=Root/TEXT("tracker-ue-fixtures")/(Mode?Profile.Plan.Fingerprint:TEXT("vanilla"));
            IFileManager::Get().MakeDirectory(*Directory,true);
            const FString Save=Directory/TEXT("sram.dat");
            Ok=init(TCHAR_TO_UTF8(*(Root/TEXT("roms/Super Metroid (Japan, USA) (En,Ja).sfc"))),TCHAR_TO_UTF8(*Save))!=0;
            if(!Ok){Error=TEXT("Native fixture init failed");break;}
            uint8* Ram=const_cast<uint8*>(simulation_ram());
            auto Word=[&](int Address,uint16 Value){FMemory::Memcpy(Ram+Address,&Value,2);};
            Word(0x998,8);Word(0x952,Mode);Word(0x9c4,99);
            Tracker.Tick(false,true,true,Root,Profile,0);
            Ok=Tracker.Status==TEXT("Disabled");
            for(int Revision=0;Revision<3 && Ok;Revision++){
                Word(0x9a4,Revision==0?0:4);Word(0x9c8,Revision==2?5:0);
                Tracker.Tick(true,true,true,Root,Profile,0);
                const double Deadline=FPlatformTime::Seconds()+10;
                while(!Tracker.Status.StartsWith(TEXT("Live")) && FPlatformTime::Seconds()<Deadline){
                    FPlatformProcess::Sleep(.01f);Tracker.Tick(true,true,true,Root,Profile,0);
                }
                Ok=Tracker.Status.StartsWith(TEXT("Live"));
                if(!Ok)Error=Tracker.Status;
            }
            // Queue a query, invalidate the session immediately, then stop.
            Word(0x9a4,0x1004);Tracker.Tick(true,true,true,Root,Profile,0);
            Tracker.Tick(false,true,true,Root,Profile,0);
            shutdown();seed_clear();
            UE_LOG(LogTemp,Display,TEXT("SM_TRACKER_UE_FIXTURE mode=%d result=%s"),Mode,Ok?TEXT("PASS"):TEXT("FAIL"));
        }
    } // join the pending worker before unloading its native/Python runtime
    FPlatformProcess::FreeDllHandle(Handle);
    return Ok;
}
#endif
