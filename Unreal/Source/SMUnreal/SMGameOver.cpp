#include "SMHUD.h"
#include "SMRom.h"
#include "SMLocalization.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

void ASMHUD::LoadGameOverPresentation(){
    const FString Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("GameOver"));
    auto Load=[&](const TCHAR* Name){
        UTexture2D* T=FImageUtils::ImportFileAsTexture2D(Directory/Name);
        if(T){T->Filter=TF_Nearest;T->AddressX=TA_Clamp;T->AddressY=TA_Clamp;T->UpdateResource();}
        return T;
    };
    GameOverWideTexture=Load(TEXT("GameOver-16x9.png"));
    GameOverClassicTexture=Load(TEXT("GameOver-4x3.png"));
    GameOverLabelsTexture=UTexture2D::CreateTransient(128,64,PF_B8G8R8A8);
    GameOverLabelsTexture->SRGB=true;GameOverLabelsTexture->Filter=TF_Nearest;GameOverLabelsTexture->UpdateResource();
    GameOverDustTexture=UTexture2D::CreateTransient(32,32,PF_B8G8R8A8);
    GameOverDustTexture->SRGB=true;GameOverDustTexture->Filter=TF_Bilinear;
    auto& Bulk=GameOverDustTexture->GetPlatformData()->Mips[0].BulkData;
    uint8* Data=static_cast<uint8*>(Bulk.Lock(LOCK_READ_WRITE));
    for(int Y=0;Y<32;Y++)for(int X=0;X<32;X++){
        float R2=FMath::Square((X-15.5f)/15.5f)+FMath::Square((Y-15.5f)/15.5f);
        uint8* P=Data+(Y*32+X)*4;P[0]=255;P[1]=242;P[2]=223;P[3]=uint8(FMath::Clamp((FMath::Exp(-R2*5)-FMath::Exp(-5.f))*255.f,0.f,255.f));
    }
    Bulk.Unlock();GameOverDustTexture->UpdateResource();
    // A soft runtime mask isolates the spotlight, leaving the title and outer
    // darkness steady. The supplied artwork itself remains unchanged.
    GameOverLampTexture=UTexture2D::CreateTransient(128,128,PF_B8G8R8A8);
    GameOverLampTexture->Filter=TF_Bilinear;
    auto& LampBulk=GameOverLampTexture->GetPlatformData()->Mips[0].BulkData;
    uint8* Mask=static_cast<uint8*>(LampBulk.Lock(LOCK_READ_WRITE));
    for(int Y=0;Y<128;Y++)for(int X=0;X<128;X++){
        const float U=(X+.5f)/128.f,V=(Y+.5f)/128.f;
        const float Edge=FMath::Clamp((.06f+.28f*V-FMath::Abs(U-.5f))/.06f,0.f,1.f);
        const float Vertical=FMath::Max(FMath::Clamp((.14f-V)/.03f,0.f,1.f),FMath::Clamp((V-.28f)/.06f,0.f,1.f));
        uint8* P=Mask+(Y*128+X)*4;P[0]=P[1]=P[2]=0;P[3]=uint8(255*Edge*Vertical);
    }
    LampBulk.Unlock();GameOverLampTexture->UpdateResource();
    using ConfigureFn=void(*)(const char*);
    auto Configure=reinterpret_cast<ConfigureFn>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_soundtrack_gameover_configure")));
    const FString StorageRoot=SMRom::DataRoot(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("..")));
    FString Music=StorageRoot/TEXT("Soundtracks/GameOver.pcm");
    // Preserve optional private development media; neither location is bundled.
    if(!IFileManager::Get().FileExists(*Music))Music=Directory/TEXT("GameOver.pcm");
    if(Configure)Configure(TCHAR_TO_UTF8(*Music));
    GameOverLabelKey=-1;GameOverTime=0;
    UE_LOG(LogTemp,Display,TEXT("SM_GAMEOVER_ASSETS wide=%d classic=%d"),GameOverWideTexture!=nullptr,GameOverClassicTexture!=nullptr);
}

