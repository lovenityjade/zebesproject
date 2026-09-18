#pragma once
#include "CoreMinimal.h"
namespace SMLocalization {
 int Language();
 void SetLanguage(int Language);
 bool Save(const FString& SettingsPath);
 void Load(const FString& SettingsPath);
 const char* Text(const char* English);
 const char* Label(const char* English);
 FString Text(const FString& English);
 inline FString Format(const FString& English,const FStringFormatOrderedArguments& Args){return FString::Format(*Text(English),Args);}
}
