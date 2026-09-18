#include "SMHUD.h"
#include "SMLocalization.h"
#include "ImageUtils.h"
#include "CanvasItem.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundWaveProcedural.h"

namespace {
constexpr float NativeSecond=736.f/44100.f;
float Ease(float V){V=FMath::Clamp(V,0.f,1.f);return V*V*(3-2*V);}
float Random(int I){uint32 N=uint32(I+1);N^=N>>16;N*=0x7feb352du;N^=N>>15;N*=0x846ca68bu;N^=N>>16;return (N&0xffffff)/16777216.f;}
struct FPlacement {const TCHAR* Name;FVector4f Wide,Classic;float Depth;};
const FPlacement Places[]={
 {TEXT("chozo"),FVector4f(25,50,335,555),FVector4f(12,43,285,472),.7f},
 {TEXT("brain"),FVector4f(1130,0,440,373),FVector4f(835,0,352,299),.9f},
 {TEXT("phantoon"),FVector4f(995,405,142,235),FVector4f(735,422,113,188),1.f},
 {TEXT("draygon"),FVector4f(1120,410,350,223),FVector4f(834,484,280,178),1.1f},
 {TEXT("ridley"),FVector4f(1230,225,365,296),FVector4f(940,314,280,227),1.2f},
 {TEXT("kraid"),FVector4f(990,520,405,339),FVector4f(735,586,324,271),1.2f},
 {TEXT("botwoon"),FVector4f(1350,515,155,190),FVector4f(1025,536,124,152),1.3f},
 {TEXT("crocomire"),FVector4f(1270,660,220,193),FVector4f(954,696,176,154),1.3f},
 {TEXT("motherBrain"),FVector4f(1460,626,135,205),FVector4f(1095,651,100,152),1.3f},
 {TEXT("samus"),FVector4f(278,261,144,389),FVector4f(161,300,129,349),1.4f},
};
}

