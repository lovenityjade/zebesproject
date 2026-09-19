#include "SMHUD.h"
#include "SMSystemMenu.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ASMHUD::ResetToTitle() {
    CloseDialogue();
    Paused=false;TeleportMenu=false;
    AudioComponent->SetPaused(true);AudioWave->ResetAudio();Shutdown();
    Ready=Init(TCHAR_TO_UTF8(*CoreRomPath),TCHAR_TO_UTF8(*CoreSavePath))!=0;
    if(!Ready){Failure=UTF8_TO_TCHAR(Error());return;}
    if(SystemMenu)SystemMenu->ConfigureNativeGeneration();
    SetWidescreen(Widescreen);SetBorder(BorderExtension);SetAssistedWallJump(AssistedWallJump);SetAssistedSpaceJump(AssistedSpaceJump);
    SetEngineWeather(EngineWeather&&Atmosphere);SetCombatEffects(Atmosphere);
    VisualEffects.Reset();PresentationRoom=LastState=-1;Accumulator=0;AchievementPrimed=false;
    AchievementKills=AchievementPower=AchievementGrapple=AchievementScrew=0;
    AudioComponent->SetPaused(false);UE_LOG(LogTemp,Display,TEXT("SM_RESET_OK save=%s"),*CoreSavePath);
}
void ASMHUD::LoadAchievements() {
    FConfigFile Profile;Profile.Read(ProfilePath);
    int Legacy=0;Profile.GetInt(TEXT("Local"),TEXT("Unlocked"),Legacy);
    AchievementBits=uint64(Legacy)&31;
    FString Stored;uint64 Expanded=0;
    if(Profile.GetString(TEXT("Local"),TEXT("Unlocked64"),Stored)&&LexTryParseString(Expanded,*Stored))AchievementBits|=Expanded&((uint64(1)<<45)-1);
}
void ASMHUD::TrackAchievements() {
    const uint64 Before=AchievementBits;
    auto Candidates=reinterpret_cast<uint64(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_achievement_candidates")));
    if(Candidates)AchievementBits|=Candidates();
    int NewKills=0;
    if(PresentationKind()==1 && State()==8) {
        const int Items=PauseData(0),Beams=PauseData(2);
        if(AchievementPrimed) {
            if((Items&~AchievementItems)||(Beams&~AchievementBeams))AchievementBits|=1;
            NewKills=FMath::Max(0,VisualState(42,0)-AchievementKills);
            AchievementTotalKills+=NewKills;if(AchievementTotalKills>=10)AchievementBits|=2;
            if(VisualState(43,0)>AchievementPower)AchievementBits|=4;
            if(VisualState(44,0)>AchievementGrapple)AchievementBits|=8;
            if(VisualState(45,0)>AchievementScrew)AchievementBits|=16;
        }
        AchievementPrimed=true;AchievementItems=Items;AchievementBeams=Beams;
        AchievementKills=VisualState(42,0);AchievementPower=VisualState(43,0);AchievementGrapple=VisualState(44,0);AchievementScrew=VisualState(45,0);
    }else AchievementPrimed=false;
    if(Before!=AchievementBits) {
        auto Notify=reinterpret_cast<void(*)(uint64)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_achievement_notify64")));
        if(Notify)Notify(AchievementBits&~Before);
    }
    if(Before!=AchievementBits||NewKills) {
        FConfigFile Profile;Profile.Read(ProfilePath);
        Profile.SetString(TEXT("Local"),TEXT("Unlocked"),*FString::FromInt(int(AchievementBits&31)));
        Profile.SetString(TEXT("Local"),TEXT("Unlocked64"),*FString::Printf(TEXT("%llu"),(unsigned long long)AchievementBits));
        Profile.SetString(TEXT("Local"),TEXT("EnemyKills"),*FString::FromInt(AchievementTotalKills));
        if(!Profile.Write(ProfilePath))UE_LOG(LogTemp,Warning,TEXT("SM_PROFILE_WRITE_FAILED %s"),*ProfilePath);
    }
}
void ASMHUD::TickPauseTest() {
    const int T=++PauseTestTicks;
    auto Require=[this](bool Pass,const TCHAR* Name){if(!Pass){PauseTestFailed=true;UE_LOG(LogTemp,Error,TEXT("SM_NATIVE_PAUSE_FAIL %s"),Name);}};
    if(ObjectivePauseTest){
        auto ObjectivePage=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_objective_pause_state")));
        if(T==100 || T==200 || T==290 || T==390 || T==480 || T==590){
            const int Expected=T==200 || T==590?2:T==390?1:0;
            Require(State()==15 && PauseData(12)==Expected,TEXT("original map/objectives/equipment page"));
            Require(ObjectivePage && ObjectivePage(0)==(Expected==2) && ObjectivePage(3)==(Expected==2),TEXT("objective resources and stable page"));
            const FString Base=FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("objective-pause-%03d"),T);
            FScreenshotRequest::RequestScreenshot(Base+TEXT(".png"),false,false);
        }
        if(T==800){
            Require(State()==8 && !Opcodes() && ObjectivePage && !ObjectivePage(0),TEXT("objective page resumes native gameplay"));
            const FString Report=FString::Printf(TEXT("{\"passed\":%s,\"nativeObjectives\":true,\"mapAndEquipment\":true,\"directUnpause\":true,\"cpuOpcodes\":%llu}"),PauseTestFailed?TEXT("false"):TEXT("true"),Opcodes());
            FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("SMTests/objective-pause-verification.json")));
            UE_LOG(LogTemp,Display,TEXT("SM_OBJECTIVE_PAUSE_%s"),PauseTestFailed?TEXT("FAIL"):TEXT("PASS"));
            PlayerOwner->ConsoleCommand(TEXT("quit"));
        }
        return;
    }
    if(T==100 || T==200 || T==350 || T==520) {
        Require(State()==15,TEXT("native pause state"));
        Require(PauseData(11)==(T==350?1:0),TEXT("native map/equipment navigation"));
        const FString Base=FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("native-pause-%03d"),T);
        TArray<uint8> Ui;Ui.Append(WideHud(),400*240*4);
        FFileHelper::SaveArrayToFile(Ui,*(Base+TEXT("-ui.bgra")));
        FScreenshotRequest::RequestScreenshot(Base+TEXT(".png"),false,false);
    }
    if(T==700) {
        Require(State()==8 && !Opcodes(),TEXT("native resume"));
        const FString Report=FString::Printf(TEXT("{\"passed\":%s,\"nativePause\":true,\"nativeNavigation\":true,\"nativeResume\":true,\"cpuOpcodes\":%llu}"),PauseTestFailed?TEXT("false"):TEXT("true"),Opcodes());
        FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("SMTests/native-pause-verification.json")));
        UE_LOG(LogTemp,Display,TEXT("SM_NATIVE_PAUSE_%s"),PauseTestFailed?TEXT("FAIL"):TEXT("PASS"));
        PlayerOwner->ConsoleCommand(TEXT("quit"));
    }
}
