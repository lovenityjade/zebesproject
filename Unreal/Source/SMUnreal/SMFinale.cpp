#include "SMHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "GlobalRenderResources.h"

namespace {
float EaseFinal(float T){T=FMath::Clamp(T,0.f,1.f);return T*T*(3-2*T);}
float NoiseFinal(int N){uint32 V=uint32(N)*747796405u+2891336453u;V=((V>>((V>>28)+4))^V)*277803737u;return float((V>>22)^V)/4294967295.f;}
}
void ASMHUD::AdvanceFinale(float Dt){
    const int Phase=Room()==0xdd58 && Atmosphere?VisualState(55,0):0;
    if(State()==8)FinaleFxTime+=Dt;
    if(FinaleFxRoom!=Room()){
        FinaleFxRoom=Room();FinaleParticles.Reset();
        FinaleMeteors.Reset();FinaleMeteorSpawn=0;FinaleCorpse.Reset();
    }
    if(!Phase){FinalePhase=FinaleComposedPhase=0;FinaleFrozen.Reset();FinaleBaby.Reset();FinaleSamus.Reset();}
    if(!Atmosphere || State()!=8)return;
    if(Phase || VisualState(59,0) || VisualState(68,0))LoadFinalePixels();
    if(VisualState(68,0) && (Room()==0x91f8 || Room()==0x92b3 || Room()==0x93fe || Room()==0xca08)){
        FinaleMeteorSpawn-=Dt;
        if(FinaleMeteorSpawn<=0 && FinaleMeteors.Num()<(RenderQuality==0?4:8)){
            const int W=UseWide()?400:256;const uint8* Meta=UseWide()?WideLayers():Layers();const uint8* Far=UseWide()?WideFar():FarMask();
            const int Id=++FinaleMeteorSerial,X=24+int(NoiseFinal(Id*71)*(W-48));int Ground=214;
            auto Background=[&](int Y){int I=Y*W+X;return Meta[I*4]!=4 && Meta[I*4]!=6 && (Far[I] || Meta[I*4]==1 || Meta[I*4]==5);};
            for(int Y=140;Y<217;Y++)if(Background(Y) && !Background(Y+1)){Ground=Y;break;}
            FFinaleMeteor M;M.Impact=FVector2D(X+CameraX()-(UseWide()?72:0),Ground+CameraY());M.Size=24+NoiseFinal(Id*33)*14;M.Seed=Id;
            FinaleMeteors.Add(M);FinaleMeteorSpawn=RenderQuality==0?.85f:.48f;
        }
    }else{FinaleMeteors.Reset();FinaleMeteorSpawn=0;}
    for(int I=FinaleMeteors.Num()-1;I>=0;I--){FinaleMeteors[I].Age+=Dt;if(FinaleMeteors[I].Age>3.7f)FinaleMeteors.RemoveAtSwap(I);}
    if(Phase!=FinalePhase){
        UE_LOG(LogTemp,Display,TEXT("SM_FINALE_PHASE phase=%d frame=%d"),Phase,Frame());
        FinalePhase=Phase;FinaleTime=0;
    }
    FinaleTime+=Dt;FinaleMuzzle=FMath::Max(0.f,FinaleMuzzle-Dt);FinaleImpact=FMath::Max(0.f,FinaleImpact-Dt);
    const int Hits=VisualState(60,0),Shots=VisualState(61,0);
    if(Shots!=FinaleShots && VisualState(59,0))FinaleMuzzle=.24f;
    if(Hits!=FinaleHits && Phase==6){
        FinaleImpact=.3f;
        const FVector2D Head(VisualState(52,0),VisualState(53,0));
        const int Count=RenderQuality==0?12:28;
        for(int I=0;I<Count && FinaleParticles.Num()<180;I++){
            const float N=NoiseFinal(I+Hits*71),A=NoiseFinal(I+Hits*313)*6.28318f;
            FFinaleParticle P;P.P=Head;P.V=FVector2D(FMath::Cos(A),FMath::Sin(A))*(18+N*62);
            P.Life=.45f+N*.8f;P.Size=I%4==0?1.7f:.7f;
            P.Color=I%4==0?FLinearColor(.52f,.24f,.18f,1):FLinearColor(.5f,.015f,.025f,1);
            FinaleParticles.Add(P);
        }
    }
    FinaleHits=Hits;FinaleShots=Shots;
    for(int I=FinaleParticles.Num()-1;I>=0;I--){auto& P=FinaleParticles[I];P.Age+=Dt;P.V.Y+=65*Dt;P.P+=P.V*Dt;if(P.Age>=P.Life)FinaleParticles.RemoveAtSwap(I);}
}

