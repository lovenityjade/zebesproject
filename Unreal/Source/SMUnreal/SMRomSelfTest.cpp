#include "SMRom.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

bool SMRunRomSelfTest(const FString& Root, FString& Error) {
    if (!IFileManager::Get().FileExists(*(Root / TEXT("ISOLATED_TEST_DIRECTORY")))) {
        Error = TEXT("Requires isolated test root"); return false;
    }
    const FString Test = Root / TEXT("rom-import-tests") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
    IFileManager::Get().MakeDirectory(*Test, true);
    struct FCleanup { FString Path; ~FCleanup() { IFileManager::Get().DeleteDirectory(*Path, false, true); } } Cleanup{Test};
    // Explicit fixture paths: a packaged launch may redirect LocalPath to the
    // real player's data directory. A self-test must never modify that copy.
    const FString Good = Root / TEXT("roms") / SMRom::Filename;
    const FString Candidate = Test / TEXT("renamed.smc"), Local = Test / TEXT("roms") / SMRom::Filename;
    TArray<uint8> Bytes;
    FString Reason;
#define CHECK(Expr) if (!(Expr)) { Error = TEXT(#Expr) + FString(TEXT(": ")) + Reason; return false; }
#if PLATFORM_LINUX
    {
        struct FRestoreEnvironment {
            FString Previous = FPlatformMisc::GetEnvironmentVariable(TEXT("SM_USER_DATA"));
            ~FRestoreEnvironment() { FPlatformMisc::SetEnvironmentVar(TEXT("SM_USER_DATA"), *Previous); }
        } RestoreEnvironment;
        FPlatformMisc::SetEnvironmentVar(TEXT("SM_USER_DATA"), *Test);
        CHECK(SMRom::LocalPath(Root) == Local);
        FPlatformMisc::SetEnvironmentVar(TEXT("SM_USER_DATA"), TEXT(""));
        CHECK(SMRom::LocalPath(Root) == Good);
        FPlatformMisc::SetEnvironmentVar(TEXT("SM_USER_DATA"), TEXT("relative-is-not-a-data-directory"));
        CHECK(SMRom::LocalPath(Root) == Good);
    }
#endif
    CHECK(SMRom::Validate(Good, Reason));
    CHECK(!SMRom::Validate(Local, Reason));
    CHECK(FFileHelper::LoadFileToArray(Bytes, *Good));
    CHECK(FFileHelper::SaveArrayToFile(Bytes, *Candidate));
    CHECK(SMRom::Validate(Candidate, Reason));
    // A file changed after selection must not be imported.
    Bytes[128] ^= 1;
    CHECK(FFileHelper::SaveArrayToFile(Bytes, *Candidate));
    CHECK(!SMRom::Import(Candidate, Local, Reason));
    CHECK(Reason.Contains(TEXT("CRC32")));
    CHECK(!IFileManager::Get().FileExists(*Local));
    Bytes[128] ^= 1;
    CHECK(FFileHelper::SaveArrayToFile(Bytes, *Candidate));
    CHECK(SMRom::Import(Candidate, Local, Reason));
    CHECK(SMRom::Validate(Local, Reason));
    CHECK(SMRom::Validate(Candidate, Reason));
    CHECK(SMRom::Import(Local, Local, Reason));
    // Check again after installation: corruption cannot pass on a later boot.
    Bytes[1024] ^= 1;
    CHECK(FFileHelper::SaveArrayToFile(Bytes, *Local));
    CHECK(!SMRom::Validate(Local, Reason));
    CHECK(SMRom::Import(Candidate, Local, Reason));
    CHECK(SMRom::Validate(Local, Reason));
    TArray<FString> Backups;
    IFileManager::Get().FindFiles(Backups, *(Local + TEXT(".invalid-*")), true, false);
    CHECK(Backups.Num() == 1);
    TArray<uint8> Backup;
    CHECK(FFileHelper::LoadFileToArray(Backup, *(FPaths::GetPath(Local) / Backups[0])));
    CHECK(Backup == Bytes);
    // Headered/truncated/empty inputs are rejected without touching valid output.
    Bytes.SetNum(SMRom::Size + 512);
    CHECK(FFileHelper::SaveArrayToFile(Bytes, *Candidate));
    CHECK(!SMRom::Import(Candidate, Local, Reason));
    Bytes.SetNum(100);
    CHECK(FFileHelper::SaveArrayToFile(Bytes, *Candidate));
    CHECK(!SMRom::Validate(Candidate, Reason));
    Bytes.Reset();
    CHECK(FFileHelper::SaveArrayToFile(Bytes, *Candidate));
    CHECK(!SMRom::Validate(Candidate, Reason));
    CHECK(SMRom::Validate(Local, Reason));
    // A destination whose parent is a regular file fails cleanly.
    CHECK(!SMRom::Import(Good, Candidate / TEXT("rom.sfc"), Reason));
#undef CHECK
    return true;
}
