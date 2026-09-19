#include "SMImGuiWidget.h"
#include "ImageUtils.h"
#include "HAL/PlatformProcess.h"
#include "imgui.h"
#include <string>
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElementTypes.h"
#include "Rendering/SlateRenderer.h"
#include "Misc/Paths.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/FileManager.h"

namespace {
struct FContextScope {
    ImGuiContext* Previous;
    explicit FContextScope(ImGuiContext* C):Previous(ImGui::GetCurrentContext()){ImGui::SetCurrentContext(C);}
    ~FContextScope(){ImGui::SetCurrentContext(Previous);}
};
ImGuiKey Translate(FKey K) {
#define KEY(UE, IM) if(K==EKeys::UE) return ImGuiKey_##IM
    KEY(Tab,Tab);KEY(Left,LeftArrow);KEY(Right,RightArrow);KEY(Up,UpArrow);KEY(Down,DownArrow);
    KEY(PageUp,PageUp);KEY(PageDown,PageDown);KEY(Home,Home);KEY(End,End);KEY(Insert,Insert);KEY(Delete,Delete);
    KEY(BackSpace,Backspace);KEY(SpaceBar,Space);KEY(Enter,Enter);KEY(Escape,Escape);
    KEY(LeftControl,LeftCtrl);KEY(RightControl,RightCtrl);KEY(LeftShift,LeftShift);KEY(RightShift,RightShift);
    KEY(LeftAlt,LeftAlt);KEY(RightAlt,RightAlt);
    KEY(Gamepad_FaceButton_Bottom,GamepadFaceDown);KEY(Gamepad_FaceButton_Right,GamepadFaceRight);
    KEY(Gamepad_FaceButton_Left,GamepadFaceLeft);KEY(Gamepad_FaceButton_Top,GamepadFaceUp);
    KEY(Gamepad_DPad_Up,GamepadDpadUp);KEY(Gamepad_DPad_Down,GamepadDpadDown);
    KEY(Gamepad_DPad_Left,GamepadDpadLeft);KEY(Gamepad_DPad_Right,GamepadDpadRight);
    KEY(Gamepad_LeftShoulder,GamepadL1);KEY(Gamepad_RightShoulder,GamepadR1);
    KEY(Gamepad_Special_Left,GamepadBack);KEY(Gamepad_Special_Right,GamepadStart);
#undef KEY
    const FString S=K.GetFName().ToString();
    if(S.Len()==1 && S[0]>='A' && S[0]<='Z') return ImGuiKey(ImGuiKey_A+S[0]-'A');
    return ImGuiKey_None;
}
int MouseButton(FKey Key) { return Key==EKeys::LeftMouseButton?0:Key==EKeys::RightMouseButton?1:Key==EKeys::MiddleMouseButton?2:-1; }
}
void SSMImGuiWidget::Construct(const FArguments&) {
    SetCanTick(true);SetClipping(EWidgetClipping::ClipToBounds);ForceVolatile(true);
    ImGuiContext* Previous=ImGui::GetCurrentContext();Context=ImGui::CreateContext();
    ImGuiIO& IO=ImGui::GetIO();IO.IniFilename=nullptr;IO.LogFilename=nullptr;
    IO.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_NavEnableGamepad;
    IO.BackendFlags|=ImGuiBackendFlags_RendererHasVtxOffset|ImGuiBackendFlags_HasGamepad;
    IO.BackendPlatformName="projectSM_Slate";IO.BackendRendererName="projectSM_Slate";
    IO.SetClipboardTextFn=[](void*,const char* Text){FPlatformApplicationMisc::ClipboardCopy(UTF8_TO_TCHAR(Text));};
    IO.GetClipboardTextFn=[](void*)->const char*{static std::string Buffer;FString Text;FPlatformApplicationMisc::ClipboardPaste(Text);Buffer=TCHAR_TO_UTF8(*Text);return Buffer.c_str();};
    const FString Font=FPaths::ProjectContentDir()/TEXT("UI/Montserrat-Regular.ttf");
    ImFontConfig FontConfig;FontConfig.OversampleH=2;FontConfig.OversampleV=2;
    if(IFileManager::Get().FileExists(*Font)) IO.Fonts->AddFontFromFileTTF(TCHAR_TO_UTF8(*Font),20.f,&FontConfig);
    if(IO.Fonts->Fonts.empty()) IO.Fonts->AddFontDefault();
    unsigned char* Pixels;int W,H;IO.Fonts->GetTexDataAsRGBA32(&Pixels,&W,&H);
    FontTexture.Reset(UTexture2D::CreateTransient(W,H,PF_R8G8B8A8));
    FontTexture->SRGB=false;FontTexture->Filter=TF_Bilinear;FontTexture->NeverStream=true;
    auto& Mip=FontTexture->GetPlatformData()->Mips[0];void* Dest=Mip.BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(Dest,Pixels,W*H*4);Mip.BulkData.Unlock();FontTexture->UpdateResource();
    FontBrush.SetResourceObject(FontTexture.Get());FontBrush.ImageSize=FVector2D(W,H);
    FontBrush.DrawAs=ESlateBrushDrawType::Image;IO.Fonts->SetTexID(1);
    ImGuiStyle& S=ImGui::GetStyle();S.WindowRounding=6;S.ChildRounding=5;S.FrameRounding=5;S.GrabRounding=4;
    S.WindowPadding=ImVec2(16,14);S.FramePadding=ImVec2(10,7);S.ItemSpacing=ImVec2(10,10);S.FrameBorderSize=1;S.ChildBorderSize=1;
    auto RGB=[](int R,int G,int B){return ImVec4(R/255.f,G/255.f,B/255.f,1);};
    S.Colors[ImGuiCol_WindowBg]=RGB(11,14,20);S.Colors[ImGuiCol_ChildBg]=RGB(17,23,34);S.Colors[ImGuiCol_PopupBg]=RGB(17,23,34);
    S.Colors[ImGuiCol_Text]=RGB(241,242,244);S.Colors[ImGuiCol_TextDisabled]=RGB(153,163,179);
    S.Colors[ImGuiCol_Border]=RGB(43,52,67);S.Colors[ImGuiCol_Separator]=RGB(43,52,67);
    S.Colors[ImGuiCol_FrameBg]=RGB(23,30,42);S.Colors[ImGuiCol_Button]=RGB(23,30,42);
    S.Colors[ImGuiCol_ButtonHovered]=S.Colors[ImGuiCol_HeaderHovered]=RGB(65,57,36);
    S.Colors[ImGuiCol_ButtonActive]=S.Colors[ImGuiCol_Header]=S.Colors[ImGuiCol_HeaderActive]=RGB(83,67,36);
    S.Colors[ImGuiCol_CheckMark]=S.Colors[ImGuiCol_SliderGrab]=S.Colors[ImGuiCol_NavCursor]=RGB(212,174,85);
    ImGui::SetCurrentContext(Previous);
}
SSMImGuiWidget::~SSMImGuiWidget(){if(Context)ImGui::DestroyContext(Context);}
void SSMImGuiWidget::ClearInput(){FContextScope Scope(Context);ImGui::GetIO().ClearInputKeys();ImGui::GetIO().ClearInputMouse();FrameReady=false;}
void SSMImGuiWidget::Tick(const FGeometry& Geo,double,float Delta) {
    if(!DrawMenu || !GetVisibility().IsVisible())return;
    PixelScale=FMath::Max(.1f,Geo.GetAccumulatedLayoutTransform().GetScale());
    FContextScope Scope(Context);ImGuiIO& IO=ImGui::GetIO();
    IO.DisplaySize=ImVec2(Geo.GetLocalSize().X,Geo.GetLocalSize().Y);IO.DeltaTime=FMath::Max(Delta,.001f);
    ImGui::NewFrame();DrawMenu();ImGui::Render();FrameReady=true;
}
int32 SSMImGuiWidget::OnPaint(const FPaintArgs&,const FGeometry& Geo,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const {
    if(!FrameReady)return Layer;
    FContextScope Scope(Context);const ImDrawData* Data=ImGui::GetDrawData();if(!Data)return Layer;

    for(const ImDrawList* List:Data->CmdLists) {
        for(const ImDrawCmd& Cmd:List->CmdBuffer) {
            if(Cmd.UserCallback || Cmd.ElemCount==0)continue;
            const uint64 Texture=uint64(Cmd.GetTexID());
            const FSlateBrush* Brush=&FontBrush;
            if(Texture>=2){const int Index=int(Texture-2);if(!HasAchievementImage(Index))continue;Brush=&AchievementBrushes[Index];}
            const auto Resource=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*Brush);
            const FVector2D TL=Geo.LocalToAbsolute(FVector2D(Cmd.ClipRect.x,Cmd.ClipRect.y));
            const FVector2D BR=Geo.LocalToAbsolute(FVector2D(Cmd.ClipRect.z,Cmd.ClipRect.w));
            Out.PushClip(FSlateClippingZone(FSlateRect(TL.X,TL.Y,BR.X,BR.Y)));
            // Compact each command to Slate's index range, honoring ImGui VtxOffset.
            TArray<FSlateVertex> Vertices;TArray<SlateIndex> Indices;
            unsigned int Min=MAX_uint32,Max=0;
            for(unsigned int I=0;I<Cmd.ElemCount;I++){unsigned int V=List->IdxBuffer[Cmd.IdxOffset+I];Min=FMath::Min(Min,V);Max=FMath::Max(Max,V);}
            Vertices.Reserve(Max-Min+1);Indices.Reserve(Cmd.ElemCount);
            for(unsigned int I=Min;I<=Max;I++) {
                const ImDrawVert& V=List->VtxBuffer[Cmd.VtxOffset+I];
                const FColor C((V.col>>IM_COL32_R_SHIFT)&255,(V.col>>IM_COL32_G_SHIFT)&255,(V.col>>IM_COL32_B_SHIFT)&255,(V.col>>IM_COL32_A_SHIFT)&255);
                Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Geo.GetAccumulatedRenderTransform(),FVector2f(V.pos.x,V.pos.y),FVector2f(V.uv.x,V.uv.y),C));
            }
            for(unsigned int I=0;I<Cmd.ElemCount;I++)Indices.Add(List->IdxBuffer[Cmd.IdxOffset+I]-Min);
            FSlateDrawElement::MakeCustomVerts(Out,Layer++,Resource,Vertices,Indices,nullptr,0,0);
            Out.PopClip();
        }
    }
    return Layer;
}
void SSMImGuiWidget::Key(const FKeyEvent& E,bool Down){
    FContextScope Scope(Context);auto& IO=ImGui::GetIO();ImGuiKey K=Translate(E.GetKey());
    if(K!=ImGuiKey_None)IO.AddKeyEvent(K,Down);
    IO.AddKeyEvent(ImGuiMod_Ctrl,E.IsControlDown());IO.AddKeyEvent(ImGuiMod_Shift,E.IsShiftDown());IO.AddKeyEvent(ImGuiMod_Alt,E.IsAltDown());
}
FReply SSMImGuiWidget::OnKeyDown(const FGeometry&,const FKeyEvent& E){
    if(!E.IsRepeat() && CaptureBinding && CaptureBinding(E.GetKey()))return FReply::Handled();
    if(!E.IsRepeat() && (E.GetKey()==EKeys::Escape || E.GetKey()==EKeys::F4 || E.GetKey()==EKeys::Gamepad_RightThumbstick || E.GetKey()==EKeys::Gamepad_Special_Left)){if(Back)Back();return FReply::Handled();}
    Key(E,true);return FReply::Handled();
}
FReply SSMImGuiWidget::OnKeyUp(const FGeometry&,const FKeyEvent& E){Key(E,false);return FReply::Handled();}
FReply SSMImGuiWidget::OnKeyChar(const FGeometry&,const FCharacterEvent& E){FContextScope Scope(Context);ImGui::GetIO().AddInputCharacter(E.GetCharacter());return FReply::Handled();}
FReply SSMImGuiWidget::OnAnalogValueChanged(const FGeometry&,const FAnalogInputEvent& E){
    FContextScope Scope(Context);auto& IO=ImGui::GetIO();float V=E.GetAnalogValue();
    auto Axis=[&](ImGuiKey Neg,ImGuiKey Pos){float N=FMath::Clamp((-V-.2f)/.8f,0.f,1.f),P=FMath::Clamp((V-.2f)/.8f,0.f,1.f);IO.AddKeyAnalogEvent(Neg,N>0,N);IO.AddKeyAnalogEvent(Pos,P>0,P);};
    if(E.GetKey()==EKeys::Gamepad_LeftX)Axis(ImGuiKey_GamepadLStickLeft,ImGuiKey_GamepadLStickRight);
    if(E.GetKey()==EKeys::Gamepad_LeftY)Axis(ImGuiKey_GamepadLStickDown,ImGuiKey_GamepadLStickUp);
    return FReply::Handled();
}
void SSMImGuiWidget::OnFocusLost(const FFocusEvent& E){SLeafWidget::OnFocusLost(E);ClearInput();}
FReply SSMImGuiWidget::OnMouseMove(const FGeometry& G,const FPointerEvent& E){FContextScope Scope(Context);const FVector2D P=G.AbsoluteToLocal(E.GetScreenSpacePosition());ImGui::GetIO().AddMousePosEvent(P.X,P.Y);return FReply::Handled();}
FReply SSMImGuiWidget::OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E){OnMouseMove(G,E);FContextScope Scope(Context);int B=MouseButton(E.GetEffectingButton());if(B>=0)ImGui::GetIO().AddMouseButtonEvent(B,true);return FReply::Handled().SetUserFocus(AsShared()).CaptureMouse(AsShared());}
FReply SSMImGuiWidget::OnMouseButtonUp(const FGeometry& G,const FPointerEvent& E){OnMouseMove(G,E);FContextScope Scope(Context);int B=MouseButton(E.GetEffectingButton());if(B>=0)ImGui::GetIO().AddMouseButtonEvent(B,false);return FReply::Handled().ReleaseMouseCapture();}
FReply SSMImGuiWidget::OnMouseWheel(const FGeometry&,const FPointerEvent& E){FContextScope Scope(Context);ImGui::GetIO().AddMouseWheelEvent(0,E.GetWheelDelta());return FReply::Handled();}

