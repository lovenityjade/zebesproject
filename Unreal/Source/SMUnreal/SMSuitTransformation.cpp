#include "SMHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ASMHUD::AdvanceSuitTransformation(float Dt){
    const int Kind=VisualState(49,0),Phase=VisualState(50,0);
    if(State()!=8 || (SuitKind && SuitRoom!=Room())){
        SuitKind=0;SuitFade=0;return;
    }
    if(Kind){
        if(Kind!=SuitKind){
            SuitKind=Kind;SuitRoom=Room();SuitTime=0;SuitReleaseTime=-1;
        }
        SuitPhase=Phase;SuitTime+=Dt;SuitFade=1;
        SuitPosition=FVector2D(SamusX(),SamusY()-5);
        // Stage 3 awards the native item and loads its palette. Stage 4 is the
        // first frame with the new suit; never award items from presentation.
        if(Phase>=4 && SuitReleaseTime<0){
            SuitReleaseTime=0;
            UE_LOG(LogTemp,Display,TEXT("SM_SUIT_RELEASE kind=%d native_frame=%d items=%04x"),Kind,Frame(),VisualState(36,0));
        }else if(SuitReleaseTime>=0)SuitReleaseTime+=Dt;
    }else if(SuitKind){
        SuitTime+=Dt;SuitReleaseTime+=Dt;SuitFade=FMath::Max(0.f,SuitFade-Dt*2.5f);
        if(SuitFade==0)SuitKind=0;
    }
}