const uint8* ASMHUD::ComposeFinale(const uint8* Source,const uint8* Meta,int W){
    if(!Source || !Meta || State()!=8 || !Atmosphere)return Source;
    if(!FinalePhase || !FinaleBackground)return ComposePixelFinale(Source,Meta,W);
    if(FinaleWidth!=W){FinaleWidth=W;FinaleFrozen.Reset();FinaleClean.Reset();FinaleComposedPhase=0;}
    const int Size=W*240*4,Offset=W==400?0:72;
    const FVector2D Camera(CameraX()-(W==400?72:0),CameraY());
    const FVector2D Baby=FVector2D(VisualState(56,0),VisualState(57,0))-Camera,Samus=FVector2D(SamusX(),SamusY())-Camera;
    const bool Frozen=FinalePhase==4 || FinalePhase==5;
    if(Frozen && FinaleComposedPhase<4 && FinaleClean.Num()==Size){
        FinaleFrozen.SetNumUninitialized(Size);FMemory::Memcpy(FinaleFrozen.GetData(),Source,Size);
        for(int I=32*W*4;I<224*W*4;I+=4)if((Meta[I]==4 || Meta[I]==6) && (Meta[I+1]==1 || Meta[I+1]==2))
            FMemory::Memcpy(FinaleFrozen.GetData()+I,FinaleClean.GetData()+I,4);
    }
    FinalePixels.SetNumUninitialized(Size);FMemory::Memcpy(FinalePixels.GetData(),Source,Size);
    // Full clean scene is reconstructed from the original room layers by the core.
    FinaleClean.SetNumUninitialized(Size);
    for(int Y=0;Y<240;Y++)FMemory::Memcpy(FinaleClean.GetData()+Y*W*4,FinaleBackground()+(Y*400+Offset)*4,W*4);
    auto Capture=[&](TArray<uint8>& Sprite,int Kind,FVector2D Pos){
        TArray<uint8> Temp;Temp.SetNumZeroed(128*128*4);int Count=0;
        for(int Y=0;Y<128;Y++)for(int X=0;X<128;X++){
            int SX=FMath::RoundToInt(Pos.X)+X-64,SY=FMath::RoundToInt(Pos.Y)+Y-64;
            if(SX<0 || SX>=W || SY<32 || SY>=224)continue;
            int I=(SY*W+SX)*4;if(Meta[I+1]!=Kind || (Meta[I]!=4 && Meta[I]!=6))continue;
            FMemory::Memcpy(Temp.GetData()+(Y*128+X)*4,Source+I,4);Count++;
        }
#if !UE_BUILD_SHIPPING
        if(FinaleTest && FinalePhase!=FinaleComposedPhase)UE_LOG(LogTemp,Display,TEXT("SM_FINALE_SPRITE phase=%d kind=%d pixels=%d x=%.1f y=%.1f"),FinalePhase,Kind,Count,Pos.X,Pos.Y);
#endif
        if(Count>24){Sprite=MoveTemp(Temp);return true;}return false;
    };
    if(!Frozen && FinalePhase<4){
        if(Capture(FinaleBaby,2,Baby))FinaleBabyPos=Baby;
        if(Capture(FinaleSamus,1,Samus))FinaleSamusPos=Samus;
    }
    if(Frozen && FinaleFrozen.Num()==Size){
        FMemory::Memcpy(FinalePixels.GetData(),FinaleFrozen.GetData(),Size);
        const float Grey=EaseFinal(VisualState(63,0)/9.f);
        for(int I=32*W*4;I<224*W*4;I+=4){float L=FinalePixels[I]*.0722f+FinalePixels[I+1]*.7152f+FinalePixels[I+2]*.2126f;
            for(int C=0;C<3;C++)FinalePixels[I+C]=FMath::Lerp(float(FinalePixels[I+C]),L*.68f,Grey);}
    }else{
        // Phase lighting is deliberately broad and quiet; the beam supplies peaks.
        const float Dim=FinalePhase==1?.8f:FinalePhase==3?.7f:1.f;
        for(int I=32*W*4;I<224*W*4;I+=4){
            if(FinalePhase==1 && Meta[I+1]==2 && (Meta[I]==4 || Meta[I]==6))FMemory::Memcpy(FinalePixels.GetData()+I,FinaleClean.GetData()+I,4);
            for(int C=0;C<3;C++)FinalePixels[I+C]*=Dim;

        }
    }
    auto Paste=[&](const TArray<uint8>& Sprite,FVector2D Pos,float Pulse,float Dissolve,bool Empower){
        if(Sprite.Num()!=128*128*4)return;
        for(int Y=0;Y<136;Y++)for(int X=0;X<136;X++){
            int U=FMath::FloorToInt((X-68)/Pulse+64),V=FMath::FloorToInt((Y-68)*Pulse+64);
            int PX=FMath::RoundToInt(Pos.X)+X-68,PY=FMath::RoundToInt(Pos.Y)+Y-68;
            if(U<0||U>=128||V<0||V>=128||PX<0||PX>=W||PY<32||PY>=224)continue;
            const uint8* S=Sprite.GetData()+(V*128+U)*4;
            if(!S[3] || NoiseFinal(V*137+U)<Dissolve)continue;
            uint8* D=FinalePixels.GetData()+(PY*W+PX)*4;FMemory::Memcpy(D,S,4);
            if(Empower){float L=.18f+.18f*FMath::Sin(FinaleTime*3);D[0]=FMath::Lerp(float(D[0]),255.f,L);D[1]=FMath::Lerp(float(D[1]),255.f,L);}
        }
    };
    if(FinalePhase==1)Paste(FinaleBaby,Baby,1.f+.035f*FMath::Sin(FinaleTime*8),0,false);
    if(Frozen){
        Paste(FinaleSamus,FinaleSamusPos,1,0,FinalePhase==5);
        Paste(FinaleBaby,FinaleBabyPos,1, EaseFinal((VisualState(63,0)-28)/175.f),false);
    }
    if(FinalePhase==6 && FinaleMuzzle>0){
        // Visual recoil only, so collisions, aiming and input remain native.
        Capture(FinaleSamus,1,Samus);
        for(int I=32*W*4;I<224*W*4;I+=4)if(Meta[I+1]==1 && (Meta[I]==4||Meta[I]==6))FMemory::Memcpy(FinalePixels.GetData()+I,FinaleClean.GetData()+I,4);
        const int Dir=VisualState(3,0)==4?-1:1;
        Paste(FinaleSamus,Samus-FVector2D(Dir*2.f*FinaleMuzzle/.24f,0),1,0,false);
    }
    FinaleComposedPhase=FinalePhase;
    if(!FinaleTexture || FinaleTexture->GetSizeX()!=W){FinaleTexture=UTexture2D::CreateTransient(W,240,PF_B8G8R8A8);FinaleTexture->Filter=TF_Nearest;FinaleTexture->SRGB=false;FinaleTexture->UpdateResource();}
    uint8* Copy=static_cast<uint8*>(FMemory::Malloc(Size));FMemory::Memcpy(Copy,FinalePixels.GetData(),Size);
    auto* Region=new FUpdateTextureRegion2D(0,0,0,0,W,240);
    FinaleTexture->UpdateTextureRegions(0,1,Region,W*4,4,Copy,[](uint8* Data,const FUpdateTextureRegion2D* R){FMemory::Free(Data);delete R;});
    return ComposePixelFinale(FinalePixels.GetData(),Meta,W);
}

