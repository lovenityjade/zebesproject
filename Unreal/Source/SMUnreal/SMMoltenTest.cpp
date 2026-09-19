#include "SMHUD.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool ASMHUD::PrepareMoltenTest(){
#if !UE_BUILD_SHIPPING
    auto RamFn=reinterpret_cast<const uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
    auto Equipment=reinterpret_cast<int(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_all_equipment")));
    if(!RamFn || !Equipment)return false;
    uint8* Ram=const_cast<uint8*>(RamFn());
    auto Word=[&](int A){return int(Ram[A])|(int(Ram[A+1])<<8);};
    auto Put=[&](int A,int V){Ram[A]=V&255;Ram[A+1]=(V>>8)&255;};
    auto Wait=[&](int N){for(int I=0;I<N;I++)if(!Step(0))return false;return true;};
    for(int I=0;I<9000 && !(State()==4 && Word(0x727)==4);I++)if(!Step(I>180 && I%120<2?8:0))return false;
    if(State()!=4 || !Wait(3) || !Step(8) || !Wait(1))return false;
    for(int I=0;I<1000 && !(State()==2 && Word(0xde2)==3);I++)if(!Step(0))return false;
    if(State()!=2)return false;
    Put(0x79f,0);Put(0x78b,0);Put(0xd914,5);Put(0x998,6);
    for(int I=0;I<2000 && State()!=8;I++)if(!Step(0))return false;
    if(State()!=8 || !Wait(440) || !Equipment())return false;
    const int Rooms[]={0xafa3,0xaf14,0xb1e5};
    if(MoltenTest<=3){
        const int Ys[]={96,384,128};
        if(!TestRoom(Rooms[MoltenTest-1],192,Ys[MoltenTest-1]) || !Wait(900))return false;
    }else if(!Teleport(MoltenTest==4?5:2) || !Wait(440))return false;
    if(State()!=8 || Opcodes())return false;
    TestFrames=0;StartupWarning=-1;TitleInputFence=false;Accumulator=0;
    Atmosphere=EngineWeather=true;ImageScaling=1;
    MoltenTestPath=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("Molten-%d"),MoltenTest));
    IFileManager::Get().MakeDirectory(*MoltenTestPath,true);
    UE_LOG(LogTemp,Display,TEXT("SM_MOLTEN_READY case=%d room=%04x fx=%d surface=%d camera=%d,%d samus=%d,%d"),MoltenTest,Room(),FxType(),VisualState(54,0),CameraX(),CameraY(),SamusX(),SamusY());
    return true;
#else
    return false;
#endif
}
void ASMHUD::TickMoltenTest(){
#if !UE_BUILD_SHIPPING
    ++MoltenTestTicks;
    WeatherTime=1.f+FMath::Max(0,MoltenTestTicks-160)/30.f;
    if(MoltenTestTicks==1){
        RefreshScenePresentation();UpdateEnvironmentMask();
        TArray<uint8> Raw;Raw.Append(WideScene(),400*240*4);
        FFileHelper::SaveArrayToFile(Raw,*(MoltenTestPath/TEXT("scene.bgra")));
        FFileHelper::SaveArrayToFile(WideMaskPixels,*(MoltenTestPath/TEXT("mask.bgra")));
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"room\":%d,\"fx\":%d,\"surface\":%d,\"camera_y\":%d,\"cpu_opcodes\":%llu}"),Room(),FxType(),VisualState(54,0),CameraY(),Opcodes()),*(MoltenTestPath/TEXT("state.json")));
    }
    if(MoltenTestTicks==140 || MoltenTestTicks==160 || MoltenTestTicks==200)
        FScreenshotRequest::RequestScreenshot(MoltenTestPath/FString::Printf(TEXT("frame-%d.png"),MoltenTestTicks-120),false,false);
    if(MoltenTestTicks==220){UE_LOG(LogTemp,Display,TEXT("SM_MOLTEN_COMPLETE case=%d cpu_opcodes=%llu"),MoltenTest,Opcodes());PlayerOwner->ConsoleCommand(TEXT("quit"));}
#endif
}