void ASMHUD::LoadTitlePresentation(){
    if(AutoTest && !TitleTest)return;
    const FString Directory=FPaths::ProjectContentDir()/TEXT("Title");
    FString Json;TSharedPtr<FJsonObject> Root;
    if(!FFileHelper::LoadFileToString(Json,*(Directory/TEXT("layers.json"))) ||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root){
        Failure=TEXT("Title artwork is missing. Restore Content/Title from the matching build.");return;
    }
    for(const auto& Pair:Root->Values){
        const auto Entry=Pair.Value->AsObject();if(!Entry)continue;
        const auto& Crop=Entry->GetArrayField(TEXT("crop"));if(Crop.Num()!=4)continue;
        UTexture2D* T=FImageUtils::ImportFileAsTexture2D(Directory/Entry->GetStringField(TEXT("file")));
        if(!T)continue;
        T->Filter=TF_Nearest;T->AddressX=TA_Clamp;T->AddressY=TA_Clamp;T->UpdateResource();
        TitleTextures.Add(FName(Pair.Key),T);
        TitleCrops.Add(FName(Pair.Key),FVector4f(Crop[0]->AsNumber(),Crop[1]->AsNumber(),Crop[2]->AsNumber(),Crop[3]->AsNumber()));
    }
    if(TitleTextures.Num()!=18){Failure=TEXT("Title artwork is incomplete. Restore Content/Title from the matching build.");return;}
    const TCHAR* Required[]={TEXT("creator"),TEXT("logo"),TEXT("skyWide"),TEXT("skyClassic"),TEXT("midWide"),TEXT("midClassic"),TEXT("frontWide"),TEXT("frontClassic")};
    for(const TCHAR* Key:Required)if(!TitleTextures.Contains(FName(Key))){Failure=TEXT("Title layer manifest does not match this build.");return;}
    for(const FPlacement& Place:Places)if(!TitleTextures.Contains(FName(Place.Name))){Failure=TEXT("Title character layer is missing.");return;}
    TitleGlow=UTexture2D::CreateTransient(64,64,PF_B8G8R8A8);TitleGlow->Filter=TF_Bilinear;
    auto& Bulk=TitleGlow->GetPlatformData()->Mips[0].BulkData;
    uint8* P=static_cast<uint8*>(Bulk.Lock(LOCK_READ_WRITE));
    for(int Y=0;Y<64;Y++)for(int X=0;X<64;X++){
        const float R=FMath::Square((X-31.5f)/31.5f)+FMath::Square((Y-31.5f)/31.5f);
        // Canvas additive blending adds RGB without multiplying texture alpha.
        // Store the falloff in RGB as well, otherwise every halo is a square.
        const int O=(Y*64+X)*4;P[O]=P[O+1]=P[O+2]=P[O+3]=uint8(255*FMath::Max(0.f,FMath::Exp(-R*5)-FMath::Exp(-5.f)));
    }
    Bulk.Unlock();TitleGlow->UpdateResource();
    TitleVignette=UTexture2D::CreateTransient(64,64,PF_B8G8R8A8);TitleVignette->Filter=TF_Bilinear;
    auto& VB=TitleVignette->GetPlatformData()->Mips[0].BulkData;uint8* VP=static_cast<uint8*>(VB.Lock(LOCK_READ_WRITE));
    for(int Y=0;Y<64;Y++)for(int X=0;X<64;X++){
        const float R=FMath::Sqrt(FMath::Square((X-31.f)/43.f)+FMath::Square((Y-29.f)/43.f));
        const int O=(Y*64+X)*4;VP[O]=VP[O+1]=VP[O+2]=0;VP[O+3]=uint8(148*Ease((R-.35f)/.68f));
    }
    VB.Unlock();TitleVignette->UpdateResource();
    // Read alphabet/digits only after the mandatory ROM validation. No font
    // extracted from Nintendo's ROM is included in Content/Title or staging.
    TArray<uint8> Rom;
    if(!FFileHelper::LoadFileToArray(Rom,*CoreRomPath) || Rom.Num()!=0x300000){Failure=TEXT("Unable to read the verified ROM title font.");return;}
    TitleFont=UTexture2D::CreateTransient(36*8,8,PF_B8G8R8A8);TitleFont->Filter=TF_Nearest;
    auto& FB=TitleFont->GetPlatformData()->Mips[0].BulkData;uint8* Font=static_cast<uint8*>(FB.Lock(LOCK_READ_WRITE));
    FMemory::Memzero(Font,36*8*8*4);
    for(int I=0;I<36;I++){
        const int Address=I<26?0xb68000+(0x30+I)*32:0x9ab200+((I-26+9)%10)*16;
        const int Offset=((Address>>16)&127)*0x8000+(Address&0x7fff),Planes=I<26?4:2;
        for(int Y=0;Y<8;Y++)for(int X=0;X<8;X++){
            int C=0;for(int B=0;B<Planes;B++)C|=((Rom[Offset+Y*2+(B/2)*16+B%2]>>(7-X))&1)<<B;
            if(C==(I<26?13:2)){uint8* Q=Font+(Y*36*8+I*8+X)*4;Q[0]=Q[1]=Q[2]=Q[3]=255;}
        }
    }
    FB.Unlock();TitleFont->UpdateResource();
    // Original 8x16 options/save-screen numbers, including both authored halves.
    TitleYearFont=UTexture2D::CreateTransient(32,16,PF_B8G8R8A8);TitleYearFont->Filter=TF_Nearest;
    auto& YB=TitleYearFont->GetPlatformData()->Mips[0].BulkData;uint8* Year=static_cast<uint8*>(YB.Lock(LOCK_READ_WRITE));
    FMemory::Memzero(Year,32*16*4);const int Digits[]={2,0,2,6};
    const int Gfx=0x70000,Palette=0x76400; // LoROM 8E:8000, 8E:E400
    for(int I=0;I<4;I++)for(int Y=0;Y<16;Y++)for(int X=0;X<8;X++){
        const int Offset=Gfx+(Digits[I]+(Y/8)*16)*32+(Y%8)*2;int C=0;
        for(int B=0;B<4;B++)C|=((Rom[Offset+(B/2)*16+B%2]>>(7-X))&1)<<B;
        if(!C)continue;const int Color=Rom[Palette+C*2]|(Rom[Palette+C*2+1]<<8);uint8* Q=Year+(Y*32+I*8+X)*4;
        for(int B=0;B<3;B++)Q[B]=((Color>>((2-B)*5))&31)*255/31;Q[3]=255;
    }
    YB.Unlock();TitleYearFont->UpdateResource();TitleAssetsReady=true;
    StartupWarning=0;WarningSeconds=0;WarningNeedsRelease=true;TitleInputFence=true;
    UE_LOG(LogTemp,Display,TEXT("SM_TITLE_ASSETS layers=%d font=verified_rom startup=photosensitivity"),TitleTextures.Num());
}

