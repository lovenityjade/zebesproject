#include "SMSeedSettings.h"
#include "Misc/Base64.h"
#include "Misc/Compression.h"
#include "Misc/Crc.h"
#include "Misc/Guid.h"

namespace SMSeedSettings {
bool ParseSeedNumber(const FString& Text,int32& Out){
    const FString S=Text.TrimStartAndEnd();if(S.IsEmpty()){Out=0;return true;}
    uint64 N=0;for(TCHAR C:S){if(C<'0'||C>'9')return false;N=N*10+(C-'0');if(N>MAX_int32)return false;}
    if(!N)return false;Out=int32(N);return true;
}
int32 RandomSeed(){
    // Unix GUIDs contain a time/counter component: mix all 128 bits, not one word.
    int32 N=0;while(!N){const FGuid Id=FGuid::NewGuid();N=int32(FCrc::MemCrc32(&Id,sizeof(Id))&MAX_int32);}return N;
}
static constexpr int32 MaxPayload=65536;
static void Put32(TArray<uint8>& Bytes,uint32 N){for(int I=0;I<4;I++)Bytes.Add(uint8(N>>(I*8)));}
static uint32 Get32(const uint8* B){return uint32(B[0])|uint32(B[1])<<8|uint32(B[2])<<16|uint32(B[3])<<24;}
bool ExportString(const FSMSeedRequest& R,FString& Out,FString& Error){
    Out.Reset();Error.Reset();if(!ValidateDraft(R,Error))return false;
    auto O=MakeShared<FJsonObject>();Write(R,O);
    O->SetNumberField(TEXT("schema"),1);O->SetStringField(TEXT("skill"),R.Skill);O->SetStringField(TEXT("progression"),R.Progression);
    O->SetBoolField(TEXT("noAdvancedTechs"),R.NoAdvancedTechs);
    auto Hunt=MakeShared<FJsonObject>();Hunt->SetBoolField(TEXT("enabled"),R.RelicHunt);Hunt->SetNumberField(TEXT("placed"),R.RelicsPlaced);Hunt->SetNumberField(TEXT("required"),R.RelicsRequired);Hunt->SetNumberField(TEXT("escapeMinutes"),R.RelicEscapeMinutes);O->SetObjectField(TEXT("relicHunt"),Hunt);
    TArray<TSharedPtr<FJsonValue>> Patches;for(const auto& P:R.Patches)Patches.Add(MakeShared<FJsonValueString>(P));O->SetArrayField(TEXT("patches"),Patches);
    const FString Json=Encode(O);FTCHARToUTF8 Utf8(*Json);
    if(Utf8.Length()>MaxPayload){Error=TEXT("Settings exceed the sharing limit.");return false;}
    int32 Size=FCompression::CompressMemoryBound(NAME_Zlib,Utf8.Length());TArray<uint8> Compressed;Compressed.SetNumUninitialized(Size);
    if(!FCompression::CompressMemory(NAME_Zlib,Compressed.GetData(),Size,Utf8.Get(),Utf8.Length())){Error=TEXT("Could not encode settings.");return false;}
    TArray<uint8> Bytes;Put32(Bytes,Utf8.Length());Put32(Bytes,FCrc::MemCrc32(Utf8.Get(),Utf8.Length()));Bytes.Append(Compressed.GetData(),Size);
    FString Encoded=FBase64::Encode(Bytes);Encoded.ReplaceInline(TEXT("+"),TEXT("-"));Encoded.ReplaceInline(TEXT("/"),TEXT("_"));Encoded.ReplaceInline(TEXT("="),TEXT(""));
    Out=TEXT("ZP1.")+Encoded;return true;
}
bool ImportString(const FString& Text,FSMSeedRequest& R,FString& Error){
    Error=TEXT("Invalid or damaged settings string. The current settings were kept.");
    FString S=Text.TrimStartAndEnd();if(S.Len()>100000 || !S.StartsWith(TEXT("ZP1.")))return false;
    S.RightChopInline(4);for(TCHAR C:S)if(!((C>='A'&&C<='Z')||(C>='a'&&C<='z')||(C>='0'&&C<='9')||C=='-'||C=='_'))return false;
    S.ReplaceInline(TEXT("-"),TEXT("+"));S.ReplaceInline(TEXT("_"),TEXT("/"));while(S.Len()%4)S+=TEXT("=");
    TArray<uint8> Bytes;if(!FBase64::Decode(S,Bytes)||Bytes.Num()<9)return false;
    const uint32 Size=Get32(Bytes.GetData());if(!Size||Size>MaxPayload)return false;
    TArray<uint8> Plain;Plain.SetNumZeroed(Size+1);
    if(!FCompression::UncompressMemory(NAME_Zlib,Plain.GetData(),Size,Bytes.GetData()+8,Bytes.Num()-8) || FCrc::MemCrc32(Plain.GetData(),Size)!=Get32(Bytes.GetData()+4))return false;
    auto O=Parse(FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Plain.GetData()))));if(!O)return false;
    const TSet<FString> Keys={TEXT("schema"),TEXT("skill"),TEXT("progression"),TEXT("noAdvancedTechs"),TEXT("relicHunt"),TEXT("patches"),TEXT("options"),TEXT("techniques"),TEXT("skillSettings")};
    if(O->Values.Num()!=Keys.Num())return false;for(const auto& P:O->Values)if(!Keys.Contains(FString(*P.Key)))return false;
    double Version=0,P=0,Q=0;const TSharedPtr<FJsonObject>* Hunt=nullptr;const TArray<TSharedPtr<FJsonValue>>* Patches=nullptr;
    FSMSeedRequest Candidate;Candidate.Seed=R.Seed;
    if(!O->TryGetNumberField(TEXT("schema"),Version)||Version!=1 || !Read(O,Candidate) ||
       !O->TryGetStringField(TEXT("skill"),Candidate.Skill)||!O->TryGetStringField(TEXT("progression"),Candidate.Progression)||
       !O->TryGetBoolField(TEXT("noAdvancedTechs"),Candidate.NoAdvancedTechs)||!O->TryGetObjectField(TEXT("relicHunt"),Hunt)||
       !(*Hunt)->TryGetBoolField(TEXT("enabled"),Candidate.RelicHunt)||!(*Hunt)->TryGetNumberField(TEXT("placed"),P)||!(*Hunt)->TryGetNumberField(TEXT("required"),Q)||
       !FMath::IsFinite(P)||!FMath::IsFinite(Q)||P<1||P>60||Q<1||Q>P||P!=FMath::FloorToDouble(P)||Q!=FMath::FloorToDouble(Q)||
       !O->TryGetArrayField(TEXT("patches"),Patches))return false;
    Candidate.RelicsPlaced=int32(P);Candidate.RelicsRequired=int32(Q);if(!ReadEscapeMinutes(*Hunt,Candidate.RelicEscapeMinutes))return false;
    for(const auto& Patch:*Patches){if(Patch->Type!=EJson::String||Patch->AsString().Len()>128)return false;Candidate.Patches.Add(Patch->AsString());}
    if(!ValidateDraft(Candidate,Error))return false;
    R=MoveTemp(Candidate);Error.Reset();return true;
}
}
