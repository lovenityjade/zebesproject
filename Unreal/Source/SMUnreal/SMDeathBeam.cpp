#include "SMHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

// All paths and light are presentation state. The native boss owns timing,
// Samus movement, damage, sound and the baby's entrance.
void ASMHUD::AdvanceDeathBeam(float Dt){
    const int Phase=VisualState(51,0);
    if(State()!=8 || Room()!=0xdd58){DeathBeamPhase=0;DeathBeamFade=0;return;}
    if(Phase){
        if(!DeathBeamPhase){DeathBeamTime=0;DeathBeamStrike=-1;}
        if(Phase>=2 && DeathBeamStrike<0){
            DeathBeamStrike=0;DeathBeamRecordedStrike=true;
            UE_LOG(LogTemp,Display,TEXT("SM_DEATH_BEAM_STRIKE style=%d frame=%d"),DeathBeamStyle,Frame());
        }
        DeathBeamSource=FVector2D(VisualState(52,0),VisualState(53,0));
        DeathBeamTarget=FVector2D(SamusX(),SamusY()-2);
        DeathBeamFade=1;
    }else DeathBeamFade=FMath::Max(0.f,DeathBeamFade-Dt*2.f);
    DeathBeamPhase=Phase;DeathBeamTime+=Dt;
    if(DeathBeamStrike>=0)DeathBeamStrike+=Dt;
}

// Enhance the body's existing cycling colors in-place on a presentation copy.
// BG2 is exclusively Mother Brain's body in this room during the native beam.
// This is surface color, not emission: no bloom source, halo or added room light.
const uint8* ASMHUD::ColorMotherBrainBody(const uint8* SourcePixels,const uint8* Meta,int Width){
    if(!Atmosphere || State()!=8 || Room()!=0xdd58 || VisualState(51,0)<2 || !SourcePixels || !Meta)return SourcePixels;
    TArray<uint8>& Output=Width==400?MotherBrainWidePixels:MotherBrainPixels;
    Output.SetNumUninitialized(Width*240*4);
    FMemory::Memcpy(Output.GetData(),SourcePixels,Output.Num());
    for(int Y=32;Y<224;Y++)for(int X=0;X<Width;X++){
        const int I=(Y*Width+X)*4;
        if(!Meta[I+3] || Meta[I]!=1)continue;
        const float B=SourcePixels[I],G=SourcePixels[I+1],R=SourcePixels[I+2];
        const float L=.2126f*R+.7152f*G+.0722f*B;
        if(L<4)continue;
        // Broad, slow waves modulate saturation across the shaded flesh.
        // Keep the original hue and luminance; preserve black outlines/highlights.
        const float WX=X+CameraX()-(Width==400?72:0)-DeathBeamSource.X;
        const float WY=Y+CameraY()-DeathBeamSource.Y;
        const float Wave=FMath::Sin(WX*.045f+WY*.033f-DeathBeamTime*1.4f)*
                         FMath::Sin(WY*.024f+DeathBeamTime*.7f);
        const float Saturation=1.20f+.08f*Wave;
        for(int C=0;C<3;C++)Output[I+C]=uint8(FMath::Clamp(FMath::RoundToInt(L+(SourcePixels[I+C]-L)*Saturation),0,255));
    }
#if !UE_BUILD_SHIPPING
    if(DeathBeamTest && DeathBeamCaptureFrame==420 && Width==400){
        TArray<uint8> Raw;Raw.Append(SourcePixels,Output.Num());
        FFileHelper::SaveArrayToFile(Raw,*(DeathBeamCapturePath/TEXT("body-before.bgra")));
        Raw.Reset();Raw.Append(Meta,Output.Num());
        FFileHelper::SaveArrayToFile(Raw,*(DeathBeamCapturePath/TEXT("body-mask.bgra")));
        FFileHelper::SaveArrayToFile(Output,*(DeathBeamCapturePath/TEXT("body-after.bgra")));
    }
#endif
    return Output.GetData();
}

