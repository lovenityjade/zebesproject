#include "SMHUD.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericWindow.h"
#include "HAL/PlatformProcess.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#elif PLATFORM_LINUX && !WITH_EDITOR
#include "SDL3/SDL.h"
#endif

void ASMHUD::ApplyRomWindowIcon(){
    if(!GEngine || !GEngine->GameViewport || !CoreHandle)return;
    const TSharedPtr<SWindow> Window=GEngine->GameViewport->GetWindow();
    if(!Window.IsValid() || !Window->GetNativeWindow().IsValid())return;
    using PixelsFn=const uint8*(*)();
    const auto IconPixels=reinterpret_cast<PixelsFn>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_window_icon_pixels")));
    if(!IconPixels)return;
    const uint8* Source=IconPixels();
    TArray<uint8> Rgba;Rgba.SetNumUninitialized(64*64*4);
    for(int Y=0;Y<64;Y++)for(int X=0;X<64;X++)
        FMemory::Memcpy(Rgba.GetData()+(Y*64+X)*4,Source+((Y/4)*16+X/4)*4,4);
    void* Handle=Window->GetNativeWindow()->GetOSWindowHandle();
#if PLATFORM_WINDOWS
    ReleaseRomWindowIcon();
    BITMAPINFO Info{};Info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    Info.bmiHeader.biWidth=64;Info.bmiHeader.biHeight=-64;
    Info.bmiHeader.biPlanes=1;Info.bmiHeader.biBitCount=32;Info.bmiHeader.biCompression=BI_RGB;
    void* Bits=nullptr;HBITMAP Color=CreateDIBSection(nullptr,&Info,DIB_RGB_COLORS,&Bits,nullptr,0);
    HBITMAP Mask=CreateBitmap(64,64,1,1,nullptr);
    if(Color && Mask && Bits){
        uint8* Bgra=static_cast<uint8*>(Bits);
        for(int I=0;I<64*64;I++){
            Bgra[I*4]=Rgba[I*4+2];Bgra[I*4+1]=Rgba[I*4+1];Bgra[I*4+2]=Rgba[I*4];Bgra[I*4+3]=Rgba[I*4+3];
        }
        ICONINFO Icon{};Icon.fIcon=1;Icon.hbmColor=Color;Icon.hbmMask=Mask;
        if(HICON Created=CreateIconIndirect(&Icon)){
            RomWindowIcon=Created;RomIconWindow=Handle;
            PreviousLargeIcon=reinterpret_cast<void*>(SendMessageW(static_cast<HWND>(Handle),WM_SETICON,ICON_BIG,reinterpret_cast<LPARAM>(Created)));
            PreviousSmallIcon=reinterpret_cast<void*>(SendMessageW(static_cast<HWND>(Handle),WM_SETICON,ICON_SMALL,reinterpret_cast<LPARAM>(Created)));
        }
    }
    if(Color)DeleteObject(Color);if(Mask)DeleteObject(Mask);
#elif PLATFORM_LINUX && !WITH_EDITOR
    SDL_Surface* Surface=SDL_CreateSurfaceFrom(64,64,SDL_PIXELFORMAT_RGBA32,Rgba.GetData(),64*4);
    if(Surface){
        if(SDL_SetWindowIcon(static_cast<SDL_Window*>(Handle),Surface))UE_LOG(LogTemp,Display,TEXT("SM_WINDOW_ICON ROM helmet applied"));
        SDL_DestroySurface(Surface);
    }
#endif
}
void ASMHUD::ReleaseRomWindowIcon(){
#if PLATFORM_WINDOWS
    if(RomWindowIcon){
        HWND Window=static_cast<HWND>(RomIconWindow);
        if(IsWindow(Window)){
            if(reinterpret_cast<void*>(SendMessageW(Window,WM_GETICON,ICON_BIG,0))==RomWindowIcon)
                SendMessageW(Window,WM_SETICON,ICON_BIG,reinterpret_cast<LPARAM>(PreviousLargeIcon));
            if(reinterpret_cast<void*>(SendMessageW(Window,WM_GETICON,ICON_SMALL,0))==RomWindowIcon)
                SendMessageW(Window,WM_SETICON,ICON_SMALL,reinterpret_cast<LPARAM>(PreviousSmallIcon));
        }
        DestroyIcon(static_cast<HICON>(RomWindowIcon));
        RomWindowIcon=RomIconWindow=PreviousLargeIcon=PreviousSmallIcon=nullptr;
    }
#endif
}
