#include "SMRom.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

namespace {
bool ReadValidated(const FString& Path, TArray<uint8>& Bytes, FString& Error) {
    Error.Reset();
    const int64 Length = IFileManager::Get().FileSize(*Path);
    if (Length < 0) { Error = TEXT("ROM not found or unreadable. Select your Super Metroid ROM below."); return false; }
    if (Length != SMRom::Size) {
        Error = FString::Printf(TEXT("Wrong ROM size: %lld bytes. Expected 3,145,728 bytes (unheadered Japan/USA ROM)."), Length);
        return false;
    }
    if (!FFileHelper::LoadFileToArray(Bytes, *Path) || Bytes.Num() != SMRom::Size) {
        Error = TEXT("Could not read the complete ROM. Select the file again."); return false;
    }
    const uint32 Actual = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
    if (Actual != SMRom::Crc) {
        Error = FString::Printf(TEXT("Incompatible ROM. CRC32: %08X; expected D63ED5F8. Use the unmodified Japan/USA ROM."), Actual);
        return false;
    }
    FSHAHash Hash;
    FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash.Hash);
    if (Hash.ToString().ToLower() != SMRom::Sha1) {
        Error = TEXT("ROM integrity check failed (SHA-1 mismatch). Use the unmodified Japan/USA ROM."); return false;
    }
    return true;
}
}
FString SMRom::DataRoot(const FString& Root) {
#if PLATFORM_LINUX
    // An AppImage is mounted read-only. AppRun supplies a persistent, writable
    // per-user directory; development and ordinary folder installs keep theirs.
    const FString UserData = FPlatformMisc::GetEnvironmentVariable(TEXT("SM_USER_DATA"));
    if (!UserData.IsEmpty() && !FPaths::IsRelative(UserData))
        return UserData;
#endif
    return Root;
}
FString SMRom::LocalPath(const FString& Root) {
    return DataRoot(Root) / TEXT("roms") / Filename;
}
bool SMRom::Validate(const FString& Path, FString& Error) {
    TArray<uint8> Bytes;
    return ReadValidated(Path, Bytes, Error);
}
bool SMRom::Import(const FString& Source, const FString& Destination, FString& Error) {
    TArray<uint8> Bytes;
    // Re-read after confirmation: the selected file could have changed since preview.
    if (!ReadValidated(Source, Bytes, Error)) return false;
    FString ExistingError;
    if (Validate(Destination, ExistingError)) return true;
    IPlatformFile& Files = FPlatformFileManager::Get().GetPlatformFile();
    if (!Files.CreateDirectoryTree(*FPaths::GetPath(Destination))) {
        Error = TEXT("Cannot create the local ROM folder. Move the game to a writable folder and retry."); return false;
    }
    const FString Token = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString Temporary = Destination + TEXT(".import-") + Token;
    if (!FFileHelper::SaveArrayToFile(Bytes, *Temporary) || !Validate(Temporary, Error)) {
        Files.DeleteFile(*Temporary);
        Error = TEXT("Could not write and verify the local copy. Check free disk space and folder permissions."); return false;
    }
    // Keep an invalid previous copy, and publish only a completely verified file.
    const FString Backup = Destination + TEXT(".invalid-") + Token;
    const bool HadPrevious = Files.FileExists(*Destination);
    if (HadPrevious && !Files.MoveFile(*Backup, *Destination)) {
        Files.DeleteFile(*Temporary);
        Error = TEXT("Cannot preserve the invalid local ROM. Check folder permissions and retry."); return false;
    }
    if (!Files.MoveFile(*Destination, *Temporary)) {
        if (HadPrevious) Files.MoveFile(*Destination, *Backup);
        Files.DeleteFile(*Temporary);
        Error = TEXT("Could not install the verified ROM. Your selected source file was not changed."); return false;
    }
    // This is also the exact check performed at the next launch.
    return Validate(Destination, Error);
}
