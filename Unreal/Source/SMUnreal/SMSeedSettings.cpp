#include "SMSeedSettings.h"
#include "SMLocalization.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SMSeedCatalog.inl"
namespace SMSeedSettings {
TSharedPtr<FJsonObject> Catalog(){
    static TSharedPtr<FJsonObject> Value=[](){
        FString Json;
        for(const char* Part:SeedCatalogJsonParts)Json+=UTF8_TO_TCHAR(Part);
        return Parse(Json);
    }();
    return Value;
}
TSharedPtr<FJsonObject> Parse(const FString& Json){
    TSharedPtr<FJsonObject> Object;
    if(Json.Len()>1048576 || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Object))return nullptr;
    return Object;
}
FString Encode(const TSharedRef<FJsonObject>& Object){FString Json;FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Json));return Json;}
bool ValidBasics(const FSMSeedRequest& R){
    const TArray<FString> Skills={TEXT("newbie"),TEXT("casual"),TEXT("regular"),TEXT("veteran"),TEXT("expert"),TEXT("master"),TEXT("samus"),TEXT("solution")};
    const TArray<FString> Speeds={TEXT("slowest"),TEXT("slow"),TEXT("medium"),TEXT("fast"),TEXT("fastest"),TEXT("basic"),TEXT("VARIAble"),TEXT("speedrun"),TEXT("random")};
    return R.Seed>=0 && Skills.Contains(R.Skill) && Speeds.Contains(R.Progression);
}
bool Write(const FSMSeedRequest& R,const TSharedRef<FJsonObject>& Object){
    const FString* Values[]={&R.OptionsJson,&R.TechniquesJson,&R.SkillSettingsJson};
    const TCHAR* Keys[]={TEXT("options"),TEXT("techniques"),TEXT("skillSettings")};
    for(int I=0;I<3;I++){auto Value=Parse(*Values[I]);if(!Value.IsValid())return false;Object->SetObjectField(Keys[I],Value);}
    return true;
}
bool Read(const TSharedPtr<FJsonObject>& Object,FSMSeedRequest& R){
    FString* Values[]={&R.OptionsJson,&R.TechniquesJson,&R.SkillSettingsJson};
    const TCHAR* Keys[]={TEXT("options"),TEXT("techniques"),TEXT("skillSettings")};
    for(int I=0;I<3;I++){
        *Values[I]=TEXT("{}");if(!Object->HasField(Keys[I]))continue;
        const TSharedPtr<FJsonObject>* Value;if(!Object->TryGetObjectField(Keys[I],Value))return false;
        *Values[I]=Encode(Value->ToSharedRef());
    }
    return true;
}
bool HasOverrides(const FSMSeedRequest& R){
    for(const FString* Value:{&R.OptionsJson,&R.TechniquesJson,&R.SkillSettingsJson}){auto O=Parse(*Value);if(!O.IsValid() || !O->Values.IsEmpty())return true;}
    return false;
}
bool Matches(const FSMSeedRequest& R,const TSharedPtr<FJsonObject>& Requested){
    if(!Requested.IsValid())return !HasOverrides(R);
    auto Expected=MakeShared<FJsonObject>();if(!Write(R,Expected))return false;
    for(const auto& Pair:Expected->Values){const auto* Actual=Requested->Values.Find(Pair.Key);if(!Actual || !Actual->IsValid() || !FJsonValue::CompareEqual(*Pair.Value,**Actual))return false;}
    return true;
}
}

bool SMSeedSettings::ValidateDraft(const FSMSeedRequest& R,FString& Error){
    if(!ValidBasics(R) || !ValidEscapeMinutes(R.RelicEscapeMinutes) || R.RelicsRequired<1 || R.RelicsRequired>R.RelicsPlaced || R.RelicsPlaced>60){Error=TEXT("Invalid seed, skill, progression or tablet quota.");return false;}
    auto O=Parse(R.OptionsJson),T=Parse(R.TechniquesJson),S=Parse(R.SkillSettingsJson),C=Catalog();
    if(!O || !T || !S || !C){Error=TEXT("Settings must contain JSON objects.");return false;}
    FString World, Race;
    if((O->TryGetStringField(TEXT("logic"),World) && World!=TEXT("vanilla")) || (O->TryGetStringField(TEXT("raceMode"),Race) && Race!=TEXT("off"))){Error=TEXT("Mirror and Race are outside the supported scope of this port.");return false;}
    TMap<FString,TSharedPtr<FJsonObject>> Fields;TSet<FString> Lists;
    for(auto& E:C->GetArrayField(TEXT("fields"))){auto F=E->AsObject();Fields.Add(F->GetStringField(TEXT("key")),F);FString K;if(F->TryGetStringField(TEXT("multiple"),K))Lists.Add(K);if(F->TryGetStringField(TEXT("custom"),K))Lists.Add(K);}
    for(const auto& P:O->Values){FString Key(*P.Key);const auto* Field=Fields.Find(Key);bool Ok=false;
        if(Lists.Contains(Key) || (Field && (*Field)->GetStringField(TEXT("type"))==TEXT("goals"))){Ok=P.Value->Type==EJson::Array;if(Ok)for(auto& V:P.Value->AsArray())Ok&=V->Type==EJson::String;}
        else if(Field){const auto F=*Field;const FString Type=F->GetStringField(TEXT("type"));
            if(Type==TEXT("number")){
                if(P.Value->Type==EJson::Number){double N=P.Value->AsNumber();Ok=FMath::IsFinite(N) && N>=F->GetNumberField(TEXT("minimum")) && N<=F->GetNumberField(TEXT("maximum")) && (F->GetBoolField(TEXT("decimal")) || N==FMath::FloorToDouble(N));}
                else if(P.Value->Type==EJson::String)Ok=P.Value->AsString()==TEXT("random") || (Key==TEXT("nbObjectivesRequired") && P.Value->AsString()==TEXT("off"));
            }else if(P.Value->Type==EJson::String)for(auto& V:F->GetArrayField(TEXT("choices")))Ok|=V->AsObject()->GetStringField(TEXT("value"))==P.Value->AsString();
        }
        if(!Ok){Error=SMLocalization::Text(FString(TEXT("Invalid gameplay option: ")))+Key;return false;}
    }
    TSet<FString> Known;for(auto& V:C->GetArrayField(TEXT("techniques")))Known.Add(V->AsObject()->GetStringField(TEXT("key")));
    for(const auto& P:T->Values){bool Ok=Known.Contains(FString(*P.Key)) && P.Value->Type==EJson::Array;
        if(Ok){auto V=P.Value->AsArray();Ok=V.Num()==2 && V[0]->Type==EJson::Boolean && V[1]->Type==EJson::Number && FMath::IsFinite(V[1]->AsNumber()) && V[1]->AsNumber()>=0 && V[1]->AsNumber()<=800;}
        if(!Ok){Error=SMLocalization::Text(FString(TEXT("Invalid technique: ")))+FString(*P.Key);return false;}
    }
    for(const auto& P:S->Values){bool Ok=false;if(P.Value->Type==EJson::String)for(const auto& G:C->GetObjectField(TEXT("skillSettings"))->Values){const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;if(G.Value->AsObject()->TryGetArrayField(P.Key,Values))for(auto& V:*Values)Ok|=V->AsString()==P.Value->AsString();}if(!Ok){Error=SMLocalization::Text(FString(TEXT("Invalid combat or traversal tolerance: ")))+FString(*P.Key);return false;}}
    return true;
}
