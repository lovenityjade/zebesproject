#include "SMSystemMenu.h"
#include "SMHUD.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

#if !UE_BUILD_SHIPPING
bool FSMSystemMenu::RunMenuFlowTest(){
    if(!IFileManager::Get().FileExists(*(FPaths::ProjectDir()/TEXT("../ISOLATED_TEST_DIRECTORY"))))return false;
    auto Require=[](bool Ok,const TCHAR* What){if(!Ok)UE_LOG(LogTemp,Error,TEXT("SM_MENU_FLOW_FAILED %s"),What);return Ok;};
    if(!Require(!IsOpen(),TEXT("boot keeps the system menu closed")))return false;
    auto Ram=reinterpret_cast<const uint8*(*)()>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_simulation_ram")));
    auto GenerationState=reinterpret_cast<int(*)()>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_generation_state")));
    auto Menu=reinterpret_cast<int(*)(int)>(FPlatformProcess::GetDllExport(Hud.CoreHandle,TEXT("sm_generation_menu")));
    auto CanStart=[&]{return Menu && Menu(3)!=0;};
    if(!Require(Ram && GenerationState,TEXT("native exports")))return false;
    auto Word=[&](int Offset){uint16 Value;FMemory::Memcpy(&Value,Ram()+Offset,2);return Value;};
    auto Press=[&](uint16 Button){return Hud.Step(Button) && Hud.Step(0);};
    auto Options=[&]{return Hud.State()==2 && Word(0xde2)==3;};
    auto Row=[&](int N){for(int I=0;I<6 && Word(0x99e)!=N;I++)if(!Press(32))return false;return Word(0x99e)==N;};
    FString Error;FSMGameProfile Bank;
    if(!Require(FSMProfiles::CreateBank(TEXT("Menu flow fixture"),nullptr,nullptr,Bank,Error) && Activate(Bank),TEXT("isolated new bank")))return false;
    for(int I=0;I<9000 && !(Hud.State()==4 && Word(0x727)==4);I++)if(!Hud.Step(I>180 && I%120<2?8:0))return false;
    if(!Require(Hud.State()==4 && Word(0x727)==4 && !IsOpen(),TEXT("native file select without popup")))return false;
    for(int I=0;I<3;I++)Hud.Step(0);
    if(!Press(8))return false;
    for(int I=0;I<1500 && !Options();I++)Hud.Step(0);
    if(!Require(Options() && CurrentSlot()==0 && CanStart(),TEXT("new Vanilla A can start immediately")))return false;
    for(int I=0;I<3;I++)Hud.Step(0);
    if(!Require(Row(1) && Press(32) && Word(0x99e)==0 && Press(32) && Word(0x99e)==1 && !TakeGenerationRequest(),TEXT("Vanilla only exposes Mode and Start")))return false;
    if(!Require(Press(128) && Menu(0)==1 && !CanStart() && !Active.Slots[0].Randomized,TEXT("Story preview cannot start or change the save")))return false;
    if(!Require(Row(0) && Press(8) && Options(),TEXT("Story start is blocked")))return false;
    if(!Require(Row(1) && Press(128) && Menu(0)==2 && CanStart(),TEXT("Boss Rush can start without modifying the Vanilla slot")))return false;
    if(!Require(Row(3) && Press(128) && Press(128) && Press(128) && Menu(1)==3,TEXT("Boss Rush offers Very Hard")))return false;
    if(!Require(Press(128) && Menu(1)==4 && Press(128) && Menu(1)==0 && Press(64) && Menu(1)==4,TEXT("Boss Rush cycles five difficulties in both directions")))return false;
    // Configure defaults before selecting Randomized, as a player can from Escape.
    SelectSection(2);SetOpen(true);Seed=12345;FCStringAnsi::Strcpy(SeedNumber,"12345");
    RelicHunt=false;NoAdvancedTechs=true;
    if(!Require(EditingSlot==-1 && SaveSeedDraft(),TEXT("default draft saved")))return false;
    SetOpen(false);
    if(!Require(Row(1) && Press(128) && !CanStart() && Active.Slots[0].Randomized &&
        Active.Slots[0].Request.Seed==12345 && Active.Slots[0].Request.NoAdvancedTechs && !Active.Slots[1].Randomized,
        TEXT("native mode uses defaults for A only")))return false;
    if(!Require(Row(0) && Press(8) && Options(),TEXT("pending Randomized cannot start")))return false;
    if(!Require(Row(2) && Press(8) && OpenSlotSettings,TEXT("native options requests editor")))return false;
    Tick();
    if(!Require(IsOpen() && Section==2 && EditingSlot==0 && Seed==12345,TEXT("editor is bound to selected slot")))return false;
    Seed=54321;FCStringAnsi::Strcpy(SeedNumber,"54321");
    if(!Require(SaveSeedDraft(),TEXT("slot draft autosave")))return false;
    FSMGameProfile Disk;
    if(!Require(FSMProfiles::Read(Active.Directory,Disk,Error) && Disk.Slots[0].Request.Seed==54321 &&
        !Disk.Slots[1].Randomized && !Disk.Slots[2].Randomized,TEXT("disk round trip and B/C isolation")))return false;
    SetOpen(false);SelectSection(0);EditingSlot=-1;Seed=7;
    SetOpen(true);SelectSection(2);
    if(!Require(EditingSlot==0 && Seed==54321,TEXT("Escape editor reloads pending slot instead of stale defaults")))return false;
    SetOpen(false);
    if(!Require(Row(1) && Press(128) && CanStart() && !Active.Slots[0].Randomized,TEXT("native Vanilla mode unlocks Start")))return false;
    if(!Require(Row(0) && Press(8),TEXT("native Start input")))return false;
    for(int I=0;I<900 && Options();I++)Hud.Step(0);
    return Require(!Options() && !IsOpen(),TEXT("Vanilla starts without opening system UI"));
}
#endif