bool ASMHUD::AdvanceStartupWarning(float Dt,bool Held){
    if(StartupWarning<0)return false;
    WarningSeconds+=FMath::Max(0.f,Dt);
    if(!Held)WarningNeedsRelease=false;
    if(WarningSeconds>=10.f || (Held && !WarningNeedsRelease)){
        UE_LOG(LogTemp,Display,TEXT("SM_STARTUP_WARNING_DONE screen=%d seconds=%.3f input=%d native_frame=%d"),StartupWarning,WarningSeconds,Held,Frame());
        ++StartupWarning;WarningSeconds=0;WarningNeedsRelease=true;Accumulator=0;
        if(StartupWarning==2){StartupWarning=-1;TitleInputFence=true;if(AudioWave)AudioWave->ResetAudio();}
    }
    return true; // Consume this entire tick, including the final warning's input.
}
bool ASMHUD::TickStartupWarnings(float Dt){
    if(StartupWarning<0)return false;
    bool Held=ReadButtons()!=0 || PlayerOwner->IsInputKeyDown(EKeys::AnyKey) ||
        PlayerOwner->IsInputKeyDown(EKeys::SpaceBar) || PlayerOwner->IsInputKeyDown(EKeys::Escape) ||
        PlayerOwner->IsInputKeyDown(EKeys::LeftMouseButton) || PlayerOwner->IsInputKeyDown(EKeys::RightMouseButton);
    if(TitleTest && TitleTestCase==1)Held=(TitleTestElapsed>=1 && TitleTestElapsed<4) || (TitleTestElapsed>=5 && TitleTestElapsed<5.3f);
    if(TitleTest && TitleTestCase==2)Held=(TitleTestElapsed>=.8f && TitleTestElapsed<1.1f) || (TitleTestElapsed>=1.5f && TitleTestElapsed<1.8f);
    return AdvanceStartupWarning(Dt,Held);
}

void ASMHUD::UpdateTitleTimeline(){
    if(!TitleAssetsReady)return;
    const int F=Frame(),S=State(),Function=CinemaState?CinemaState(2):0;
    if(F<TitleLastFrame){TitleLastState=-1;TitlePhase=0;}
    int Next=TitlePhase;
    if(S==0)Next=0;
    else if(S==1){
        if(TitleLastState!=1 || Function==0x9b68){Next=1;TitleOpeningFrame=F;}
        // Read-only native cinematic cues; never change instruction timers,
        // music queues, skip behavior, demos, save selection or controller input.
        switch(Function){case 0x9d17:Next=2;break;case 0x9d90:Next=3;break;
            case 0x9e12:Next=4;break;case 0x9e8b:Next=5;break;
            case 0x9f28:Next=6;break;case 0x9f29:Next=7;break;default:break;}
    }
    if(Next!=TitlePhase || TitleLastState!=S){
        TitlePhaseFrame=F;TitlePhase=Next;
        if(S<=1)UE_LOG(LogTemp,Display,TEXT("SM_TITLE_CUE phase=%d frame=%d function=%04x"),Next,F,Function);
    }
    TitleLastFrame=F;TitleLastState=S;
}

