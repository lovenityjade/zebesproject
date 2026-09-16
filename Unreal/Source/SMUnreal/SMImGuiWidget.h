#pragma once
#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Styling/SlateBrush.h"
class UTexture2D;
struct ImGuiContext;

// Single-viewport integration: Slate owns presentation, focus, DPI and clipping.
class SSMImGuiWidget : public SLeafWidget {
public:
    SLATE_BEGIN_ARGS(SSMImGuiWidget) {} SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual ~SSMImGuiWidget() override;
    TFunction<void()> DrawMenu;
    TFunction<void()> Back;
    TFunction<bool(FKey)> CaptureBinding;
    void ClearInput();
    float GetPixelScale() const { return PixelScale; }
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1280,720); }
    virtual void Tick(const FGeometry&, double, float) override;
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
    virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent&) override;
    virtual FReply OnKeyUp(const FGeometry&, const FKeyEvent&) override;
    virtual FReply OnKeyChar(const FGeometry&, const FCharacterEvent&) override;
    virtual FReply OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent&) override;
    virtual void OnFocusLost(const FFocusEvent&) override;
    virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
    virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent&) override;
private:
    ImGuiContext* Context=nullptr;
    TStrongObjectPtr<UTexture2D> FontTexture;
    FSlateBrush FontBrush;
    bool FrameReady=false;
    float PixelScale=1.f;
    void Key(const FKeyEvent&,bool);
};
