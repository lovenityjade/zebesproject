#include "SMProfiles.h"
#include "SMLocalization.h"
#include "SMSeedSettings.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Misc/Base64.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

namespace {
FString TestProfileRoot;
FString Digest(const FString& Text) {
    FTCHARToUTF8 Bytes(*Text); FSHAHash Hash;
    FSHA1::HashBuffer(Bytes.Get(), Bytes.Length(), Hash.Hash);
    return Hash.ToString().ToLower();
}
bool CommitFile(const FString& Path,const FString& Temporary) {
#if PLATFORM_WINDOWS
    // IPlatformFile::MoveFile uses MoveFileW on Windows and refuses an existing
    // destination. Keep the old profile intact until the replacement commits.
    const FString From=FPaths::ConvertRelativePathToFull(Temporary);
    const FString To=FPaths::ConvertRelativePathToFull(Path);
    return ::MoveFileExW(*From,*To,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return FPlatformFileManager::Get().GetPlatformFile().MoveFile(*Path,*Temporary);
#endif
}
bool AtomicWrite(const FString& Path, const FString& Text) {
    return FFileHelper::SaveStringToFile(Text, *(Path+TEXT(".tmp")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) &&
        CommitFile(Path,Path+TEXT(".tmp"));
}
bool WriteSram(const FString& Path,const TArray<uint8>& Bytes){
    return Bytes.Num()==8192 && FFileHelper::SaveArrayToFile(Bytes,*(Path+TEXT(".tmp"))) && CommitFile(Path,Path+TEXT(".tmp"));
}
bool RecoverBank(const FString& Directory,FString& Error){
    const FString Path=Directory/TEXT("bank.transaction.json");
    if(!IFileManager::Get().FileExists(*Path))return true;
    FString Json,Encoded,Metadata,Id;double Schema=0;TSharedPtr<FJsonObject> Obj,Profile;TArray<uint8> Bytes;
    if(!FFileHelper::LoadFileToString(Json,*Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Obj) ||
       !Obj.IsValid() || !Obj->TryGetStringField(TEXT("sram"),Encoded) || !Obj->TryGetStringField(TEXT("profile"),Metadata) ||
       !FBase64::Decode(Encoded,Bytes) || Bytes.Num()!=8192 ||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Metadata),Profile) || !Profile.IsValid() ||
       !Profile->TryGetStringField(TEXT("id"),Id) || Id!=FPaths::GetCleanFilename(Directory) || !Profile->TryGetNumberField(TEXT("schema"),Schema) || Schema!=3){Error=TEXT("Invalid save-bank recovery journal");return false;}
    if(!WriteSram(Directory/TEXT("bank.sram.dat"),Bytes) || !AtomicWrite(Directory/TEXT("profile.json"),Metadata) ||
       !IFileManager::Get().Delete(*Path)){Error=TEXT("Cannot finish save-bank recovery");return false;}
    return true;
}
void WriteRuleRequest(const TSharedRef<FJsonObject>& R,const FSMSeedRequest& Request){
    SMSeedSettings::Write(Request,R);
    TArray<TSharedPtr<FJsonValue>> Patches;for(const auto& P:Request.Patches)Patches.Add(MakeShared<FJsonValueString>(P));R->SetArrayField(TEXT("patches"),Patches);
    R->SetBoolField(TEXT("noAdvancedTechs"),Request.NoAdvancedTechs);
    auto Relic=MakeShared<FJsonObject>();Relic->SetBoolField(TEXT("enabled"),Request.RelicHunt);
    Relic->SetNumberField(TEXT("placed"),Request.RelicsPlaced);Relic->SetNumberField(TEXT("required"),Request.RelicsRequired);Relic->SetNumberField(TEXT("escapeMinutes"),Request.RelicEscapeMinutes);
    R->SetObjectField(TEXT("relicHunt"),Relic);
}
bool ReadRuleRequest(const TSharedPtr<FJsonObject>& R,FSMSeedRequest& Request){
    if(!SMSeedSettings::Read(R,Request))return false;
    Request.Patches.Reset();if(R->HasField(TEXT("patches"))){const TArray<TSharedPtr<FJsonValue>>* Patches=nullptr;if(!R->TryGetArrayField(TEXT("patches"),Patches))return false;for(const auto& P:*Patches){if(P->Type!=EJson::String)return false;Request.Patches.Add(P->AsString());}}
    if(R->HasField(TEXT("noAdvancedTechs")) && !R->TryGetBoolField(TEXT("noAdvancedTechs"),Request.NoAdvancedTechs))return false;
    const TSharedPtr<FJsonObject>* Relic;
    if(R->HasField(TEXT("relicHunt"))){
        double Placed=0,Required=0;
        if(!R->TryGetObjectField(TEXT("relicHunt"),Relic) || !(*Relic)->TryGetBoolField(TEXT("enabled"),Request.RelicHunt) ||
           !(*Relic)->TryGetNumberField(TEXT("placed"),Placed) || !(*Relic)->TryGetNumberField(TEXT("required"),Required) ||
           Required<1 || Required>Placed || Placed>60 || Placed!=FMath::FloorToDouble(Placed) || Required!=FMath::FloorToDouble(Required))return false;
        Request.RelicsPlaced=int32(Placed);Request.RelicsRequired=int32(Required);if(!SMSeedSettings::ReadEscapeMinutes(*Relic,Request.RelicEscapeMinutes))return false;
    }
    return true;
}
bool Supported(const FSMSeedPlan& Plan, FString& Error) {
    if(Plan.PendingPatches.Num()) { Error=TEXT("This seed requires patches that are not implemented in the native port."); return false; }
    for(const FString& Behavior : Plan.RequiredNativeBehavior) {
        if(Behavior!=TEXT("zebes-awake") && Behavior!=TEXT("morph-eye-item-check") &&
           Behavior!=TEXT("native-animals-v1") && Behavior!=TEXT("respin-v1") && Behavior!=TEXT("infinite-spacejump-v1") && Behavior!=TEXT("item-sounds-v1") && Behavior!=TEXT("native-suits-v1") && Behavior!=TEXT("round-robin-cf-v1") && Behavior!=TEXT("momentum-landing-v1") && Behavior!=TEXT("nerfed-charge-v1") && Behavior!=TEXT("native-escape-v1") && Behavior!=TEXT("native-minimizer-v1") && Behavior!=TEXT("native-fast-tourian-v1") && Behavior!=TEXT("native-scavenger-v1") && Behavior!=TEXT("native-objectives-v1") && Behavior!=TEXT("native-area-connections-v1") && Behavior!=TEXT("native-initial-doors-v1") && Behavior!=TEXT("native-door-colors-v1") && Behavior!=TEXT("native-boss-connections-v1") && Behavior!=TEXT("native-door-indicators-v1") && Behavior!=TEXT("native-start-v1") && Behavior!=TEXT("seed-interface-v1") && Behavior!=TEXT("hidden-items-v1") && Behavior!=TEXT("fast-elevators-v1") && Behavior!=TEXT("hud-counts-v1") && Behavior!=TEXT("fast-doors-v1") && Behavior!=TEXT("nerfed-rainbow-v1") && Behavior!=TEXT("save-refill-v1") && Behavior!=TEXT("chozo-relic-v1") && Behavior!=TEXT("empty-pickup-v1") && Behavior!=TEXT("item-location-save-identity") && Behavior!=TEXT("red-tower-blue-doors") && Behavior!=TEXT("blue-brinstar-blue-door")) {
            Error=SMLocalization::Text(FString(TEXT("Unsupported seed behavior: ")))+Behavior; return false;
        }
    }
    return true;
}
}
FString FSMProfiles::Root() {
    if(!TestProfileRoot.IsEmpty())return TestProfileRoot;
    FString TemporarySave;
    if(FParse::Value(FCommandLine::Get(),TEXT("SMTemporarySave="),TemporarySave)){
        TemporarySave=FPaths::ConvertRelativePathToFull(TemporarySave);
        FPaths::NormalizeFilename(TemporarySave);
        FPaths::CollapseRelativeDirectories(TemporarySave);
        if(TemporarySave.Contains(TEXT("/SMTests/")) && IFileManager::Get().FileSize(*TemporarySave)==8192)
            return FPaths::GetPath(TemporarySave)/TEXT("Profiles");
    }
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SM/Profiles"));
}
bool FSMProfiles::Read(FString Directory, FSMGameProfile& Out, FString& Error,bool Recover) {
    if(Recover){if(!RecoverBank(Directory,Error))return false;}
    else if(IFileManager::Get().FileExists(*(Directory/TEXT("bank.transaction.json")))){Error=SMLocalization::Text(FString(TEXT("Save bank awaits recovery: ")))+Directory;return false;}
    Out={}; FString Text,Mode,Id; double Schema=0; bool Flag=false;
    TSharedPtr<FJsonObject> Obj;
    if(FFileHelper::LoadFileToString(Text,*(Directory/TEXT("profile.json"))) &&
       FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Obj) && Obj.IsValid() &&
       Obj->TryGetNumberField(TEXT("schema"),Schema) && Schema==3) {
        const TArray<TSharedPtr<FJsonValue>>* Entries=nullptr;FGuid Guid;
        if(!Obj->TryGetStringField(TEXT("id"),Out.Id) || Out.Id!=FPaths::GetCleanFilename(Directory) ||
           !FGuid::ParseExact(Out.Id,EGuidFormats::Digits,Guid) || !Obj->TryGetStringField(TEXT("name"),Out.Name) ||
           !Obj->TryGetArrayField(TEXT("slots"),Entries) || Entries->Num()!=3) {Error=TEXT("Invalid A/B/C save bank");return false;}
        Out.Directory=Directory;Out.SramPath=Directory/TEXT("bank.sram.dat");Obj->TryGetStringField(TEXT("createdUtc"),Out.CreatedUtc);
        for(const auto& Entry:*Entries){
            FString Child;FSMGameProfile Slot;
            if(!Entry->TryGetString(Child) || !FGuid::ParseExact(Child,EGuidFormats::Digits,Guid) ||
               !Read(Directory/TEXT("slots")/Child,Slot,Error) || !Slot.Slots.IsEmpty()) {Error=TEXT("Invalid slot manifest");return false;}
            Out.Slots.Add(MoveTemp(Slot));
        }
        return true;
    }
    Obj.Reset();Schema=0;
    if(!FFileHelper::LoadFileToString(Text, *(Directory/TEXT("profile.json"))) ||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Obj) || !Obj.IsValid() ||
       !Obj->TryGetNumberField(TEXT("schema"),Schema) || (Schema!=1 && Schema!=2) ||
       !Obj->TryGetStringField(TEXT("id"),Id) || Id!=FPaths::GetCleanFilename(Directory) ||
       !Obj->TryGetStringField(TEXT("name"),Out.Name) || Out.Name.IsEmpty() ||
       !Obj->TryGetStringField(TEXT("mode"),Mode) || (Mode!=TEXT("vanilla") && Mode!=TEXT("randomized")) ||
       !Obj->TryGetBoolField(TEXT("randomized"),Flag) || Flag!=(Mode==TEXT("randomized"))) {
        Error=SMLocalization::Text(FString(TEXT("Invalid save profile: ")))+Directory; return false;
    }
    FGuid Guid; if(!FGuid::ParseExact(Id,EGuidFormats::Digits,Guid)) {Error=TEXT("Invalid profile ID.");return false;}
    Out.Id=Id; Out.Directory=Directory; Out.Randomized=Flag;
    Obj->TryGetStringField(TEXT("createdUtc"),Out.CreatedUtc);
    if(Flag && Schema==2) {
        FString Phase;
        if(!Obj->TryGetStringField(TEXT("generation"),Phase) || (Phase!=TEXT("pending") && Phase!=TEXT("ready"))) {Error=TEXT("Invalid generation state");return false;}
        Out.Generated=Phase==TEXT("ready");
        if(!Out.Generated) {
            double Seed=0;const TSharedPtr<FJsonObject>* Request;
            if(!Obj->TryGetObjectField(TEXT("request"),Request) ||
               !(*Request)->TryGetNumberField(TEXT("seed"),Seed) || Seed<0 || Seed>2147483647 || Seed!=FMath::FloorToDouble(Seed) ||
               !(*Request)->TryGetStringField(TEXT("skill"),Out.Request.Skill) ||
               !(*Request)->TryGetStringField(TEXT("progression"),Out.Request.Progression) ||
               !SMSeedSettings::ValidBasics(Out.Request)) {
                Error=TEXT("Invalid pending randomizer settings");return false;
            }
            if(!ReadRuleRequest(*Request,Out.Request)){Error=TEXT("Invalid completion or technique settings");return false;}
            Out.Request.Seed=int32(Seed);if(!SMSeedSettings::ValidateDraft(Out.Request,Error))return false;Out.SramPath=Directory/TEXT("pending.sram.dat");return true;
        }
    }
    if(Flag) {
        FString Json,Hash,StoredDigest; double Seed=0;
        if(!Obj->TryGetStringField(TEXT("seedFingerprint"),Hash) ||
           !Obj->TryGetStringField(TEXT("manifestDigest"),StoredDigest) ||
           !Obj->TryGetNumberField(TEXT("seed"),Seed) ||
           !FFileHelper::LoadFileToString(Json, *(Directory/TEXT("seed.json"))) || Digest(Json)!=StoredDigest ||
           !FSMRandomizer::ReadPlan(Json,Out.Plan,Error) || Out.Plan.Fingerprint!=Hash || Out.Plan.Seed!=Seed ||
           !Supported(Out.Plan,Error)) {
            if(Error.IsEmpty()) Error=TEXT("Seed manifest is missing, changed or incompatible.");
            return false;
        }
        if(Schema==2 && Out.Plan.TrackerJson.IsEmpty()){Error=TEXT("Missing tracker data in generated profile");return false;}
        auto Envelope=SMSeedSettings::Parse(Out.Plan.Json);auto Manifest=Envelope->GetObjectField(TEXT("manifest"));
        Out.Request.Seed=Out.Plan.Seed;
        Manifest->TryGetStringField(TEXT("skill"),Out.Request.Skill);Manifest->TryGetStringField(TEXT("progression"),Out.Request.Progression);
        const TSharedPtr<FJsonObject>* Rules=nullptr;
        if(Manifest->TryGetObjectField(TEXT("rules"),Rules) && !ReadRuleRequest(*Rules,Out.Request)){Error=TEXT("Invalid saved seed rules");return false;}
        // Requested choices, including random pools, differ from resolved values.
        const TSharedPtr<FJsonObject>* Requested=nullptr;
        if(Manifest->TryGetObjectField(TEXT("requestedSettings"),Requested) && !SMSeedSettings::Read(*Requested,Out.Request)){Error=TEXT("Invalid saved seed request");return false;}
        if(!Manifest->TryGetStringArrayField(TEXT("patches"),Out.Request.Patches)){Error=TEXT("Invalid saved patch selection");return false;}
        Out.SramPath=Directory/Out.Plan.Fingerprint/TEXT("sram.dat");
    } else Out.SramPath=Directory/TEXT("sram.dat");
    return true;
}
void FSMProfiles::List(TArray<FSMGameProfile>& Out,TArray<FString>& Warnings) {
    Out.Reset(); Warnings.Reset(); TArray<FString> Directories;
    IFileManager::Get().FindFiles(Directories,*(Root()/TEXT("*")),false,true);
    for(const FString& Dir:Directories) {
        FGuid Id; if(!FGuid::ParseExact(Dir,EGuidFormats::Digits,Id)) continue;
        FSMGameProfile Profile; FString Error;
        if(Read(Root()/Dir,Profile,Error,false)) Out.Add(MoveTemp(Profile)); else Warnings.Add(Error);
    }
    Out.Sort([](const FSMGameProfile& A,const FSMGameProfile& B){return A.CreatedUtc>B.CreatedUtc;});
    FString LegacyPath=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SM/sram.dat"));
    FString TemporarySave;
    if(FParse::Value(FCommandLine::Get(),TEXT("SMTemporarySave="),TemporarySave)){
        LegacyPath=FPaths::ConvertRelativePathToFull(TemporarySave);
        FPaths::NormalizeFilename(LegacyPath);
        FPaths::CollapseRelativeDirectories(LegacyPath);
    }
    if(IFileManager::Get().FileExists(*LegacyPath)) {
        FSMGameProfile Legacy;Legacy.Id=TEXT("legacy-vanilla");Legacy.Name=TEXT("Original save");Legacy.Legacy=true;
        Legacy.SramPath=LegacyPath;Legacy.Directory=FPaths::GetPath(LegacyPath);Out.Add(MoveTemp(Legacy));
    }
}
bool FSMProfiles::Create(const FString& Name,const FSMSeedPlan* Seed,FSMGameProfile& Out,FString& Error,const FSMSeedRequest* PendingRequest) {
    Error.Reset(); FSMSeedPlan Verified;
    if(PendingRequest){auto Validated=MakeShared<FJsonObject>();if(!SMSeedSettings::ValidateDraft(*PendingRequest,Error) || !SMSeedSettings::Write(*PendingRequest,Validated)){Error=TEXT("Invalid complete seed draft");return false;}}
    const bool Randomized=Seed || PendingRequest;
    if(Seed && (!FSMRandomizer::ReadPlan(Seed->Json,Verified,Error) || !Supported(Verified,Error))) return false;
    const FString Id=FGuid::NewGuid().ToString(EGuidFormats::Digits), Dir=Root()/Id, Temp=Root()/(Id+TEXT(".pending"));
    if(!IFileManager::Get().MakeDirectory(*Temp,true)) {Error=TEXT("Cannot create save directory.");return false;}
    TSharedRef<FJsonObject> Obj=MakeShared<FJsonObject>();
    Obj->SetNumberField(TEXT("schema"),PendingRequest?2:1);Obj->SetStringField(TEXT("id"),Id);
    Obj->SetStringField(TEXT("name"),Name.TrimStartAndEnd().IsEmpty()?(Randomized?TEXT("Randomized game"):TEXT("Vanilla game")):Name.Left(64));
    Obj->SetStringField(TEXT("mode"),Randomized?TEXT("randomized"):TEXT("vanilla"));Obj->SetBoolField(TEXT("randomized"),Randomized);
    Obj->SetStringField(TEXT("createdUtc"),FDateTime::UtcNow().ToIso8601());
    bool Ok=true;
    if(PendingRequest) {
        Obj->SetStringField(TEXT("generation"),TEXT("pending"));
        TSharedRef<FJsonObject> Request=MakeShared<FJsonObject>();Request->SetNumberField(TEXT("seed"),PendingRequest->Seed);
        Request->SetStringField(TEXT("skill"),PendingRequest->Skill);Request->SetStringField(TEXT("progression"),PendingRequest->Progression);
        WriteRuleRequest(Request, *PendingRequest);
        Obj->SetObjectField(TEXT("request"),Request);
    }
    if(Seed) {
        Obj->SetNumberField(TEXT("seed"),Verified.Seed);Obj->SetStringField(TEXT("seedFingerprint"),Verified.Fingerprint);
        Obj->SetStringField(TEXT("manifestDigest"),Digest(Verified.Json));
        Ok=AtomicWrite(Temp/TEXT("seed.json"),Verified.Json) && IFileManager::Get().MakeDirectory(*(Temp/Verified.Fingerprint),true);
    }
    FString Json;FJsonSerializer::Serialize(Obj,TJsonWriterFactory<>::Create(&Json));
    Ok=Ok && AtomicWrite(Temp/TEXT("profile.json"),Json);
    // Directory rename publishes complete metadata and manifest together.
    if(!Ok || !IFileManager::Get().Move(*Dir,*Temp,false,true)) {
        Error=TEXT("Cannot publish save profile. Existing saves were preserved."); return false;
    }
    return Read(Dir,Out,Error);
}

