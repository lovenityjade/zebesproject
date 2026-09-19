#include "SMHUD.h"
#include "Misc/ConfigCacheIni.h"
#include "UnrealClient.h"
#include "Components/AudioComponent.h"
#include "SMSystemMenu.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"

void ASMHUD::TickPlaytestCheck(){
#if !UE_BUILD_SHIPPING
    ++PlaytestTicks;
    if(AchievementCheck){
        auto Require=[this](bool Ok,const TCHAR* Name){if(!Ok){UE_LOG(LogTemp,Error,TEXT("SM_ACHIEVEMENTS_FAIL %s"),Name);PlayerOwner->ConsoleCommand(TEXT("quit"));}return Ok;};
        if(PlaytestTicks==1){
            if(!Require(State()==8,TEXT("gameplay fixture")))return;
            auto RamFn=reinterpret_cast<uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
            if(!Require(RamFn!=nullptr,TEXT("RAM fixture")))return;
            uint8* Ram=RamFn();
            auto Put=[Ram](int Address,uint16 Value){FMemory::Memcpy(Ram+Address,&Value,2);};
            FConfigFile Legacy;Legacy.SetString(TEXT("Local"),TEXT("Unlocked"),TEXT("24"));Legacy.Write(ProfilePath);
            LoadAchievements();if(!Require(AchievementBits==24,TEXT("legacy migration")))return;
            Legacy.SetString(TEXT("Local"),TEXT("Unlocked64"),*FString::Printf(TEXT("%llu"),(unsigned long long)((uint64(1)<<44)|24)));Legacy.Write(ProfilePath);LoadAchievements();
            Put(0x9a4,0);Put(0x9a8,0);Put(0x9c4,99);Put(0x9c8,0);FMemory::Memzero(Ram+0xd828,8);
            TrackAchievements();Put(0x9a4,5);Ram[0xd829]=1;TrackAchievements();
            const uint64 Expected=(uint64(1)<<44)|(uint64(1)<<5)|(uint64(1)<<7)|(uint64(1)<<15)|24;
            if(!Require((AchievementBits&Expected)==Expected,TEXT("live equipment/boss awards and high-bit preservation")))return;
            AchievementBits=0;LoadAchievements();if(!Require((AchievementBits&Expected)==Expected,TEXT("INI reload")))return;
            const FString FixtureProfile=ProfilePath;
            SettingsPath=CoreSavePath+TEXT(".presentation.ini");
            SystemMenu=MakeShared<FSMSystemMenu>(*this);SystemMenu->Initialize(false);
            ProfilePath=FixtureProfile;LoadAchievements();
            if(!Require(SystemMenu->AchievementIconsReady(),TEXT("all twenty badge textures")))return;
            SystemMenu->ShowAchievements(1);
        }
        if(PlaytestTicks==40)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests/achievements-vanilla.png"),true,false);
        if(PlaytestTicks==60)SystemMenu->ShowAchievements(2);
        if(PlaytestTicks==90)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests/achievements-randomizer.png"),true,false);
        if(PlaytestTicks==110){
            FFileHelper::SaveStringToFile(TEXT("{\"passed\":true,\"vanilla\":20,\"randomizer\":20,\"legacyMigration\":true,\"highBitRoundTrip\":true,\"liveNativeAwards\":true,\"icons\":20}"),*(FPaths::ProjectSavedDir()/TEXT("SMTests/achievements-runtime.json")));
            UE_LOG(LogTemp,Display,TEXT("SM_ACHIEVEMENTS_PASS"));PlayerOwner->ConsoleCommand(TEXT("quit"));
        }
        return;
    }
    if(SporeCheck){
        WeatherTime=PlaytestTicks/60.f; // Animate atmosphere over a frozen native frame.
        if(PlaytestTicks==1){
            if(!TestRoom || !TestAllEquipment() || !TestRoom(0x9dc7,128,640)){
                UE_LOG(LogTemp,Error,TEXT("SM_SPORE_FIXTURE_FAILED"));PlayerOwner->ConsoleCommand(TEXT("quit"));return;
            }
            for(int I=0;I<440;I++)Step(0);
            if(State()!=8 || Room()!=0x9dc7){UE_LOG(LogTemp,Error,TEXT("SM_SPORE_ROOM_FAILED"));PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
            EngineWeather=false;Atmosphere=true;
        }
        // Let the normal Tick upload the new room textures once before freezing.
        if(PlaytestTicks==2){Paused=true;AudioComponent->SetPaused(true);}
        if(PlaytestTicks==60)EngineWeather=true;
        const TCHAR* Name=PlaytestTicks==40?TEXT("spore-before"):PlaytestTicks==100?TEXT("spore-after"):PlaytestTicks==160?TEXT("spore-motion"):nullptr;
        if(Name)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("playtest-%s.png"),Name),false,false);
        if(PlaytestTicks==180){
            UE_LOG(LogTemp,Display,TEXT("SM_SPORE_RENDER_PASS room=%04x cpu=%llu"),Room(),Opcodes());
            PlayerOwner->ConsoleCommand(TEXT("quit"));
        }
        return;
    }
    if(RecapCheck){
        auto RouteState=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_route_state")));
        auto Fixture=reinterpret_cast<int(*)(int,int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_playtest")));
        auto RamFn=reinterpret_cast<uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
        auto Require=[this](bool Ok,const TCHAR* Name){if(!Ok){UE_LOG(LogTemp,Error,TEXT("SM_RECAP_CHECK_FAILED %s"),Name);PlayerOwner->ConsoleCommand(TEXT("quit"));}return Ok;};
        if(!Require(RouteState && Fixture && RamFn,TEXT("exports")))return;
        if(PlaytestTicks==1){
            if(!Require(Teleport(0)!=0,TEXT("teleport")))return;
            for(int I=0;I<500;I++)Step(0);
            for(int I=0;I<240;I++)Step(I<80||I>=160?128:64);
            if(!Require(RouteState(0)>4 && Fixture(0,0)==1 && RouteState(1),TEXT("record and finish")))return;
            // Advance the already validated native ending to its final state.
            // The normal TickRunRecap path must open automatically on this state.
            uint16 Ending=40;FMemory::Memcpy(RamFn()+0x998,&Ending,2);
        }
        if(PlaytestTicks==2){
            if(!Require(RouteVisible && RouteAutoShown,TEXT("automatic opening")))return;
            TArray<uint8> Before;Before.Append(RamFn(),131072);
            RoutePlaying=false;RoutePreviousButtons=0;
            AdvanceRunRecap(0,128,false);int Right=RouteCursor;
            AdvanceRunRecap(0,0,false);AdvanceRunRecap(0,64,false);
            if(!Require(Right>RouteCursor,TEXT("seek")))return;
            AdvanceRunRecap(0,0,false);AdvanceRunRecap(0,2048,false);
            if(!Require(RouteSpeed==2,TEXT("speed")))return;
            AdvanceRunRecap(0,0,false);AdvanceRunRecap(0,256,false);
            if(!Require(RoutePlaying,TEXT("play")))return;
            AdvanceRunRecap(.1f,0,false);AdvanceRunRecap(0,256,false);
            if(!Require(!RoutePlaying && RouteCursor>0,TEXT("pause")))return;
            if(!Require(FMemory::Memcmp(Before.GetData(),RamFn(),Before.Num())==0,TEXT("simulation unchanged")))return;
            UE_LOG(LogTemp,Display,TEXT("SM_RECAP_CONTROLS_PASS points=%d cpu=%llu"),RouteState(0),Opcodes());
        }
        if(PlaytestTicks==40)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests/playtest-recap.png"),false,false);
        if(PlaytestTicks==70){AdvanceRunRecap(0,2,false);if(!Require(!RouteVisible,TEXT("close")))return;}
        if(PlaytestTicks==71){
            if(!Require(!RouteVisible,TEXT("no automatic reopen")))return;
            UE_LOG(LogTemp,Display,TEXT("SM_RECAP_RENDER_PASS"));PlayerOwner->ConsoleCommand(TEXT("quit"));
        }
        return;
    }
    if(PlaytestTicks==1){
        if(!Teleport(0)){UE_LOG(LogTemp,Error,TEXT("SM_PLAYTEST_TELEPORT_FAILED"));return;}
        for(int I=0;I<500;I++)Step(0);
        TestAllEquipment();
    }
    if(PlaytestTicks==20){
        auto RamFn=reinterpret_cast<uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
        if(!RamFn || !Teleport(1)){UE_LOG(LogTemp,Error,TEXT("SM_PLAYTEST_SAVE_FIXTURE_FAILED"));PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
        for(int I=0;I<440;I++)Step(0);
        for(int I=0;I<25;I++)Step(64);
        auto Put=[&](int Address,uint16 Value){FMemory::Memcpy(RamFn()+Address,&Value,2);};
        Put(0xaf6,96);Put(0xafa,120);Put(0xb2e,0);Put(0xb2c,0);Put(0x1e75,0);
        for(int I=0;I<140 && !MessageActive();I++)Step(0);
        bool Prompt=MessageActive()!=0;for(int I=0;I<40;I++)Step(0);
        int Before=VisualEffects.SaveBursts;
        for(int I=0;I<700 && VisualEffects.SaveBursts==Before;I++){
            Step(I%60<5?256:0);
            if(PresentationKind()==1 && State()==8 && !MessageActive())VisualEffects.Advance(VisualState,Room(),FxType(),WaterY(),1.f/60);
        }
        if(!Prompt || VisualEffects.SaveBursts!=Before+1){UE_LOG(LogTemp,Error,TEXT("SM_PLAYTEST_REAL_SAVE_EFFECT_FAILED"));PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
        UE_LOG(LogTemp,Display,TEXT("SM_PLAYTEST_REAL_SAVE_EFFECT_PASS"));
    }
    if(PlaytestTicks==40){
        auto Notify=reinterpret_cast<void(*)(unsigned)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_achievement_notify")));
        if(Notify)Notify(1);
    }
    if(PlaytestTicks==110){
        // Finish the station's control lock and acknowledge its saved message
        // before moving to the running fixture.
        for(int I=0;I<600;I++)Step(I%60<5?256:0);
        // Keep the visual inspection outside Norfair's post-pickup rising
        // lava: the lava overlay would hide the blue light being inspected.
        if(!Teleport(0)){UE_LOG(LogTemp,Error,TEXT("SM_PLAYTEST_SPEED_ROOM_FAILED"));PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
        for(int I=0;I<440;I++)Step(0);
        TestAllEquipment();int Before=VisualEffects.SpeedFrames;
        uint16 RunButtons=64;for(int I=0;I<12;I++)if(VisualState(48,0)&(0x8000>>I))RunButtons|=1<<I;
        // Render a precharged boost, independently of this corridor's run-up.
        // The separate native regression tests charging from ordinary running.
        for(int I=0;I<10;I++)Step(RunButtons);
        auto RamFn=reinterpret_cast<uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
        if(!RamFn){UE_LOG(LogTemp,Error,TEXT("SM_PLAYTEST_SPEED_RAM_FAILED"));PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
        auto Put=[&](int Address,uint16 Value){FMemory::Memcpy(RamFn()+Address,&Value,2);};
        Put(0xb3c,1);Put(0xb3e,0x401);Put(0xb42,7);
        UE_LOG(LogTemp,Display,TEXT("SM_PLAYTEST_SPEED_START precharged=1 pos=%d,%d run=%x input=%x"),SamusX(),SamusY(),VisualState(48,0),RunButtons);
        for(int I=0;I<20;I++){
            Step(RunButtons);
            // Hold the presentation fixture's native boost contact flag. This
            // capture tests the renderer, not the charge-up/terrain traversal.
            Put(0xa6e,1);
            if(PresentationKind()==1 && State()==8 && !MessageActive())VisualEffects.Advance(VisualState,Room(),FxType(),WaterY(),1.f/60);
        }
        if(VisualEffects.SpeedFrames<Before+20){UE_LOG(LogTemp,Error,TEXT("SM_PLAYTEST_SPEED_EFFECT_FAILED room=%x pos=%d,%d state=%d message=%d input=%x"),Room(),SamusX(),SamusY(),State(),MessageActive(),VisualState(26,0));PlayerOwner->ConsoleCommand(TEXT("quit"));return;}
        UE_LOG(LogTemp,Display,TEXT("SM_PLAYTEST_SPEED_EFFECT_PASS frames=%d"),VisualEffects.SpeedFrames);
    }
    if(PlaytestTicks==220){Step(8);Step(0);for(int I=0;I<160;I++)Step(0);}
    if(PlaytestTicks==360){
        SettingsPath=CoreSavePath+TEXT(".presentation.ini");
        SystemMenu=MakeShared<FSMSystemMenu>(*this);SystemMenu->Initialize(false);SystemMenu->ShowAchievements();
    }
    const TCHAR* Name=PlaytestTicks==80?TEXT("save-toast"):PlaytestTicks==111?TEXT("speed-booster"):PlaytestTicks==300?TEXT("map"):PlaytestTicks==410?TEXT("achievements"):nullptr;
    if(Name){
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("playtest-%s.png"),Name),PlaytestTicks==410,false);
        UE_LOG(LogTemp,Display,TEXT("SM_PLAYTEST_CAPTURE %s state=%d cpu=%llu"),Name,State(),Opcodes());
    }
    if(PlaytestTicks==440)PlayerOwner->ConsoleCommand(TEXT("quit"));
#endif
}