namespace {
FSlateFontInfo CleanFont(UFont* Font,float Pixels){return FSlateFontInfo(Font,FMath::Max(10,FMath::RoundToInt(Pixels*.75f)));}
}
UFont* ASMHUD::GetTitleUiFont(){
    if(!TitleUiFont){
        // Canvas requires a UFont even for runtime Slate fonts. A filename-only
        // FSlateFontInfo measures correctly but fails Canvas HasValidText().
        TitleUiFont=NewObject<UFont>(this);TitleUiFont->FontCacheType=EFontCacheType::Runtime;
        TitleUiFont->GetMutableInternalCompositeFont()=FCompositeFont(FName(TEXT("Regular")),
            FPaths::ProjectContentDir()/TEXT("UI/Montserrat-Regular.ttf"),EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
    }
    return TitleUiFont;
}
void ASMHUD::DrawBuildVersion(){
    if(!Ready || State()>4)return;
    const float S=FMath::Min(Canvas->SizeX/1280.f,Canvas->SizeY/720.f);
    const FSlateFontInfo Font=CleanFont(GetTitleUiFont(),16*S);const FText Label=FText::FromString(TEXT(SM_BUILD_VERSION));
    const FVector2D Size=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label,Font);
    FCanvasTextItem Item(FVector2D(Canvas->SizeX-Size.X-18*S,Canvas->SizeY-Size.Y-12*S),Label,Font,FLinearColor(.65f,.69f,.75f));
    Item.EnableShadow(FLinearColor::Black,FVector2D(1,1));Canvas->DrawItem(Item);
}
void ASMHUD::DrawStartupWarning(){
    const float W=Canvas->SizeX,H=Canvas->SizeY,S=FMath::Min(W/1280.f,H/720.f);
    DrawRect(FLinearColor::Black,0,0,W,H);
    auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    auto Text=[&](const FString& Line,float X,float Y,float Size,FLinearColor Color){
        FCanvasTextItem Item(FVector2D(FMath::RoundToFloat(X),FMath::RoundToFloat(Y)),FText::FromString(SMLocalization::Text(Line)),CleanFont(GetTitleUiFont(),Size),Color);Canvas->DrawItem(Item);};
    auto Center=[&](const FString& Original,float Y,float Size,FLinearColor Color){const FString Line=SMLocalization::Text(Original);const FVector2D Extent=Measure->Measure(Line,CleanFont(GetTitleUiFont(),Size));Text(Line,(W-Extent.X)/2,Y,Size,Color);};
    const float Width=FMath::Min(W-80*S,980*S),Left=(W-Width)/2;
    Center(StartupWarning==0?TEXT("PHOTOSENSITIVITY WARNING"):TEXT("AI TRANSPARENCY"),H*.23f,32*S,FLinearColor::White);
    const TArray<FString> Paragraphs=StartupWarning==0?TArray<FString>{
        TEXT("This game contains flashing lights, bright visual effects, and rapidly changing images. These may trigger seizures or other symptoms in people with photosensitive epilepsy, including those with no previous history of seizures."),
        TEXT("Play in a well-lit room and take regular breaks. Stop playing immediately if you experience dizziness, visual disturbances, involuntary movements, or discomfort.")}:TArray<FString>{
        TEXT("We believe in being open about how The Zebes Project is made. The voice performances and music are created and performed by real people. The game's code is developed by humans with AI assistance."),
        TEXT("Some graphics in this development version were generated using AI as temporary placeholders. These assets are not intended to be final and will be replaced with human-created artwork.")};
    float Y=H*.37f;const float Size=23*S;const FSlateFontInfo Font=CleanFont(GetTitleUiFont(),Size);
    for(const FString& Paragraph:Paragraphs){
        TArray<FString> Words;SMLocalization::Text(Paragraph).ParseIntoArrayWS(Words);FString Line;
        for(const FString& Word:Words){const FString Candidate=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;
            if(Measure->Measure(Candidate,Font).X>Width && !Line.IsEmpty()){Text(Line,Left,Y,Size,FLinearColor(.87f,.89f,.92f));Y+=34*S;Line=Word;}else Line=Candidate;}
        Text(Line,Left,Y,Size,FLinearColor(.87f,.89f,.92f));Y+=57*S;
    }
    Center(TEXT("Press any button to continue"),H*.83f,17*S,FLinearColor(.56f,.60f,.66f));
}