bool SSMImGuiWidget::HasAchievementImage(int Icon) const {
    return AchievementTextures.IsValidIndex(Icon)&&AchievementTextures[Icon].IsValid();
}
void SSMImGuiWidget::LoadAchievementImages(void* CoreHandle) {
    const TCHAR* Names[]={TEXT("morph"),TEXT("bombs"),TEXT("varia"),TEXT("gravity"),TEXT("speed"),TEXT("space"),TEXT("screw"),TEXT("tablet"),TEXT("beams"),TEXT("energy"),TEXT("missiles"),TEXT("kraid"),TEXT("phantoon"),TEXT("draygon"),TEXT("ridley"),TEXT("seals"),TEXT("miniboss"),TEXT("map"),TEXT("animals"),TEXT("ship")};
    auto SetIcon=reinterpret_cast<int(*)(int,const uint8*,int,int)>(FPlatformProcess::GetDllExport(CoreHandle,TEXT("sm_achievement_set_icon")));
    AchievementTextures.SetNum(20);AchievementBrushes.SetNum(20);
    for(int I=0;I<20;I++){
        UTexture2D* Texture=FImageUtils::ImportFileAsTexture2D(FPaths::ProjectContentDir()/TEXT("Achievements")/(FString(Names[I])+TEXT(".png")));
        if(!Texture){UE_LOG(LogTemp,Warning,TEXT("SM_ACHIEVEMENT_ICON_MISSING %s"),Names[I]);continue;}
        // Keep full-resolution authored files, but only retain a small UI texture.
        // Nearest sampling preserves hard pixel edges at the actual display size.
        if(Texture->GetPixelFormat()==PF_B8G8R8A8){
            auto& Source=Texture->GetPlatformData()->Mips[0];
            const uint8* Src=static_cast<const uint8*>(Source.BulkData.LockReadOnly());
            UTexture2D* Small=UTexture2D::CreateTransient(64,64,PF_B8G8R8A8);
            auto& Target=Small->GetPlatformData()->Mips[0];
            uint8* Dst=static_cast<uint8*>(Target.BulkData.Lock(LOCK_READ_WRITE));
            for(int Y=0;Y<64;Y++)for(int X=0;X<64;X++)FMemory::Memcpy(Dst+(Y*64+X)*4,Src+((Y*Source.SizeY/64)*Source.SizeX+X*Source.SizeX/64)*4,4);
            Target.BulkData.Unlock();Source.BulkData.Unlock();Small->SRGB=Texture->SRGB;Texture=Small;
        }
        Texture->Filter=TF_Nearest;Texture->NeverStream=true;
        if(SetIcon&&Texture->GetPixelFormat()==PF_B8G8R8A8){
            auto& Mip=Texture->GetPlatformData()->Mips[0];
            const uint8* Pixels=static_cast<const uint8*>(Mip.BulkData.LockReadOnly());
            SetIcon(I,Pixels,Mip.SizeX,Mip.SizeY);Mip.BulkData.Unlock();
        }
        Texture->UpdateResource();AchievementTextures[I].Reset(Texture);
        FSlateBrush& Brush=AchievementBrushes[I];Brush.SetResourceObject(Texture);Brush.ImageSize=FVector2D(Texture->GetSizeX(),Texture->GetSizeY());Brush.DrawAs=ESlateBrushDrawType::Image;
    }
}
