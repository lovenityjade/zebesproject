#include "SMHUD.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

namespace {
float Grain(int N){uint32 V=uint32(N)*747796405u+2891336453u;V=((V>>((V>>28)+4))^V)*277803737u;return float((V>>22)^V)/4294967295.f;}
float Smooth(float T){T=FMath::Clamp(T,0.f,1.f);return T*T*(3-2*T);}
FVector2D BeamAxis(int D){
    static const FVector2D Directions[]={{0,-1},{.707,-.707},{1,0},{.707,.707},{0,1},{0,1},{-.707,.707},{-1,0},{-.707,-.707},{0,-1}};
    return Directions[FMath::Clamp(D&15,0,9)];
}
}

void ASMHUD::LoadFinalePixels(){
    if(FinaleAtlasAttempted)return;FinaleAtlasAttempted=true;
    UTexture2D* Texture=FImageUtils::ImportFileAsTexture2D(FPaths::ProjectContentDir()/TEXT("Finale/PixelEffects-v2.png"));
    if(!Texture || !Texture->GetPlatformData() || Texture->GetPixelFormat()!=PF_B8G8R8A8){
        UE_LOG(LogTemp,Warning,TEXT("SM_FINALE_PIXEL_ATLAS unavailable; keeping native Hyper sprite"));return;
    }
    FinaleAtlasW=Texture->GetSizeX();FinaleAtlasH=Texture->GetSizeY();
    auto& Mip=Texture->GetPlatformData()->Mips[0];
    const void* Data=Mip.BulkData.LockReadOnly();
    if(Data){FinaleAtlasPixels.SetNumUninitialized(FinaleAtlasW*FinaleAtlasH*4);FMemory::Memcpy(FinaleAtlasPixels.GetData(),Data,FinaleAtlasPixels.Num());}
    Mip.BulkData.Unlock();
    if(FinaleAtlasPixels.Num()){
        if(auto Configure=reinterpret_cast<void(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_set_finale_textures"))))Configure(1);
        UE_LOG(LogTemp,Display,TEXT("SM_FINALE_PIXEL_ATLAS ready=%dx%d native_grid=1"),FinaleAtlasW,FinaleAtlasH);
    }
}

const uint8* ASMHUD::ComposePixelFinale(const uint8* Source,const uint8* Meta,int W){
    const bool Escape=VisualState(68,0)!=0,Hyper=VisualState(59,0)!=0;
    if(!Atmosphere || State()!=8 || (!FinalePhase && !Escape && !Hyper) || FinaleAtlasPixels.IsEmpty())return Source;
    const int Size=W*240*4;
    FinaleFxPixels.SetNumUninitialized(Size);FMemory::Memcpy(FinaleFxPixels.GetData(),Source,Size);
    const float Time=FinaleFxTime,Flash=FMath::Clamp(FlashStrength,0.f,1.f);
    const FVector2D Camera(CameraX()-(W==400?72:0),CameraY());
    const FVector2D Head=FVector2D(VisualState(52,0),VisualState(53,0))-Camera;
    const FVector2D Baby=FVector2D(VisualState(56,0),VisualState(57,0))-Camera;
    const uint8* Far=W==400?WideFar():FarMask();
    auto Background=[&](int X,int Y){int I=Y*W+X;return Meta[I*4]!=4 && Meta[I*4]!=6 && (Far[I] || Meta[I*4]==1 || Meta[I*4]==5);};
    auto Pixel=[&](int X,int Y,FColor C,float Alpha,int Mask){
        if(X<0||X>=W||Y<32||Y>=224)return;
        int I=(Y*W+X)*4;
        if(Mask==1 && !Background(X,Y))return;
        if(Mask==2 && Meta[I+1]!=3)return;
        float A=FMath::Clamp(Alpha,0.f,1.f);uint8* D=FinaleFxPixels.GetData()+I;
        // Clustered atlas colors are quantized to the game's 5-bit channels.
        D[0]=FMath::Lerp(float(D[0]),float(C.B&248),A);
        D[1]=FMath::Lerp(float(D[1]),float(C.G&248),A);
        D[2]=FMath::Lerp(float(D[2]),float(C.R&248),A);
    };
    auto Stamp=[&](int Cell,FVector2D Center,float Width,float Height,FVector2D Axis,float Opacity,int Mask){
        const FVector2D N(-Axis.Y,Axis.X);
        const int RX=FMath::CeilToInt(FMath::Abs(Axis.X)*Width*.5+FMath::Abs(N.X)*Height*.5);
        const int RY=FMath::CeilToInt(FMath::Abs(Axis.Y)*Width*.5+FMath::Abs(N.Y)*Height*.5);
        const int CX=FMath::RoundToInt(Center.X),CY=FMath::RoundToInt(Center.Y);
        for(int Y=FMath::Max(32,CY-RY);Y<FMath::Min(224,CY+RY+1);Y++)for(int X=FMath::Max(0,CX-RX);X<FMath::Min(W,CX+RX+1);X++){
            FVector2D D(X-CX,Y-CY);float U=FVector2D::DotProduct(D,Axis)/Width+.5f,V=FVector2D::DotProduct(D,N)/Height+.5f;
            if(U<0||U>=1||V<0||V>=1)continue;
            int TX=FMath::Clamp(int(((Cell%4)+U)*FinaleAtlasW*.25f),0,FinaleAtlasW-1);
            int TY=FMath::Clamp(int(((Cell/4)+V)*FinaleAtlasH*.25f),0,FinaleAtlasH-1);
            const uint8* S=FinaleAtlasPixels.GetData()+(TY*FinaleAtlasW+TX)*4;
            if(S[3]<100)continue;
            Pixel(X,Y,FColor(S[2],S[1],S[0]),Opacity,Mask);
        }
    };
    auto Arc=[&](FVector2D A,FVector2D B,int Seed,float Power,int Mask){
        FVector2D D=B-A,N=FVector2D(-D.Y,D.X).GetSafeNormal();
        int Steps=FMath::Clamp(FMath::CeilToInt(D.Size()),1,240);
        for(int I=0;I<=Steps;I++){
            float T=float(I)/Steps;
            float Bend=(Grain(Seed+I/5*43)-.5f)*6*FMath::Sin(T*PI);
            FVector2D P=A+D*T+N*Bend;int X=FMath::RoundToInt(P.X),Y=FMath::RoundToInt(P.Y);
            Pixel(X,Y,FColor(180,245,182),Power,Mask);
            Pixel(X+1,Y,FColor(50,161,135),Power*.4f,Mask);
        }
    };

    if(Escape){
        // Slow overlapping circuits flicker spatially; never a full-screen strobe.
        const float Circuit=.5f+.5f*FMath::Sin(Time*3.2f);
        const float Fault=FMath::Pow(FMath::Max(0.f,FMath::Sin(Time*1.9f)),8.f);
        for(int Y=32;Y<224;Y++)for(int X=0;X<W;X++){
            const int I=(Y*W+X)*4;if(Meta[I]==4 || Meta[I]==6)continue;
            float Zone=.5f+.5f*FMath::Sin((X+Camera.X)*.019f+(Y+Camera.Y)*.013f+Time*2);
            float Dim=1-Flash*(.08f+.14f*Circuit*Zone+.12f*Fault);
            for(int C=0;C<3;C++)FinaleFxPixels[I+C]*=Dim;
            float Alarm=Flash*.13f*Circuit*Zone;
            FinaleFxPixels[I+2]=FMath::Min(255.f,FinaleFxPixels[I+2]+Alarm*36);
        }
        // Authored exterior rooms only. Projectiles and explosions are clipped
        // behind terrain and actors; impacts share the same world-space endpoint.
        for(const auto& M:FinaleMeteors){
            FVector2D Impact=M.Impact-Camera;
            if(M.Age<2.5f){
                float Travel=Smooth(M.Age/2.5f);
                FVector2D P=Impact-FVector2D((1-Travel)*100,(1-Travel)*250)-FVector2D(M.Size*.22f,M.Size*.22f);
                Stamp(4+(int(Time*9+M.Seed)%4),P,M.Size,M.Size,FVector2D(1,0),.9f,1);
            }else{
                float A=M.Age-2.5f;
                Stamp(8+FMath::Min(3,int(A*3.4f)),Impact-FVector2D(0,12),M.Size*1.8f,M.Size*1.5f,FVector2D(1,0),1-A/1.2f,1);
            }
        }
    }

    if(FinalePhase==1){
        const float Drain=FMath::Clamp(VisualState(70,0)/1024.f,0.f,1.f),Energy=1-Drain;
        const float BaseY=FMath::Min(218.f,float(Head.Y)+116);
        const float Boundary=FMath::Lerp(BaseY,float(Head.Y)-24,Drain);
        for(int Y=32;Y<224;Y++)for(int X=0;X<W;X++){
            int I=(Y*W+X)*4;
            if(Meta[I+1]!=3 || (Meta[I]!=1 && Meta[I]!=4 && Meta[I]!=6))continue;
            float Stone=Smooth((Y-Boundary+12)/24.f);
            float L=FinaleFxPixels[I]*.0722f+FinaleFxPixels[I+1]*.7152f+FinaleFxPixels[I+2]*.2126f;
            for(int C=0;C<3;C++)FinaleFxPixels[I+C]=FMath::Lerp(float(FinaleFxPixels[I+C]),L*(.68f+.07f*C),Stone*.88f);
            // Intermittent upward veins live only on still-energized flesh.
            float Vein=FMath::Sin(X*.42f+Y*.12f+FMath::Sin(Y*.1f))*FMath::Sin(Y*.2f+Time*8);
            if(Vein>.83f && Y<Boundary+8)Pixel(X,Y,FColor(143,237,155),(.3f+.4f*Energy)*(1-Stone),2);
        }
        int Paths=2+int(Energy*9);
        for(int I=0;I<Paths;I++){
            float T=FMath::Frac(Time*(.9f+I*.06f)+I*.173f);
            FVector2D From(Head.X+(Grain(I*51)-.5f)*48,Boundary-8);
            FVector2D End=Baby+FVector2D((I%3-1)*5,7);
            FVector2D P=FMath::Lerp(From,End,double(T));P.X+=FMath::Sin(T*9+I)*7*(1-T);
            Arc(P,FMath::Lerp(P,End,.27),I*71+int(Time*10),(.2f+.65f*Energy)*(1-T*.3f),0);
            Stamp(14,P,6,8,FVector2D(1,0),.25f+.7f*Energy,0);
        }
        // The red internal nuclei brighten and contract as the energy arrives.
        for(int Y=FMath::Max(32,int(Baby.Y)-23);Y<FMath::Min(224,int(Baby.Y)+24);Y++)for(int X=FMath::Max(0,int(Baby.X)-25);X<FMath::Min(W,int(Baby.X)+25);X++){
            int I=(Y*W+X)*4;uint8* D=FinaleFxPixels.GetData()+I;
            if((Meta[I]==4 || Meta[I]==6) && Meta[I+1]==2 && D[2]>D[1]*1.18f){
                float Pulse=(.5f+.5f*FMath::Sin(Time*9))*(.25f+.4f*Drain);
                D[2]=FMath::Min(255.f,D[2]*(1+Pulse));D[1]=FMath::Min(255.f,D[1]+Pulse*24);
            }
        }
    }
    if(Hyper){
        for(int I=0;I<5;I++)if(VisualState(5,I) && (VisualState(8,I)&0xfff)==0x18){
            const FVector2D End=FVector2D(VisualState(6,I),VisualState(7,I))-Camera,Axis=BeamAxis(VisualState(69,I));
            // Same native projectile position and direction; no cannon-to-target
            // overlay. A thick textured lance trails the real colliding head.
            const int Cel=(int(Time*18)+I)%4;
            Stamp(Cel,End-Axis*21,66,29,Axis,1,0);
            for(int K=0;K<5;K++){
                float T=FMath::Frac(Time*3+K*.19f);
                FVector2D N(-Axis.Y,Axis.X),P=End-Axis*(24+T*40)+N*FMath::Sin(K*4+Time*12)*(4+T*5);
                Stamp(14,P,3+T*2,4+T*3,Axis,(1-T)*.6f,0);
            }
        }
        if(FinaleMuzzle>0){
            const FVector2D Muzzle=FVector2D(VisualState(65,0),VisualState(66,0))-Camera;
            const FVector2D Axis=BeamAxis(VisualState(67,0));
            Stamp(0,Muzzle,18,19,Axis,FMath::Min(1.f,FinaleMuzzle*6)*(.35f+.65f*Flash),0);
        }
    }
    if(FinalePhase==7){
        const int Fade=VisualState(64,0);const float D=Smooth(Fade/110.f);
        if(!Fade){
            TArray<uint8> Capture;Capture.SetNumZeroed(128*128*4);int Count=0;
            for(int V=0;V<128;V++)for(int U=0;U<128;U++){
                int X=FMath::RoundToInt(Head.X)+U-64,Y=FMath::RoundToInt(Head.Y)+V-64;
                if(X<0||X>=W||Y<32||Y>=224)continue;
                int I=(Y*W+X)*4;
                // The corpse is OBJ. BG2 keeps a black body-shaped mask after
                // the native collapse; caching that would resurrect a silhouette.
                if(Meta[I+1]!=3 || (Meta[I]!=4 && Meta[I]!=6) ||
                   U<24 || U>104 || V<28 || V>102 ||
                   FMath::Max3(Source[I],Source[I+1],Source[I+2])<12)continue;
                FMemory::Memcpy(Capture.GetData()+(V*128+U)*4,Source+I,4);Count++;
            }
            if(Count>12){FinaleCorpse=MoveTemp(Capture);FinaleCorpseWorld=Head+Camera;}
        }
        if(Fade && FinaleCorpse.Num()==128*128*4 && FinaleClean.Num()==Size){
            for(int Y=32;Y<224;Y++)for(int X=0;X<W;X++){
                int I=(Y*W+X)*4;
                if(Meta[I+1]==3 && (Meta[I]==4 || Meta[I]==6))FMemory::Memcpy(FinaleFxPixels.GetData()+I,FinaleClean.GetData()+I,4);
            }
            const FVector2D Center=FinaleCorpseWorld-Camera;
            for(int V=0;V<128;V++)for(int U=0;U<128;U++){
                const uint8* S=FinaleCorpse.GetData()+(V*128+U)*4;if(!S[3])continue;
                float Threshold=Grain((U/3)*733+(V/3)*191)*.65f+.35f*(1-V/128.f);
                if(D>Threshold)continue;
                int X=FMath::RoundToInt(Center.X)+U-64,Y=FMath::RoundToInt(Center.Y)+V-64;
                Pixel(X,Y,Threshold-D<.035f && Grain(U*79+V*17)>.6f?FColor(255,168,65):FColor(S[2],S[1],S[0]),1,0);
            }
            for(int I=0;I<(RenderQuality==0?28:64);I++){
                float Birth=Grain(I*37)*.65f,T=(D-Birth)*1.6f;if(T<0||T>1)continue;
                int Pick=int(Grain(I*67)*16383),Found=-1;
                for(int J=0;J<256;J++){
                    int K=(Pick+J*61)&16383;if(FinaleCorpse[K*4+3]){Found=K;break;}
                }
                if(Found<0)continue;
                FVector2D From=Center+FVector2D(Found%128-64,Found/128-64);
                FVector2D P=From+FVector2D((Grain(I*83)-.5f)*T*85,-T*28+T*T*38);
                Stamp(I%3==0?13:12,P,5+I%4,5+I%3,FVector2D(1,0),1-T,0);
                if(I%4==0)Stamp(15,P,10,10,FVector2D(1,0),(1-T)*Flash*.65f,0);
            }
        }
    }
    for(const auto& P:FinaleParticles)Stamp(13,P.P-Camera,4+P.Size*2,4+P.Size*2,FVector2D(1,0),1-P.Age/P.Life,0);
    return FinaleFxPixels.GetData();
}
