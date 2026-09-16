#include "SMNativeLibrary.h"
#if PLATFORM_WINDOWS
#include "Misc/Paths.h"
#include "Windows/WindowsHWrapper.h"

void* SMNativeLibrary::Open(const FString& Root) {
    // UE's Windows GetDllHandle returns GetModuleHandle for an already loaded
    // DLL, without retaining it. Validation/generation release their handle
    // while the HUD still uses its exports. Every Open must own a reference.
    const FString Path=FPaths::ConvertRelativePathToFull(Root / TEXT("Native/build/sm_native.dll"));
    void* Handle=::LoadLibraryExW(*Path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if(!Handle)UE_LOG(LogTemp,Error,TEXT("SM_NATIVE_LOAD_FAILED path=%s windows_error=%lu"),*Path,::GetLastError());
    return Handle;
}
#endif
