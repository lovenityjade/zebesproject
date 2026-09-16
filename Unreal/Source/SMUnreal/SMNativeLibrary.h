#pragma once
#include "CoreMinimal.h"
#include "HAL/PlatformProcess.h"

namespace SMNativeLibrary {
inline void* Open(const FString& Root) {
#if PLATFORM_WINDOWS
    return FPlatformProcess::GetDllHandle(*(Root / TEXT("Native/build/sm_native.dll")));
#else
    return FPlatformProcess::GetDllHandle(*(Root / TEXT("Native/build/libsm_native.so")));
#endif
}
}