bool ASMHUD::DrawGameOver(){
    if(!GameOverState || !GameOverState(0))return false;
    UTexture2D* Art=Widescreen?GameOverWideTexture:GameOverClassicTexture;
    if(!Art || !GameOverLabelsTexture)return false;
    const float Aspect=float(Art->GetSizeX())/Art->GetSizeY();
    const float SourceW=Widescreen?400.f:256.f,SourceH=SourceW/Aspect;
    float Scale=FMath::Min(Canvas->SizeX/SourceW,Canvas->SizeY/SourceH);
    if(ImageScaling==1 && Scale>=1.f)Scale=FMath::FloorToFloat(Scale);
    const float W=SourceW*Scale,H=SourceH*Scale;
    const float X=FMath::FloorToFloat((Canvas->SizeX-W)/2),Y=FMath::FloorToFloat((Canvas->SizeY-H)/2);
    const float Fade=Brightness()/15.f;
    const float Phase=FMath::Fmod(GameOverTime,5.7f);
    float Lamp=.96f+.025f*FMath::Sin(GameOverTime*7)+.01f*FMath::Sin(GameOverTime*19);
    if(Phase>1.70f && Phase<1.77f)Lamp=.28f;
    else if(Phase>1.82f && Phase<1.88f)Lamp=.5f;
    else if(Phase>1.96f && Phase<2.00f)Lamp=.37f;
    DrawTexture(Art,X,Y,W,H,0,0,1,1,FLinearColor(Fade,Fade,Fade,1),BLEND_Opaque);
    DrawTexture(GameOverLampTexture,X,Y,W,H,0,0,1,1,FLinearColor(1,1,1,1-Lamp),BLEND_Translucent);
    // Small soft motes drift across the whole scene, brighter inside the cone.
    // They are drawn behind the labels and never alter the supplied image files.
    for(int I=0;I<160;I++){
        const float Seed=float((uint32(I+1)*2654435761u)%65521)/65521.f;
        const float Seed2=float((uint32(I+7)*2246822519u)%65521)/65521.f;
        const float U=FMath::Frac(Seed+GameOverTime*(.002f+Seed2*.004f)+.014f*FMath::Sin(GameOverTime*.37f+I)+1.f);
        const float V=FMath::Frac(Seed2+GameOverTime*(.008f+Seed*.012f));
        const float Cone=FMath::Clamp(1.f-FMath::Abs(U-.5f)/(.07f+.25f*V),0.f,1.f);
        const float Alpha=Fade*(.16f+Cone*.7f*Lamp)*(.75f+.25f*FMath::Sin(I+GameOverTime));
        const float Size=FMath::Max(1.f,W/1280.f*(2.5f+Seed*3.5f));
        DrawTexture(GameOverDustTexture,X+U*W,Y+V*H,Size,Size,0,0,1,1,FLinearColor(1,1,1,Alpha),BLEND_Translucent);
    }
    const int Key=GameOverState(1)+Brightness()*2+SMLocalization::Language()*64;
    if(Key!=GameOverLabelKey){
        auto* Copy=static_cast<uint8*>(FMemory::Malloc(128*64*4));FMemory::Memcpy(Copy,GameOverLabels(),128*64*4);
        auto* Region=new FUpdateTextureRegion2D(0,0,0,0,128,64);
        GameOverLabelsTexture->UpdateTextureRegions(0,1,Region,128*4,4,Copy,
            [](uint8* Bytes,const FUpdateTextureRegion2D* R){FMemory::Free(Bytes);delete R;});
        GameOverLabelKey=Key;
    }
    const float LabelScale=H/224.f;
    if(SMLocalization::Language()){
        // The source illustration has an English title baked into it. A native
        // lettering panel translates that title without changing the artwork,
        // spotlight, particles or recorded audio files.
        const float PX=X+W*.11f,PY=Y+H*.125f,PW=W*.78f,PH=H*.165f,Edge=FMath::Max(1.f,LabelScale);
        DrawRect(FLinearColor(.2f*Fade,.3f*Fade,.4f*Fade,1),PX,PY,PW,PH);
        DrawRect(FLinearColor(.01f*Fade,.018f*Fade,.025f*Fade,1),PX+Edge,PY+Edge,PW-2*Edge,PH-2*Edge);
        const float TitleScale=W*.6f/104.f;
        DrawTexture(GameOverLabelsTexture,X+(W-128*TitleScale)/2,Y+H*.2075f-8*TitleScale,128*TitleScale,16*TitleScale,0,.75f,1,.25f,FLinearColor::White,BLEND_Translucent);
    }
    DrawTexture(GameOverLabelsTexture,X+(W-128*LabelScale)/2,Y+H*.32f,128*LabelScale,48*LabelScale,0,0,1,.75f,FLinearColor::White,BLEND_Translucent);
    return true;
}
