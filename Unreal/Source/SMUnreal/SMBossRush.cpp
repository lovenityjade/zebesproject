#include "SMHUD.h"
#include "SMSystemMenu.h"
#include "SMLocalization.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "Sound/SoundWaveProcedural.h"
#include "Components/AudioComponent.h"
namespace {
UTexture2D* WireTexture(const TArray<uint8>& Bytes,int W,int H,TextureFilter Filter=TF_Nearest){
 auto* T=UTexture2D::CreateTransient(W,H,PF_B8G8R8A8);
 T->Filter=Filter;T->SRGB=false;T->NeverStream=true;
 auto& M=T->GetPlatformData()->Mips[0];FMemory::Memcpy(M.BulkData.Lock(LOCK_READ_WRITE),Bytes.GetData(),W*H*4);
 M.BulkData.Unlock();T->UpdateResource();return T;
}
}
void ASMHUD::CaptureRushWire(int R,bool Black){
 RushEdges[R].Reset();
 if(Black){
  TArray<uint8> PixelsCopy;PixelsCopy.SetNumZeroed(400*224*4);
  for(int I=3;I<PixelsCopy.Num();I+=4)PixelsCopy[I]=255;
  WireRooms[R]=WireTexture(PixelsCopy,400,224);return;
 }
        TArray<uint8> PixelsCopy,MetaCopy;
        PixelsCopy.SetNumZeroed(400*224*4);MetaCopy.SetNumZeroed(400*224*4);
        if(Widescreen && WideAvailable()){
            FMemory::Memcpy(PixelsCopy.GetData(),WideScene(),PixelsCopy.Num());
            FMemory::Memcpy(MetaCopy.GetData(),WideLayers(),MetaCopy.Num());
        }else for(int Y=0;Y<224;Y++){
            FMemory::Memcpy(PixelsCopy.GetData()+(Y*400+72)*4,Scene()+Y*256*4,256*4);
            FMemory::Memcpy(MetaCopy.GetData()+(Y*400+72)*4,Layers()+Y*256*4,256*4);
        }
        const TArray<uint8> OriginalPixels=PixelsCopy;
        const uint8* Meta=MetaCopy.GetData();
        const int SX=SamusX()-CameraX()+72,SY=SamusY()-CameraY();
        RushSamusPos[R]=FVector2D(SX,SY);
        int SpritePixels=0;
        for(int Y=FMath::Max(32,SY-30);Y<FMath::Min(224,SY+29);Y++)for(int X=FMath::Max(0,SX-16);X<FMath::Min(400,SX+17);X++){
            const int I=(Y*400+X)*4;
            if(!Meta[I+3] || (Meta[I]!=4 && Meta[I]!=6))continue;
            FMemory::Memcpy(RushSprite[R].GetData()+I,PixelsCopy.GetData()+I,4);RushSprite[R][I+3]=255;
            // Remove Samus from the frozen backdrop, replacing only her sprite
            // pixels with the nearest non-sprite sample on the same scanline.
            int J=I;
            for(int D=1;D<40;D++){
                const int XX=X+(X<SX?-D:D);
                if(XX<0 || XX>=400)break;
                const int K=(Y*400+XX)*4;
                if(Meta[K]!=4 && Meta[K]!=6){J=K;break;}
            }
            FMemory::Memcpy(PixelsCopy.GetData()+I,OriginalPixels.GetData()+J,4);++SpritePixels;
        }
        for(int I=3;I<PixelsCopy.Num();I+=4)PixelsCopy[I]=255;
        WireRooms[R]=WireTexture(PixelsCopy,400,224);
        // Four-native-pixel squares: 16 times as many cells as the old 16px
        // grid. The mask follows foreground and visible enemies, never BG2.
        TArray<uint8> Shape;Shape.SetNumZeroed(400*224);
        auto IsSamus=[&](int X,int Y){return X>=SX-16 && X<SX+17 && Y>=SY-30 && Y<SY+29 && (Meta[(Y*400+X)*4]==4 || Meta[(Y*400+X)*4]==6);};
        for(int Y=0;Y<224;Y++)for(int X=0;X<400;X++){
            int I=(Y*400+X)*4;int Layer=Meta[I];
            Shape[Y*400+X]=Meta[I+3] && (Layer==0 || Layer==4 || Layer==6) && !IsSamus(X,Y);
        }
        auto Inside=[&](int X,int Y){return X>=0 && X<400 && Y>=0 && Y<224 && Shape[Y*400+X]!=0;};
        auto Contrast=[&](int X,int Y,int XX,int YY){
            int A=(Y*400+X)*4,B=(YY*400+XX)*4;
            int Diff=0;for(int C=0;C<3;C++)Diff=FMath::Max(Diff,FMath::Abs(int(PixelsCopy[A+C])-int(PixelsCopy[B+C])));
            return Diff;
        };
        bool Solid[56][100]={};
        for(int CY=0;CY<56;CY++)for(int CX=0;CX<100;CX++){
            int N=0;for(int DY=0;DY<4;DY++)for(int DX=0;DX<4;DX++)N+=Inside(CX*4+DX,CY*4+DY);
            Solid[CY][CX]=N>=8;
        }
        auto Occupied=[&](int X,int Y){return X>=0 && X<100 && Y>=0 && Y<56 && Solid[Y][X];};
        auto Edge=[&](float X,float Y,float XX,float YY,bool Boundary,uint8 Kind=0){RushEdges[R].Add({FVector2D(X,Y),FVector2D(XX,YY),Boundary,Kind});};
        for(int Y=0;Y<56;Y++)for(int X=0;X<100;X++)if(Solid[Y][X]){
            Edge(X*4,Y*4,X*4+4,Y*4,false);
            Edge(X*4,Y*4,X*4,Y*4+4,false);
            if(!Occupied(X,Y+1))Edge(X*4,Y*4+4,X*4+4,Y*4+4,false);
            if(!Occupied(X+1,Y))Edge(X*4+4,Y*4,X*4+4,Y*4+4,false);
        }
        const int GridCount=RushEdges[R].Num();
        // Exact pixel silhouette plus strong, sustained palette boundaries:
        // pipes, stone edges, armor joints and enemy details remain legible.
        // Merge collinear runs; reject isolated interior dither speckles.
        for(int Axis=0;Axis<2;Axis++){
            const int Rows=Axis==0?225:401,Cols=Axis==0?400:224;
            for(int Row=0;Row<Rows;Row++){
                int Start=-1,Previous=0;
                for(int Col=0;Col<=Cols;Col++){
                    int Kind=0;
                    if(Col<Cols){
                        int X=Axis==0?Col:Row,Y=Axis==0?Row:Col;
                        int XX=X-(Axis==1),YY=Y-(Axis==0);
                        bool A=Inside(X,Y),B=Inside(XX,YY);
                        if(A!=B)Kind=2;
                        else if(A && B && Contrast(X,Y,XX,YY)>=36)Kind=1;
                    }
                    if(Kind!=Previous || Col==Cols || (Start>=0 && Col-Start==8)){
                        if(Start>=0 && (Previous==2 || Col-Start>=2)){
                            if(Axis==0)Edge(Start,Row,Col,Row,Previous==2,1);
                            else Edge(Row,Start,Row,Col,Previous==2,1);
                        }
                        Start=Kind?Col:-1;Previous=Kind;
                    }
                }
            }
        }

}
void ASMHUD::MatchRushWire(){
 WirePairs.Reset();WireSamusEdges.Reset();RushSamusPairs.Reset();
    if(RushInfo(5))RushSprite[0]=RushSprite[1];
    WireSamus=WireTexture(RushSprite[0],400,224);
    RushSamusTarget=WireTexture(RushSprite[1],400,224);
    TArray<FWireEdge> SamusEdges[2];
    for(int R=0;R<2;R++){
    auto& Edges=SamusEdges[R];const auto& Sprite=RushSprite[R];
    auto SP=[&](int X,int Y){return X>=0 && X<400 && Y>=0 && Y<224 && Sprite[(Y*400+X)*4+3]!=0;};
    // Preserve the original face/visor, shoulders, chest, cannon and legs,
    // rather than showing only a hollow outline of the frontal sprite.
    for(int Y=32;Y<224;Y++)for(int X=0;X<400;X++)if(SP(X,Y)){
        auto Detail=[&](int XX,int YY){
            if(!SP(XX,YY))return true;
            int A=(Y*400+X)*4,B=(YY*400+XX)*4;
            int Diff=0;for(int C=0;C<3;C++)Diff=FMath::Max(Diff,FMath::Abs(int(Sprite[A+C])-int(Sprite[B+C])));
            return Diff>=44;
        };
        if(Detail(X,Y-1))Edges.Add({FVector2D(X,Y),FVector2D(X+1,Y),!SP(X,Y-1),1});
        if(Detail(X-1,Y))Edges.Add({FVector2D(X,Y),FVector2D(X,Y+1),!SP(X-1,Y),1});
        if(!SP(X,Y+1))Edges.Add({FVector2D(X,Y+1),FVector2D(X+1,Y+1),true,1});
        if(!SP(X+1,Y))Edges.Add({FVector2D(X+1,Y),FVector2D(X+1,Y+1),true,1});
    }
    }
    WireSamusEdges=SamusEdges[0]; // Death retains the original silhouette.
    if(!RushDeath && !RushFinal){
        TArray<bool> UsedSamus;UsedSamus.Init(false,SamusEdges[1].Num());
        const FVector2D Delta=RushSamusPos[1]-RushSamusPos[0];
        for(const auto& E:SamusEdges[0]){
            int Best=-1;double Score=1.e30;
            const auto Mid=(E.A+E.B)*.5;
            for(int J=0;J<SamusEdges[1].Num();J++)if(!UsedSamus[J]){
                const auto& D=SamusEdges[1][J];
                if((E.A.X==E.B.X)!=(D.A.X==D.B.X))continue;
                double S=FVector2D::DistSquared(Mid+Delta,(D.A+D.B)*.5)+(E.Boundary!=D.Boundary?64:0);
                if(S<Score){Best=J;Score=S;if(S==0)break;}
            }
            if(Best>=0){UsedSamus[Best]=true;RushSamusPairs.Add({E,SamusEdges[1][Best]});}
            else RushSamusPairs.Add({E,{Mid+Delta,Mid+Delta,false,1}});
        }
        for(int J=0;J<SamusEdges[1].Num();J++)if(!UsedSamus[J]){
            const auto& E=SamusEdges[1][J];const auto Mid=(E.A+E.B)*.5-Delta;
            RushSamusPairs.Add({{Mid,Mid,false,1},E});
        }
        UE_LOG(LogTemp,Display,TEXT("SM_RUSH_SAMUS_MORPH from=%g,%g to=%g,%g edges=%d,%d pairs=%d"),RushSamusPos[0].X,RushSamusPos[0].Y,RushSamusPos[1].X,RushSamusPos[1].Y,SamusEdges[0].Num(),SamusEdges[1].Num(),RushSamusPairs.Num());
    }
    // Exact correspondence first; spatial buckets keep detailed art matching
    // bounded. All remaining movement is between parallel straight segments.
    auto EdgeKey=[](const FWireEdge& E){return uint64(E.A.X)|(uint64(E.A.Y)<<9)|(uint64(E.B.X)<<17)|(uint64(E.B.Y)<<26)|(uint64(E.Kind)<<34);};
    auto BucketKey=[](int X,int Y,const FWireEdge& E){return X+Y*32+int(E.A.X==E.B.X)*1024+int(E.Kind)*2048;};
    TMap<uint64,int> Exact;
    TMultiMap<int,int> Buckets;
    for(int J=0;J<RushEdges[1].Num();J++){
        const auto& E=RushEdges[1][J];Exact.Add(EdgeKey(E),J);
        auto Mid=(E.A+E.B)*.5;Buckets.Add(BucketKey(int(Mid.X)/16,int(Mid.Y)/16,E),J);
    }
    TArray<bool> Used;Used.Init(false,RushEdges[1].Num());
    TArray<int> Matches;Matches.Init(-1,RushEdges[0].Num());
    for(int I=0;I<RushEdges[0].Num();I++)if(const int* J=Exact.Find(EdgeKey(RushEdges[0][I]))){
        if(!Used[*J]){Matches[I]=*J;Used[*J]=true;}
    }
    TArray<int> Candidates;
    int Common=0,Moved=0;
    for(int I=0;I<RushEdges[0].Num();I++){
        const auto& E=RushEdges[0][I];const auto Mid=(E.A+E.B)*.5;
        int Best=Matches[I];double Score=1.e30;
        if(Best>=0)++Common;
        else for(int Y=FMath::Max(0,int(Mid.Y)/16-2);Y<=FMath::Min(14,int(Mid.Y)/16+2);Y++)for(int X=FMath::Max(0,int(Mid.X)/16-2);X<=FMath::Min(25,int(Mid.X)/16+2);X++){
            Candidates.Reset();Buckets.MultiFind(BucketKey(X,Y,E),Candidates);
            for(int J:Candidates)if(!Used[J]){
                const auto& D=RushEdges[1][J];
                double S=FVector2D::DistSquared(Mid,(D.A+D.B)*.5);
                if(S>32*32)continue;
                if(E.Boundary!=D.Boundary)S+=250;
                if(S<Score){Score=S;Best=J;}
            }
        }
        if(Best>=0){Used[Best]=true;WirePairs.Add({E,RushEdges[1][Best]});if(Matches[I]<0)++Moved;}
        else WirePairs.Add({E,{Mid,Mid,false,E.Kind}});
    }
    for(int J=0;J<RushEdges[1].Num();J++)if(!Used[J]){
        const auto& E=RushEdges[1][J];const auto Mid=(E.A+E.B)*.5;
        WirePairs.Add({{Mid,Mid,false,E.Kind},E});
    }
    UE_LOG(LogTemp,Display,TEXT("SM_WIRE_MATCH common=%d moved=%d samus_edges=%d spatial_wave=0"),Common,Moved,WireSamusEdges.Num());
    int BentSegments=0;
    for(const auto& Pair:WirePairs)for(float M:{.125f,.5f,.875f}){
        const auto A=FMath::Lerp(Pair.From.A,Pair.To.A,M),B=FMath::Lerp(Pair.From.B,Pair.To.B,M);
        BentSegments+=FMath::Abs(A.X-B.X)>.0001 && FMath::Abs(A.Y-B.Y)>.0001;
    }
    UE_LOG(LogTemp,Display,TEXT("SM_WIRE_RIGID diagonal_segments=%d"),BentSegments);
    check(BentSegments==0);
    TArray<uint8> Glow;Glow.SetNumUninitialized(64*64*4);
    for(int Y=0;Y<64;Y++)for(int X=0;X<64;X++){
        float R=FMath::Square((X-31.5f)/31.5f)+FMath::Square((Y-31.5f)/31.5f);
        uint8 V=uint8(255*FMath::Max(0.f,(FMath::Exp(-R*6)-FMath::Exp(-6.f))/(1-FMath::Exp(-6.f))));
        int I=(Y*64+X)*4;Glow[I]=Glow[I+1]=Glow[I+2]=V;Glow[I+3]=255;
    }
    WireGlow=WireTexture(Glow,64,64,TF_Bilinear);

}
bool ASMHUD::TickBossRush(float Dt){
 if(!RushInfo && CoreHandle){
  RushInfo=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_rush_info")));
  RushCommand=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_rush_command")));
  RushPractice=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_rush_practice")));
  RushName=reinterpret_cast<const char*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_rush_name")));
  LoadBossRushAudio();
  RushScreen=reinterpret_cast<const uint8*(*)(int,int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_rush_screen")));
  RushCompletedArt=FImageUtils::ImportFileAsTexture2D(FPaths::ProjectContentDir()/TEXT("BossRush/Completed-16x9.png"));
  RushFailedArt=FImageUtils::ImportFileAsTexture2D(FPaths::ProjectContentDir()/TEXT("BossRush/Failed-16x9.png"));
  for(auto* T:{RushCompletedArt.Get(),RushFailedArt.Get()})if(T){T->Filter=TF_Bilinear;T->UpdateResource();}
  PrepareRushResultEffects();
  TArray<uint8> Empty;Empty.SetNumZeroed(256*128*4);RushLabels=WireTexture(Empty,256,128);

 }
 if(!RushInfo || !RushCommand)return false;
 if(RushTest)TickBossRushTest();
 const int Stage=RushInfo(0);
 auto Press=[&](FKey K){return PlayerOwner && PlayerOwner->WasInputKeyJustPressed(K);};
 const bool Confirm=Press(EKeys::Enter)||Press(EKeys::Gamepad_FaceButton_Bottom);
 const bool PauseKey=Press(EKeys::Escape)||Press(EKeys::Gamepad_Special_Right);
 if(!Stage){RushVisual=RushPaused=false;RushScreenStage=-1;return false;}
 if(Stage==4 && (PauseKey || (!RushPaused && Press(EKeys::Enter)))){
  RushPaused=!RushPaused;RushSelection=0;Accumulator=0;
  if(AudioWave)AudioWave->ResetAudio();PlayerOwner->FlushPressedKeys();return true;
 }
 if(Stage==7 || Stage==8 || (Stage==4 && RushPaused)){
  RushVisual=false;
  if(RushScreenStage!=Stage){RushScreenStage=Stage;RushSelection=0;RushResultTime=0;}
  if(Stage==7 || Stage==8)RushResultTime+=FMath::Min(Dt,.05f);
  if(Press(EKeys::Up)||Press(EKeys::Gamepad_DPad_Up))RushSelection=(RushSelection+2)%3;
  if(Press(EKeys::Down)||Press(EKeys::Gamepad_DPad_Down))RushSelection=(RushSelection+1)%3;
  if(Confirm){
   if(Stage==8)RushCommand(8);
   else if(RushSelection==0){if(!RushPaused)RushCommand(6);}
   else RushCommand(RushSelection==1?7:8);
   RushPaused=false;RushScreenStage=-1;Accumulator=0;PlayerOwner->FlushPressedKeys();
   if(AudioWave)AudioWave->ResetAudio();
  }
  return true;
 }
 if(Stage==4){RushVisual=false;RushScreenStage=-1;RushAtmosphereTime+=FMath::Min(Dt,.05f);return false;}
 if(!RushVisual){
  RushVisual=true;RushDeath=Stage==5;RushFinal=Stage==6;RushTime=0;RushLoadingFrames=0;
  WireRooms.Reset();WireRooms.SetNum(2);WireSamus=nullptr;RushSamusTarget=nullptr;WirePairs.Reset();WireSamusEdges.Reset();RushSamusPairs.Reset();
  for(auto& Sprite:RushSprite)Sprite.Init(0,400*224*4);
  WireDirection=FMath::RandRange(0,3);
  CaptureRushWire(0,Stage==1 && RushInfo(5));
  if(RushDeath || RushFinal){CaptureRushWire(1,true);MatchRushWire();}
  else RushCommand(0);
  if(AudioWave)AudioWave->ResetAudio();
  if(SystemMenu)SystemMenu->SetOpen(false);
  PlayerOwner->FlushPressedKeys();Accumulator=0;
  UE_LOG(LogTemp,Display,TEXT("SM_RUSH_TRANSITION stage=%d encounter=%d practice=%d"),Stage,RushInfo(1),RushInfo(3));
 }
 if(RushInfo(0)==2){
  // Bounded loading work; native AI and inputs remain gated until reveal.
  for(int I=0;I<12 && RushInfo(0)==2;I++){
   if(!Step(0)){Failure=UTF8_TO_TCHAR(Error());return true;}
   if(++RushLoadingFrames>1800){RushCommand(4);RushVisual=false;Failure=TEXT("Boss Rush arena failed to become ready.");return true;}
  }
  if(RushInfo(0)!=3)return true;
  CaptureRushWire(1,false);if(RushInfo(5))RushSamusPos[0]=RushSamusPos[1];MatchRushWire();
  UE_LOG(LogTemp,Display,TEXT("SM_RUSH_ARENA_READY room=%04x frames=%d edges=%d"),Room(),RushLoadingFrames,WirePairs.Num());
 }
 if(RushTime==0 && RushAudioTransition){
  RushAudioTransition(0);
  UE_LOG(LogTemp,Display,TEXT("SM_RUSH_AUDIO_SWEEP forward encounter=%d cursor=%llu"),RushInfo(1),RushAudioStatus?RushAudioStatus(1):0);
 }
 const float PreviousTime=RushTime;
 RushTime+=FMath::Min(Dt,.05f);
 const float ReverseAt=RushDeath?4.05f:2.05f;
 if(PreviousTime<ReverseAt && RushTime>=ReverseAt && RushAudioTransition){
  RushAudioTransition(1);
  UE_LOG(LogTemp,Display,TEXT("SM_RUSH_AUDIO_SWEEP reverse encounter=%d cursor=%llu"),RushInfo(1),RushAudioStatus?RushAudioStatus(1):0);
 }
 // Let every death fragment fade, followed by a short clean blackout.
 const float Duration=RushDeath?5.6f:2.7f;
 if(RushTime>=Duration){
  if(RushDeath || RushFinal){RushResultTime=0;RushScreenStage=-1;}
  RushCommand(RushDeath?2:RushFinal?3:1);
  RushVisual=false;Accumulator=0;PlayerOwner->FlushPressedKeys();
  if(AudioWave)AudioWave->ResetAudio();
  RefreshScenePresentation();
 }
 return true;
}
bool ASMHUD::DrawBossRush(){
 if(RushVisual){
  if(WireRooms.Num()==2 && WireRooms[0] && WireRooms[1] && WireGlow)DrawWireLab();
  return true;
 }
 if(!RushInfo || !RushInfo(0))return false;
 const int Stage=RushInfo(0);
 if(Stage==7 || Stage==8 || (Stage==4 && RushPaused)){
  // Only the supplied 16:9 versions: preserve aspect ratio on every display.
  const float S=FMath::Min(Canvas->SizeX/800.f,Canvas->SizeY/450.f);
  const float X=(Canvas->SizeX-800*S)*.5f,Y=(Canvas->SizeY-450*S)*.5f;
  if(RushPaused){
   const float SW=UseWide()?400.f:256.f;
   const float Fit=FMath::Min(Canvas->SizeX/SW,Canvas->SizeY/224.f);
   const float GS=ImageScaling==1 && Fit>=1?FMath::FloorToFloat(Fit):Fit;
   const float GX=(Canvas->SizeX-SW*GS)*.5f,GY=(Canvas->SizeY-224*GS)*.5f;
   if(PresentMaterial){UpdatePresentation();DrawMaterial(PresentMaterial,GX,GY,SW*GS,224*GS,0,0,1,224.f/240.f);}
   else if(GameTexture)DrawTexture(GameTexture,GX,GY,SW*GS,224*GS,0,0,1,224.f/240.f,FLinearColor::White,BLEND_Opaque);
   DrawRect(FLinearColor(0,.005f,.015f,.65f),0,0,Canvas->SizeX,Canvas->SizeY);
  }else if(auto* Art=Stage==8?RushCompletedArt.Get():RushFailedArt.Get())
   DrawTexture(Art,X,Y,800*S,450*S,0,0,1,1,FLinearColor::White,BLEND_Opaque);
  if(!RushPaused)DrawRushResultEffects(X,Y,S,Stage==8);
  const float PX=RushPaused?240:45,PY=RushPaused?140:Stage==8?270:240;
  // Pause/failure panels, borders, cursor and lettering are native ROM pixels.
  if(Stage==8)DrawRect(FLinearColor(.005f,.015f,.03f,.88f),X+(PX-10)*S,Y+(PY-10)*S,340*S,180*S);
  if(RushLabels && RushScreen){
   auto& M=RushLabels->GetPlatformData()->Mips[0];
   FMemory::Memcpy(M.BulkData.Lock(LOCK_READ_WRITE),RushScreen(RushSelection,RushPaused),256*128*4);
   M.BulkData.Unlock();RushLabels->UpdateResource();
   DrawTexture(RushLabels,X+PX*S,Y+PY*S,320*S,160*S,0,0,1,1,FLinearColor::White,BLEND_Translucent);
  }
  if(!RushPaused){
   // Fade the complete composition: art, electrical effects and native labels.
   // Match the one-second smoothstep envelope of the one-shot music.
   const float T=FMath::Clamp(RushResultTime,0.f,1.f);
   const float Visibility=T*T*(3.f-2.f*T);
   if(Visibility<1.f)DrawRect(FLinearColor(0,0,0,1.f-Visibility),0,0,Canvas->SizeX,Canvas->SizeY);
  }
  return true;
 }
 return false;
}

void ASMHUD::DrawRushSimulation(float X,float Y,float Scale){
 if(!RushInfo || RushInfo(0)!=4 || RushPaused)return;
 const int Width=Widescreen && WideAvailable()?400:256;
 const float T=RushAtmosphereTime;
 // Sparse descending data, masked to background pixels. HUD, Samus, enemies
 // and solid architecture stay untouched; no flashing full-screen wash.
 const uint8* Meta=Width==400?WideLayers():Layers();
 if(!Meta)return;
 for(int Column=0;Column<28;Column++){
  const int CX=6+(Column*47)% (Width-12);
  const float Head=FMath::Fmod(T*(8+Column%5)+Column*29.f,210.f)+48;
  for(int Glyph=0;Glyph<7;Glyph++){
   const int CY=int(Head)-Glyph*7;
   if(CY<48 || CY>216)continue;
   const float Alpha=.11f*(1-Glyph/8.f);
   const uint32 Bits=uint32(Column*139+Glyph*73+int(T*.7f))*2654435761u;
   for(int DY=0;DY<4;DY++)for(int DX=0;DX<3;DX++)if((Bits>>(DY*3+DX))&1){
    const int I=((CY+DY)*Width+CX+DX)*4,L=Meta[I];
    if(!Meta[I+3] || (L!=1 && L!=5))continue;
    DrawRect(FLinearColor(.04f,.8f,.55f,Alpha),X+(CX+DX)*Scale,Y+(CY+DY)*Scale,Scale,Scale);
   }
  }
 }
 // A steady, low-contrast scan grid reinforces the training-room appearance.
 for(int CY=52;CY<224;CY+=24)for(int CX=0;CX<Width;CX+=4){
  const int I=(CY*Width+CX)*4;
  if(Meta[I+3] && (Meta[I]==1 || Meta[I]==5))
   DrawRect(FLinearColor(.08f,.5f,.55f,.025f),X+CX*Scale,Y+CY*Scale,3*Scale,.35f*Scale);
 }
}

void ASMHUD::TickBossRushTest(){
#if !UE_BUILD_SHIPPING
 if(!RushTest)return;
 ++RushTestTicks;
 const bool ResultAudioTest=FParse::Param(FCommandLine::Get(),TEXT("SMRushResultsTest"));
 const int ResultHold=ResultAudioTest?1140:100;
 static FKey PendingKey;static int ReleaseAt=0;
 auto Press=[&](FKey K){PlayerOwner->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,IE_Pressed,FPlatformTime::Cycles64()));PendingKey=K;ReleaseAt=RushTestTicks+1;};
 if(RushTestTicks==ReleaseAt)PlayerOwner->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,PendingKey,IE_Released,FPlatformTime::Cycles64()));
 auto Require=[&](bool OK,const TCHAR* Label){if(!OK){UE_LOG(LogTemp,Error,TEXT("SM_RUSH_UI_FAIL %s"),Label);PlayerOwner->ConsoleCommand(TEXT("quit"));}};
 const FString Path=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SMTests/BossRush"));
 auto Shot=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(Path/Name,false,false);};
 auto Request=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_rush_request")));
 auto RamFn=reinterpret_cast<const uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
 auto Cinema=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_cinematic")));
 uint8* Ram=const_cast<uint8*>(RamFn());
 auto Word=[&](int A){return int(Ram[A])|(int(Ram[A+1])<<8);};
 if(RushTestPhase==0){
  StartupWarning=-1;TitleInputFence=false;
  if(State()==1)Cinema(5);
  else if(State()==4 && Word(0x727)==4){Step(0);Step(0);Step(8);Step(0);}
  else if(State()==2 && Word(0xde2)==3){
   RushPractice(1);Request(4);RushTestPhase=1;
  }
 }
 const int Stage=RushInfo(0);
 if(Stage==1 && RushTestPhase==45)RushTestPhase=5;
 if(Stage==4){
  ++RushTestCombat;
  if(RushTestPhase==1 && RushInfo(1)==3 && RushTestCombat==60){RushPaused=true;Shot(TEXT("pause.png"));}
  if(RushTestPhase==1 && RushInfo(1)==3 && RushTestCombat==65)RushPaused=false;
  if(RushTestCombat==30)Shot(FString::Printf(TEXT("arena-%02d.png"),RushInfo(1)));
  if(RushTestPhase==2 && RushTestCombat==60){
   RushTestElapsed=RushInfo(6);
   Ram[0x9c2]=Ram[0x9c3]=0;Step(0);RushTestPhase=3;RushTestCombat=0;
  }else if(RushTestPhase==1 && RushTestCombat==90){RushCommand(5);}
  if(RushTestPhase==4){
   if(RushTestCombat==1)Require(RushInfo(9)==1 && RushInfo(6)>=RushTestElapsed,TEXT("Continue preserves failed time"));
   if(RushTestCombat==30)Press(EKeys::Escape);
   if(RushTestCombat==32){Require(RushPaused,TEXT("Pause opens"));RushTestElapsed=RushInfo(6);}
   if(RushTestCombat==34)Require(RushPaused && RushInfo(6)==RushTestElapsed,TEXT("Pause freezes timer"));
   if(RushTestCombat==35)Press(EKeys::Down);
   if(RushTestCombat==39)Require(RushSelection==1,TEXT("Retry selection"));
   if(RushTestCombat==40){Press(EKeys::Enter);RushTestPhase=45;}
  }else if(RushTestPhase==5){
   if(RushTestCombat==1)Require(RushInfo(1)==0 && RushInfo(6)==0 && RushInfo(8)==0 && RushInfo(9)==0,TEXT("Retry clears run"));
   if(RushTestCombat==30)Press(EKeys::Escape);
   if(RushTestCombat==35 || RushTestCombat==38)Press(EKeys::Down);
   if(RushTestCombat==41)Require(RushPaused && RushSelection==2,TEXT("End selection"));
   if(RushTestCombat==42){Press(EKeys::Enter);RushTestPhase=6;RushTestGameOver=0;}
  }
 }else RushTestCombat=0;
 if(RushVisual){
  if(RushTime>.32f && RushTime<.37f)Shot(FString::Printf(TEXT("scan-%02d-%d.png"),RushInfo(1),RushTestPhase));
  if(RushTime>1.2f && RushTime<1.27f)Shot(FString::Printf(TEXT("morph-%02d-%d.png"),RushInfo(1),RushTestPhase));
  if(RushTime>2.4f && RushTime<2.47f)Shot(FString::Printf(TEXT("reveal-%02d-%d.png"),RushInfo(1),RushTestPhase));
  if(RushDeath)Shot(FString::Printf(TEXT("death-%04d.png"),RushTestDeathFrame++));
 }
 if(Stage==8 && RushTestPhase==1){
  ++RushTestGameOver;
  if(RushTestGameOver==1)Shot(TEXT("results-fade-start.png"));
  if(RushTestGameOver==8)Shot(TEXT("results-fade-middle.png"));
  if(RushTestGameOver==35)Shot(TEXT("results-fade-end.png"));
  if(RushTestGameOver==15)Shot(TEXT("results.png"));
  if(RushTestGameOver==75)Shot(TEXT("results-animated.png"));
  if(ResultAudioTest && RushTestGameOver==1100){
   Require(RushAudioStatus && RushAudioStatus(3)==4 && RushAudioStatus(4)==1 && RushAudioStatus(1)==RushAudioStatus(5),TEXT("Success one-shot EOF"));
   UE_LOG(LogTemp,Display,TEXT("SM_RUSH_RESULT_AUDIO_EOF cue=4 cursor=%llu"),RushAudioStatus(1));
  }
  if(RushTestGameOver==ResultHold){RushCommand(4);RushPractice(0);Request(1);RushTestPhase=2;RushTestGameOver=0;}
 }
 if(Stage==7 && RushTestPhase==3){
  ++RushTestGameOver;
  if(RushTestGameOver==1)Shot(TEXT("failed-fade-start.png"));
  if(RushTestGameOver==8)Shot(TEXT("failed-fade-middle.png"));
  if(RushTestGameOver==35)Shot(TEXT("failed-fade-end.png"));
  if(RushTestGameOver==15)Shot(TEXT("game-over.png"));
  if(RushTestGameOver==75)Shot(TEXT("game-over-animated.png"));
  if(ResultAudioTest && RushTestGameOver==1100){
   Require(RushAudioStatus && RushAudioStatus(3)==3 && RushAudioStatus(4)==1 && RushAudioStatus(1)==RushAudioStatus(5),TEXT("Failed one-shot EOF"));
   UE_LOG(LogTemp,Display,TEXT("SM_RUSH_RESULT_AUDIO_EOF cue=3 cursor=%llu"),RushAudioStatus(1));
  }
  if(RushTestGameOver==ResultHold){Press(EKeys::Enter);RushTestPhase=4;}
 }
 if(Stage==0 && RushTestPhase==6 && ++RushTestGameOver==60){
  Require(State()==1,TEXT("End returns to native title"));
  UE_LOG(LogTemp,Display,TEXT("SM_RUSH_TEST_COMPLETE arenas=10 death=1 opcodes=%llu"),Opcodes());
  UE_LOG(LogTemp,Display,TEXT("SM_RUSH_UI_PASS continue=1 pause=1 retry=1 end=1"));
  Shot(TEXT("end-title.png"));
 }
 if((RushTestPhase==6 && RushTestGameOver>=65) || RushTestTicks>7000)PlayerOwner->ConsoleCommand(TEXT("quit"));
#endif
}
