#include "SMHUD.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"

int ASMHUD::LoadRoomDecorations() {
    ClearDecorations();
    FString Folder=FPaths::ConvertRelativePathToFull(AutoTest?
        FPaths::ProjectSavedDir()/TEXT("SMTests/Decorations"):
        FPaths::ProjectDir()/TEXT("../Config/RoomDecorations"));
    if(AutoTest)FParse::Value(FCommandLine::Get(),TEXT("SMDecorDir="),Folder);
    TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(Folder/TEXT("*.json")),true,false);Files.Sort();
    auto Number=[](const TSharedPtr<FJsonObject>& O,const TCHAR* Key,int Min,int Max,int& Out) {
        double V;if(!O.IsValid()||!O->TryGetNumberField(Key,V)||!FMath::IsFinite(V)||V<Min||V>Max||V!=FMath::FloorToDouble(V))return false;
        Out=int(V);return true;
    };
    auto ImagePath=[&](const TSharedPtr<FJsonObject>& O,const TCHAR* Key,FString& Path){
        const TSharedPtr<FJsonValue>* V=O->Values.Find(Key);
        if(!V||(*V)->IsNull())return true;
        FString Id;if(!(*V)->TryGetString(Id)||Id.Len()!=64)return false;
        for(TCHAR C:Id)if(!(C>='0'&&C<='9')&&!(C>='a'&&C<='f'))return false;
        Path=Folder/TEXT("images")/(Id+TEXT(".bgra"));
        if(!IFileManager::Get().FileExists(*Path))Path=Folder/TEXT("images")/(Id+TEXT(".romtiles"));
        return IFileManager::Get().FileExists(*Path);
    };
    int Count=0;
    for(const FString& File:Files) {
        FString Text,Hash,Atlas;TSharedPtr<FJsonObject> Object;
        int Version=0,RoomId=0,StateId=0,Set=0;
        const TArray<TSharedPtr<FJsonValue>>* Array=nullptr;
        bool Valid=FFileHelper::LoadFileToString(Text,*(Folder/File)) && Text.Len()<=16000000 &&
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Object) && Object.IsValid() &&
            Object->TryGetStringField(TEXT("romSha256"),Hash) && Hash==TEXT("12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72") &&
            Number(Object,TEXT("version"),1,2,Version) && Number(Object,TEXT("room"),0x8000,0xffff,RoomId) &&
            Number(Object,TEXT("state"),0x8000,0xffff,StateId) && Number(Object,TEXT("tileset"),0,28,Set) &&
            Object->TryGetArrayField(TEXT("cells"),Array) && Array->Num()<=100000-Count;
        struct FCell {int X,Y,Layer,Tile,Expected,Radius=64,Opacity=25;bool Secret=false,Custom=false;};TArray<FCell> Cells;
        struct FPlane {int Id,SX,SY,OX,OY;bool RX,RY;FString Path;};TArray<FPlane> Planes;
        if(Valid)for(const auto& Value:*Array) {
            if(!Value.IsValid() || Value->Type!=EJson::Object){Valid=false;break;}
            const auto O=Value->AsObject();FCell C;
            if(!Number(O,TEXT("x"),-128,511,C.X)||!Number(O,TEXT("y"),-128,511,C.Y)||
               !Number(O,TEXT("layer"),0,1,C.Layer)||!Number(O,TEXT("tile"),0,4095,C.Tile)||
               !Number(O,TEXT("expected"),-1,65535,C.Expected)){Valid=false;break;}
            if(Version==2 && (!O->TryGetBoolField(TEXT("secret"),C.Secret)||!O->TryGetBoolField(TEXT("custom"),C.Custom)||
                !Number(O,TEXT("radius"),16,256,C.Radius)||!Number(O,TEXT("opacity"),0,100,C.Opacity)||(C.Secret&&C.Layer))){Valid=false;break;}
            Cells.Add(C);
        }
        if(Valid&&Version==2){
            Valid=ImagePath(Object,TEXT("tilesetImage"),Atlas);
            const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
            Valid=Valid&&Object->TryGetArrayField(TEXT("parallax"),Rows)&&Rows->Num()<=3;
            TSet<int> Seen;
            if(Valid)for(const auto& Value:*Rows){
                if(!Value.IsValid()||Value->Type!=EJson::Object){Valid=false;break;}
                const auto O=Value->AsObject();FPlane P;
                if(!Number(O,TEXT("id"),-1,RoomId==0x91f8?2:0,P.Id)||Seen.Contains(P.Id)||
                    !Number(O,TEXT("speedX"),0,200,P.SX)||!Number(O,TEXT("speedY"),0,200,P.SY)||
                    !Number(O,TEXT("offsetX"),-8192,8192,P.OX)||!Number(O,TEXT("offsetY"),-8192,8192,P.OY)||
                    !O->TryGetBoolField(TEXT("repeatX"),P.RX)||!O->TryGetBoolField(TEXT("repeatY"),P.RY)||!ImagePath(O,TEXT("image"),P.Path)){Valid=false;break;}
                Seen.Add(P.Id);Planes.Add(P);
            }
            if(Seen.Contains(-1)&&Seen.Num()>1)Valid=false;
            for(const FCell& C:Cells)if(C.Custom&&Atlas.IsEmpty())Valid=false;
        }
        if(!Valid){UE_LOG(LogTemp,Warning,TEXT("SM_DECOR_REJECTED %s"),*File);continue;}
        if(!Atlas.IsEmpty())SetDecorationAtlas(RoomId,StateId,TCHAR_TO_UTF8(*Atlas));
        for(const FPlane& P:Planes)SetDecorationPlane(RoomId,StateId,P.Id,P.SX,P.SY,P.OX,P.OY,int(P.RX)|int(P.RY)<<1,TCHAR_TO_UTF8(*P.Path));
        for(const FCell& C:Cells)if(SetDecorationEx(RoomId,StateId,Set,C.Layer,C.X,C.Y,C.Tile,C.Expected,C.Secret,C.Radius,C.Opacity,C.Custom))++Count;
        // Event notes are deliberately not interpreted as code or effects.
    }
    UE_LOG(LogTemp,Display,TEXT("SM_DECOR_LOADED cells=%d files=%d folder=%s"),Count,Files.Num(),*Folder);
    return Count;
}