void ASMHUD::DrawDeathBeam(float X,float Y,float Scale){
    if(!Atmosphere || DeathBeamFade<=0 || State()!=8 || Room()!=0xdd58 || !Canvas)return;
    if(!DeathBeamGlow){
        DeathBeamGlow=UTexture2D::CreateTransient(128,128,PF_B8G8R8A8);
        DeathBeamGlow->Filter=TF_Bilinear;DeathBeamGlow->SRGB=false;DeathBeamGlow->NeverStream=true;
        auto& M=DeathBeamGlow->GetPlatformData()->Mips[0];auto* P=static_cast<FColor*>(M.BulkData.Lock(LOCK_READ_WRITE));
        for(int J=0;J<128;J++)for(int I=0;I<128;I++){
            const float R=FMath::Square((I-63.5f)/63.5f)+FMath::Square((J-63.5f)/63.5f);
            const uint8 V=uint8(255*FMath::Max(0.f,(FMath::Exp(-R*5)-FMath::Exp(-5.f))/(1-FMath::Exp(-5.f))));
            P[J*128+I]=FColor(V,V,V,255);
        }
        M.BulkData.Unlock();DeathBeamGlow->UpdateResource();
    }
    const float W=UseWide()?400.f:256.f,T=DeathBeamTime;
    const float Flash=FMath::Clamp(FlashStrength,0.f,1.f),Fade=DeathBeamFade;
    // Reducing flashes must not make a damaging attack invisible.
    const float Light=.25f+.75f*Flash;
    const FVector2D Camera(CameraX()-(UseWide()?72:0),CameraY());
    const FVector2D A=DeathBeamSource-Camera,B=DeathBeamTarget-Camera;
    const FVector2D D=B-A,Axis=D.GetSafeNormal(),N(-Axis.Y,Axis.X);
    const bool Cataclysm=DeathBeamStyle==3;
    // Continuous growth until the native script fires; no synthetic firing timer.
    const float Charge=Cataclysm?1.f-FMath::Exp(-T/5.f):FMath::Clamp(T/7.8f,0.f,1.f);
    const bool Firing=DeathBeamPhase>=2;
    const float Strike=FMath::Max(0.f,DeathBeamStrike);
    const float Power=(Firing?1.f:Charge)*Fade;
    const FLinearColor Main=DeathBeamStyle==1?FLinearColor(.25f,.55f,1):DeathBeamStyle==2?FLinearColor(.48f,.12f,1):FLinearColor(1,.16f,.025f);
    const FLinearColor Accent=DeathBeamStyle==1?FLinearColor(1,.18f,.48f):DeathBeamStyle==2?FLinearColor(.12f,.95f,1):FLinearColor(1,.7f,.1f);
    auto Glow=[&](FVector2D P,float Radius,FLinearColor C,float Strength){
        const float L=FMath::Max(0.f,float(P.X)-Radius),Top=FMath::Max(32.f,float(P.Y)-Radius);
        const float R=FMath::Min(W,float(P.X)+Radius),Bottom=FMath::Min(224.f,float(P.Y)+Radius);
        if(R<=L || Bottom<=Top || Strength<=0 || Radius<=0)return;
        C*=Strength*Fade;
        DrawTexture(DeathBeamGlow,X+L*Scale,Y+Top*Scale,(R-L)*Scale,(Bottom-Top)*Scale,
            (L-P.X+Radius)/(2*Radius),(Top-P.Y+Radius)/(2*Radius),(R-L)/(2*Radius),(Bottom-Top)/(2*Radius),C,BLEND_Additive);
    };
    auto Line=[&](FVector2D P,FVector2D Q,FLinearColor C,float Strength,float Width){
        if(Strength<=0)return;
        // Reject ribbons crossing the HUD; all broad illumination is UV-clipped.
        const float Margin=Width*.5f;
        if(FMath::Min(P.X,Q.X)<Margin || FMath::Max(P.X,Q.X)>W-Margin || FMath::Min(P.Y,Q.Y)<32+Margin || FMath::Max(P.Y,Q.Y)>224-Margin)return;
        const FVector2D V=Q-P,Mid=(P+Q)*.5;const float Length=V.Size()+Width;
        C*=Strength*Fade;
        DrawTexture(DeathBeamGlow,X+(Mid.X-Length*.5f)*Scale,Y+(Mid.Y-Width*.5f)*Scale,Length*Scale,Width*Scale,0,0,1,1,C,BLEND_Additive,1,false,
            FMath::RadiansToDegrees(FMath::Atan2(float(V.Y),float(V.X))),FVector2D(.5,.5));
    };
    // Slow exposure compression makes the discharge feel bright while preserving
    // the native silhouettes. No continuous full-screen strobe.
    DrawRect(FLinearColor(0,0,0,(Firing?.32f:.22f*Charge)*Fade*Flash),X,Y+32*Scale,W*Scale,192*Scale);
    Glow(A,Cataclysm?24+Charge*96:85,Main,(Cataclysm?Power*.85f:.25f+Power*.5f)*Light);
    const float Aperture=Firing?18:45-30*Charge;
    for(int Ring=0;!Cataclysm && Ring<3;Ring++){
        FVector2D Prev;
        for(int J=0;J<=72;J++){
            const float Angle=J*2*PI/72+T*(Ring%2?-1:1)+Ring;
            const float Radius=Aperture+Ring*6;
            const FVector2D P=A+N*(FMath::Sin(Angle)*Radius)+Axis*(FMath::Cos(Angle)*Radius*.32f);
            if(J)Line(Prev,P,Ring==1?Accent:Main,Power*.8f*Light,1.5f);
            Prev=P;
        }
    }
    // Inward particles collapse into the eye during charge, then become a
    // directed energy stream with perspective depth and a distinct impact plume.
    for(int I=0;!Cataclysm && I<145;I++){
        const float Seed=FMath::Frac(I*.61803399f),S=FMath::Frac(I*.41421356f),Angle=I*2.399963f;
        const float Life=FMath::Frac(Seed+T*(Firing?1.5f:.35f+.4f*Charge));
        FVector2D P;float Alpha=FMath::Sin(PI*Life);
        if(Firing){
            const float Z=FMath::Sin(Angle+T*3),Perspective=1/(1+Z*.25f);
            P=A+D*Life+N*(FMath::Cos(Angle+T*4)*(DeathBeamStyle==2?15:24)*Perspective);
        }else{
            const float Radius=12+FMath::Square(1-Life)*130;
            P=A+FVector2D(FMath::Cos(Angle+Life),FMath::Sin(Angle+Life)*.8f)*Radius;
        }
        Glow(P,2+S*3,I%3?Main:Accent,Alpha*Power*Light);
        Glow(P,.65f+S*.5f,FLinearColor(.9f,.95f,1),Alpha*Power*Light);
    }
    if(Firing){
        const float Width=DeathBeamStyle==1?24.f:DeathBeamStyle==2?12.f:32.f;
        const float Surge=.92f+.08f*FMath::Sin(T*11);
        // Smooth intersecting volumes give the beam depth; no rescaled SNES cone.
        for(int I=0;I<=30;I++){
            const float U=I/30.f;
            const FVector2D P=A+D*U;
            Glow(P,Width*2.8f,Main,.012f*Light);
            Glow(P,Width*(.7f+U*.35f),Main,.045f*Surge*Light);
            Glow(P,Width*.37f,Accent,.06f*Light);
            Glow(P,Width*.12f,FLinearColor(.84f,.94f,1),.32f*Light);
        }
        // Cataclysm has a solid spectral core, never rotating filaments or rings.
        const int Filaments=Cataclysm?0:DeathBeamStyle==2?12:6;
        for(int I=0;I<Filaments;I++){
            FVector2D Prev=A;
            for(int J=1;J<=32;J++){
                const float U=J/32.f;
                float Offset=FMath::Sin(U*25-T*13+I*1.2f)*Width*.28f;
                if(DeathBeamStyle==2)Offset=FMath::PerlinNoise2D(FVector2D(J*1.73f+I*7.13f,T*12.f))*38;
                Offset*=FMath::Sin(PI*U);
                const FVector2D P=A+D*U+N*Offset;
                const FLinearColor C=DeathBeamStyle==1?FLinearColor::MakeFromHSV8(uint8(I*42+T*15),190,255):(I%2?Main:Accent);
                Line(Prev,P,C,.26f*Light,DeathBeamStyle==2?2.7f:2.5f);
                Line(Prev,P,FLinearColor(.86f,.96f,1),.35f*Light,.8f);
                if(DeathBeamStyle==2 && I<4 && J%9==0){
                    const FVector2D Fork=P+Axis*10+N*((I&1?-1:1)*(9+6*FMath::Sin(T*7+J)));
                    Line(P,Fork,Accent,.35f*Light,1.2f);
                }
                Prev=P;
            }
        }
        if(Cataclysm)Line(A,B,FLinearColor(1,.98f,.87f),.65f*Light,5.f);
        Glow(A,50,Accent,.65f*Light);Glow(A,17,FLinearColor(.9f,.95f,1),.8f*Light);
        Glow(B,90,Main,.3f*Light);Glow(B,38,Accent,.65f*Light);Glow(B,13,FLinearColor::White,.9f*Light);
        for(int I=0;I<100;I++){
            const float S=FMath::Frac(I*.61803399f),Life=FMath::Frac(S+Strike*(.7f+FMath::Frac(I*.31f)));
            const float Angle=I*2.39996f;
            const FVector2D V=Axis*(30+60*S)+N*(FMath::Sin(Angle)*100);
            const FVector2D P=B+V*Life+FVector2D(0,Life*Life*35);
            const float Strength=(1-Life)*Light;
            Glow(P,3+S*3,I%3?Accent:Main,Strength);
            Line(P-V*.025f,P,FLinearColor(1,.95f,.8f),Strength,1.f);
        }
        const float Pulse=FMath::Exp(-Strike*7.f)*Flash;
        DrawRect(FLinearColor(.83f,.92f,1,Pulse*.7f),X,Y+32*Scale,W*Scale,192*Scale);
    }else if(DeathBeamPhase==1){
        Glow(A,Cataclysm?6+Charge*37:10+Charge*12,Accent,Charge*Light);
        Glow(A,Cataclysm?2+Charge*17:3+Charge*5,FLinearColor::White,Charge*Light);
    }
}

