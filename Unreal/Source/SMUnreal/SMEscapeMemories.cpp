#include "SMHUD.h"
#include "SMSystemMenu.h"
#include "ImageUtils.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"

namespace {
struct FMemoryCue {const TCHAR* Portrait;const TCHAR* Text;const TCHAR* Reply;};
const FMemoryCue Memories[]={
 {TEXT("virginia_aran"),TEXT("You can do it, Samus. Just a little more."),TEXT("Mother...")},
 {TEXT("rodney_aran"),TEXT("You became a great kid, kiddo!"),TEXT("Father...")},
 {TEXT("old_bird"),TEXT("This is your new home, Samus. Zebes."),TEXT("I'm destroying my home...")},
 {TEXT("ridley"),TEXT("We took over the planet, and you're alone."),TEXT("I must persevere!")},
 {TEXT("adam_malkovich"),TEXT("You know your mission. Any objections, Lady?"),TEXT("Plenty, as always, Adam.")},
 {TEXT("mother_brain"),TEXT("You won't survive that much longer."),TEXT("I will destroy Zebes!")},
 {TEXT("baby_metroid"),TEXT("Mo...ther... Lives..."),TEXT("I WILL NOT FALL!!!")}
};
constexpr float FirstCue=8.f,CueSpacing=20.f,ExchangeDuration=5.8f,ReplyAt=3.15f;
}

UTexture2D* ASMHUD::EscapePortrait(const TCHAR* Name){
    if(auto* Existing=EscapePortraits.Find(Name))return *Existing;
    UTexture2D* Texture=FImageUtils::ImportFileAsTexture2D(FPaths::ProjectContentDir()/TEXT("Story/Portraits")/(FString(Name)+TEXT(".png")));
    if(Texture){Texture->Filter=TF_Nearest;Texture->SRGB=true;Texture->AddressX=TA_Clamp;Texture->AddressY=TA_Clamp;Texture->UpdateResource();}
    else UE_LOG(LogTemp,Warning,TEXT("SM_ESCAPE_PORTRAIT_MISSING %s"),Name);
    EscapePortraits.Add(Name,Texture);return Texture;
}

void ASMHUD::AdvanceEscapeMemories(float Dt){
    // Shared native Zebes destruction event: Vanilla, Vanilla+, Story and
    // Randomizer (including tablets). Never Ceres or the Boss Rush simulation.
    const bool Escape=VisualState(68,0) && State()>=8 && State()<=18 && !(RushInfo && RushInfo(0));
    if(!Escape){
        if(DialogueAutomatic)FinishDialogue(false);
        EscapeMemoryActive=false;EscapeMemoryTime=EscapeExchangeAge=EscapeMemoryBlend=0;
        EscapeExchange=EscapeLine=-1;return;
    }
    if(!EscapeMemoryActive){
        EscapeMemoryActive=true;
        for(const auto& Cue:Memories)EscapePortrait(Cue.Portrait);
        EscapePortrait(TEXT("zero_suit"));
        UE_LOG(LogTemp,Display,TEXT("SM_ESCAPE_MEMORIES_BEGIN frame=%d"),Frame());
    }
    // Door loads, map/pause and item messages cannot consume a readable line.
    if(State()!=8 || Paused || TeleportMenu || (MessageActive && MessageActive()) || (SystemMenu && SystemMenu->IsOpen()))return;
    EscapeMemoryTime+=Dt;
    int Cue=FMath::FloorToInt((EscapeMemoryTime-FirstCue)/CueSpacing);
    float Age=EscapeMemoryTime-(FirstCue+Cue*CueSpacing);
    if(Cue<0 || Cue>=UE_ARRAY_COUNT(Memories) || Age>=ExchangeDuration){
        if(DialogueAutomatic)FinishDialogue(true);
        EscapeMemoryBlend=0;return;
    }
    EscapeExchangeAge=Age;
    const int Line=Age<ReplyAt?0:1;
    if(Cue!=EscapeExchange || Line!=EscapeLine || !DialogueVisible){
        if(DialogueVisible && !DialogueAutomatic)return; // Do not steal a scripted dialogue.
        if(DialogueAutomatic)FinishDialogue(true);
        const auto& M=Memories[Cue];
        if(!OpenDialogue(Line?M.Reply:M.Text,EscapePortrait(Line?TEXT("zero_suit"):M.Portrait),true))return;
        EscapeExchange=Cue;EscapeLine=Line;DialogueTop=(Cue%2)==0;
        UE_LOG(LogTemp,Display,TEXT("SM_ESCAPE_MEMORY cue=%d line=%d top=%d time=%.3f frame=%d portrait=%s"),Cue+1,Line,DialogueTop,EscapeMemoryTime,Frame(),Line?TEXT("zero_suit"):M.Portrait);
    }
    DialogueAuto(Line?(Age-ReplyAt)/(ExchangeDuration-ReplyAt-.22f):FMath::Max(0.f,Age-.18f)/(ReplyAt-.18f));
    const float Ramp=FMath::Clamp(FMath::Min(Age/.25f,(ExchangeDuration-Age)/.25f),0.f,1.f);
    EscapeMemoryBlend=Ramp*Ramp*(3-2*Ramp);
}

const uint8* ASMHUD::ComposeEscapeMemory(const uint8* Source,int Width){
    if(!Source || !DialogueAutomatic || State()!=8 || EscapeMemoryBlend<=0 || (MessageActive && MessageActive()))return Source;
    EscapeMemoryPixels.SetNumUninitialized(Width*240*4);
    FMemory::Memcpy(EscapeMemoryPixels.GetData(),Source,EscapeMemoryPixels.Num());
    // Colour only the world, before lighting; HUD, timer and dialogue stay crisp.
    for(int Y=32;Y<224;Y++)for(int X=0;X<Width;X++){
        uint8* P=EscapeMemoryPixels.GetData()+(Y*Width+X)*4;
        const float Luma=P[2]*.299f+P[1]*.587f+P[0]*.114f;
        for(int C=0;C<3;C++)P[C]=FMath::Clamp(FMath::RoundToInt(FMath::Lerp(float(P[C]),Luma*.93f+12.f,.42f*EscapeMemoryBlend)),0,255);
    }
    return EscapeMemoryPixels.GetData();
}
