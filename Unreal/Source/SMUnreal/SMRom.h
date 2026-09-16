#pragma once
#include "CoreMinimal.h"

// One compatibility contract for boot, selection and the installed copy.
namespace SMRom {
inline constexpr int32 Size = 3145728;
inline constexpr uint32 Crc = 0xd63ed5f8;
inline constexpr const TCHAR* Filename = TEXT("Super Metroid (Japan, USA) (En,Ja).sfc");
inline constexpr const TCHAR* Sha1 = TEXT("da957f0d63d14cb441d215462904c4fa8519c613");
FString DataRoot(const FString& Root);
FString LocalPath(const FString& Root);
bool Validate(const FString& Path, FString& Error);
bool Import(const FString& Source, const FString& Destination, FString& Error);
}
bool SMRunRomSelfTest(const FString& Root, FString& Error);
