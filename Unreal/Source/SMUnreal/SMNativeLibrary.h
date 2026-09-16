#pragma once
#include "CoreMinimal.h"
#include "HAL/PlatformProcess.h"

namespace SMNativeLibrary {
#if PLATFORM_WINDOWS
void* Open(const FString& Root);
#else
inline void* Open(const FString& Root) {
    return FPlatformProcess::GetDllHandle(*(Root / TEXT("Native/build/libsm_native.so")));
}
#endif
}