bool ASMHUD::DrawTitlePresentation(){
    if(!TitleAssetsReady || !Ready || (AutoTest && !TitleTest) || State()>1)return false;
    const bool Wide=Widescreen;const float CW=Wide?1600:1200,CH=900;
    const float Fit=FMath::Min(Canvas->SizeX/CW,Canvas->SizeY/CH);
    const FVector2D Origin((Canvas->SizeX-CW*Fit)/2,(Canvas->SizeY-CH*Fit)/2);
    // Camera moves must never spill into a 4:3 or ultrawide letterbox.
    ON_SCOPE_EXIT {
        DrawRect(FLinearColor::Black,0,0,Origin.X,Canvas->SizeY);
        DrawRect(FLinearColor::Black,Origin.X+CW*Fit,0,Origin.X,Canvas->SizeY);
        DrawRect(FLinearColor::Black,0,0,Canvas->SizeX,Origin.Y);
        DrawRect(FLinearColor::Black,0,Origin.Y+CH*Fit,Canvas->SizeX,Origin.Y);
    };
    const float T=Frame()*NativeSecond,Age=(Frame()-TitlePhaseFrame)*NativeSecond;
    float Bright=FMath::Clamp(Brightness()/15.f,0.f,1.f);
    // Fade within the native shot windows, preserving music/queue timing.
    if(TitlePhase>=2 && TitlePhase<=4){const int Durations[]={215,147,216};
        Bright*=Ease(Age/.45f)*Ease((Durations[TitlePhase-2]*NativeSecond-Age)/.45f);
    }else if(TitlePhase==5)Bright*=Ease(Age/.55f);
    const float FX=Atmosphere?FMath::Clamp(FlashStrength,0.f,1.f):0;
    float Zoom=1;FVector2D Focus(CW*.5f,CH*.5f);
    if(TitlePhase==2){Zoom=1.85f;Focus=FVector2D(CW*.20f,430+Age*22);}
    if(TitlePhase==3){Zoom=1.70f;Focus=FVector2D(CW*.82f,240+Age*30);}
    if(TitlePhase==4){Zoom=1.50f;Focus=FVector2D(CW*.76f,660-Age*19);}
    if(TitlePhase==5){const float Q=Ease(Age/(380*NativeSecond));Zoom=FMath::Lerp(1.48f,1.f,Q);Focus=FMath::Lerp(FVector2D(CW*.40f,575),FVector2D(CW*.5f,450),Q);}
    if(!FX){Zoom=1;Focus=FVector2D(CW*.5f,CH*.5f);}
    auto Project=[&](FVector4f R,float D){
        const float Z=1+(Zoom-1)*(.75f+D*.25f);
        const float DriftX=FX*D*3*FMath::Sin(T*.17f),DriftY=FX*D*2*FMath::Sin(T*.13f);
        const float CX=FMath::Clamp(float(Focus.X),CW/(2*Z),CW-CW/(2*Z)),CY=FMath::Clamp(float(Focus.Y),CH/(2*Z),CH-CH/(2*Z));
        return FVector4f(Origin.X+(CW*.5f+(R.X-CX)*Z+DriftX)*Fit,Origin.Y+(CH*.5f+(R.Y-CY)*Z+DriftY)*Fit,R.Z*Z*Fit,R.W*Z*Fit);
    };
    auto Image=[&](const TCHAR* Key,FVector4f R,float D,float Alpha,EBlendMode Blend=BLEND_Translucent){
        UTexture2D* Tex=TitleTextures.FindRef(FName(Key));const FVector4f* Crop=TitleCrops.Find(FName(Key));if(!Tex||!Crop)return;
        if(R.W<=0)R.W=R.Z*(Crop->W-Crop->Y)/(Crop->Z-Crop->X);
        const FVector4f P=Project(R,D);
        DrawTexture(Tex,P.X,P.Y,P.Z,P.W,Crop->X/Tex->GetSizeX(),Crop->Y/Tex->GetSizeY(),(Crop->Z-Crop->X)/Tex->GetSizeX(),(Crop->W-Crop->Y)/Tex->GetSizeY(),FLinearColor(1,1,1,Alpha*Bright),Blend);
    };
    auto Glow=[&](float X,float Y,float Radius,float D,FLinearColor Color,float Alpha){
        if(FX<=0)return;const FVector4f P=Project(FVector4f(X-Radius,Y-Radius,Radius*2,Radius*2),D);Color*=Alpha*Bright*FX;Color.A=1;
        DrawTexture(TitleGlow,P.X,P.Y,P.Z,P.W,0,0,1,1,Color,BLEND_Additive);
    };
    auto Text=[&](const FString& English,float X,float Y,float Scale,float Alpha){
        const FString Value=SMLocalization::Text(English);
        for(int I=0;I<Value.Len();I++){const TCHAR C=Value[I];const int Tile=C>='A'&&C<='Z'?C-'A':C>='0'&&C<='9'?26+C-'0':-1;if(Tile<0)continue;
            const float TX=Origin.X+(X-Value.Len()*4*Scale+I*8*Scale)*Fit,TY=Origin.Y+Y*Fit;
            // A one-pixel dark edge keeps the native lettering readable over Zebes.
            for(int J=0;J<4;J++)DrawTexture(TitleFont,TX+(J==0?-1:J==1?1:0)*Scale*Fit,TY+(J==2?-1:J==3?1:0)*Scale*Fit,8*Scale*Fit,8*Scale*Fit,Tile/36.f,0,1/36.f,1,FLinearColor(0,0,.01f,Alpha*Bright*.85f),BLEND_Translucent);
            DrawTexture(TitleFont,TX,TY,8*Scale*Fit,8*Scale*Fit,Tile/36.f,0,1/36.f,1,FLinearColor(1,1,1,Alpha*Bright),BLEND_Translucent);}
    };
    if(State()==0){const float LW=Wide?540:470;Image(TEXT("creator"),FVector4f((CW-LW)/2,220,LW,0),0,1);Glow(CW*.5f,425,190,0,FLinearColor(.38f,.57f,.85f),.15f);return true;}
    if(TitlePhase==1){const float A=Ease(Age/.3f)*Ease((129*NativeSecond-Age)/.4f);DrawTexture(TitleYearFont,Origin.X+(CW*.5f-96)*Fit,Origin.Y+402*Fit,192*Fit,96*Fit,0,0,1,1,FLinearColor(1,1,1,A*Bright),BLEND_Translucent);return true;}
    // The original cinematic holds black between each authored pan.
    if(CinemaState(2)==0x9a47 && TitlePhase>=2 && TitlePhase<=4)return true;
    Image(Wide?TEXT("skyWide"):TEXT("skyClassic"),FVector4f(-20,-14,CW+40,CH+28),.1f,1);
    Glow(CW*.29f,620,240,.15f,FLinearColor(.25f,.55f,1),.28f);
    // Stars live at different depths and project with the cinematic camera.
    if(FX>0)for(int I=0;I<240;I++){
        const float StarDepth=.2f+Random(I+101)*1.2f,X=Random(I)*CW,Y=Random(I+41)*750;
        const float Pulse=FMath::Pow(.5f+.5f*FMath::Sin(T*(.35f+Random(I+9)) +I),6.f);
        const FVector4f P=Project(FVector4f(X,Y,1.2f+Random(I+77)*1.8f,1.2f+Random(I+77)*1.8f),StarDepth);
        DrawRect(FLinearColor(.65f,.82f,1,(.16f+Pulse*.65f)*Bright*FX),P.X,P.Y,P.Z,P.W);
        if(I%9==0){Glow(X,Y,12,StarDepth,FLinearColor(.4f,.64f,1),Pulse*.6f);if(Pulse>.6f){DrawRect(FLinearColor(.8f,.9f,1,Pulse*Bright*FX),P.X-3*Fit,P.Y,8*Fit,Fit);DrawRect(FLinearColor(.8f,.9f,1,Pulse*Bright*FX),P.X,P.Y-3*Fit,Fit,8*Fit);}}
    }
    Image(Wide?TEXT("midWide"):TEXT("midClassic"),FVector4f(0,0,CW,CH),.5f,1);
    TMap<FName,FVector4f> Bounds;
    auto Place=[&](const FPlacement& P){FVector4f R=Wide?P.Wide:P.Classic;const FVector4f C=TitleCrops.FindChecked(FName(P.Name));
        const float Scale=FMath::Min(R.Z/(C.Z-C.X),R.W/(C.W-C.Y));const float W=(C.Z-C.X)*Scale,H=(C.W-C.Y)*Scale;R.X+=(R.Z-W)/2;R.Y+=(R.W-H)/2;R.Z=W;R.W=H;Bounds.Add(FName(P.Name),R);Image(P.Name,R,P.Depth,FCString::Strcmp(P.Name,TEXT("brain"))==0?.84f:1.f);};
    Place(Places[0]);Image(Wide?TEXT("frontWide"):TEXT("frontClassic"),FVector4f(0,0,CW,CH),.8f,1);
    for(int I=1;I<UE_ARRAY_COUNT(Places)-1;I++)Place(Places[I]);
    auto Eye=[&](const TCHAR* Name,float U,float V,float Phase,FLinearColor Color){const FVector4f R=Bounds.FindChecked(FName(Name));float D=1.2f;for(const FPlacement& P:Places)if(FCString::Strcmp(P.Name,Name)==0)D=P.Depth;
        const float Pulse=.40f+.23f*FMath::Sin(T*.85f+Phase)+.10f*FMath::Sin(T*.31f+Phase);
        Glow(R.X+U*R.Z,R.Y+V*R.W,24,D,Color,Pulse);Glow(R.X+U*R.Z,R.Y+V*R.W,8,D,FLinearColor(1,.75f,.35f),Pulse*.6f);};
    Eye(TEXT("kraid"),.34f,.21f,0,FLinearColor(1,.13f,.03f));Eye(TEXT("ridley"),.215f,.30f,1,FLinearColor(1,.1f,.03f));
    Eye(TEXT("brain"),.44f,.42f,2,FLinearColor(1,.1f,.03f));Eye(TEXT("brain"),.58f,.42f,2,FLinearColor(1,.1f,.03f));
    Eye(TEXT("phantoon"),.50f,.54f,3,FLinearColor(1,.08f,.42f));Eye(TEXT("botwoon"),.12f,.18f,4,FLinearColor(1,.32f,.05f));
    Eye(TEXT("draygon"),.27f,.31f,4.5f,FLinearColor(1,.32f,.05f));Eye(TEXT("motherBrain"),.48f,.40f,5,FLinearColor(1,.12f,.03f));
    for(int I=0;I<4;I++)Eye(TEXT("crocomire"),.27f+I*.075f,.085f+I*.042f,5.5f,FLinearColor(1,.42f,.08f));
    // Preserve the approved cave lip across the base of the stacked statues.
    const float LipX=Wide?975.f:720.f;const FVector4f Lip=Project(FVector4f(LipX,845,CW-LipX,55),1.15f);
    DrawTexture(TitleTextures.FindChecked(FName(Wide?TEXT("frontWide"):TEXT("frontClassic"))),Lip.X,Lip.Y,Lip.Z,Lip.W,LipX/CW,845/CH,(CW-LipX)/CW,55/CH,FLinearColor(1,1,1,Bright),BLEND_Translucent);
    Place(Places[UE_ARRAY_COUNT(Places)-1]);
    // Soft motes in front of the scenery; quiet fog at the base of the cave.
    if(FX>0)for(int I=0;I<90;I++){const float X=FMath::Frac(Random(I+400)+T*(.001f+Random(I+20)*.002f))*CW,Y=500+Random(I+230)*390+FMath::Sin(T*.3f+I)*15;
        Glow(X,Y,2+Random(I+44)*3,1.5f,FLinearColor(.55f,.75f,1),.2f+Random(I)*.3f);}
    for(int I=0;I<9;I++)Glow(CW*(I+.5f)/9+FMath::Sin(T*.09f+I)*45,825+FMath::Sin(T*.12f+I)*22,125,1.5f,FLinearColor(.18f,.28f,.48f),.06f);
    DrawTexture(TitleVignette,Origin.X,Origin.Y,CW*Fit,CH*Fit,0,0,1,1,FLinearColor::White,BLEND_Translucent);
    if(TitlePhase>=6){
        // The title and prompt appear on the native reveal/ready cues, not a
        // wall-clock timer. The SNES music driver continues at its original rate.
        const float A=TitlePhase==6?Ease(Age/(64*NativeSecond)):1;
        const FVector4f Logo=Wide?FVector4f(440,165,545,0):FVector4f(294,165,457,0);
        for(int I=0;I<8;I++){FVector4f R=Logo;R.X+=FMath::Cos(I*PI/4)*4;R.Y+=FMath::Sin(I*PI/4)*4;Image(TEXT("logo"),R,0,A*.045f*FX);}
        Image(TEXT("logo"),Logo,0,A);
        if(TitlePhase>=7){const float X=Wide?712:520,Y=Wide?583:552;
            Text(TEXT("PRESS START"),X,Y,3,.65f+.35f*FMath::Square(FMath::Sin(T*1.8f)));
            Text(TEXT("A SUPER METROID FAN PROJECT"),X,Y+53,1,.55f);
        }
    }
    return true;
}