bool ASMHUD::PrepareDeathBeamCapture(){
#if !UE_BUILD_SHIPPING
    auto RamFn=reinterpret_cast<const uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
    auto Test=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_motherbrain_beam")));
    auto Equipment=reinterpret_cast<int(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_all_equipment")));
    if(!RamFn || !Test || !Equipment)return false;
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
    if(!Equipment())return false;
    if(FParse::Param(FCommandLine::Get(),TEXT("SMFinaleEscapeTest"))){
        if(!Teleport(0) || !Wait(440) || Room()!=0x91f8 || !TestEscapeTimer())return false;
        Ram[0xd821]|=0x40;Put(0xa76,0x8000);FinaleTestRam=RamFn;SetCombatEffects(1);
        if(FParse::Param(FCommandLine::Get(),TEXT("SMEscapeMemoriesTest"))){
            if(!Wait(4))return false;
            Ram[0x947]=3;Ram[0x946]=Ram[0x945]=0; // Three-minute isolated render fixture.
        }
    }else{
    if(!TestRoom(0xdd58,180,136) || !Wait(300) || Room()!=0xdd58 || !Test(0) || !Wait(1800) || !Test(1))return false;
    FinaleTestRam=RamFn;
    if(FinaleTest){Put(0x9c2,1499);Put(0x9c0,0);Put(0x9d6,0);}
    if(FinaleTest && FParse::Param(FCommandLine::Get(),TEXT("SMFinaleDuelTest"))){
        auto Rendering=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_rendering")));
        if(!Rendering)return false;
        SetCombatEffects(1);Rendering(0);
        for(int I=0;I<15000 && VisualState(55,0)!=6;I++){
            if(Word(0x9c2)<99)Put(0x9c2,99);
            if(!Step(0)){Rendering(1);return false;}
        }
        Rendering(1);if(VisualState(55,0)!=6 || !Wait(2))return false;
    }
    }
    TestFrames=0;StartupWarning=-1;TitleInputFence=false;Accumulator=0;
    Atmosphere=true;FlashStrength=1;ImageScaling=1;
    DeathBeamCapturePath=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SMTests")/FString::Printf(TEXT("DeathBeam-%d"),DeathBeamStyle));
    IFileManager::Get().MakeDirectory(*DeathBeamCapturePath,true);
    UE_LOG(LogTemp,Display,TEXT("SM_DEATH_BEAM_READY style=%d room=%04x"),DeathBeamStyle,Room());
    return true;
#else
    return false;
#endif
}
void ASMHUD::CaptureDeathBeamFrame(){
#if !UE_BUILD_SHIPPING
    if(FinaleTest && FParse::Param(FCommandLine::Get(),TEXT("SMFinaleEscapeTest"))){
        const bool MemoriesTest=FParse::Param(FCommandLine::Get(),TEXT("SMEscapeMemoriesTest"));
        ++FinaleTestFrames;
        if(MemoriesTest){
            const float Local=FMath::Fmod(EscapeMemoryTime-8.f,20.f);
            if(DialogueAutomatic && ((Local>1.f && Local<1.04f) || (Local>4.3f && Local<4.34f)))
                FScreenshotRequest::RequestScreenshot(DeathBeamCapturePath/FString::Printf(TEXT("escape-memory-%d-%d.png"),EscapeExchange+1,EscapeLine),false,false);
        }else if(FinaleTestFrames%30==0 || FinaleTestFrames%60==5)FScreenshotRequest::RequestScreenshot(DeathBeamCapturePath/FString::Printf(TEXT("escape-pixels-%d.png"),FinaleTestFrames),false,false);
        if((MemoriesTest && EscapeMemoryTime>136.f) || (!MemoriesTest && FinaleTestFrames==420)){
            if(MemoriesTest)UE_LOG(LogTemp,Display,TEXT("SM_ESCAPE_MEMORIES_COMPLETE time=%.3f cue=%d line=%d visible=%d frame=%d"),EscapeMemoryTime,EscapeExchange+1,EscapeLine,DialogueVisible,Frame());
            UE_LOG(LogTemp,Display,TEXT("SM_FINALE_ESCAPE_COMPLETE room=%04x escape=%d hyper=%d meteors=%d atlas=%d cpu_opcodes=%llu"),Room(),VisualState(68,0),VisualState(59,0),FinaleMeteors.Num(),FinaleAtlasPixels.Num(),Opcodes());
            PlayerOwner->ConsoleCommand(TEXT("quit"));
        }
        return;
    }
    if(FinaleTest){
        const int Phase=VisualState(55,0),Age=VisualState(58,0);
        if(Phase!=FinaleTestCapturePhase || (Age<400 && Age%60<3) || ((Phase==1 || Phase==7) && Age%120<3) || (Phase==7 && VisualState(64,0)>0 && VisualState(64,0)%20<3)){
            FScreenshotRequest::RequestScreenshot(DeathBeamCapturePath/FString::Printf(TEXT("finale-%d-%d.png"),Phase,Age),false,false);
            FinaleTestCapturePhase=Phase;
        }
        if((Phase==8 && Age>60) || ++FinaleTestFrames>9000){
            UE_LOG(LogTemp,Display,TEXT("SM_FINALE_COMPLETE phase=%d frames=%d hits=%d shots=%d cpu_opcodes=%llu"),Phase,FinaleTestFrames,VisualState(60,0),VisualState(61,0),Opcodes());
            PlayerOwner->ConsoleCommand(TEXT("quit"));
        }
        return;
    }
    if(DeathBeamCaptureFrame<750){
        FScreenshotRequest::RequestScreenshot(DeathBeamCapturePath/FString::Printf(TEXT("frame-%04d.png"),DeathBeamCaptureFrame),false,false);
    }else{
        FFileHelper::SaveArrayToFile(DeathBeamAudio,*(DeathBeamCapturePath/TEXT("audio.s16le")));
        UE_LOG(LogTemp,Display,TEXT("SM_DEATH_BEAM_COMPLETE style=%d frames=%d strike=%d health=%d cpu_opcodes=%llu"),DeathBeamStyle,DeathBeamCaptureFrame,DeathBeamRecordedStrike,VisualState(21,0),Opcodes());
        PlayerOwner->ConsoleCommand(TEXT("quit"));
    }
    ++DeathBeamCaptureFrame;
#endif
}
