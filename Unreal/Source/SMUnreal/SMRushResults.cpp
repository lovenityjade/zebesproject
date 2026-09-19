#include "SMHUD.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/Texture2D.h"

void ASMHUD::PrepareRushResultEffects(){
 RushResultPaths.Reset();
 if(!RushCompletedArt || !RushCompletedArt->GetPlatformData())return;
 auto& M=RushCompletedArt->GetPlatformData()->Mips[0];
 const int W=M.SizeX,H=M.SizeY;
 if(RushCompletedArt->GetPlatformData()->PixelFormat!=PF_B8G8R8A8 || M.BulkData.GetBulkDataSize()<int64(W)*H*4)return;
 const FColor* Source=static_cast<const FColor*>(M.BulkData.LockReadOnly());
 // Sample the authored blue ink. Moving lights cannot invent another diagram
 // or wander across the title, the result panel, or Samus's painted armor.
 const int GW=(W+1)/2,GH=(H+1)/2;
 TArray<uint8> Ink;Ink.Init(0,GW*GH);
 for(int Y=0;Y<GH;Y++)for(int X=0;X<GW;X++){
  const float U=X*2.f/W*800,V=Y*2.f/H*450;
  if(U>570 || (U<365 && V>180 && V<258) || (U<380 && V>=258))continue;
  for(int DY=0;DY<2 && Y*2+DY<H;DY++)for(int DX=0;DX<2 && X*2+DX<W;DX++){
   const FColor C=Source[(Y*2+DY)*W+X*2+DX];
   if(C.B>C.G+5 && C.G>C.R+5 && C.B>25 && C.R<60)Ink[Y*GW+X]=1;
  }
 }
 // The visor mask comes from its original green pigment, excluding the dark
 // pupil and the helmet. It is a runtime light mask, not replacement artwork.
 RushVisorMask=UTexture2D::CreateTransient(256,144,PF_B8G8R8A8);
 RushVisorMask->SRGB=false;RushVisorMask->Filter=TF_Bilinear;
 auto& VM=RushVisorMask->GetPlatformData()->Mips[0];
 uint8* MaskPixels=static_cast<uint8*>(VM.BulkData.Lock(LOCK_READ_WRITE));
 for(int Y=0;Y<144;Y++)for(int X=0;X<256;X++){
  const FColor C=Source[FMath::Min(H-1,Y*H/144)*W+FMath::Min(W-1,X*W/256)];
  const bool Green=X>184 && Y>33 && Y<88 && C.G>C.R*1.06f && C.G>C.B*1.25f && C.G>35;
  const uint8 Value=Green?uint8(FMath::Min(180,int(C.G))):0;
  uint8* P=MaskPixels+(Y*256+X)*4;P[0]=P[1]=P[2]=Value;P[3]=255;
 }
 VM.BulkData.Unlock();RushVisorMask->UpdateResource();M.BulkData.Unlock();
 const FVector2D Seeds[]={{84,34},{48,72},{124,65},{241,88},{274,45},{248,157},
  {409,63},{478,83},{441,163},{454,216},{507,115},{385,29},{531,203},{108,143}};
 for(int N=0;N<UE_ARRAY_COUNT(Seeds);N++){
  int At=-1;float Best=1.e30f;
  for(int I=0;I<Ink.Num();I++)if(Ink[I]){
   const FVector2D P((I%GW)*2.f/W*800,(I/GW)*2.f/H*450);
   const float D=FVector2D::DistSquared(P,Seeds[N]);if(D<Best){Best=D;At=I;}
  }
  if(At<0)continue;
  TArray<uint8> Visited;Visited.Init(0,Ink.Num());TArray<FVector2D> Route;
  FVector2D Direction(N%2?1:0,N%2?0:1);
  for(int Hop=0;Hop<500;Hop++){
   Visited[At]=1;const int X=At%GW,Y=At/GW;
   Route.Add(FVector2D((X*2+1.f)/W*800,(Y*2+1.f)/H*450));
   int Next=-1;float Score=-1.e30f;FVector2D NextDirection;
   for(int DY=-1;DY<=1;DY++)for(int DX=-1;DX<=1;DX++){
    if(!DX && !DY)continue;
    const int XX=X+DX,YY=Y+DY,I=YY*GW+XX;
    if(XX<0 || XX>=GW || YY<0 || YY>=GH || !Ink[I] || Visited[I])continue;
    const FVector2D D=FVector2D(DX,DY).GetSafeNormal();
    const float S=FVector2D::DotProduct(D,Direction)*3+((I*13+N*29)%17)*.015f;
    if(S>Score){Score=S;Next=I;NextDirection=D;}
   }
   if(Next<0)break;At=Next;Direction=NextDirection;
  }
  if(Route.Num()>=12)RushResultPaths.Add(MoveTemp(Route));
 }
 UE_LOG(LogTemp,Display,TEXT("SM_RUSH_RESULT_EFFECTS source_ink_paths=%d visor_mask=1"),RushResultPaths.Num());
}

