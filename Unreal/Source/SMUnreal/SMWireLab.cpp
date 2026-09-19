#include "SMHUD.h"
#include "CanvasItem.h"
#include "GlobalRenderResources.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace {
UTexture2D* WireTexture(const TArray<uint8>& Bytes,int W,int H,TextureFilter Filter=TF_Nearest){
    auto* T=UTexture2D::CreateTransient(W,H,PF_B8G8R8A8);
    T->Filter=Filter;T->SRGB=false;T->NeverStream=true;
    auto& M=T->GetPlatformData()->Mips[0];
    FMemory::Memcpy(M.BulkData.Lock(LOCK_READ_WRITE),Bytes.GetData(),W*H*4);
    M.BulkData.Unlock();T->UpdateResource();return T;
}
float Ease(float V){V=FMath::Clamp(V,0.f,1.f);return V*V*V*(V*(V*6-15)+10);}
}

bool ASMHUD::PrepareWireLab(){
#if !UE_BUILD_SHIPPING
    auto RamFn=reinterpret_cast<const uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
    auto Equipment=reinterpret_cast<int(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_all_equipment")));
    auto Front=reinterpret_cast<int(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_test_wire_pose")));
    if(!RamFn || !Equipment || !Front || !WideScene || !WideLayers)return false;
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
    if(State()!=8 || !Wait(440) || !Equipment())return false;
    WirePath=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SMTests/WireTransition"));
    IFileManager::Get().MakeDirectory(*WirePath,true);
    TArray<FWireEdge> Edges[2];
    TArray<uint8> Sprite;Sprite.SetNumZeroed(400*224*4);
    const int Rooms[]={0x948c,0x96ba};
    for(int R=0;R<2;R++){
        // Real native rooms, fully settled before snapshotting. All save writes
        // belong to SMTests; the user's slot is never used as the live save.
        if(!TestRoom(Rooms[R],128,160) || !Wait(900) || !Front() || !Wait(6) || State()!=8 || Opcodes())return false;
        TArray<uint8> PixelsCopy;PixelsCopy.Append(WideScene(),400*224*4);
        const uint8* Meta=WideLayers();
        TArray<uint8> Raw;Raw.Append(Meta,400*240*4);
        FFileHelper::SaveArrayToFile(Raw,*(WirePath/FString::Printf(TEXT("layers-%d.bgra"),R)));
        FFileHelper::SaveArrayToFile(PixelsCopy,*(WirePath/FString::Printf(TEXT("room-%d.bgra"),R)));
        const int SX=SamusX()-CameraX()+72,SY=SamusY()-CameraY();
        int SpritePixels=0;
        for(int Y=FMath::Max(32,SY-30);Y<FMath::Min(224,SY+29);Y++)for(int X=FMath::Max(0,SX-16);X<FMath::Min(400,SX+17);X++){
            const int I=(Y*400+X)*4;
            if(!Meta[I+3] || (Meta[I]!=4 && Meta[I]!=6))continue;
            if(R==0){FMemory::Memcpy(Sprite.GetData()+I,PixelsCopy.GetData()+I,4);Sprite[I+3]=255;}
            // Remove Samus from the frozen backdrop, replacing only her sprite
            // pixels with the nearest non-sprite sample on the same scanline.
            int J=I;
            for(int D=1;D<40;D++){
                const int XX=X+(X<SX?-D:D);
                if(XX<0 || XX>=400)break;
                const int K=(Y*400+XX)*4;
                if(Meta[K]!=4 && Meta[K]!=6){J=K;break;}
            }
            FMemory::Memcpy(PixelsCopy.GetData()+I,WideScene()+J,4);++SpritePixels;
        }
        for(int I=3;I<PixelsCopy.Num();I+=4)PixelsCopy[I]=255;
        WireRooms.Add(WireTexture(PixelsCopy,400,224));
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
        auto Edge=[&](float X,float Y,float XX,float YY,bool Boundary,uint8 Kind=0){Edges[R].Add({FVector2D(X,Y),FVector2D(XX,YY),Boundary,Kind});};
        for(int Y=0;Y<56;Y++)for(int X=0;X<100;X++)if(Solid[Y][X]){
            Edge(X*4,Y*4,X*4+4,Y*4,false);
            Edge(X*4,Y*4,X*4,Y*4+4,false);
            if(!Occupied(X,Y+1))Edge(X*4,Y*4+4,X*4+4,Y*4+4,false);
            if(!Occupied(X+1,Y))Edge(X*4+4,Y*4,X*4+4,Y*4+4,false);
        }
        const int GridCount=Edges[R].Num();
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
        UE_LOG(LogTemp,Display,TEXT("SM_WIRE_DETAIL room=%04x grid_step=4 grid_edges=%d art_edges=%d"),Room(),GridCount,Edges[R].Num()-GridCount);
        UE_LOG(LogTemp,Display,TEXT("SM_WIRE_ROOM room=%04x edges=%d pose=%04x samus=%d,%d extracted=%d"),Room(),Edges[R].Num(),Word(0xa1c),SX,SY,SpritePixels);
        if(Edges[R].Num()<20 || SpritePixels<50)return false;
    }
    WireSamus=WireTexture(Sprite,400,224);
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
        if(Detail(X,Y-1))WireSamusEdges.Add({FVector2D(X,Y),FVector2D(X+1,Y),!SP(X,Y-1),1});
        if(Detail(X-1,Y))WireSamusEdges.Add({FVector2D(X,Y),FVector2D(X,Y+1),!SP(X-1,Y),1});
        if(!SP(X,Y+1))WireSamusEdges.Add({FVector2D(X,Y+1),FVector2D(X+1,Y+1),true,1});
        if(!SP(X+1,Y))WireSamusEdges.Add({FVector2D(X+1,Y),FVector2D(X+1,Y+1),true,1});
    }
    // Exact correspondence first; spatial buckets keep detailed art matching
    // bounded. All remaining movement is between parallel straight segments.
    auto EdgeKey=[](const FWireEdge& E){return uint64(E.A.X)|(uint64(E.A.Y)<<9)|(uint64(E.B.X)<<17)|(uint64(E.B.Y)<<26)|(uint64(E.Kind)<<34);};
    auto BucketKey=[](int X,int Y,const FWireEdge& E){return X+Y*32+int(E.A.X==E.B.X)*1024+int(E.Kind)*2048;};
    TMap<uint64,int> Exact;
    TMultiMap<int,int> Buckets;
    for(int J=0;J<Edges[1].Num();J++){
        const auto& E=Edges[1][J];Exact.Add(EdgeKey(E),J);
        auto Mid=(E.A+E.B)*.5;Buckets.Add(BucketKey(int(Mid.X)/16,int(Mid.Y)/16,E),J);
    }
    TArray<bool> Used;Used.Init(false,Edges[1].Num());
    TArray<int> Matches;Matches.Init(-1,Edges[0].Num());
    for(int I=0;I<Edges[0].Num();I++)if(const int* J=Exact.Find(EdgeKey(Edges[0][I]))){
        if(!Used[*J]){Matches[I]=*J;Used[*J]=true;}
    }
    TArray<int> Candidates;
    int Common=0,Moved=0;
    for(int I=0;I<Edges[0].Num();I++){
        const auto& E=Edges[0][I];const auto Mid=(E.A+E.B)*.5;
        int Best=Matches[I];double Score=1.e30;
        if(Best>=0)++Common;
        else for(int Y=FMath::Max(0,int(Mid.Y)/16-2);Y<=FMath::Min(14,int(Mid.Y)/16+2);Y++)for(int X=FMath::Max(0,int(Mid.X)/16-2);X<=FMath::Min(25,int(Mid.X)/16+2);X++){
            Candidates.Reset();Buckets.MultiFind(BucketKey(X,Y,E),Candidates);
            for(int J:Candidates)if(!Used[J]){
                const auto& D=Edges[1][J];
                double S=FVector2D::DistSquared(Mid,(D.A+D.B)*.5);
                if(S>32*32)continue;
                if(E.Boundary!=D.Boundary)S+=250;
                if(S<Score){Score=S;Best=J;}
            }
        }
        if(Best>=0){Used[Best]=true;WirePairs.Add({E,Edges[1][Best]});if(Matches[I]<0)++Moved;}
        else WirePairs.Add({E,{Mid,Mid,false,E.Kind}});
    }
    for(int J=0;J<Edges[1].Num();J++)if(!Used[J]){
        const auto& E=Edges[1][J];const auto Mid=(E.A+E.B)*.5;
        WirePairs.Add({{Mid,Mid,false,E.Kind},E});
    }
    UE_LOG(LogTemp,Display,TEXT("SM_WIRE_MATCH common=%d moved=%d samus_edges=%d spatial_wave=0"),Common,Moved,WireSamusEdges.Num());
    int BentSegments=0;
    for(const auto& Pair:WirePairs)for(float M:{.125f,.5f,.875f}){
        const auto A=FMath::Lerp(Pair.From.A,Pair.To.A,M),B=FMath::Lerp(Pair.From.B,Pair.To.B,M);
        BentSegments+=FMath::Abs(A.X-B.X)>.0001 && FMath::Abs(A.Y-B.Y)>.0001;
    }
    UE_LOG(LogTemp,Display,TEXT("SM_WIRE_RIGID diagonal_segments=%d"),BentSegments);
    if(BentSegments)return false;
    TArray<uint8> Glow;Glow.SetNumUninitialized(64*64*4);
    for(int Y=0;Y<64;Y++)for(int X=0;X<64;X++){
        float R=FMath::Square((X-31.5f)/31.5f)+FMath::Square((Y-31.5f)/31.5f);
        uint8 V=uint8(255*FMath::Max(0.f,(FMath::Exp(-R*6)-FMath::Exp(-6.f))/(1-FMath::Exp(-6.f))));
        int I=(Y*64+X)*4;Glow[I]=Glow[I+1]=Glow[I+2]=V;Glow[I+3]=255;
    }
    WireGlow=WireTexture(Glow,64,64,TF_Bilinear);
    TestFrames=0;StartupWarning=-1;TitleInputFence=false;Accumulator=0;
    WireNativeFrame=Frame();WireRamHash=FCrc::MemCrc32(Ram,0x20000);WireDirection=FMath::RandRange(0,3);
    UE_LOG(LogTemp,Display,TEXT("SM_WIRE_READY pairs=%d direction=%d frame=%d ram=%08x"),WirePairs.Num(),WireDirection,WireNativeFrame,WireRamHash);
    return true;
#else
    return false;
#endif
}

void ASMHUD::TickWireLab(){
#if !UE_BUILD_SHIPPING
    // Deliberately no native Step, user-input dispatch, menu tick or timer tick.
    ++WireTicks;
    if(WireTicks<=120)return;
    WireFrame=WireTicks-121;
    if(WireFrame<420){
        FScreenshotRequest::RequestScreenshot(WirePath/FString::Printf(TEXT("frame-%04d.png"),WireFrame),false,false);
    }else{
        auto RamFn=reinterpret_cast<const uint8*(*)()>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_simulation_ram")));
        const bool Frozen=Frame()==WireNativeFrame && FCrc::MemCrc32(RamFn(),0x20000)==WireRamHash;
        UE_LOG(LogTemp,Display,TEXT("SM_WIRE_COMPLETE frames=420 native_frozen=%d cpu_opcodes=%llu"),Frozen,Opcodes());
        PlayerOwner->FlushPressedKeys();PlayerOwner->ConsoleCommand(TEXT("quit"));
    }
#endif
}

void ASMHUD::DrawWireLab(){
    const double DrawStart=FPlatformTime::Seconds();
    int SubmittedTriangles=0,SubmittedBatches=0;
    const float Scale=FMath::Min(Canvas->SizeX/(RushVisual && !Widescreen?256.f:400.f),Canvas->SizeY/224.f);
    const float OX=(Canvas->SizeX-400*Scale)*.5f,OY=(Canvas->SizeY-224*Scale)*.5f;
    // Two seven-second examples, reversed geometry and different sweep edge.
    const int Pass=RushVisual?0:WireFrame/210;
    const float T=RushVisual?(RushTime<.65f?1.f+RushTime/.65f: RushTime<2.05f?2.f+(RushTime-.65f)*2.5f/1.4f: 4.5f+(RushTime-2.05f)/.65f):(WireFrame%210)/30.f;
    const int From=Pass%2,To=1-From,Direction=(WireDirection+Pass)%4;
    const float Scan=FMath::Clamp((T-1.f)/1.f,0.f,1.f);
    const float Morph=Ease((T-2.f)/2.5f),Reveal=FMath::Clamp((T-4.5f)/1.f,0.f,1.f);
    auto Coord=[&](FVector2D P){return Direction==0?float(P.X/400):Direction==1?float(1-P.X/400):Direction==2?float(P.Y/224):float(1-P.Y/224);};
    auto Region=[&](int RoomIndex,bool ShowConverted,float Progress){
        if(Progress<=0 && ShowConverted)return;
        if(Progress>=1 && !ShowConverted)return;
        float L=0,Top=0,R=400,B=224;
        const float V=ShowConverted?Progress:1-Progress;
        if(Direction==0){if(ShowConverted)R=400*V;else L=400*(1-V);}
        if(Direction==1){if(ShowConverted)L=400*(1-V);else R=400*V;}
        if(Direction==2){if(ShowConverted)B=224*V;else Top=224*(1-V);}
        if(Direction==3){if(ShowConverted)Top=224*(1-V);else B=224*V;}
        if(R>L && B>Top)DrawTexture(WireRooms[RoomIndex],OX+L*Scale,OY+Top*Scale,(R-L)*Scale,(B-Top)*Scale,L/400,Top/224,(R-L)/400,(B-Top)/224,FLinearColor::White,BLEND_Opaque);
    };
    if(T<2)Region(From,false,Scan);
    if(T>=4.5f)Region(To,true,Reveal);
    // World-space ribbon vertices share a transform and texture. Rotating one
    // Canvas tile per edge previously created tens of thousands of render batches.
    TArray<FCanvasUVTri> Lines,Halos;
    Lines.Reserve(WirePairs.Num()*3);Halos.Reserve(2048);
    auto Quad=[&](TArray<FCanvasUVTri>& List,FVector2D A,FVector2D B,FVector2D C,FVector2D D,FLinearColor Color){
        FCanvasUVTri U,V;
        U.V0_Pos=A;U.V1_Pos=B;U.V2_Pos=C;
        V.V0_Pos=A;V.V1_Pos=C;V.V2_Pos=D;
        U.V0_UV=V.V0_UV=FVector2D(0,0);U.V1_UV=FVector2D(1,0);
        U.V2_UV=V.V1_UV=FVector2D(1,1);V.V2_UV=FVector2D(0,1);
        U.V0_Color=U.V1_Color=U.V2_Color=V.V0_Color=V.V1_Color=V.V2_Color=Color;
        List.Add(U);List.Add(V);
    };
    auto Flush=[&]{
        auto Draw=[&](TArray<FCanvasUVTri>& List,const FTexture* Texture){
            if(List.IsEmpty())return;
            SubmittedTriangles+=List.Num();++SubmittedBatches;
            FCanvasTriangleItem Batch(FVector2D::ZeroVector,FVector2D::ZeroVector,FVector2D::ZeroVector,Texture);
            Swap(Batch.TriangleList,List);Batch.BlendMode=SE_BLEND_Additive;Canvas->DrawItem(Batch);
            Swap(Batch.TriangleList,List);List.Reset();
        };
        Draw(Lines,GWhiteTexture);Draw(Halos,WireGlow->GetResource());
    };
    auto Glow=[&](FVector2D P,float Radius,FLinearColor Color,float Alpha){
        if(TransitionQuality==0 || Alpha<=.001f)return;
        const FVector2D A(OX+(P.X-Radius)*Scale,OY+(P.Y-Radius)*Scale),B=A+FVector2D(Radius*2*Scale,Radius*2*Scale);
        Color*=Alpha;
        Quad(Halos,A,FVector2D(B.X,A.Y),B,FVector2D(A.X,B.Y),Color);
    };
    auto Line=[&](FVector2D A,FVector2D B,FLinearColor C,float Width){
        if(C.A<=.001f)return;
        FVector2D D=B-A;const double Length=D.Size();if(Length<.0001)return;
        C.R*=C.A;C.G*=C.A;C.B*=C.A;C.A=1;
        const FVector2D N(-D.Y/Length*Width*.5f*Scale,D.X/Length*Width*.5f*Scale);
        A=FVector2D(OX,OY)+A*Scale;B=FVector2D(OX,OY)+B*Scale;
        Quad(Lines,A-N,B-N,B+N,A+N,C);
    };
    const float Lift=4.f*Morph*(1.f-Morph);
    auto Project=[&](FVector2D P,float WireDepth){
        const float Z=WireDepth*Lift;
        const float Perspective=420.f/(420.f+Z);
        return FVector2D(200+(P.X-200)*Perspective+Z*.24f,128+(P.Y-128)*Perspective-Z*.3f);
    };
    auto Visible=[&](FVector2D P){const float C=Coord(P);return T<2?C<Scan:T>=4.5f?C>=Reveal:true;};
    int Index=0;
    if(T>=1 && T<5.5f)for(const auto& Pair:WirePairs){
        const auto& A=From==0?Pair.From:Pair.To;const auto& B=From==0?Pair.To:Pair.From;
        // Keep silhouettes at every tier. Thin interior subdivisions scale down.
        const int Stride=TransitionQuality==0?4:TransitionQuality==1?2:1;
        if(!A.Boundary && !B.Boundary && Index%Stride){++Index;continue;}
        FVector2D P=FMath::Lerp(A.A,B.A,Morph),Q=FMath::Lerp(A.B,B.B,Morph);
        const auto Mid=(P+Q)*.5;
        if(!Visible(Mid)){++Index;continue;}
        // A single rigid plane for the entire mesh. No spatial sine wave,
        // per-segment curl, bending or wandering endpoint illumination.
        const float WireDepth=18.f;
        FVector2D PP=Project(P,WireDepth),QQ=Project(Q,WireDepth);
        if(FVector2D::DistSquared(PP,QQ)<.01){++Index;continue;}
        const float Boundary=FMath::Lerp(A.Boundary?1.f:A.Kind?.52f:.28f,B.Boundary?1.f:B.Kind?.52f:.28f,Morph);
        const float L=.62f;
        const FLinearColor C(.06f,.64f,.95f,1);
        if(TransitionQuality>=2 && Boundary>.8f && Index%4==0){
            FVector2D BackP=Project(P,WireDepth+12),BackQ=Project(Q,WireDepth+12);
            Line(BackP,BackQ,FLinearColor(.015f,.12f,.22f,1),.36f);
            if(Index%8==0)Line(PP,BackP,FLinearColor(.03f,.26f,.39f,1),.4f);
        }
        if(TransitionQuality>=1 && Boundary>.7f)Line(PP,QQ,FLinearColor(.015f,.10f,.17f,1)*Boundary,1.6f);
        FLinearColor Ink=FLinearColor(.20f,.78f,1,1)*Boundary;
        // Thin grid ink needs full coverage, not a second alpha attenuation;
        // otherwise the dense squares disappear in the encoded video.
        if(!A.Kind)Ink.A=1;
        Line(PP,QQ,Ink,A.Kind?.4f:.55f);
        if(Index%(TransitionQuality>=3?7:TransitionQuality==2?11:33)==0 && Boundary>.7f){Glow(PP,3.5f,C,L*.32f);Glow(PP,.85f,FLinearColor(.65f,.95f,1),.8f);}
        ++Index;
    }
    Flush(); // Preserve layering: arena behind the captured Samus sprite.
    const bool BlackEntry=RushVisual && RushInfo(5);
    const bool SamusMorph=RushVisual && !RushDeath && !RushFinal;
    const FVector2D SamusOffset=SamusMorph?(RushSamusPos[1]-RushSamusPos[0])*Morph:FVector2D::ZeroVector;
    const FVector2D SamusCenter=RushVisual?RushSamusPos[0]+SamusOffset:FVector2D(200,152);
    const float SamusWire=RushVisual && (RushDeath || RushFinal) && T>=2?1.f:T<2?Ease((Scan-Coord(SamusCenter)+.10f)/.20f):T>=4.5f?1-Ease((Reveal-Coord(SamusCenter)+.10f)/.20f):1;
    // The reveal uses the captured destination sprite, including its suit and
    // pose. The departing sprite must not flash back at the end of the morph.
    if(SamusWire<1 && (!BlackEntry || T>=4.5f))DrawTexture(SamusMorph && T>=4.5f?RushSamusTarget:WireSamus,OX,OY,400*Scale,224*Scale,0,0,1,1,FLinearColor(1,1,1,1-SamusWire),BLEND_Translucent);
    if(SamusWire>0 && (!BlackEntry || T>=2)){
        const float Burst=RushVisual && RushDeath?FMath::Clamp((RushTime-4.05f)/1.35f,0.f,1.f):0.f;
        const float ExitFade=RushVisual && RushFinal?1-FMath::Clamp((RushTime-2.05f)/.65f,0.f,1.f):1.f;
        if(SamusMorph){
            const auto Travel=RushSamusPos[1]-RushSamusPos[0];
            const float StretchX=FMath::Clamp(float(FMath::Abs(Travel.X))/96.f,0.f,1.f)*Lift;
            const float StretchY=FMath::Clamp(float(FMath::Abs(Travel.Y))/96.f,0.f,1.f)*Lift;
            auto SamusProject=[&](FVector2D P){
                const auto Local=P-SamusCenter;
                // A brief directional stretch makes relocation visible even
                // when both arenas use the exact same frontal armor sprite.
                // It is zero at either endpoint and preserves straight lines.
                return Project(SamusCenter+FVector2D(Local.X*(1+.35f*StretchX-.12f*StretchY),Local.Y*(1+.35f*StretchY-.12f*StretchX)),18.f);
            };
            for(const auto& Pair:RushSamusPairs){
                const auto A=FMath::Lerp(Pair.From.A,Pair.To.A,Morph),B=FMath::Lerp(Pair.From.B,Pair.To.B,Morph);
                // Same perspective plane and timing as the arena. Individual
                // segment correspondence reshapes Samus while she relocates.
                const auto P=SamusProject(A),Q=SamusProject(B);
                if(FVector2D::DistSquared(P,Q)<.01)continue;
                const float Bright=FMath::Lerp(Pair.From.Boundary?1.f:.55f,Pair.To.Boundary?1.f:.55f,Morph);
                Line(P,Q,FLinearColor(.25f,.9f,1,1)*(SamusWire*Bright),Bright>.75f?.5f:.35f);
            }
        }else{
        // Death keeps Samus intact until her two-second hold ends.
        int Fragment=0;
        for(const auto& E:WireSamusEdges){
            FVector2D Offset=SamusOffset;
            if(Burst>0){
                float Angle=Fragment*2.39996323f;
                Offset+=FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*(80+(Fragment%31)*7)*FMath::Pow(Burst,.7f);
                Offset.Y+=Burst*Burst*42;
            }
            Line(E.A+Offset,E.B+Offset,FLinearColor(.25f,.9f,1,1)*(SamusWire*(E.Boundary?1.f:.55f)*(1-Burst)*ExitFade),E.Boundary?.5f:.35f);
            if(Burst>0 && Fragment%5==0){
                const auto P=(E.A+E.B)*.5+Offset;
                Glow(P,4+(Fragment%5),FLinearColor(.1f,.6f,1),.7f*(1-Burst));
                Glow(P,1.1f,FLinearColor(.8f,.95f,1),1-Burst);
                const FVector2D Tail=Offset*.07f;
                Line(P,P-Tail,FLinearColor(.35f,.85f,1,.8f*(1-Burst)),.65f);
            }
            ++Fragment;
        }
        }
        if(Burst>0){
            const float Flash=FMath::Sin(FMath::Min(Burst*4.f,1.f)*PI)*.8f*FMath::Clamp(FlashStrength,0.f,1.f);
            Glow(SamusCenter,90+120*Burst,FLinearColor(.05f,.4f,1),FMath::Max(0.f,Flash));
            Glow(SamusCenter,32+65*Burst,FLinearColor(.8f,.95f,1),FMath::Max(0.f,Flash));
            for(int I=0;I<36;I++){
                const float A=I*2.39996323f;
                const FVector2D D(FMath::Cos(A),FMath::Sin(A));
                const auto P=SamusCenter+D*(15+230*Burst)*(1+(I%4)*.13f);
                Line(P-D*(8+24*Burst),P,FLinearColor(.25f,.75f,1,1-Burst),.8f);
                Glow(P,3,FLinearColor(.2f,.7f,1),1-Burst);
            }
        }
        const auto LightCenter=SamusCenter-FVector2D(0,7);
        Glow(SamusMorph?Project(LightCenter,18.f):LightCenter,24,FLinearColor(.03f,.32f,.5f),.2f*SamusWire*(1-Burst)*ExitFade);
    }
    if((T>=1 && T<2) || (T>=4.5f && T<5.5f)){
        float P=T<2?Scan:Reveal;
        FVector2D A,B;
        if(Direction<2){float X=400*(Direction==0?P:1-P);A=FVector2D(X,0);B=FVector2D(X,224);}
        else{float Y=224*(Direction==2?P:1-P);A=FVector2D(0,Y);B=FVector2D(400,Y);}
        Line(A,B,FLinearColor(.05f,.27f,.4f,1),3.f);
        Line(A,B,FLinearColor(.7f,.95f,1,1),.6f);
        for(int I=0;I<25;I++)Glow(FMath::Lerp(A,B,I/24.f),6,FLinearColor(.03f,.5f,.9f),.22f);
    }
    Flush();
#if !UE_BUILD_SHIPPING
    if(RushTest)UE_LOG(LogTemp,Display,TEXT("SM_WIRE_PERF tier=%d pairs=%d triangles=%d batches=%d cpu_ms=%.3f"),TransitionQuality,WirePairs.Num(),SubmittedTriangles,SubmittedBatches,(FPlatformTime::Seconds()-DrawStart)*1000);
#endif
}
