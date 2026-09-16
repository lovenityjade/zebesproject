#include "SMHUD.h"
#include "Misc/Paths.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"

void ASMHUD::TickTitleTest(float Dt){
#if !UE_BUILD_SHIPPING
    if(!TitleTest)return;
    ++TitleTestTicks;TitleTestElapsed+=Dt;TitleTestInput=false;
    auto Fail=[&](const TCHAR* Reason){UE_LOG(LogTemp,Error,TEXT("SM_TITLE_TEST FAIL %s"),Reason);FPlatformMisc::RequestExitWithStatus(false,1);};
    if(TitleTestElapsed>100){Fail(TEXT("timeout"));return;}
    if(StartupWarning>=0 && Frame()!=0){Fail(TEXT("native clock advanced during warnings"));return;}
    if(TitleTestCase==1 && TitleTestElapsed>2 && TitleTestElapsed<3.5f && StartupWarning!=1){Fail(TEXT("held input skipped more than one warning"));return;}
    auto Capture=[&](int Bit,const TCHAR* Name){if(TitleTestCaptureBits&Bit)return;TitleTestCaptureBits|=Bit;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("title-%s-%s.png"),Widescreen?TEXT("wide"):TEXT("classic"),Name),false,false);
        UE_LOG(LogTemp,Display,TEXT("SM_TITLE_CAPTURE name=%s native_frame=%d phase=%d warning=%d"),Name,Frame(),TitlePhase,StartupWarning);};
    if(StartupWarning==0 && WarningSeconds>.5f)Capture(1,TEXT("warning-photosensitivity"));
    if(StartupWarning==1 && WarningSeconds>.5f)Capture(2,TEXT("warning-transparency"));
    if(StartupWarning<0){
        if(State()==0 && Frame()>45)Capture(4,TEXT("creator"));
        const float Age=(Frame()-TitlePhaseFrame)*(736.f/44100.f);
        if(State()==1 && TitlePhase==2){
            if(Age>.10f && Age<.30f)Capture(2048,TEXT("shot-fade-in"));
            if(Age>3.40f && Age<3.58f)Capture(4096,TEXT("shot-fade-out"));
            if(Age>3.70f && Age<4.00f)Capture(8192,TEXT("shot-black"));
        }
        if(State()==1 && Age>.7f){
            switch(TitlePhase){case 1:Capture(8,TEXT("2026"));break;case 2:Capture(16,TEXT("pan-samus"));break;
            case 3:Capture(32,TEXT("pan-bosses"));break;case 4:Capture(64,TEXT("pan-base"));break;
            case 5:Capture(128,TEXT("pullback"));break;case 6:Capture(256,TEXT("logo-reveal"));break;
            case 7:Capture(512,TEXT("ready"));break;default:break;}
        }
        if(TitleTestCase==2 && Frame()>=350 && Frame()<354)TitleTestInput=true;
        if(State()==1 && TitlePhase==7 && Age>1.2f)TitleTestInput=true;
        if(State()==4 && CinemaState(3)==4 && Brightness()==15){
            // Wait for the native texture upload, then leave enough rendered
            // frames for the screenshot before requesting engine shutdown.
            if(++TitleTestSaveTicks>=4)Capture(1024,TEXT("save-select"));
            if(TitleTestSaveTicks>=8){
                const int Required=TitleTestCase==2?1|2|4|8|512|1024:2047;
                if((TitleTestCaptureBits&Required)!=Required){Fail(TEXT("missing startup presentation stage"));return;}
                UE_LOG(LogTemp,Display,TEXT("SM_TITLE_TEST PASS case=%d captures=%d native_frame=%d seconds=%.2f"),TitleTestCase,TitleTestCaptureBits,Frame(),TitleTestElapsed);
                FPlatformMisc::RequestExitWithStatus(false,0);
            }
        }
    }
#endif
}
