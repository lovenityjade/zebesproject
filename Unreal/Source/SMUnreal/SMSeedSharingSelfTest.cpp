#include "SMSeedSettings.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool SMRunSeedSharingSelfTest(const FString& Root,FString& Error){
    if(!IFileManager::Get().FileExists(*(Root/TEXT("ISOLATED_TEST_DIRECTORY")))){Error=TEXT("Isolated test root required");return false;}
#define CHECK(E) if(!(E)){Error=TEXT(#E)+FString(TEXT(": "))+Error;return false;}
    int32 Number=42;
    CHECK(SMSeedSettings::ParseSeedNumber(TEXT(""),Number) && Number==0);
    CHECK(SMSeedSettings::ParseSeedNumber(TEXT(" 2147483647 "),Number) && Number==MAX_int32);
    for(const auto& Bad:{TEXT("0"),TEXT("-1"),TEXT("1.5"),TEXT("123abc"),TEXT("2147483648"),TEXT("9999999999999999999999")}){Number=42;CHECK(!SMSeedSettings::ParseSeedNumber(Bad,Number) && Number==42);}
    TSet<int32> Numbers;for(int I=0;I<64;I++){int32 N=SMSeedSettings::RandomSeed();CHECK(N>0);Numbers.Add(N);}CHECK(Numbers.Num()>60);
    FSMSeedRequest Source;Source.Seed=123;Source.Skill=TEXT("newbie");Source.Progression=TEXT("slow");Source.NoAdvancedTechs=true;Source.RelicHunt=true;Source.RelicsPlaced=25;Source.RelicsRequired=15;Source.RelicEscapeMinutes=7;
    Source.OptionsJson=TEXT("{\"morphPlacement\":\"early\",\"energyQty\":\"medium\"}");
    const auto Catalog=SMSeedSettings::Catalog();const FString Tech=Catalog->GetArrayField(TEXT("techniques"))[0]->AsObject()->GetStringField(TEXT("key"));
    auto Techniques=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> V={MakeShared<FJsonValueBoolean>(false),MakeShared<FJsonValueNumber>(5)};Techniques->SetArrayField(Tech,V);Source.TechniquesJson=SMSeedSettings::Encode(Techniques);
    Source.SkillSettingsJson=SMSeedSettings::Encode(Catalog->GetObjectField(TEXT("skillPresets"))->GetObjectField(TEXT("newbie"))->GetObjectField(TEXT("Settings")).ToSharedRef());
    Source.Patches={TEXT("spin_jump_restart")};
    FString Text;CHECK(SMSeedSettings::ExportString(Source,Text,Error));
    FSMSeedRequest Target;Target.Seed=987;CHECK(SMSeedSettings::ImportString(Text,Target,Error));CHECK(Target.Seed==987 && Target.Skill==Source.Skill && Target.Progression==Source.Progression && Target.Patches==Source.Patches && Target.NoAdvancedTechs && Target.RelicHunt && Target.RelicsRequired==15 && Target.RelicsPlaced==25 && Target.RelicEscapeMinutes==7);
    auto Expected=MakeShared<FJsonObject>();SMSeedSettings::Write(Source,Expected);CHECK(SMSeedSettings::Matches(Target,Expected));
    FString Again;CHECK(SMSeedSettings::ExportString(Target,Again,Error));CHECK(Again==Text); // Seed excluded.
    FString Broken=Text;Broken[Broken.Len()/2]=Broken[Broken.Len()/2]=='A'?'B':'A';
    for(const FString& Bad:{Broken,Text.Left(12),FString(TEXT("ZP2."))+Text.Mid(4),FString(TEXT("ZP1.invalid!")),FString::ChrN(100001,'A')}){
        CHECK(!SMSeedSettings::ImportString(Bad,Target,Error));FString Kept;CHECK(SMSeedSettings::ExportString(Target,Kept,Error) && Kept==Text && Target.Seed==987);
    }
    // Real embedded generation: random blank request, then same number + string reproduces it.
    FSMSeedRequest Blank;Blank.Skill=TEXT("casual");Blank.Progression=TEXT("medium");FSMSeedPlan First,Replay,Second;
    CHECK(SMSeedSettings::ExportString(Blank,Text,Error));CHECK(FSMRandomizer::Generate(Root,Blank,First,Error));CHECK(First.Seed>0 && Blank.Seed==0);
    FSMSeedRequest Imported;Imported.Seed=First.Seed;CHECK(SMSeedSettings::ImportString(Text,Imported,Error));CHECK(FSMRandomizer::Generate(Root,Imported,Replay,Error));CHECK(First.Fingerprint==Replay.Fingerprint);
    CHECK(FSMRandomizer::Generate(Root,Blank,Second,Error));CHECK(Second.Seed>0 && Second.Seed!=First.Seed);
    CHECK(FFileHelper::SaveStringToFile(FString::Printf(TEXT("PASS\nRandom seeds: %d, %d\nReplay fingerprint: %s\nSettings: %s\n"),First.Seed,Second.Seed,*First.Fingerprint,*Text),*(Root/TEXT("seed-sharing-result.txt"))));
#undef CHECK
    return true;
}