void ASMHUD::DrawRushResultEffects(float X,float Y,float S,bool Success){
 if(!WireGlow)return;
 const float T=RushResultTime,EffectGain=FMath::Clamp(FlashStrength,0.f,1.f);
 auto Glow=[&](FVector2D P,float R,FLinearColor C,float Gain){
  C.R*=Gain;C.G*=Gain;C.B*=Gain;C.A=1;
  DrawTexture(WireGlow,X+(P.X-R)*S,Y+(P.Y-R)*S,R*2*S,R*2*S,0,0,1,1,C,BLEND_Additive);
 };
 auto Line=[&](FVector2D A,FVector2D B,FLinearColor C,float Gain,float Width){
  if(Gain<.002f)return;
  C.R*=Gain;C.G*=Gain;C.B*=Gain;C.A=1;
  const FVector2D D=B-A;
  // Canvas lines are opaque regardless of alpha. An additive rotated quad
  // fades to transparent light instead of drawing a black wire at zero gain.
  FCanvasTileItem Ribbon(FVector2D(X+A.X*S,Y+(A.Y-Width*.5f)*S),FVector2D(D.Size()*S,Width*S),C);
  Ribbon.BlendMode=SE_BLEND_Additive;Ribbon.PivotPoint=FVector2D(0,.5);
  Ribbon.Rotation=FRotator(0,FMath::RadiansToDegrees(FMath::Atan2(D.Y,D.X)),0);
  Canvas->DrawItem(Ribbon);
 };
 if(Success){
  for(int N=0;N<RushResultPaths.Num();N++){
   const auto& Route=RushResultPaths[N];
   const float F=FMath::Fmod(T*(14+N%5)+N*11.f,float(Route.Num()-1));
   const float Fade=FMath::Min(1.f,FMath::Min(F/5,(Route.Num()-1-F)/5));
   for(int Tail=5;Tail>=0;Tail--){
    const float P=FMath::Max(0.f,F-Tail*1.3f);const int I=int(P);
    const FVector2D Pos=FMath::Lerp(Route[I],Route[FMath::Min(I+1,Route.Num()-1)],P-I);
    const float A=Fade*(1-Tail/6.f)*(.55f+.3f*EffectGain);
    Glow(Pos,5,FLinearColor(.08f,.55f,1),A*.55f);
    Glow(Pos,Tail?1.f:1.65f,FLinearColor(.55f,.92f,1),A);
   }
  }
  if(RushVisorMask){
   const float Sweep=.71f+FMath::Fmod(T*.038f,.38f);
   for(int B=-6;B<=6;B++){
    const float U=Sweep+B*.006f;if(U<.71f || U>=1)continue;
    const float Width=FMath::Min(.006f,1-U);
    const float A=FMath::Exp(-B*B/9.f)*(.1f+.14f*EffectGain);
    DrawTexture(RushVisorMask,X+U*800*S,Y,Width*800*S,450*S,U,0,Width,1,FLinearColor(A*.65f,A,A*.78f,1),BLEND_Additive);
   }
  }
  return;
 }
 const FVector2D Terminals[]={{480,72},{652,81},{483,208},{624,304},{578,38}};
 for(int N=0;N<UE_ARRAY_COUNT(Terminals);N++){
  const FVector2D A=Terminals[N];
  const float Pulse=FMath::Pow(FMath::Max(0.f,FMath::Sin(T*2.6f+N*2.2f)),5.f)*(.2f+.65f*EffectGain);
  const FVector2D End=A+FVector2D(N%2?-8:10,N==4?19:36);
  FVector2D Last=A;
  for(int I=1;I<=9;I++){
   const float Q=I/9.f;
   const float J=FMath::Sin(I*13.7f+FMath::FloorToFloat(T*12)*3.1f+N)*5*FMath::Sin(Q*PI);
   const FVector2D P=FMath::Lerp(A,End,Q)+FVector2D(J,J*.3f);
   Line(Last,P,FLinearColor(.08f,.32f,.8f),Pulse,2.1f);
   Line(Last,P,FLinearColor(.65f,.88f,1),Pulse,.55f);
   if(I%3==0)Glow(P,5,FLinearColor(.05f,.4f,1),Pulse*.45f);
   Last=P;
  }
  Glow(A,12,FLinearColor(.1f,.35f,1),Pulse*.5f);
  for(int I=0;I<10;I++){
   const float Clock=T+N*.61f+I*.21f,Period=3.1f;
   const float Age=FMath::Fmod(Clock,Period);if(Age>.8f)continue;
   const int Cycle=int(Clock/Period);const float Seed=N*31+I*17+Cycle*53;
   const FVector2D V(FMath::Sin(Seed*1.73f)*35, -18-FMath::Abs(FMath::Cos(Seed))*30);
   const FVector2D P=A+V*Age+FVector2D(0,90*Age*Age);
   const float Life=(1-Age/.8f)*(.3f+.6f*EffectGain);
   Line(P-V*.025f,P,FLinearColor(.4f,.75f,1),Life,.6f);
   Glow(P,4,FLinearColor(.08f,.45f,1),Life*.6f);
   Glow(P,1.2f,FLinearColor(.85f,.93f,1),Life);
  }
 }
 // Slow breathing from the existing red chamber; no full-screen flash.
 Glow(FVector2D(570,293),45,FLinearColor(.7f,.015f,.05f),.065f+.025f*FMath::Sin(T*1.4f));
}
