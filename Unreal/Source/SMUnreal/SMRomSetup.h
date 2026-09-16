#pragma once
#include "CoreMinimal.h"
class APlayerController;
class SSMImGuiWidget;

struct FSMRomSetup {
    FSMRomSetup(const FString& Destination, const FString& Reason);
    ~FSMRomSetup();
    void Show(APlayerController* Player);
    bool Completed = false;
private:
    void Draw();
    void Browse(const FString& Directory);
    void Select(const FString& File);
    TSharedPtr<SSMImGuiWidget> Widget;
    FString Destination, Status, CurrentDirectory;
    TArray<FString> Directories, Files;
    char Candidate[4096] = {};
    char Folder[4096] = {};
    bool Verified = false, BrowserOpen = true;
};