void ASMHUD::DrawSuitTransformation(float X,float Y,float Scale){
    if(!Atmosphere || !SuitKind || State()!=8 || !Canvas || MessageActive())return;
    if(!SuitGlow){
        SuitGlow=UTexture2D::CreateTransient(128,128,PF_B8G8R8A8);
        SuitGlow->Filter=TF_Bilinear;SuitGlow->SRGB=false;SuitGlow->NeverStream=true;
        auto& Mip=SuitGlow->GetPlatformData()->Mips[0];
        auto* GlowPixels=static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
        for(int J=0;J<128;J++)for(int I=0;I<128;I++){
            const float R=FMath::Square((I-63.5f)/63.5f)+FMath::Square((J-63.5f)/63.5f);
            const uint8 V=uint8(255*FMath::Max(0.f,(FMath::Exp(-R*5)-FMath::Exp(-5.f))/(1-FMath::Exp(-5.f))));
            GlowPixels[J*128+I]=FColor(V,V,V,255); // Additive Canvas uses RGB, not alpha.
        }
        Mip.BulkData.Unlock();SuitGlow->UpdateResource();
    }
    const float Width=UseWide()?400.f:256.f;
    const FVector2D Center(SuitPosition.X-CameraX()+(UseWide()?72:0),SuitPosition.Y-CameraY());
    const bool Released=SuitReleaseTime>=0;
    const float T=SuitTime,R=FMath::Max(0.f,SuitReleaseTime);
    const float Charge=FMath::Clamp(T/1.07f,0.f,1.f);
    const float Tail=Released?FMath::Exp(-R*2.3f):1.f;
    const float Light=FMath::Clamp(FlashStrength,0.f,1.f);
    const FLinearColor SuitColor=SuitKind==1?FLinearColor(1.f,.42f,.055f):FLinearColor(.58f,.16f,1.f);
    const FLinearColor Cyan(.1f,.85f,1.f);
    auto Glow=[&](FVector2D P,float Radius,FLinearColor Color,float Power){
        if(Radius<=0 || Power<=0)return;
        const float L=FMath::Max(0.f,float(P.X)-Radius),Top=FMath::Max(32.f,float(P.Y)-Radius);
        const float Right=FMath::Min(Width,float(P.X)+Radius),Bottom=FMath::Min(224.f,float(P.Y)+Radius);
        if(Right<=L || Bottom<=Top)return;
        Color*=Power*SuitFade;
        DrawTexture(SuitGlow,X+L*Scale,Y+Top*Scale,(Right-L)*Scale,(Bottom-Top)*Scale,
            (L-P.X+Radius)/(2*Radius),(Top-P.Y+Radius)/(2*Radius),(Right-L)/(2*Radius),(Bottom-Top)/(2*Radius),Color,BLEND_Additive);
    };
    auto InBounds=[&](FVector2D P){return P.X>=0 && P.X<Width && P.Y>=32 && P.Y<224;};
    auto Line=[&](FVector2D A,FVector2D B,FLinearColor Color,float Power,float Thickness){
        if(!InBounds(A) || !InBounds(B) || Power<=0)return;
        // Include the ribbon's feathered edges in the HUD/viewport guard.
        if(FMath::Min(A.X,B.X)<Thickness || FMath::Max(A.X,B.X)>Width-Thickness ||
           FMath::Min(A.Y,B.Y)<32+Thickness || FMath::Max(A.Y,B.Y)>224-Thickness)return;
        const FVector2D D=B-A,Mid=(A+B)*.5;
        const float Length=D.Size()+Thickness;
        Color*=Power*SuitFade;Color.A=1;
        // Canvas line primitives ignore additive blending on this renderer.
        // A rotated soft ribbon really adds light instead of painting dark ink.
        DrawTexture(SuitGlow,X+(Mid.X-Length*.5f)*Scale,Y+(Mid.Y-Thickness*.5f)*Scale,
            Length*Scale,Thickness*Scale,0,0,1,1,Color,BLEND_Additive,1,false,
            FMath::RadiansToDegrees(FMath::Atan2(float(D.Y),float(D.X))),FVector2D(.5,.5));
    };
    // Perspective-projected XYZ paths: particles nearer the viewer grow and
    // speed up, while rings rotate through depth around the unchanged sprite.
    auto Project=[&](FVector P){
        const float Perspective=240.f/FMath::Max(90.f,240.f+float(P.Z));
        return Center+FVector2D(P.X,P.Y)*Perspective;
    };
    const float Dim=(Released?.3f*Tail:.30f*Charge)*SuitFade;
    DrawRect(FLinearColor(0,0,0,Dim),X,Y+32*Scale,Width*Scale,192*Scale);
    Glow(Center,Released?90+R*240:80+35*Charge,SuitColor,(Released?.9f*Tail:.12f+.35f*Charge)*Light);
    // The energy shell contracts before the discharge. Thin arcs leave the
    // native armor legible; their orientations are intentionally asymmetric.
    for(int Orbit=0;Orbit<3;Orbit++){
        const float Radius=Released?25+R*(105+Orbit*36):92-64*Charge+Orbit*5;
        const float Tilt=.45f+Orbit*.85f,Spin=T*(1.1f+Orbit*.24f)+Orbit*1.8f;
        FVector2D Prev;
        for(int J=0;J<=96;J++){
            const float A=J*(2*PI/96)+Spin;
            const FVector V(Radius*FMath::Cos(A),Radius*FMath::Sin(A)*FMath::Cos(Tilt),Radius*FMath::Sin(A)*FMath::Sin(Tilt));
            const FVector2D P=Project(V);
            if(J && J%24<19)Line(Prev,P,Orbit==1?Cyan:SuitColor,(Released?.60f*Tail:.32f+.40f*Charge)*Light,.48f);
            Prev=P;
        }
    }
    // Continuous incoming streams, then a single expanding cloud; deterministic
    // trajectories avoid frame-rate-dependent randomness and temporal popping.
    for(int I=0;I<190;I++){
        const float Seed=FMath::Frac(I*.61803398875f),Seed2=FMath::Frac(I*.41421356237f);
        const float A=I*2.39996323f;
        FVector V;float Alpha,Size;
        if(!Released){
            const float Life=FMath::Frac(Seed+T*(.45f+Seed2*.6f));
            const float Radius=14+FMath::Square(1-Life)*(180+80*Seed2);
            const float Angle=A+T*2.6f+Life*2;
            V=FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius*.74f,FMath::Sin(A*1.7f+T*3)*Radius*.6f);
            Alpha=FMath::Sin(Life*PI)*(.32f+.68f*Charge);Size=.4f+Seed2*.6f;
        }else{
            const float Speed=65+Seed2*240,Radius=10+Speed*R;
            V=FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius*.8f,FMath::Sin(A*1.7f)*FMath::Min(Radius*.55f,125.f));
            Alpha=FMath::Exp(-R*(1.3f+Seed))*(.5f+.5f*Seed2);Size=.6f+Seed2;
        }
        const FVector2D P=Project(V);
        const float DepthScale=240.f/FMath::Max(90.f,240.f+float(V.Z));
        const FLinearColor Color=I%4==0?Cyan:SuitColor;
        Glow(P,Size*DepthScale*5,Color,Alpha*(.3f+.7f*Light));
        Glow(P,Size*DepthScale,FLinearColor(.8f,.92f,1.f),Alpha*(.2f+.8f*Light));
        if(Released && R<.8f){
            const FVector2D Back=Center+(P-Center)*.9f;
            Line(Back,P,Color,Alpha*.45f*Light,.4f);
        }
    }
    // Electric filaments curve from the outer field into armor contact points.
    // Smoothly moving noise, instead of an alternating full-screen strobe.
    const float ArcPower=(Released?FMath::Exp(-R*5):Charge*Charge)*Light;
    for(int I=0;I<9;I++){
        const float A=I*2*PI/9+T*.52f;
        const FVector2D Start=Center+FVector2D(FMath::Cos(A)*(Released?75:65+45*(1-Charge)),FMath::Sin(A)*70);
        const FVector2D End=Center+FVector2D((I%3-1)*8,(I%4-1.5f)*9);
        const FVector2D D=End-Start,N(-D.Y,D.X);
        FVector2D Prev=Start;
        for(int J=1;J<=12;J++){
            const float U=J/12.f;
            const float Noise=FMath::Sin(J*17.7f+I*5.3f+T*23)*FMath::Sin(U*PI)*.13f;
            const FVector2D P=Start+D*U+N*Noise;
            Line(Prev,P,SuitColor,ArcPower*.35f,6.f);
            Line(Prev,P,Cyan,ArcPower*.8f,2.f);
            Line(Prev,P,FLinearColor(.85f,.95f,1),ArcPower*1.3f,.8f);
            Prev=P;
        }
    }
    if(Released){
        // One bloom pulse at the actual palette switch, followed by two
        // expanding pressure fronts. FlashStrength=0 removes the pulse.
        const float Pulse=FMath::Exp(-R*11.f)*Light*SuitFade;
        DrawRect(FLinearColor(.90f,.96f,1.f,Pulse*.93f),X,Y+32*Scale,Width*Scale,192*Scale);
        Glow(Center,35+R*270,FLinearColor(.9f,.95f,1.f),Pulse*2);
        for(int Wave=0;Wave<2;Wave++){
            const float Age=R-Wave*.12f;if(Age<0)continue;
            const float Radius=12+Age*340,Power=FMath::Exp(-Age*4.f)*Light;
            FVector2D Prev=Center+FVector2D(Radius,0);
            for(int J=1;J<=160;J++){
                const float A=J*2*PI/160;
                const FVector2D P=Center+FVector2D(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius*.86f);
                Line(Prev,P,SuitColor,Power*.55f,9.f);
                Line(Prev,P,Wave?Cyan:FLinearColor(.85f,.95f,1),Power*1.5f,2.5f);
                Prev=P;
            }
        }
    }else Glow(Center,24+Charge*14,Cyan,.3f*Charge*Light);
}

