#include "SMNativeLibrary.h"
#include "SMRandomizer.h"
#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

bool SMRunNativeLifetimeSelfTest(const FString& Root,FString& Error) {
#if !UE_BUILD_SHIPPING && PLATFORM_WINDOWS
    if(!IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")))){Error=TEXT("Isolated test root required");return false;}
    const FString Path=FPaths::ConvertRelativePathToFull(Root/TEXT("Native/build/sm_native.dll"));
    const bool Legacy=FParse::Param(FCommandLine::Get(),TEXT("SMLegacyDllLifetime"));
    auto Open=[&]{return Legacy?FPlatformProcess::GetDllHandle(*Path):SMNativeLibrary::Open(Root);};
    void* Owner=Open();if(!Owner){Error=TEXT("Initial native load failed");return false;}
    auto Frame=reinterpret_cast<int(*)()>(FPlatformProcess::GetDllExport(Owner,TEXT("sm_frame")));
    if(!Frame){FPlatformProcess::FreeDllHandle(Owner);Error=TEXT("Missing frame export");return false;}
    const int Before=Frame();
    for(int I=0;I<4;I++){
        void* Temporary=Open();
        if(!Temporary){FPlatformProcess::FreeDllHandle(Owner);Error=TEXT("Temporary load failed");return false;}
        FPlatformProcess::FreeDllHandle(Temporary);
        // Check before calling any stale function pointer: the old code unloads
        // the HUD's module here on Windows, even with Owner still outstanding.
        if(!::GetModuleHandleW(*Path)){Error=TEXT("Temporary release unloaded the live HUD module");return false;}
        if(Frame()!=Before){FPlatformProcess::FreeDllHandle(Owner);Error=TEXT("Temporary load changed frame state");return false;}
        FSMSeedRequest Request;Request.Seed=14092026+I;Request.Skill=TEXT("regular");
        auto Worker=Async(EAsyncExecution::Thread,[Root,Request]{return FSMRandomizer::ValidateSettings(Root,Request);});
        auto Result=Worker.Get();
        if(!::GetModuleHandleW(*Path)){Error=TEXT("Settings worker unloaded the live HUD module");return false;}
        if(!Result.Ok || Frame()!=Before){FPlatformProcess::FreeDllHandle(Owner);Error=Result.Issues.Num()?Result.Issues[0].Message:TEXT("Settings validation or frame preservation failed");return false;}
    }
    FPlatformProcess::FreeDllHandle(Owner);
    if(::GetModuleHandleW(*Path)){Error=TEXT("Unbalanced native module references");return false;}
    return true;
#else
    Error=TEXT("Windows development test only");return false;
#endif
}
