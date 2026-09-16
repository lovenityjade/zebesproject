#pragma once
#include "SMRandomizer.h"
#include "Dom/JsonObject.h"
namespace SMSeedSettings {
    inline bool ValidEscapeMinutes(int32 N){return N==3||N==5||N==6||N==7||N==10;}
    inline bool ReadEscapeMinutes(const TSharedPtr<FJsonObject>& O,int32& N){double D=5;if(O->HasField(TEXT("escapeMinutes")) && !O->TryGetNumberField(TEXT("escapeMinutes"),D))return false;if(!FMath::IsFinite(D)||D!=FMath::FloorToDouble(D)||D<0||D>10)return false;N=int32(D);return ValidEscapeMinutes(N);}

bool ParseSeedNumber(const FString& Text,int32& Out); // Empty => automatic (0), invalid leaves Out unchanged.
int32 RandomSeed();
bool ExportString(const FSMSeedRequest& Request,FString& Out,FString& Error);
bool ImportString(const FString& Text,FSMSeedRequest& Request,FString& Error);
TSharedPtr<FJsonObject> Catalog();
bool Read(const TSharedPtr<FJsonObject>& Object,FSMSeedRequest& Request);
bool Write(const FSMSeedRequest& Request,const TSharedRef<FJsonObject>& Object);
bool ValidBasics(const FSMSeedRequest& Request);
bool ValidateDraft(const FSMSeedRequest& Request,FString& Error);
bool HasOverrides(const FSMSeedRequest& Request);
bool Matches(const FSMSeedRequest& Request,const TSharedPtr<FJsonObject>& Requested);
TSharedPtr<FJsonObject> Parse(const FString& Json);
FString Encode(const TSharedRef<FJsonObject>& Object);
}