bool FSMProfiles::CompleteGeneration(const FSMGameProfile& Draft,const FSMSeedPlan& Seed,FSMGameProfile& Out,FString& Error) {
    FSMGameProfile OnDisk;FSMSeedPlan Verified;
    if(!Read(Draft.Directory,OnDisk,Error) || !OnDisk.Randomized ||
       !FSMRandomizer::ReadPlan(Seed.Json,Verified,Error) || !Supported(Verified,Error) || Verified.TrackerJson.IsEmpty()) {
        if(Error.IsEmpty())Error=TEXT("Profile is not awaiting a complete seed and tracker contract");return false;
    }
    // Retry a saved-but-not-activated result without rewriting an existing ready profile.
    if(OnDisk.Generated) {
        if(OnDisk.Plan.Fingerprint!=Verified.Fingerprint){Error=TEXT("This profile already owns another generated game");return false;}
        Out=MoveTemp(OnDisk);return true;
    }
    if(Verified.Seed!=OnDisk.Request.Seed){Error=TEXT("Seed does not match the new game's settings");return false;}
    TSharedPtr<FJsonObject> Obj,Envelope;FString Json;
    FFileHelper::LoadFileToString(Json,*(Draft.Directory/TEXT("profile.json")));
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Obj) || !Obj.IsValid() ||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Verified.Json),Envelope) || !Envelope.IsValid()) {Error=TEXT("Invalid profile metadata");return false;}
    const TSharedPtr<FJsonObject> Manifest=Envelope->GetObjectField(TEXT("manifest"));
    if(Manifest->GetStringField(TEXT("skill"))!=OnDisk.Request.Skill || Manifest->GetStringField(TEXT("progression"))!=OnDisk.Request.Progression) {Error=TEXT("Seed rules differ from the new game request");return false;}
    const TSharedPtr<FJsonObject>* Rules;
    FSMSeedRequest Effective;
    if(Manifest->TryGetObjectField(TEXT("rules"),Rules) && !ReadRuleRequest(*Rules,Effective)){Error=TEXT("Invalid effective rules");return false;}
    if(Effective.NoAdvancedTechs!=OnDisk.Request.NoAdvancedTechs || Effective.RelicHunt!=OnDisk.Request.RelicHunt ||
       (Effective.RelicHunt && (Effective.RelicsPlaced!=OnDisk.Request.RelicsPlaced || Effective.RelicsRequired!=OnDisk.Request.RelicsRequired))){
        Error=TEXT("Completion or technique settings changed during generation");return false;
    }
    const TSharedPtr<FJsonObject>* Requested=nullptr;
    Manifest->TryGetObjectField(TEXT("requestedSettings"),Requested);
    if(!SMSeedSettings::Matches(OnDisk.Request,Requested?*Requested:nullptr)){Error=TEXT("Seed options changed during generation");return false;}
    const FString SramDir=Draft.Directory/Verified.Fingerprint;
    TArray<uint8> EmptySram;EmptySram.SetNumZeroed(8192);
    if(!IFileManager::Get().MakeDirectory(*SramDir,true) ||
       (!IFileManager::Get().FileExists(*(SramDir/TEXT("sram.dat"))) && !FFileHelper::SaveArrayToFile(EmptySram,*(SramDir/TEXT("sram.dat")))) ||
       !AtomicWrite(Draft.Directory/TEXT("seed.json"),Verified.Json)) {Error=TEXT("Cannot save generated game and tracker data");return false;}
    Obj->SetNumberField(TEXT("schema"),2);Obj->SetStringField(TEXT("generation"),TEXT("ready"));
    Obj->SetNumberField(TEXT("seed"),Verified.Seed);Obj->SetStringField(TEXT("seedFingerprint"),Verified.Fingerprint);
    Obj->SetStringField(TEXT("manifestDigest"),Digest(Verified.Json));
    Json.Reset();FJsonSerializer::Serialize(Obj.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
    if(!AtomicWrite(Draft.Directory/TEXT("profile.json"),Json)){Error=TEXT("Cannot publish generated game metadata");return false;}
    return Read(Draft.Directory,Out,Error);
}