// Developer-only, reproducible recording of the real native routines rendered
// by Unreal. Requires an isolated root and a copied SRAM (SMTests). No trigger
// or inventory mutation is exposed in normal gameplay or Shipping builds.
bool ASMHUD::PrepareSuitCapture(){
#if !UE_BUILD_SHIPPING
    auto RamFn=reinterpret_cast<const uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
    auto Test=reinterpret_cast<int(*)(int,int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_suit_pickup")));
    if(!RamFn || !Test)return false;
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
    if(State()!=8 || !Wait(440))return false;
    // Set boss-cleared bits in this fixture so Gravity's room is powered.
    Ram[0xd829]|=1;Ram[0xd82b]|=1;
    const int Target=SuitTest==1?0xa6e2:0xce40;
    if(!TestRoom(Target,120,160))return false;
    for(int I=0;I<1000;I++){if(!Step(0))return false;if(State()==8 && Room()==Target)break;}
    // Room-preview loading runs the Zebes arrival palette FX. Let it finish
    // before acquisition so it cannot restore Power Suit over the new palette.
    if(State()!=8 || Room()!=Target || !Wait(440) || !Test(SuitTest,0))return false;
    TestFrames=0;StartupWarning=-1;TitleInputFence=false;Accumulator=0;
    SuitCapturePath=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SMTests")/(SuitTest==1?TEXT("SuitVaria"):TEXT("SuitGravity")));
    IFileManager::Get().MakeDirectory(*SuitCapturePath,true);
    FParse::Value(FCommandLine::Get(),TEXT("SMFlashStrength="),FlashStrength);FlashStrength=FMath::Clamp(FlashStrength,0.f,1.f);
    UE_LOG(LogTemp,Display,TEXT("SM_SUIT_CAPTURE_READY kind=%d room=%04x path=%s"),SuitTest,Room(),*SuitCapturePath);
    return true;
#else
    return false;
#endif
}
void ASMHUD::CaptureSuitFrame(){
#if !UE_BUILD_SHIPPING
    if(SuitCaptureFrame==90){
        auto Test=reinterpret_cast<int(*)(int,int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_suit_pickup")));
        if(!Test || !Test(SuitTest,1)){Failure=TEXT("Suit capture trigger rejected");Ready=false;return;}
    }
    if(SuitCaptureFrame<360){
        FScreenshotRequest::RequestScreenshot(SuitCapturePath/FString::Printf(TEXT("frame-%04d.png"),SuitCaptureFrame),false,false);
    }else{
        const bool Passed=State()==8 && !VisualState(49,0) && (VisualState(36,0)&(SuitTest==1?1:32)) && !Opcodes();
        FFileHelper::SaveArrayToFile(SuitCaptureAudio,*(SuitCapturePath/TEXT("audio.s16le")));
        UE_LOG(LogTemp,Display,TEXT("SM_SUIT_CAPTURE_COMPLETE passed=%d kind=%d frames=%d items=%04x cpu_opcodes=%llu"),Passed,SuitTest,SuitCaptureFrame,VisualState(36,0),Opcodes());
        PlayerOwner->ConsoleCommand(TEXT("quit"));
    }
    ++SuitCaptureFrame;
#endif
}
