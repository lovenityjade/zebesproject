#include "SMLocalization.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
namespace {
int CurrentLanguage=0;
const struct FEntry {const char* English;const char* French;} Entries[]={
#include "../../../Native/sm_locale_catalog.inc"
};
// Values own their UTF-8 buffers: pointers survive map growth and the frame.
TMap<FString,TSharedPtr<TArray<ANSICHAR>>> Labels;
}
int SMLocalization::Language(){return CurrentLanguage;}
void SMLocalization::SetLanguage(int Value){CurrentLanguage=Value==1;}
bool SMLocalization::Save(const FString& Path){FConfigFile C;C.Read(Path);C.SetString(TEXT("Localization"),TEXT("Language"),CurrentLanguage?TEXT("fr-CA"):TEXT("en"));IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);return C.Write(Path);}
void SMLocalization::Load(const FString& Path){FConfigFile C;C.Read(Path);FString Code;C.GetString(TEXT("Localization"),TEXT("Language"),Code);SetLanguage(Code==TEXT("fr-CA"));}
const char* SMLocalization::Text(const char* English){
 if(!CurrentLanguage || !English)return English;
 int Low=0,High=UE_ARRAY_COUNT(Entries);
 while(Low<High){int Mid=(Low+High)/2,C=FCStringAnsi::Strcmp(English,Entries[Mid].English);if(!C)return Entries[Mid].French;if(C<0)High=Mid;else Low=Mid+1;}
 return English;
}
FString SMLocalization::Text(const FString& English){return UTF8_TO_TCHAR(Text(TCHAR_TO_UTF8(*English)));}
const char* SMLocalization::Label(const char* English){
 if(!CurrentLanguage || !English || (English[0]=='#' && English[1]=='#'))return English;
 const FString Key=UTF8_TO_TCHAR(English);
 if(auto* Found=Labels.Find(Key))return (*Found)->GetData();
 FString Visible=Key,Id;int32 At;
 if(Key.FindChar('#',At) && Key.Mid(At,2)==TEXT("##"))Visible=Key.Left(At);
 const FString Translated=Text(Visible);
 if(Translated==Visible)return English;
 // Preserve ImGui identity when switching language, including explicit ### IDs.
 const int32 StableAt=Key.Find(TEXT("###"));
 Id=StableAt==INDEX_NONE?Key:Key.Mid(StableAt+3);
 FTCHARToUTF8 Bytes(*(Translated+TEXT("###")+Id));
 auto Buffer=MakeShared<TArray<ANSICHAR>>();Buffer->Append(Bytes.Get(),Bytes.Length()+1);
 const char* Result=Buffer->GetData();Labels.Add(Key,MoveTemp(Buffer));return Result;
}
