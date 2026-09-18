#include "SMRomSetup.h"
#include "SMRom.h"
#include "SMImGuiWidget.h"
#include "SMLocalizedImGui.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

FSMRomSetup::FSMRomSetup(const FString& Local, const FString& Reason)
    : Destination(Local), Status(Reason) {
    Browse(FPlatformProcess::UserHomeDir());
}
FSMRomSetup::~FSMRomSetup() {
    if (Widget) {
        Widget->DrawMenu = nullptr;
        if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Widget.ToSharedRef());
    }
}
void FSMRomSetup::Show(APlayerController* Player) {
    Widget = SNew(SSMImGuiWidget);
    Widget->DrawMenu = [this] { Draw(); };
    GEngine->GameViewport->AddViewportWidgetContent(Widget.ToSharedRef(), 100);
    Player->bShowMouseCursor = true;
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(Widget);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Player->SetInputMode(Mode);
    FSlateApplication::Get().SetAllUserFocus(Widget.ToSharedRef());
}
void FSMRomSetup::Browse(const FString& Directory) {
    FString Path = FPaths::ConvertRelativePathToFull(Directory);
    FPaths::NormalizeDirectoryName(Path);
    FPaths::CollapseRelativeDirectories(Path);
    if (!IFileManager::Get().DirectoryExists(*Path)) {
        Status = TEXT("Folder not found. Enter a folder path or choose Home."); return;
    }
    CurrentDirectory = Path;
    FCStringAnsi::Strncpy(Folder, TCHAR_TO_UTF8(*Path), UE_ARRAY_COUNT(Folder));
    IFileManager::Get().FindFiles(Directories, *(Path / TEXT("*")), false, true);
    IFileManager::Get().FindFiles(Files, *(Path / TEXT("*")), true, false);
    Directories.RemoveAll([](const FString& Name) { return Name.StartsWith(TEXT(".")); });
    Files.RemoveAll([](const FString& Name) { return !Name.EndsWith(TEXT(".sfc")) && !Name.EndsWith(TEXT(".smc")); });
    Directories.Sort(); Files.Sort();
}
void FSMRomSetup::Select(const FString& File) {
    FCStringAnsi::Strncpy(Candidate, TCHAR_TO_UTF8(*File), UE_ARRAY_COUNT(Candidate));
    Verified = SMRom::Validate(File, Status);
    if (Verified) { Status = TEXT("Compatible ROM confirmed. CRC32: D63ED5F8. SHA-1 verified."); BrowserOpen = false; }
}
void FSMRomSetup::Draw() {
    const ImVec2 Screen = SMUI::GetIO().DisplaySize;
    SMUI::SetNextWindowPos(ImVec2(0, 0));
    SMUI::SetNextWindowSize(Screen);
    SMUI::Begin("ROM setup", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    SMUI::TextColored(ImVec4(.83f,.68f,.33f,1), "THE ZEBES PROJECT");
    SMUI::TextUnformatted("Set up Super Metroid");
    int Language=SMLocalization::Language();SMUI::SetNextItemWidth(230);
    if(SMUI::Combo("Language",&Language,"English\0French (Canada)\0")){SMLocalization::SetLanguage(Language);SMLocalization::Save(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SM/Presentation.ini")));}
    SMUI::Separator();
    SMUI::TextWrapped("Select your unmodified Japan/USA ROM. It will be verified and copied into the game's local roms folder.");
    SMUI::TextUnformatted("Super Metroid (Japan, USA) (En,Ja).sfc");
    SMUI::TextUnformatted("CRC32: D63ED5F8  |  Size: 3,145,728 bytes  |  No copier header");
    SMUI::TextDisabled("A different filename is fine; the file contents must match.");
    SMUI::Spacing();
    SMUI::SetNextItemWidth(-1);
    if (SMUI::InputTextWithHint("##rom", "Paste a ROM file path, or browse below", Candidate, UE_ARRAY_COUNT(Candidate))) {
        Verified = false; Status.Reset();
    }
    if (SMUI::Button("Verify selected ROM")) Select(UTF8_TO_TCHAR(Candidate));
    SMUI::SameLine();
    if (SMUI::Button(BrowserOpen ? "Hide browser" : "Browse files")) BrowserOpen = !BrowserOpen;
    if (BrowserOpen) {
        SMUI::Spacing();
        if (SMUI::Button("Home")) Browse(FPlatformProcess::UserHomeDir());
        SMUI::SameLine();
        if (SMUI::Button("Up")) Browse(CurrentDirectory / TEXT(".."));
#if PLATFORM_LINUX
        SMUI::SameLine(); if (SMUI::Button("Drives")) Browse(TEXT("/run/media"));
        SMUI::SameLine(); if (SMUI::Button("Filesystem")) Browse(TEXT("/"));
#endif
        SMUI::SetNextItemWidth(-80);
        bool Go = SMUI::InputText("##folder", Folder, UE_ARRAY_COUNT(Folder), ImGuiInputTextFlags_EnterReturnsTrue);
        SMUI::SameLine(); Go |= SMUI::Button("Go");
        if (Go) Browse(UTF8_TO_TCHAR(Folder));
        SMUI::BeginChild("Files", ImVec2(0, FMath::Clamp(Screen.y - 410.f, 100.f, 310.f)), ImGuiChildFlags_Borders);
        FString NextDirectory, NextFile;
        for (const FString& Name : Directories) {
            const FString Label = SMLocalization::Text(FString(TEXT("[Folder] "))) + Name;
            if (ImGui::Selectable(TCHAR_TO_UTF8(*Label))) NextDirectory = CurrentDirectory / Name;
        }
        for (const FString& Name : Files)
            if (ImGui::Selectable(TCHAR_TO_UTF8(*Name))) NextFile = CurrentDirectory / Name;
        if (Directories.IsEmpty() && Files.IsEmpty()) SMUI::TextDisabled("No folders or .sfc/.smc files here.");
        SMUI::EndChild();
        if (!NextDirectory.IsEmpty()) Browse(NextDirectory);
        if (!NextFile.IsEmpty()) Select(NextFile);
    }
    SMUI::Spacing();
    if (!Status.IsEmpty()) {
        SMUI::PushStyleColor(ImGuiCol_Text, Verified ? ImVec4(.4f,.85f,.65f,1) : ImVec4(1,.65f,.4f,1));
        SMUI::TextWrapped("%s", TCHAR_TO_UTF8(*Status));
        SMUI::PopStyleColor();
    }
    SMUI::BeginDisabled(!Verified);
    if (SMUI::Button("Confirm, copy ROM and continue")) {
        Completed = SMRom::Import(UTF8_TO_TCHAR(Candidate), Destination, Status);
        if (!Completed) Verified = false;
    }
    SMUI::EndDisabled();
    SMUI::SameLine();
    if (SMUI::Button("Exit game")) FPlatformMisc::RequestExit(false);
    SMUI::TextWrapped("Local copy: %s", SMUI::Raw(TCHAR_TO_UTF8(*Destination)));
    SMUI::TextDisabled("The local copy is checked again at every launch. Your source file is kept unchanged.");
    SMUI::End();
}
