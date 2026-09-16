#include "SMHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "InputCoreTypes.h"
void ASMHUD::OpenRunRecap(){
    auto StateFn=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_route_state")));
    if(!StateFn || StateFn(0)<1)return;
    if(!RouteTexture){RouteTexture=UTexture2D::CreateTransient(528,320,PF_B8G8R8A8);RouteTexture->SRGB=true;RouteTexture->Filter=TF_Nearest;RouteTexture->UpdateResource();}
    RouteVisible=true;RoutePlaying=true;RouteCursor=0;RoutePosition=0;RouteLastUploaded=-1;RoutePreviousButtons=ReadButtons();AudioComponent->SetPaused(true);
}
bool ASMHUD::TickRunRecap(float Dt){
    auto StateFn=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_route_state")));
    if(!StateFn)return false;
    if(!RouteVisible && State()==40 && StateFn(1) && StateFn(0)>0 && !RouteAutoShown){RouteAutoShown=true;OpenRunRecap();}
    if(State()==8 && !StateFn(1))RouteAutoShown=false;
    if(!RouteVisible)return false;
    return AdvanceRunRecap(Dt,ReadButtons(),PlayerOwner->WasInputKeyJustPressed(EKeys::Escape));
}
bool ASMHUD::AdvanceRunRecap(float Dt,uint16 Buttons,bool Close){
    const uint16 Pressed=Buttons&~RoutePreviousButtons;RoutePreviousButtons=Buttons;
    if((Pressed&(2|8)) || Close){
        RouteVisible=false;AudioComponent->SetPaused(Paused);PlayerOwner->FlushPressedKeys();return true;
    }
    if(Pressed&256)RoutePlaying=!RoutePlaying;
    if(Pressed&1024)RouteSpeed=FMath::Max(0,RouteSpeed-1);
    if(Pressed&2048)RouteSpeed=FMath::Min(2,RouteSpeed+1);
    auto StateFn=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_route_state")));
    int Count=StateFn?StateFn(0):0;if(Count<1){RouteVisible=false;AudioComponent->SetPaused(Paused);return true;}
    if(Pressed&64){RouteCursor=FMath::Max(0,RouteCursor-FMath::Max(1,Count/100));RoutePosition=RouteCursor;}
    if(Pressed&128){RouteCursor=FMath::Min(Count-1,RouteCursor+FMath::Max(1,Count/100));RoutePosition=RouteCursor;}
    const float Speeds[]={20,120,600};
    if(RoutePlaying){RoutePosition+=Dt*Speeds[RouteSpeed];RouteCursor=FMath::Min(Count-1,int(RoutePosition));if(RouteCursor==Count-1)RoutePlaying=false;}
    if(RouteCursor!=RouteLastUploaded){
        auto PixelsFn=reinterpret_cast<const uint8*(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_route_pixels")));
        const uint8* Source=PixelsFn?PixelsFn(RouteCursor):nullptr;
        if(Source){
            auto* Copy=static_cast<uint8*>(FMemory::Malloc(528*320*4));FMemory::Memcpy(Copy,Source,528*320*4);
            auto* Region=new FUpdateTextureRegion2D(0,0,0,0,528,320);
            RouteTexture->UpdateTextureRegions(0,1,Region,528*4,4,Copy,[](uint8* D,const FUpdateTextureRegion2D* R){FMemory::Free(D);delete R;});RouteLastUploaded=RouteCursor;
        }
    }
    return true;
}
bool ASMHUD::DrawRunRecap(){
    if(!RouteVisible||!RouteTexture)return false;
    float Scale=FMath::Min(Canvas->SizeX/528.f,Canvas->SizeY/320.f);
    if(ImageScaling==1 && Scale>=1)Scale=FMath::FloorToFloat(Scale);
    float W=528*Scale,H=320*Scale;
    DrawTexture(RouteTexture,(Canvas->SizeX-W)/2,(Canvas->SizeY-H)/2,W,H,0,0,1,1,FLinearColor::White,BLEND_Opaque);return true;
}