namespace {
bool PublishBank(const FSMGameProfile& Bank,FString& Error,const TArray<uint8>* Sram=nullptr){
    TSharedRef<FJsonObject> Obj=MakeShared<FJsonObject>();Obj->SetNumberField(TEXT("schema"),3);
    Obj->SetStringField(TEXT("id"),Bank.Id);Obj->SetStringField(TEXT("name"),Bank.Name);Obj->SetStringField(TEXT("createdUtc"),Bank.CreatedUtc);
    TArray<TSharedPtr<FJsonValue>> Slots;for(const auto& Slot:Bank.Slots)Slots.Add(MakeShared<FJsonValueString>(Slot.Id));
    Obj->SetArrayField(TEXT("slots"),Slots);FString Json;FJsonSerializer::Serialize(Obj,TJsonWriterFactory<>::Create(&Json));
    if(Sram){
        if(Sram->Num()!=8192){Error=TEXT("Invalid SRAM transaction");return false;}
        auto Transaction=MakeShared<FJsonObject>();Transaction->SetStringField(TEXT("sram"),FBase64::Encode(*Sram));Transaction->SetStringField(TEXT("profile"),Json);
        FString Journal;FJsonSerializer::Serialize(Transaction,TJsonWriterFactory<>::Create(&Journal));
        if(!AtomicWrite(Bank.Directory/TEXT("bank.transaction.json"),Journal)){Error=TEXT("Cannot prepare save-bank transaction");return false;}
        return RecoverBank(Bank.Directory,Error);
    }
    if(!AtomicWrite(Bank.Directory/TEXT("profile.json"),Json)){Error=TEXT("Cannot publish A/B/C slot metadata");return false;}return true;
}
bool CreateChild(const FSMGameProfile& Bank,const FString& Name,const FSMSeedRequest* Request,const FSMGameProfile* Copy,FSMGameProfile& Out,FString& Error){
    const FSMSeedPlan* Plan=Copy && Copy->Randomized && Copy->Generated?&Copy->Plan:nullptr;
    if(Copy && Copy->Randomized && !Copy->Generated)Request=&Copy->Request;
    if(!FSMProfiles::Create(Name,Plan,Out,Error,Request))return false;
    const FString Destination=Bank.Directory/TEXT("slots")/Out.Id;
    if(!IFileManager::Get().MakeDirectory(*(Bank.Directory/TEXT("slots")),true) ||
       !IFileManager::Get().Move(*Destination,*Out.Directory,false,true)){Error=TEXT("Cannot create slot manifest");return false;}
    return FSMProfiles::Read(Destination,Out,Error);
}
}
bool FSMProfiles::CreateBank(const FString& Name,const FSMSeedRequest* Request,const FSMGameProfile* Import,FSMGameProfile& Out,FString& Error){
    FSMGameProfile Bank;Bank.Id=FGuid::NewGuid().ToString(EGuidFormats::Digits);Bank.Directory=Root()/Bank.Id;
    Bank.Name=Name.TrimStartAndEnd().IsEmpty()?TEXT("Samus saves"):Name.Left(64);Bank.CreatedUtc=FDateTime::UtcNow().ToIso8601();
    Bank.SramPath=Bank.Directory/TEXT("bank.sram.dat");
    if(!IFileManager::Get().MakeDirectory(*Bank.Directory,true)){Error=TEXT("Cannot create save bank");return false;}
    TArray<uint8> Sram;Sram.SetNumZeroed(8192);
    if(Import && IFileManager::Get().FileExists(*Import->SramPath) &&
       (!FFileHelper::LoadFileToArray(Sram,*Import->SramPath) || Sram.Num()!=8192)){Error=TEXT("Cannot import SRAM");return false;}
    bool Occupied[3]={};bool AnyOccupied=false;const int Offsets[]={0x10,0x66c,0xcc8};
    auto Word=[&](int Offset){return uint16(Sram[Offset]|(uint16(Sram[Offset+1])<<8));};
    for(int I=0;I<3;I++){
        uint16 Sum=0;for(int O=0;O<1628;O+=2)Sum+=Word(Offsets[I]+O);
        Occupied[I]=(Word(2*I)==Sum && Word(8+2*I)==uint16(~Sum)) || (Word(0x1ff0+2*I)==Sum && Word(0x1ff8+2*I)==uint16(~Sum));
        AnyOccupied|=Occupied[I];
    }
    for(int I=0;I<3;I++){
        FSMGameProfile Child;
        // Preserve played historical slots. Empty B/C slots stay configurable;
        // retain an unstarted historical seed/request in A when no slot is saved.
        const FSMGameProfile* Source=Import && (Occupied[I] || (!AnyOccupied && I==0))?Import:nullptr;
        if(!CreateChild(Bank,FString::Printf(TEXT("Samus %c"),'A'+I),I==0?Request:nullptr,Source,Child,Error))return false;
        Bank.Slots.Add(MoveTemp(Child));
    }
    if(Import && IFileManager::Get().FileExists(*(Import->Directory/TEXT("Achievements.ini"))) &&
       IFileManager::Get().Copy(*(Bank.Directory/TEXT("Achievements.ini")),*(Import->Directory/TEXT("Achievements.ini")),false)!=COPY_OK){Error=TEXT("Cannot import existing achievements");return false;}
    if(!FFileHelper::SaveArrayToFile(Sram,*Bank.SramPath) || !PublishBank(Bank,Error))return false;
    return Read(Bank.Directory,Out,Error);
}
bool FSMProfiles::ReplaceSlot(FSMGameProfile& Bank,int Index,const FSMSeedRequest* Request,const FSMGameProfile* Copy,FString& Error,const TArray<uint8>* Sram){
    if(Bank.Slots.Num()!=3 || Index<0 || Index>2){Error=TEXT("Invalid save slot");return false;}
    FSMGameProfile Child;
    if(!CreateChild(Bank,FString::Printf(TEXT("Samus %c"),'A'+Index),Request,Copy,Child,Error))return false;
    FSMGameProfile Updated=Bank;Updated.Slots[Index]=MoveTemp(Child);
    if(!PublishBank(Updated,Error,Sram))return false;Bank=MoveTemp(Updated);return true;
}
bool FSMProfiles::ResolveSeed(FSMGameProfile& Slot,FSMSeedRequest& Out,FString& Error){
    if(!Slot.Randomized || Slot.Generated){Error=TEXT("This slot is not awaiting generation.");return false;}
    FSMGameProfile Stored;if(!Read(Slot.Directory,Stored,Error) || Stored.Generated)return false;
    Out=Stored.Request;
    if(Out.Seed==0){Out.Seed=SMSeedSettings::RandomSeed();if(!SaveRequest(Stored,Out,Error))return false;}
    Slot.Request=Out;return true;
}
bool FSMProfiles::SaveRequest(const FSMGameProfile& Slot,const FSMSeedRequest& Request,FString& Error){
    auto Validated=MakeShared<FJsonObject>();
    if(!SMSeedSettings::ValidateDraft(Request,Error) || !SMSeedSettings::Write(Request,Validated)){Error=TEXT("Invalid complete seed draft");return false;}
    FSMGameProfile Checked;if(!Read(Slot.Directory,Checked,Error) || !Checked.Randomized || Checked.Generated)return false;
    FString Json;TSharedPtr<FJsonObject> Obj;
    if(!FFileHelper::LoadFileToString(Json,*(Slot.Directory/TEXT("profile.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Obj))return false;
    auto R=MakeShared<FJsonObject>();R->SetNumberField(TEXT("seed"),Request.Seed);R->SetStringField(TEXT("skill"),Request.Skill);R->SetStringField(TEXT("progression"),Request.Progression);
    WriteRuleRequest(R,Request);
    Obj->SetObjectField(TEXT("request"),R);Json.Reset();FJsonSerializer::Serialize(Obj.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
    if(!AtomicWrite(Slot.Directory/TEXT("profile.json"),Json)){Error=TEXT("Cannot save slot settings");return false;}return true;
}

bool FSMProfiles::CompleteBankGeneration(FSMGameProfile& Bank,int Slot,const FSMSeedPlan& Plan,const TArray<uint8>& Sram,FString& Error){
    if(!Bank.Slots.IsValidIndex(Slot))return false;
    FSMGameProfile Current;
    if(!Read(Bank.Directory,Current,Error) || !Current.Slots.IsValidIndex(Slot) || Current.Slots[Slot].Id!=Bank.Slots[Slot].Id){
        Error=TEXT("The target save slot changed during generation");return false;
    }
    // Complete an unreferenced child first. Publication then changes the slot's
    // manifest reference and SRAM together through a replayable write-ahead file.
    FSMGameProfile Pending,Completed;
    if(!CreateChild(Current,Current.Slots[Slot].Name,nullptr,&Current.Slots[Slot],Pending,Error) ||
       !CompleteGeneration(Pending,Plan,Completed,Error))return false;
    FSMGameProfile Updated=Current;Updated.Slots[Slot]=MoveTemp(Completed);
    if(!PublishBank(Updated,Error,&Sram))return false;
    Bank=MoveTemp(Updated);return true;
}

#if !UE_BUILD_SHIPPING
bool SMRunGameplayProfileSelfTest(const FString& IsolatedRoot,FString& Error,int Mode){
    const bool Suits=Mode==1, Movement=Mode==2, Animals=Mode==3;
    if(!IFileManager::Get().FileExists(*(IsolatedRoot/TEXT("ISOLATED_TEST_DIRECTORY")))){Error=TEXT("Isolated marker missing");return false;}
    TestProfileRoot=IsolatedRoot/TEXT("gameplay/profile-tests")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
#define VERIFY_GAMEPLAY(Condition) if(!(Condition)){Error=FString(TEXT(#Condition))+TEXT(": ")+Error;return false;}
    FSMGameProfile Bank,Reload;TArray<uint8> Bytes;
    VERIFY_GAMEPLAY(FSMProfiles::CreateBank(TEXT("Gameplay options"),nullptr,nullptr,Bank,Error));
    VERIFY_GAMEPLAY(FFileHelper::LoadFileToArray(Bytes,*(IsolatedRoot/TEXT("slot-results/bank.sram.dat"))) && Bytes.Num()==8192);
    for(int I=0;I<(Suits?3:2);I++){
        FString Json;FSMSeedPlan Plan;FSMSeedRequest Request;
        VERIFY_GAMEPLAY(FFileHelper::LoadFileToString(Json,*(IsolatedRoot/(Animals?TEXT("animals/public"):Movement?TEXT("movement-pickups/public"):Suits?TEXT("suits/public"):TEXT("gameplay/public"))/FString::Printf(TEXT("seed-%02d.json"),I))));
        VERIFY_GAMEPLAY(FSMRandomizer::ReadPlan(Json,Plan,Error));
        VERIFY_GAMEPLAY(Animals?Plan.AnimalsMode>0:(Plan.NativeRules&(Movement?14336:Suits?1536:448))==uint32(Movement?(I==0?14336:4096):Suits?(I==0?0:I==1?512:1024):(I==0?448:256)));
        auto Envelope=SMSeedSettings::Parse(Json);auto Manifest=Envelope->GetObjectField(TEXT("manifest"));
        Request.Seed=Plan.Seed;Request.Skill=Manifest->GetStringField(TEXT("skill"));Request.Progression=Manifest->GetStringField(TEXT("progression"));Request.NoAdvancedTechs=Plan.NoAdvancedTechs;
        VERIFY_GAMEPLAY(SMSeedSettings::Read(Manifest->GetObjectField(TEXT("requestedSettings")),Request));
        VERIFY_GAMEPLAY(FSMProfiles::ReplaceSlot(Bank,I,&Request,nullptr,Error));
        VERIFY_GAMEPLAY(FSMProfiles::CompleteBankGeneration(Bank,I,Plan,Bytes,Error));
        VERIFY_GAMEPLAY(FSMProfiles::Read(Bank.Directory,Reload,Error));
        VERIFY_GAMEPLAY(Reload.Slots[I].Plan.AnimalsMode==Plan.AnimalsMode && Reload.Slots[I].Plan.AnimalsCatalog==Plan.AnimalsCatalog && Reload.Slots[I].Plan.NativeRules==Plan.NativeRules && (Suits || !Reload.Slots[2].Randomized));
        Manifest->GetObjectField(TEXT("rules"))->GetObjectField(TEXT("options"))->SetStringField(Animals?TEXT("animals"):Movement?TEXT("Infinite_Space_Jump"):Suits?TEXT("gravityBehaviour"):TEXT("nerfedCharge"),Suits?TEXT("unknown"):TEXT("off"));
        FSMSeedPlan Bad;VERIFY_GAMEPLAY(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(Envelope.ToSharedRef()),Bad,Error));Error.Reset();
    }
    if(Suits){
        VERIFY_GAMEPLAY((Reload.Slots[0].Plan.NativeRules&1536)==0 && (Reload.Slots[1].Plan.NativeRules&1536)==512 && (Reload.Slots[2].Plan.NativeRules&1536)==1024);
        // Old Vanilla manifests remain Vanilla without regeneration or SRAM
        // migration when native-suits-v1 was not yet emitted.
        FString LegacyJson;FSMSeedPlan Legacy;
        VERIFY_GAMEPLAY(FFileHelper::LoadFileToString(LegacyJson,*(IsolatedRoot/TEXT("gameplay/public/seed-00.json"))));
        VERIFY_GAMEPLAY(FSMRandomizer::ReadPlan(LegacyJson,Legacy,Error) && (Legacy.NativeRules&1536)==0);
        VERIFY_GAMEPLAY(!Legacy.RequiredNativeBehavior.Contains(TEXT("native-suits-v1")));
    }else if(Animals){
        VERIFY_GAMEPLAY(Reload.Slots[0].Plan.AnimalsMode>0 && Reload.Slots[1].Plan.AnimalsMode>0 && !Reload.Slots[2].Randomized);
    }else if(Movement){
        VERIFY_GAMEPLAY((Reload.Slots[0].Plan.NativeRules&14336)==14336 && (Reload.Slots[1].Plan.NativeRules&14336)==4096 && !Reload.Slots[2].Randomized);
    }else VERIFY_GAMEPLAY((Reload.Slots[0].Plan.NativeRules&448)==448 && (Reload.Slots[1].Plan.NativeRules&448)==256);
    TestProfileRoot.Reset();return true;
#undef VERIFY_GAMEPLAY
}
bool SMRunProfileSelfTest(const FString& IsolatedRoot,FString& Error){
    if(!IFileManager::Get().FileExists(*(IsolatedRoot/TEXT("ISOLATED_TEST_DIRECTORY")))){Error=TEXT("Isolated marker missing");return false;}
    TestProfileRoot=IsolatedRoot/TEXT("profile-tests")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
#define VERIFY_PROFILE(Condition) if(!(Condition)){Error=FString(TEXT(#Condition))+TEXT(": ")+Error;return false;}
    FSMGameProfile Bank,Reload;FSMSeedRequest Request;Request.Seed=14092026;Request.Skill=TEXT("casual");Request.Progression=TEXT("medium");
    VERIFY_PROFILE(FSMProfiles::CreateBank(TEXT("Independent slots"),nullptr,nullptr,Bank,Error));
    VERIFY_PROFILE(Bank.Slots.Num()==3 && !Bank.Slots[0].Randomized);
    VERIFY_PROFILE(FSMProfiles::ReplaceSlot(Bank,1,&Request,nullptr,Error));
    VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Reload,Error));
    VERIFY_PROFILE(Reload.Slots[1].Randomized && !Reload.Slots[1].Generated && !Reload.Slots[0].Randomized && !Reload.Slots[2].Randomized);
    Request.Seed=0;VERIFY_PROFILE(FSMProfiles::SaveRequest(Bank.Slots[1],Request,Error));
    VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Reload,Error) && Reload.Slots[1].Request.Seed==0);
    FSMSeedRequest Resolved,Retry;
    VERIFY_PROFILE(FSMProfiles::ResolveSeed(Bank.Slots[1],Resolved,Error) && Resolved.Seed>0);
    VERIFY_PROFILE(FSMProfiles::ResolveSeed(Bank.Slots[1],Retry,Error) && Retry.Seed==Resolved.Seed);
    VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Reload,Error) && Reload.Slots[1].Request.Seed==Resolved.Seed);
    Request.Seed=14092027;Request.Skill=TEXT("regular");Request.Progression=TEXT("slow");
    VERIFY_PROFILE(FSMProfiles::SaveRequest(Bank.Slots[1],Request,Error));
    VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Bank,Error));
    VERIFY_PROFILE(Bank.Slots[1].Request.Seed==14092027);
    Request.Seed=14092026;Request.Skill=TEXT("casual");Request.Progression=TEXT("medium");
    VERIFY_PROFILE(FSMProfiles::SaveRequest(Bank.Slots[1],Request,Error));
    VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Bank,Error));
    TArray<uint8> Bytes;VERIFY_PROFILE(FFileHelper::LoadFileToArray(Bytes,*(IsolatedRoot/TEXT("slot-results/bank.sram.dat"))) && Bytes.Num()==8192);
    for(int Slot=1;Slot<3;Slot++){
        FString Json;FSMSeedPlan Plan;
        VERIFY_PROFILE(FFileHelper::LoadFileToString(Json,*(IsolatedRoot/TEXT("results")/FString::Printf(TEXT("seed-%d.json"),14092025+Slot))));
        VERIFY_PROFILE(FSMRandomizer::ReadPlan(Json,Plan,Error));
        if(Slot==2){Request.Seed=14092027;Request.Skill=TEXT("regular");Request.Progression=TEXT("slow");VERIFY_PROFILE(FSMProfiles::ReplaceSlot(Bank,2,&Request,nullptr,Error));}
        VERIFY_PROFILE(FSMProfiles::CompleteBankGeneration(Bank,Slot,Plan,Bytes,Error));
    }
    VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Reload,Error));
    VERIFY_PROFILE(!Reload.Slots[0].Randomized && Reload.Slots[1].Plan.Seed==14092026 && Reload.Slots[2].Plan.Seed==14092027);
    VERIFY_PROFILE(!Reload.Slots[1].Plan.TrackerJson.IsEmpty() && Reload.Slots[1].Plan.Fingerprint!=Reload.Slots[2].Plan.Fingerprint);
    const FString BHash=Bank.Slots[1].Plan.Fingerprint;FSMGameProfile Copy=Bank.Slots[1];
    VERIFY_PROFILE(FSMProfiles::ReplaceSlot(Bank,0,nullptr,&Copy,Error,&Bytes));
    VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Reload,Error));
    VERIFY_PROFILE(Reload.Slots[0].Plan.Fingerprint==BHash && Reload.Slots[0].Id!=Reload.Slots[1].Id);
    VERIFY_PROFILE(FSMProfiles::ReplaceSlot(Bank,0,nullptr,nullptr,Error,&Bytes));
    VERIFY_PROFILE(!Bank.Slots[0].Randomized && Bank.Slots[1].Plan.Fingerprint==BHash);
    // Replay a durable transaction interrupted before its files were published.
    FString Metadata;VERIFY_PROFILE(FFileHelper::LoadFileToString(Metadata,*(Bank.Directory/TEXT("profile.json"))));
    auto Journal=MakeShared<FJsonObject>();Journal->SetStringField(TEXT("profile"),Metadata);Journal->SetStringField(TEXT("sram"),FBase64::Encode(Bytes));
    FString Text;FJsonSerializer::Serialize(Journal,TJsonWriterFactory<>::Create(&Text));
    VERIFY_PROFILE(AtomicWrite(Bank.Directory/TEXT("bank.transaction.json"),Text));
    VERIFY_PROFILE(AtomicWrite(Bank.Directory/TEXT("profile.json"),TEXT("interrupted")));
    TArray<FSMGameProfile> Listed;TArray<FString> Warnings;FSMProfiles::List(Listed,Warnings);
    VERIFY_PROFILE(IFileManager::Get().FileExists(*(Bank.Directory/TEXT("bank.transaction.json"))) && Warnings.Num()>0);
    VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Reload,Error));
    TArray<uint8> Restored;VERIFY_PROFILE(FFileHelper::LoadFileToArray(Restored,*Reload.SramPath) && Restored==Bytes);
    VERIFY_PROFILE(Reload.Slots[1].Plan.Fingerprint==BHash && !IFileManager::Get().FileExists(*(Bank.Directory/TEXT("bank.transaction.json"))));
    FSMGameProfile Historical,Imported;const FSMSeedPlan OldPlan=Bank.Slots[1].Plan;
    VERIFY_PROFILE(FSMProfiles::Create(TEXT("Historical seed"),&OldPlan,Historical,Error));
    // A generated but unstarted historical profile keeps its seed in A only.
    VERIFY_PROFILE(FSMProfiles::CreateBank(TEXT("Imported empty seed"),nullptr,&Historical,Imported,Error));
    VERIFY_PROFILE(Imported.Slots[0].Randomized && !Imported.Slots[1].Randomized && !Imported.Slots[2].Randomized);
    VERIFY_PROFILE(FFileHelper::SaveArrayToFile(Bytes,*Historical.SramPath));
    VERIFY_PROFILE(FSMProfiles::CreateBank(TEXT("Imported saves"),nullptr,&Historical,Imported,Error));
    VERIFY_PROFILE(!Imported.Slots[0].Randomized && Imported.Slots[1].Randomized && Imported.Slots[2].Randomized);
    TArray<uint8> Source;VERIFY_PROFILE(FFileHelper::LoadFileToArray(Source,*Historical.SramPath) && Source==Bytes);
    // Relic rule identity survives pending edit, publication and copy. A seed
    // generated with a different quota must never replace this pending slot.
    const FString RelicPath=IsolatedRoot/TEXT("full-options/verified-0.json");
    if(IFileManager::Get().FileExists(*RelicPath)){
        FSMSeedPlan RelicPlan;FString RelicJson;
        VERIFY_PROFILE(FFileHelper::LoadFileToString(RelicJson,*RelicPath));
        VERIFY_PROFILE(FSMRandomizer::ReadPlan(RelicJson,RelicPlan,Error));
        FSMSeedRequest RelicRequest;RelicRequest.Seed=15092026;RelicRequest.NoAdvancedTechs=true;RelicRequest.RelicHunt=true;
        VERIFY_PROFILE(FSMProfiles::ReplaceSlot(Bank,0,&RelicRequest,nullptr,Error));
        VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Reload,Error));
        VERIFY_PROFILE(Reload.Slots[0].Request.RelicHunt && Reload.Slots[0].Request.NoAdvancedTechs && Reload.Slots[0].Request.RelicsRequired==20);
        RelicRequest.RelicsRequired=19;
        VERIFY_PROFILE(FSMProfiles::SaveRequest(Bank.Slots[0],RelicRequest,Error));
        VERIFY_PROFILE(!FSMProfiles::CompleteBankGeneration(Bank,0,RelicPlan,Bytes,Error));Error.Reset();
        RelicRequest.RelicsRequired=20;VERIFY_PROFILE(FSMProfiles::SaveRequest(Bank.Slots[0],RelicRequest,Error));
        VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Bank,Error));
        VERIFY_PROFILE(FSMProfiles::CompleteBankGeneration(Bank,0,RelicPlan,Bytes,Error));
        VERIFY_PROFILE(Bank.Slots[0].Plan.RelicsRequired==20 && Bank.Slots[0].Plan.NoAdvancedTechs);
        FSMGameProfile RelicCopy=Bank.Slots[0];
        VERIFY_PROFILE(FSMProfiles::ReplaceSlot(Bank,2,nullptr,&RelicCopy,Error,&Bytes));
        VERIFY_PROFILE(FSMProfiles::Read(Bank.Directory,Reload,Error));
        VERIFY_PROFILE(Reload.Slots[2].Plan.RelicsRequired==20 && Reload.Slots[2].Plan.Fingerprint==RelicPlan.Fingerprint && Reload.Slots[1].Plan.Fingerprint==BHash);
        UE_LOG(LogTemp,Display,TEXT("SM_RELIC_PROFILE_TEST PASS quota=20 independent=1 mismatch_rejected=1"));
    }
    // Complete draft round-trip: custom JSON remains value-owned across slots.
    FSMSeedRequest Full;Full.Seed=15092050;Full.Skill=TEXT("expert");Full.Progression=TEXT("VARIAble");
    Full.OptionsJson=TEXT(R"({"missileQty":2.5,"majorsSplit":"Major","progressionSpeed":"random","progressionSpeedMultiSelect":["slow","fast"],"objective":["kill all G4"],"refill_before_save":"on"})");
    Full.TechniquesJson=TEXT(R"({"Mockball":[false,5],"WallJump":[true,1]})");
    Full.SkillSettingsJson=TEXT(R"({"Ice":"No thanks","Kraid":"Quick Kill"})");
    FSMGameProfile DraftBank;VERIFY_PROFILE(FSMProfiles::CreateBank(TEXT("Complete drafts"),&Full,nullptr,DraftBank,Error));
    VERIFY_PROFILE(FSMProfiles::Read(DraftBank.Directory,Reload,Error));
    auto Expected=MakeShared<FJsonObject>();VERIFY_PROFILE(SMSeedSettings::Write(Full,Expected));
    VERIFY_PROFILE(SMSeedSettings::Matches(Reload.Slots[0].Request,Expected));
    FConfigFile Ini;const FString IniPath=TestProfileRoot/TEXT("editor-settings.ini");
    const FString Pretty=SMSeedSettings::Encode(SMSeedSettings::Parse(Full.OptionsJson).ToSharedRef());
    Ini.SetString(TEXT("Randomizer"),TEXT("Options"),*Pretty);VERIFY_PROFILE(Ini.Write(IniPath));
    FConfigFile IniReload;IniReload.Read(IniPath);FString IniOptions;
    VERIFY_PROFILE(IniReload.GetString(TEXT("Randomizer"),TEXT("Options"),IniOptions));
    VERIFY_PROFILE(IniOptions==Pretty && SMSeedSettings::Parse(IniOptions).IsValid());

    VERIFY_PROFILE(Reload.Slots[0].Request.Skill==TEXT("expert") && Reload.Slots[0].Request.Progression==TEXT("VARIAble"));
    FSMGameProfile DraftCopy=Reload.Slots[0];VERIFY_PROFILE(FSMProfiles::ReplaceSlot(DraftBank,1,nullptr,&DraftCopy,Error));
    Full.OptionsJson=TEXT(R"({"missileQty":8.5})");VERIFY_PROFILE(FSMProfiles::SaveRequest(DraftBank.Slots[0],Full,Error));
    VERIFY_PROFILE(FSMProfiles::Read(DraftBank.Directory,Reload,Error));
    VERIFY_PROFILE(SMSeedSettings::Matches(Reload.Slots[1].Request,Expected) && !SMSeedSettings::Matches(Reload.Slots[0].Request,Expected) && !Reload.Slots[2].Randomized);
    Full.TechniquesJson=TEXT(R"({"Mockball":true})");VERIFY_PROFILE(!FSMProfiles::SaveRequest(DraftBank.Slots[0],Full,Error));Error.Reset();
    FString FullJson;const FString FullPath=IsolatedRoot/TEXT("menu-options/seed-0.json");
    if(FFileHelper::LoadFileToString(FullJson,*FullPath)){
        FSMSeedPlan FullPlan;VERIFY_PROFILE(FSMRandomizer::ReadPlan(FullJson,FullPlan,Error));
        auto M=SMSeedSettings::Parse(FullJson)->GetObjectField(TEXT("manifest"));
        FSMSeedRequest Complete;Complete.Seed=FullPlan.Seed;Complete.Skill=M->GetStringField(TEXT("skill"));Complete.Progression=M->GetStringField(TEXT("progression"));
        VERIFY_PROFILE(SMSeedSettings::Read(M->GetObjectField(TEXT("requestedSettings")),Complete));
        VERIFY_PROFILE(FSMProfiles::ReplaceSlot(DraftBank,2,&Complete,nullptr,Error));
        auto Modified=SMSeedSettings::Parse(Complete.OptionsJson);Modified->SetNumberField(TEXT("missileQty"),8.9);FSMSeedRequest Stale=Complete;Stale.OptionsJson=SMSeedSettings::Encode(Modified.ToSharedRef());
        VERIFY_PROFILE(FSMProfiles::SaveRequest(DraftBank.Slots[2],Stale,Error));
        VERIFY_PROFILE(!FSMProfiles::CompleteBankGeneration(DraftBank,2,FullPlan,Bytes,Error));Error.Reset();
        VERIFY_PROFILE(FSMProfiles::SaveRequest(DraftBank.Slots[2],Complete,Error));
        VERIFY_PROFILE(FSMProfiles::CompleteBankGeneration(DraftBank,2,FullPlan,Bytes,Error));
        VERIFY_PROFILE(SMSeedSettings::Matches(DraftBank.Slots[2].Request,M->GetObjectField(TEXT("requestedSettings"))));
        VERIFY_PROFILE(DraftBank.Slots[2].Plan.RefillBeforeSave);
        VERIFY_PROFILE(!FSMProfiles::SaveRequest(DraftBank.Slots[2],Stale,Error));Error.Reset();
        UE_LOG(LogTemp,Display,TEXT("SM_FULL_SETTINGS_PUBLICATION PASS requested=1 stale_rejected=1 ready_immutable=1 refill=1"));
    }
    for(const auto& Case:TArray<TPair<int,int>>{{4,28},{5,3}}){
        FString Fixture;FSMSeedPlan FlagsPlan;
        const FString Path=IsolatedRoot/TEXT("menu-options")/FString::Printf(TEXT("seed-%d.json"),Case.Key);
        if(FFileHelper::LoadFileToString(Fixture,*Path)){
            VERIFY_PROFILE(FSMRandomizer::ReadPlan(Fixture,FlagsPlan,Error));
            VERIFY_PROFILE(FlagsPlan.NativeRules==uint32(Case.Value));
        }
    }
    for(int I=0;I<5;I++){
        FString Fixture;FSMSeedPlan Plan;
        const FString Path=IsolatedRoot/TEXT("elevators-hud")/FString::Printf(TEXT("seed-%d.json"),I);
        if(FFileHelper::LoadFileToString(Fixture,*Path)){
            VERIFY_PROFILE(FSMRandomizer::ReadPlan(Fixture,Plan,Error));
            VERIFY_PROFILE(Plan.NativeRules==32 && Plan.HudCounts.Num()==100);
            auto Envelope=SMSeedSettings::Parse(Fixture);auto Manifest=Envelope->GetObjectField(TEXT("manifest"));
            auto First=Manifest->GetArrayField(TEXT("placements"))[0]->AsObject();
            First->RemoveField(TEXT("hudCounted"));
            FSMSeedPlan BadCounts;VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(Envelope.ToSharedRef()),BadCounts,Error));Error.Reset();
        }
    }
    int StartFixtures=0;
    for(int I=0;I<66;I++){
        FString Fixture;FSMSeedPlan Plan;
        const FString Path=IsolatedRoot/(I<13?TEXT("start-seeds"):I<17?TEXT("tweak-seeds"):I<21?TEXT("indicator-seeds"):I<25?TEXT("boss-seeds"):I<29?TEXT("door-color-seeds"):I<35?TEXT("area-seeds"):I<38?TEXT("objective-seeds"):I<45?TEXT("objective-logic-seeds"):I<54?TEXT("objective-public-seeds"):I<57?TEXT("scavenger/public"):I<60?TEXT("fast-tourian/public"):I<63?TEXT("minimizer/public"):TEXT("escape/integration"))/FString::Printf(TEXT("seed-%02d.json"),I<13?I:I<17?I-13:I<21?I-17:I<25?I-21:I<29?I-25:I<35?I-29:I<38?I-35:I<45?I-38:I<54?I-45:I<57?I-54:I<60?I-57:I<63?I-60:I-63);
        if(FFileHelper::LoadFileToString(Fixture,*Path)){
            VERIFY_PROFILE(FSMRandomizer::ReadPlan(Fixture,Plan,Error));++StartFixtures;
            auto Envelope=SMSeedSettings::Parse(Fixture);auto Manifest=Envelope->GetObjectField(TEXT("manifest"));
            auto World=Manifest->GetObjectField(TEXT("nativeContext"))->GetObjectField(TEXT("world"));
            VERIFY_PROFILE(Plan.StartSpawn==int32(World->GetNumberField(TEXT("startSpawn"))) && Plan.WorldCatalog.Len()==64);
            FSMSeedRequest StartRequest;StartRequest.NoAdvancedTechs=Plan.NoAdvancedTechs;StartRequest.Seed=Plan.Seed;StartRequest.Skill=Manifest->GetStringField(TEXT("skill"));StartRequest.Progression=Manifest->GetStringField(TEXT("progression"));
            VERIFY_PROFILE(SMSeedSettings::Read(Manifest->GetObjectField(TEXT("requestedSettings")),StartRequest));
            FSMGameProfile Temporary;VERIFY_PROFILE(FSMProfiles::CreateBank(TEXT("Start contract"),&StartRequest,nullptr,Temporary,Error));
            VERIFY_PROFILE(FSMProfiles::CompleteBankGeneration(Temporary,0,Plan,Bytes,Error));
            FSMGameProfile Reloaded;VERIFY_PROFILE(FSMProfiles::Read(Temporary.Directory,Reloaded,Error));
            VERIFY_PROFILE(Reloaded.Slots[0].Plan.StartSpawn==Plan.StartSpawn && Reloaded.Slots[0].Plan.WorldPatches==Plan.WorldPatches && Reloaded.Slots[0].Plan.WorldCatalog==Plan.WorldCatalog);
            VERIFY_PROFILE(Reloaded.Slots[0].Plan.DoorIndicators.Num()==Plan.DoorIndicators.Num());
            VERIFY_PROFILE(Reloaded.Slots[0].Plan.ObjectivesCatalog==Plan.ObjectivesCatalog && FMemory::Memcmp(&Reloaded.Slots[0].Plan.Objectives,&Plan.Objectives,sizeof(SmObjectivePlan))==0);
            VERIFY_PROFILE(Reloaded.Slots[0].Plan.EscapeCatalog==Plan.EscapeCatalog && FMemory::Memcmp(&Reloaded.Slots[0].Plan.EscapeClock,&Plan.EscapeClock,sizeof(SmEscapeClockPlan))==0 && FMemory::Memcmp(&Reloaded.Slots[0].Plan.EscapeRouting,&Plan.EscapeRouting,sizeof(SmEscapeRoutingPlan))==0);
            VERIFY_PROFILE(Reloaded.Slots[0].Plan.MinimizerCatalog==Plan.MinimizerCatalog && FMemory::Memcmp(&Reloaded.Slots[0].Plan.Minimizer,&Plan.Minimizer,sizeof(SmMinimizerPlan))==0);
            VERIFY_PROFILE(Reloaded.Slots[0].Plan.ScavengerCatalog==Plan.ScavengerCatalog && FMemory::Memcmp(&Reloaded.Slots[0].Plan.Scavenger,&Plan.Scavenger,sizeof(SmScavengerPlan))==0);
            if(Plan.EscapeClock.version){
                auto BadEnvelope=SMSeedSettings::Parse(Fixture);auto Topology=BadEnvelope->GetObjectField(TEXT("manifest"))->GetObjectField(TEXT("nativeContext"))->GetObjectField(TEXT("topology"));
                Topology->GetObjectField(TEXT("nativeEscape"))->GetObjectField(TEXT("clock"))->SetNumberField(TEXT("timer"),0x1a00);
                FSMSeedPlan Wrong;VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(BadEnvelope.ToSharedRef()),Wrong,Error));Error.Reset();
                BadEnvelope=SMSeedSettings::Parse(Fixture);Topology=BadEnvelope->GetObjectField(TEXT("manifest"))->GetObjectField(TEXT("tracker"))->GetObjectField(TEXT("topology"));
                Topology->RemoveField(TEXT("nativeEscape"));
                VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(BadEnvelope.ToSharedRef()),Wrong,Error));Error.Reset();
            }
            if(Plan.Scavenger.count){
                auto BadEnvelope=SMSeedSettings::Parse(Fixture);
                auto Hunt=BadEnvelope->GetObjectField(TEXT("manifest"))->GetObjectField(TEXT("nativeContext"))->GetObjectField(TEXT("scavenger"));
                auto Words=Hunt->GetArrayField(TEXT("words"));Words[0]=MakeShared<FJsonValueNumber>(65535);Hunt->SetArrayField(TEXT("words"),Words);
                FSMSeedPlan Bad;VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(BadEnvelope.ToSharedRef()),Bad,Error));Error.Reset();
            }
            if(Plan.RequiredNativeBehavior.Contains(TEXT("native-objectives-v1"))){
                auto EffectiveGoals=Manifest->GetObjectField(TEXT("nativeContext"))->GetObjectField(TEXT("objectives"));
                VERIFY_PROFILE(Plan.Objectives.count==EffectiveGoals->GetArrayField(TEXT("goals")).Num() && Plan.Objectives.required==EffectiveGoals->GetIntegerField(TEXT("required")) && Plan.ObjectivesCatalog.Len()==64);
                if(I<38)VERIFY_PROFILE(Plan.Objectives.count==4 && Plan.Objectives.required==4);
                for(int Variant=0;Variant<6;Variant++){
                    auto BadEnvelope=SMSeedSettings::Parse(Fixture);auto BadManifest=BadEnvelope->GetObjectField(TEXT("manifest"));
                    auto Context=BadManifest->GetObjectField(TEXT("nativeContext"));auto Goals=Context->GetObjectField(TEXT("objectives"));
                    if(Variant==0)Goals->SetNumberField(TEXT("required"),0);
                    if(Variant==1)Goals->SetStringField(TEXT("catalogSha256"),TEXT("bad"));
                    if(Variant==2){auto Names=Goals->GetArrayField(TEXT("names"));Names[0]=MakeShared<FJsonValueString>(TEXT("wrong goal"));Goals->SetArrayField(TEXT("names"),Names);}
                    if(Variant==3){auto Counted=Goals->GetArrayField(TEXT("areaCounted"));Counted[0]=MakeShared<FJsonValueNumber>(Plan.Objectives.area_counted[0]?0:1);Goals->SetArrayField(TEXT("areaCounted"),Counted);}
                    if(Variant==4)Context->RemoveField(TEXT("objectives"));
                    if(Variant==5)BadManifest->GetObjectField(TEXT("tracker"))->GetObjectField(TEXT("settings"))->RemoveField(TEXT("nativeObjectives"));
                    FSMSeedPlan Bad;VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(BadEnvelope.ToSharedRef()),Bad,Error));Error.Reset();
                }
                VERIFY_PROFILE(FSMProfiles::ReplaceSlot(Temporary,1,nullptr,&Reloaded.Slots[0],Error));
                VERIFY_PROFILE(FSMProfiles::Read(Temporary.Directory,Reloaded,Error));
                VERIFY_PROFILE(FMemory::Memcmp(&Reloaded.Slots[1].Plan.Objectives,&Plan.Objectives,sizeof(SmObjectivePlan))==0);
                VERIFY_PROFILE(FMemory::Memcmp(&Reloaded.Slots[1].Plan.Scavenger,&Plan.Scavenger,sizeof(SmScavengerPlan))==0);
                VERIFY_PROFILE(FSMProfiles::ReplaceSlot(Temporary,0,nullptr,nullptr,Error));
                VERIFY_PROFILE(FSMProfiles::Read(Temporary.Directory,Reloaded,Error));
                VERIFY_PROFILE(!Reloaded.Slots[0].Plan.Minimizer.version && FMemory::Memcmp(&Reloaded.Slots[1].Plan.Minimizer,&Plan.Minimizer,sizeof(SmMinimizerPlan))==0);
                VERIFY_PROFILE(!Reloaded.Slots[0].Plan.Scavenger.count);
                VERIFY_PROFILE(!Reloaded.Slots[0].Plan.Objectives.count && Reloaded.Slots[1].Plan.Objectives.count==Plan.Objectives.count);
                // Preserve the original fixture's slot for subsequent world assertions.
                VERIFY_PROFILE(FSMProfiles::ReplaceSlot(Temporary,0,nullptr,&Reloaded.Slots[1],Error));
                VERIFY_PROFILE(FSMProfiles::Read(Temporary.Directory,Reloaded,Error));
            }

            VERIFY_PROFILE(Reloaded.Slots[0].Plan.AreaDestinations==Plan.AreaDestinations && Reloaded.Slots[0].Plan.AreasCatalog==Plan.AreasCatalog && Reloaded.Slots[0].Plan.InitialDoors==Plan.InitialDoors);
            if(Plan.RequiredNativeBehavior.Contains(TEXT("native-initial-doors-v1"))){
                VERIFY_PROFILE(Plan.InitialDoors.Num()>=9);
                auto BadEnvelope=SMSeedSettings::Parse(Fixture);auto Initial=BadEnvelope->GetObjectField(TEXT("manifest"))->GetObjectField(TEXT("nativeContext"))->GetObjectField(TEXT("world"))->GetObjectField(TEXT("initialDoors"));
                auto Doors=Initial->GetArrayField(TEXT("opened"));Doors[0]=MakeShared<FJsonValueNumber>(255);Initial->SetArrayField(TEXT("opened"),Doors);
                FSMSeedPlan Wrong;VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(BadEnvelope.ToSharedRef()),Wrong,Error));Error.Reset();
            }
            if(Plan.RequiredNativeBehavior.Contains(TEXT("native-area-connections-v1"))){
                VERIFY_PROFILE(Plan.AreaDestinations.Num()==32 && Plan.AreasCatalog.Len()==64);
                auto BadEnvelope=SMSeedSettings::Parse(Fixture);auto Native=BadEnvelope->GetObjectField(TEXT("manifest"))->GetObjectField(TEXT("nativeContext"))->GetObjectField(TEXT("topology"))->GetObjectField(TEXT("nativeAreas"));
                auto Targets=Native->GetArrayField(TEXT("destinations"));Targets[0]=MakeShared<FJsonValueNumber>(32);Native->SetArrayField(TEXT("destinations"),Targets);
                FSMSeedPlan Wrong;VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(BadEnvelope.ToSharedRef()),Wrong,Error));Error.Reset();
                BadEnvelope=SMSeedSettings::Parse(Fixture);auto Topology=BadEnvelope->GetObjectField(TEXT("manifest"))->GetObjectField(TEXT("tracker"))->GetObjectField(TEXT("topology"));
                Topology->SetStringField(TEXT("mode"),TEXT("vanilla"));
                VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(BadEnvelope.ToSharedRef()),Wrong,Error));Error.Reset();
            }
            VERIFY_PROFILE(Reloaded.Slots[0].Plan.DoorColors==Plan.DoorColors && Reloaded.Slots[0].Plan.DoorColorsCatalog==Plan.DoorColorsCatalog);
            if(Plan.RequiredNativeBehavior.Contains(TEXT("native-door-colors-v1"))){
                VERIFY_PROFILE(Plan.DoorColors.Num()==SM_DOOR_COLOR_COUNT && Plan.DoorColorsCatalog.Len()==64);
                auto BadEnvelope=SMSeedSettings::Parse(Fixture);auto Contract=BadEnvelope->GetObjectField(TEXT("manifest"))->GetObjectField(TEXT("nativeContext"))->GetObjectField(TEXT("doorColors"));
                auto Colors=Contract->GetArrayField(TEXT("colors"));Colors[0]=MakeShared<FJsonValueNumber>(99);Contract->SetArrayField(TEXT("colors"),Colors);
                FSMSeedPlan Wrong;VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(BadEnvelope.ToSharedRef()),Wrong,Error));Error.Reset();
                BadEnvelope=SMSeedSettings::Parse(Fixture);Contract=BadEnvelope->GetObjectField(TEXT("manifest"))->GetObjectField(TEXT("tracker"))->GetObjectField(TEXT("topology"))->GetObjectField(TEXT("doorColors"));
                Contract->SetStringField(TEXT("catalogSha256"),TEXT("bad"));
                VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(BadEnvelope.ToSharedRef()),Wrong,Error));Error.Reset();
            }
            VERIFY_PROFILE(Reloaded.Slots[0].Plan.ConnectionsCatalog==Plan.ConnectionsCatalog && Reloaded.Slots[0].Plan.BossDestinations==Plan.BossDestinations);
            if(Plan.RequiredNativeBehavior.Contains(TEXT("native-boss-connections-v1"))){
                VERIFY_PROFILE(Plan.BossDestinations.Num()==8 && Plan.ConnectionsCatalog.Len()==64);
                auto InvalidEnvelope=SMSeedSettings::Parse(Fixture);auto Topology=InvalidEnvelope->GetObjectField(TEXT("manifest"))->GetObjectField(TEXT("nativeContext"))->GetObjectField(TEXT("topology"));
                auto Native=Topology->GetObjectField(TEXT("native"));auto Destinations=Native->GetArrayField(TEXT("destinations"));
                Destinations[0]=MakeShared<FJsonValueNumber>(0);Native->SetArrayField(TEXT("destinations"),Destinations);
                FSMSeedPlan Wrong;VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(InvalidEnvelope.ToSharedRef()),Wrong,Error));Error.Reset();
                InvalidEnvelope=SMSeedSettings::Parse(Fixture);Topology=InvalidEnvelope->GetObjectField(TEXT("manifest"))->GetObjectField(TEXT("tracker"))->GetObjectField(TEXT("topology"));
                Topology->SetStringField(TEXT("mode"),TEXT("vanilla"));
                VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(InvalidEnvelope.ToSharedRef()),Wrong,Error));Error.Reset();
            }
            for(int J=0;J<Plan.DoorIndicators.Num();J++)VERIFY_PROFILE(Reloaded.Slots[0].Plan.DoorIndicators[J].location==Plan.DoorIndicators[J].location && Reloaded.Slots[0].Plan.DoorIndicators[J].plm==Plan.DoorIndicators[J].plm);
            if(Plan.RequiredNativeBehavior.Contains(TEXT("native-door-indicators-v1"))){
                World->RemoveField(TEXT("indicators"));FSMSeedPlan Missing;
                VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(Envelope.ToSharedRef()),Missing,Error));Error.Reset();
            }
            World->RemoveField(TEXT("catalogSha256"));FSMSeedPlan Invalid;
            VERIFY_PROFILE(!FSMRandomizer::ReadPlan(SMSeedSettings::Encode(Envelope.ToSharedRef()),Invalid,Error));Error.Reset();
        }
    }
    if(StartFixtures)UE_LOG(LogTemp,Display,TEXT("SM_START_PROFILE PASS fixtures=%d publication=1 reload=1 malformed_rejected=1"),StartFixtures);
    VERIFY_PROFILE(OldPlan.NativeRules==0 && OldPlan.HudCounts.IsEmpty() && !OldPlan.Objectives.count && OldPlan.ObjectivesCatalog.IsEmpty());
    UE_LOG(LogTemp,Display,TEXT("SM_ELEVATORS_HUD_PROFILE PASS native_flags=1 counted_contract=1 malformed_rejected=1 historical=1"));
    UE_LOG(LogTemp,Display,TEXT("SM_FULL_SETTINGS_PROFILE PASS full_roundtrip=1 independent_copy=1 malformed_rejected=1"));
    // Invalid seeds and settings cannot replace a generated or pending slot.
    FString Before;FFileHelper::LoadFileToString(Before,*(Bank.Directory/TEXT("profile.json")));
    FSMSeedPlan Bad=Bank.Slots[1].Plan;Bad.Json=TEXT("{}");
    VERIFY_PROFILE(!FSMProfiles::CompleteBankGeneration(Bank,2,Bad,Bytes,Error));
    FString After;FFileHelper::LoadFileToString(After,*(Bank.Directory/TEXT("profile.json")));VERIFY_PROFILE(Before==After);
    Error.Reset();
    VERIFY_PROFILE(FFileHelper::SaveStringToFile(TEXT("Unreal profile checks passed: independent modes/seeds, requests, generation publication, copy, clear, recovery, invalid plan rejection.\n"),*(IsolatedRoot/TEXT("profile-verification.txt"))));
    TestProfileRoot.Reset();return true;
#undef VERIFY_PROFILE
}
#endif