void ASMHUD::DrawFinale(float X,float Y,float Scale){
    if(!FinalePhase || State()!=8 || !Atmosphere || !Canvas)return;
    const float W=UseWide()?400:256,Time=FinaleTime,Light=.25f+.75f*FMath::Clamp(FlashStrength,0.f,1.f);
    const FVector2D Camera(CameraX()-(UseWide()?72:0),CameraY());
    const FVector2D Head=FVector2D(VisualState(52,0),VisualState(53,0))-Camera;
    const FVector2D Baby=FVector2D(VisualState(56,0),VisualState(57,0))-Camera;
    const FVector2D Samus=FVector2D(SamusX(),SamusY())-Camera;
    const bool Frozen=FinalePhase==4 || FinalePhase==5;
    if(Frozen && FinaleTexture)DrawTexture(FinaleTexture,X,Y+32*Scale,W*Scale,192*Scale,0,32.f/240,1,192.f/240,FLinearColor::White,BLEND_Opaque);
    // Shared glow atlas is generated by the preceding native death-beam scene.
    if(!DeathBeamGlow){
        DeathBeamGlow=UTexture2D::CreateTransient(128,128,PF_B8G8R8A8);DeathBeamGlow->Filter=TF_Bilinear;DeathBeamGlow->SRGB=false;
        auto& M=DeathBeamGlow->GetPlatformData()->Mips[0];auto* P=static_cast<FColor*>(M.BulkData.Lock(LOCK_READ_WRITE));
        for(int J=0;J<128;J++)for(int I=0;I<128;I++){
            float R=FMath::Square((I-63.5f)/63.5f)+FMath::Square((J-63.5f)/63.5f);
            uint8 V=255*FMath::Max(0.f,(FMath::Exp(-R*5)-FMath::Exp(-5.f))/(1-FMath::Exp(-5.f)));P[J*128+I]=FColor(V,V,V,255);
        }
        M.BulkData.Unlock();DeathBeamGlow->UpdateResource();
    }
    auto Glow=[&](FVector2D P,float R,FLinearColor C,float Power){
        float L=FMath::Max(0.f,float(P.X)-R),T=FMath::Max(32.f,float(P.Y)-R),Right=FMath::Min(W,float(P.X)+R),B=FMath::Min(224.f,float(P.Y)+R);
        if(Right<=L||B<=T)return;C*=Power*Light;
        DrawTexture(DeathBeamGlow,X+L*Scale,Y+T*Scale,(Right-L)*Scale,(B-T)*Scale,(L-P.X+R)/(2*R),(T-P.Y+R)/(2*R),(Right-L)/(2*R),(B-T)/(2*R),C,BLEND_Additive);
    };
    // Solid ribbons/particles use one triangle batch, avoiding per-line transforms.
    TArray<FCanvasUVTri> Triangles;
    auto Line=[&](FVector2D A,FVector2D B,float Width,FLinearColor C){
        if(FMath::Min(A.X,B.X)<0||FMath::Max(A.X,B.X)>W||FMath::Min(A.Y,B.Y)<32||FMath::Max(A.Y,B.Y)>224)return;
        FVector2D N=FVector2D(-(B-A).Y,(B-A).X).GetSafeNormal()*Width*.5;
        FVector2D P[4]={A-N,A+N,B+N,B-N};for(auto& V:P)V=FVector2D(X,Y)+V*Scale;
        FCanvasUVTri T;T.V0_Pos=P[0];T.V1_Pos=P[1];T.V2_Pos=P[2];T.V0_UV=T.V1_UV=T.V2_UV=FVector2D::ZeroVector;T.V0_Color=T.V1_Color=T.V2_Color=C;Triangles.Add(T);
        T.V1_Pos=P[2];T.V2_Pos=P[3];Triangles.Add(T);
    };

    if(FinalePhase==3 || (FinalePhase==4 && Time<.3f)){
        float Power=FinalePhase==3?EaseFinal(Time/.45f):1-Time/.3f;
        Glow(Head,12+Power*26,FLinearColor(1,.17f,.05f),Power);
        if(FinalePhase==4 || Time>.35f){
            FVector2D End=FinalePhase==4?FinaleBabyPos:Baby;
            Line(Head,End,11,FLinearColor(1,.045f,.01f,.35f*Power*Light));
            Line(Head,End,4,FLinearColor(1,.65f,.3f,Power*Light));
            Line(Head,End,1.4f,FLinearColor(1,1,.85f,Power*Light));Glow(End,32,FLinearColor(1,.6f,.25f),Power);
        }
    }
    if(Frozen){
        const float Age=VisualState(63,0)/60.f;
        const int Count=RenderQuality==0?55:130;
        for(int I=0;I<Count;I++){
            float Start=.45f+NoiseFinal(I*31)*1.9f,T=(Age-Start)/(1.2f+NoiseFinal(I*67));if(T<0||T>1)continue;
            FVector2D From=FinaleBabyPos+FVector2D((NoiseFinal(I*113)-.5f)*52,(NoiseFinal(I*191)-.5f)*38);
            FVector2D P=FMath::Lerp(From,FinaleSamusPos-FVector2D(0,8),double(EaseFinal(T)));P.X+=FMath::Sin(T*6.28f+I)*14*FMath::Sin(T*3.14159f);
            Line(P,P+FVector2D(0,1.5f+T*2),.8f,FLinearColor(.45f,1,.8f,Light));
            if(I%16==0)Glow(P,6,FLinearColor(.15f,1,.6f),.5f);
        }
        float Charge=EaseFinal((Age-1.3f)/2.8f);Glow(FinaleSamusPos,20+Charge*45,FLinearColor(.05f,.7f,1),Charge*.7f);
        for(int I=0;I<9;I++){float A=I*.698f+Time*1.1f;FVector2D P=FinaleSamusPos+FVector2D(FMath::Cos(A)*17,FMath::Sin(A)*26);
            Line(P,P+FVector2D(FMath::Sin(A*3+Time*13)*4,5),.6f,FLinearColor(.4f,.85f,1,Charge*.75f*Light));}
    }



    if(Triangles.Num()){FCanvasTriangleItem Item(Triangles,GWhiteTexture);Item.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Item);}
}
