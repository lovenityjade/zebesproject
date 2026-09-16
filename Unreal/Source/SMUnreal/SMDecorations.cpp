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
    int Count=0;
    for(const FString& File:Files) {
        FString Text,Hash;TSharedPtr<FJsonObject> Object;
        int Version=0,RoomId=0,StateId=0,Set=0;
        const TArray<TSharedPtr<FJsonValue>>* Array=nullptr;
        bool Valid=FFileHelper::LoadFileToString(Text,*(Folder/File)) && Text.Len()<=16000000 &&
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Object) && Object.IsValid() &&
            Object->TryGetStringField(TEXT("romSha256"),Hash) && Hash==TEXT("12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72") &&
            Number(Object,TEXT("version"),1,1,Version) && Number(Object,TEXT("room"),0x8000,0xffff,RoomId) &&
            Number(Object,TEXT("state"),0x8000,0xffff,StateId) && Number(Object,TEXT("tileset"),0,28,Set) &&
            Object->TryGetArrayField(TEXT("cells"),Array) && Array->Num()<=100000-Count;
        struct FCell {int X,Y,Layer,Tile,Expected;};TArray<FCell> Cells;
        if(Valid)for(const auto& Value:*Array) {
            if(!Value.IsValid() || Value->Type!=EJson::Object){Valid=false;break;}
            const auto O=Value->AsObject();FCell C;
            if(!Number(O,TEXT("x"),-128,511,C.X)||!Number(O,TEXT("y"),-128,511,C.Y)||
               !Number(O,TEXT("layer"),0,1,C.Layer)||!Number(O,TEXT("tile"),0,4095,C.Tile)||
               !Number(O,TEXT("expected"),-1,65535,C.Expected)){Valid=false;break;}
            Cells.Add(C);
        }
        if(!Valid){UE_LOG(LogTemp,Warning,TEXT("SM_DECOR_REJECTED %s"),*File);continue;}
        for(const FCell& C:Cells)if(SetDecoration(RoomId,StateId,Set,C.Layer,C.X,C.Y,C.Tile,C.Expected))++Count;
    }
    UE_LOG(LogTemp,Display,TEXT("SM_DECOR_LOADED cells=%d files=%d folder=%s"),Count,Files.Num(),*Folder);
    return Count;
}
