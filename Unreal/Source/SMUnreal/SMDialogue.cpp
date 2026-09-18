#include "SMHUD.h"
#include "SMLocalization.h"
#include "SMSystemMenu.h"
#include "Components/AudioComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "InputCoreTypes.h"

bool ASMHUD::ShowDialogue(const FString& Text,UTexture2D* Portrait){
    // No automatic trigger. Future story scripts own text, portraits and timing.
    if(!Ready || !CoreHandle || !PlayerOwner || !State || State()!=8 ||
       DialogueVisible || StartupWarning>=0 || Paused || TeleportMenu || RouteVisible ||
       (MessageActive && MessageActive()) || (SystemMenu && SystemMenu->IsOpen()))return false;
    if(!DialogueOpen){
        DialogueOpen=reinterpret_cast<int(*)(const char*,int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_dialogue_open")));
        DialogueTick=reinterpret_cast<void(*)(float,int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_dialogue_tick")));
        DialogueClose=reinterpret_cast<void(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_dialogue_close")));
        DialogueState=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_dialogue_state")));
        DialoguePixels=reinterpret_cast<const uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_dialogue_pixels")));
    }
    if(!DialogueOpen || !DialogueTick || !DialogueClose || !DialogueState || !DialoguePixels)return false;
    const int W=UseWide()?400:256;
    if(!DialogueTexture || DialogueWidth!=W){
        DialogueTexture=UTexture2D::CreateTransient(W,52,PF_B8G8R8A8);
        if(!DialogueTexture)return false;
        DialogueTexture->SRGB=true;DialogueTexture->Filter=TF_Nearest;
        DialogueTexture->AddressX=TA_Clamp;DialogueTexture->AddressY=TA_Clamp;
        DialogueTexture->UpdateResource();
    }
    if(!DialogueOpen(TCHAR_TO_UTF8(*SMLocalization::Text(Text)),W))return false;
    DialogueWidth=W;DialoguePortrait=Portrait;DialogueVisible=true;DialogueInputFence=false;
    Accumulator=0;
    if(AudioComponent)AudioComponent->SetPaused(true);
    return true;
}
bool ASMHUD::IsDialogueOpen() const {return DialogueVisible;}
void ASMHUD::FinishDialogue(bool Completed){
    if(!DialogueVisible)return;
    if(DialogueClose)DialogueClose();
    DialogueVisible=false;DialoguePortrait=nullptr;DialogueInputFence=true;Accumulator=0;
    if(AudioComponent)AudioComponent->SetPaused(Paused);
    OnDialogueClosed.Broadcast(Completed);
}
void ASMHUD::CloseDialogue(){FinishDialogue(false);}
bool ASMHUD::TickDialogue(float Dt){
    if(DialogueInputFence){
        // Do not carry the final confirm into a jump, shot or pause. Wait for
        // every gameplay button to be released before resuming the core.
        if(!ReadButtons() && !PlayerOwner->IsInputKeyDown(EKeys::SpaceBar))DialogueInputFence=false;
        Accumulator=0;return true;
    }
    if(!DialogueVisible)return false;
    const bool Confirm=(ReadButtons()&(256|8)) || PlayerOwner->IsInputKeyDown(EKeys::SpaceBar);
    DialogueTick(Dt,Confirm);
    if(!DialogueState(0)){FinishDialogue(DialogueState(4)!=0);return true;}
    // Simulation and its input remain suspended while presentation advances.
    Accumulator=0;return true;
}
void ASMHUD::DrawDialogue(float X,float Y,float Scale){
    if(!DialogueVisible || !DialogueTexture || !DialoguePixels)return;
    const int Bytes=DialogueWidth*52*4;
    auto* Copy=static_cast<uint8*>(FMemory::Malloc(Bytes));FMemory::Memcpy(Copy,DialoguePixels(),Bytes);
    auto* Region=new FUpdateTextureRegion2D(0,0,0,0,DialogueWidth,52);
    DialogueTexture->UpdateTextureRegions(0,1,Region,DialogueWidth*4,4,Copy,
      [](uint8* Data,const FUpdateTextureRegion2D* R){FMemory::Free(Data);delete R;});
    DrawTexture(DialogueTexture,X,Y+172*Scale,DialogueWidth*Scale,52*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
    if(DialoguePortrait){
        const float Ratio=32.f/FMath::Max(DialoguePortrait->GetSizeX(),DialoguePortrait->GetSizeY());
        const int W=FMath::Max(1,FMath::RoundToInt(DialoguePortrait->GetSizeX()*Ratio));
        const int H=FMath::Max(1,FMath::RoundToInt(DialoguePortrait->GetSizeY()*Ratio));
        DrawTexture(DialoguePortrait,X+((52-W)/2)*Scale,Y+(172+(52-H)/2)*Scale,W*Scale,H*Scale,
            0,0,1,1,FLinearColor::White,BLEND_Translucent);
    }
}
