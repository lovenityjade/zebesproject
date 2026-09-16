#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "SMProfiles.h"
#include "SMRom.h"
bool SMRunSeedSharingSelfTest(const FString& Root,FString& Error);
bool SMRunTrackerSelfTest(const FString& Root,FString& Error);
bool SMRunNativeLifetimeSelfTest(const FString& Root,FString& Error);
class FSMGameModule : public FDefaultGameModuleImpl {
public:
    virtual void StartupModule() override {
#if !UE_BUILD_SHIPPING
        FString Root;
        if(FParse::Value(FCommandLine::Get(),TEXT("SMNativeLifetimeSelfTest="),Root)){
            FString Error;const bool Ok=SMRunNativeLifetimeSelfTest(Root,Error);
            UE_LOG(LogTemp,Display,TEXT("SM_NATIVE_LIFETIME_SELF_TEST %s %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Error);
            FPlatformMisc::RequestExitWithStatus(true,Ok?0:1);
        }
        if(FParse::Value(FCommandLine::Get(),TEXT("SMSeedSharingSelfTest="),Root)){
            FString Error;const bool Ok=SMRunSeedSharingSelfTest(Root,Error);
            UE_LOG(LogTemp,Display,TEXT("SM_SEED_SHARING_SELF_TEST %s %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Error);
            FPlatformMisc::RequestExitWithStatus(true,Ok?0:1);
        }
        if(FParse::Value(FCommandLine::Get(),TEXT("SMRomSelfTest="),Root)){
            FString Error;const bool Ok=SMRunRomSelfTest(Root,Error);
            UE_LOG(LogTemp,Display,TEXT("SM_ROM_SELF_TEST %s %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Error);
            FPlatformMisc::RequestExitWithStatus(true,Ok?0:1);
        }
        if(FParse::Value(FCommandLine::Get(),TEXT("SMAnimalsSelfTest="),Root)){
            FString Error;const bool Ok=SMRunGameplayProfileSelfTest(Root,Error,3);
            UE_LOG(LogTemp,Display,TEXT("SM_ANIMALS_SELF_TEST %s %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Error);
            FPlatformMisc::RequestExitWithStatus(true,Ok?0:1);
        }
        if(FParse::Value(FCommandLine::Get(),TEXT("SMMovementPickupSelfTest="),Root)){
            FString Error;const bool Ok=SMRunGameplayProfileSelfTest(Root,Error,2);
            UE_LOG(LogTemp,Display,TEXT("SM_MOVEMENT_PICKUP_PROFILE_SELF_TEST %s %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Error);
            FPlatformMisc::RequestExitWithStatus(true,Ok?0:1);
        }
        if(FParse::Value(FCommandLine::Get(),TEXT("SMSuitsProfileSelfTest="),Root)){
            FString Error;const bool Ok=SMRunGameplayProfileSelfTest(Root,Error,true);
            UE_LOG(LogTemp,Display,TEXT("SM_SUITS_PROFILE_SELF_TEST %s %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Error);
            FPlatformMisc::RequestExitWithStatus(true,Ok?0:1);
        }
        if(FParse::Value(FCommandLine::Get(),TEXT("SMGameplayProfileSelfTest="),Root)){
            FString Error;const bool Ok=SMRunGameplayProfileSelfTest(Root,Error);
            UE_LOG(LogTemp,Display,TEXT("SM_GAMEPLAY_PROFILE_SELF_TEST %s %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Error);
            FPlatformMisc::RequestExitWithStatus(true,Ok?0:1);
        }
        if(FParse::Value(FCommandLine::Get(),TEXT("SMTrackerSelfTest="),Root)){
            FString Error;const bool Ok=SMRunTrackerSelfTest(Root,Error);
            UE_LOG(LogTemp,Display,TEXT("SM_TRACKER_SELF_TEST %s %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Error);
            FPlatformMisc::RequestExitWithStatus(true,Ok?0:1);
        }
        if(FParse::Value(FCommandLine::Get(),TEXT("SMProfileSelfTest="),Root)){
            FString Error;const bool Ok=SMRunProfileSelfTest(Root,Error);
            UE_LOG(LogTemp,Display,TEXT("SM_PROFILE_SELF_TEST %s %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Error);
            FPlatformMisc::RequestExitWithStatus(true,Ok?0:1);
        }
#endif
    }
};
IMPLEMENT_PRIMARY_GAME_MODULE(FSMGameModule, SMUnreal, "SMUnreal");
