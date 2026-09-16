#include "SMSystemMenu.h"
#include "SMSeedSettings.h"
#include "SMHUD.h"
#include "imgui.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace {
const TArray<FString> Skills={TEXT("casual"),TEXT("regular"),TEXT("veteran"),TEXT("newbie"),TEXT("expert"),TEXT("master"),TEXT("samus"),TEXT("solution")};
const TArray<FString> Speeds={TEXT("slow"),TEXT("medium"),TEXT("fast"),TEXT("slowest"),TEXT("fastest"),TEXT("basic"),TEXT("VARIAble"),TEXT("speedrun"),TEXT("random")};
using JV=TSharedPtr<FJsonValue>;
JV String(const FString& S){return MakeShared<FJsonValueString>(S);}
FString Text(JV V){return V.IsValid() && V->Type==EJson::String?V->AsString():FString();}
TArray<JV> Array(JV V){return V.IsValid() && V->Type==EJson::Array?V->AsArray():TArray<JV>();}
JV Get(const TSharedPtr<FJsonObject>& O,FStringView K,JV Default=nullptr){const auto* V=O->Values.Find(K);return V?*V:Default;}
bool Contains(const TArray<JV>& Values,const FString& S){return Values.ContainsByPredicate([&](const JV& V){return Text(V)==S;});}
bool Choose(const char* Id,FString& Current,const TArray<JV>& Values,bool Objects=false){
    FString Preview=Current;
    if(Objects)for(const auto& V:Values)if(V->AsObject()->GetStringField(TEXT("value"))==Current)Preview=V->AsObject()->GetStringField(TEXT("label"));
    bool Changed=false;
    if(ImGui::BeginCombo(Id,TCHAR_TO_UTF8(*Preview))){
        for(const auto& V:Values){FString Value=Objects?V->AsObject()->GetStringField(TEXT("value")):V->AsString();
            const FString Label=Objects?V->AsObject()->GetStringField(TEXT("label")):Value;
            if(ImGui::Selectable(TCHAR_TO_UTF8(*Label),Value==Current)){Current=Value;Changed=true;}
        }ImGui::EndCombo();
    }return Changed;
}
bool GoalConflict(const TSharedPtr<FJsonObject>& Catalog,const TArray<JV>& Selected,const TSharedPtr<FJsonObject>& Candidate){
    auto Ex=Candidate->GetObjectField(TEXT("constraints"))->GetObjectField(TEXT("exclusion"));
    const FString Name=Candidate->GetStringField(TEXT("name")),Type=Candidate->GetStringField(TEXT("type"));
    FString LimitedType;Ex->TryGetStringField(TEXT("type"),LimitedType);int Count=0;
    for(auto& V:Catalog->GetArrayField(TEXT("objectives"))){auto G=V->AsObject();FString N=G->GetStringField(TEXT("name"));if(!Contains(Selected,N))continue;
        auto Other=G->GetObjectField(TEXT("constraints"))->GetObjectField(TEXT("exclusion"));
        if(Contains(Array(Get(Ex,TEXT("list"))),N) || Contains(Array(Get(Other,TEXT("list"))),Name))return true;
        if(!LimitedType.IsEmpty() && G->GetStringField(TEXT("type"))==LimitedType)Count++;
        FString OtherType;double Limit=0;if(Other->TryGetStringField(TEXT("type"),OtherType) && OtherType==Type && Other->TryGetNumberField(TEXT("limit"),Limit)){
            int Existing=0;for(auto& Q:Catalog->GetArrayField(TEXT("objectives")))if(Contains(Selected,Q->AsObject()->GetStringField(TEXT("name"))) && Q->AsObject()->GetStringField(TEXT("type"))==Type)Existing++;
            if(Existing>=Limit)return true;
        }
    }
    double Limit=0;return !LimitedType.IsEmpty() && Ex->TryGetNumberField(TEXT("limit"),Limit) && Count>Limit;
}
TArray<JV> Names(const TSharedPtr<FJsonObject>& O){TArray<FString> Keys;for(const auto& Pair:O->Values)Keys.Add(FString(*Pair.Key));Keys.Sort();TArray<JV> V;for(auto& K:Keys)V.Add(String(K));return V;}
}
void FSMSystemMenu::LoadSeedRequest(const FSMSeedRequest& R){
    Seed=R.Seed;SeedNumber[0]=0;if(Seed>0)FCStringAnsi::Snprintf(SeedNumber,sizeof(SeedNumber),"%d",Seed);SeedPatches=R.Patches;Skill=Skills.IndexOfByKey(R.Skill);if(Skill<0)Skill=0;
    Progression=Speeds.IndexOfByKey(R.Progression);if(Progression<0)Progression=1;
    NoAdvancedTechs=R.NoAdvancedTechs;RelicHunt=R.RelicHunt;RelicsPlaced=R.RelicsPlaced;RelicsRequired=R.RelicsRequired;RelicEscapeMinutes=R.RelicEscapeMinutes;
    SeedOptions=R.OptionsJson;SeedTechniques=R.TechniquesJson;SeedSkillSettings=R.SkillSettingsJson;
}
bool FSMSystemMenu::DrawSeedCatalog(bool Busy){
    const auto Catalog=SMSeedSettings::Catalog();
    auto Options=SMSeedSettings::Parse(SeedOptions),Techniques=SMSeedSettings::Parse(SeedTechniques),Adjustments=SMSeedSettings::Parse(SeedSkillSettings);
    if(!Catalog || !Options || !Techniques || !Adjustments){ImGui::TextWrapped("Cannot read seed settings. Reload the slot before editing.");return false;}
    bool Changed=false;const bool All=Search[0]!=0;
    ImGui::BeginDisabled(Busy);
    if(RandomPage==0 || All){
        if(Row("Settings preset","Apply a bundled VARIA configuration. This replaces gameplay options and applies the preset's base skill when supplied. Custom skill overrides remain separate.")){
            auto Presets=Catalog->GetObjectField(TEXT("settingsPresets"));FString Name=TEXT("Choose a preset...");
            if(Choose("##preset",Name,Names(Presets))){Options=SMSeedSettings::Parse(SMSeedSettings::Encode(Presets->GetObjectField(Name).ToSharedRef()));FString PresetSkill;if(Catalog->GetObjectField(TEXT("settingsPresetSkills"))->TryGetStringField(Name,PresetSkill))Skill=Skills.IndexOfByKey(PresetSkill);Changed=true;Status=TEXT("Settings preset loaded: ")+Name+TEXT(". Native compatibility is checked before generation.");}EndRow();
        }
        if(Row("Settings string","Copy your complete randomizer configuration to share it. Import replaces the draft settings but keeps your seed number. It does not change an already generated save.")){
            if(SettingsString.IsEmpty())SettingsString.SetNumZeroed(100001);
            ImGui::InputTextMultiline("##settings-string",SettingsString.GetData(),SettingsString.Num(),ImVec2(-1,72));
            if(ImGui::Button("Copy settings")){
                FString Text,Error;
                if(SMSeedSettings::ExportString(NextRequest(),Text,Error)){
                    FCStringAnsi::Strncpy(SettingsString.GetData(),TCHAR_TO_UTF8(*Text),SettingsString.Num());ImGui::SetClipboardText(SettingsString.GetData());Status=TEXT("Settings string copied. Share the seed number separately to reproduce the same world.");
                }else Status=Error;
            }
            ImGui::SameLine();if(ImGui::Button("Paste")){
                const char* Text=ImGui::GetClipboardText();
                if(Text && FCStringAnsi::Strlen(Text)<SettingsString.Num())FCStringAnsi::Strncpy(SettingsString.GetData(),Text,SettingsString.Num());
                else Status=TEXT("Clipboard text is too large.");
            }
            ImGui::SameLine();if(ImGui::Button("Import settings")){
                auto R=NextRequest();FString Error;
                if(SMSeedSettings::ImportString(UTF8_TO_TCHAR(SettingsString.GetData()),R,Error)){
                    LoadSeedRequest(R);Options=SMSeedSettings::Parse(SeedOptions);Techniques=SMSeedSettings::Parse(SeedTechniques);Adjustments=SMSeedSettings::Parse(SeedSkillSettings);Changed=true;Status=TEXT("Settings imported. Your seed number was kept.");
                }else Status=Error;
            }EndRow();
        }
        if(Row("Import / export configuration","Local JSON includes every seed option, technique, tolerance and tablet quota. Imports replace this draft; generated saves remain immutable.")){
            if(!PresetPath[0])FCStringAnsi::Strncpy(PresetPath,TCHAR_TO_UTF8(*(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SM/Presets/SeedSettings.json")))),UE_ARRAY_COUNT(PresetPath));
            ImGui::InputText("##path",PresetPath,UE_ARRAY_COUNT(PresetPath));
            if(ImGui::Button("Export")){
                auto R=NextRequest();auto O=MakeShared<FJsonObject>();SMSeedSettings::Write(R,O);
                TArray<TSharedPtr<FJsonValue>> Patches;for(const auto& P:R.Patches)Patches.Add(MakeShared<FJsonValueString>(P));O->SetArrayField(TEXT("patches"),Patches);
                O->SetNumberField(TEXT("seed"),R.Seed);O->SetStringField(TEXT("skill"),R.Skill);O->SetStringField(TEXT("progression"),R.Progression);O->SetBoolField(TEXT("noAdvancedTechs"),R.NoAdvancedTechs);
                auto Hunt=MakeShared<FJsonObject>();Hunt->SetBoolField(TEXT("enabled"),R.RelicHunt);Hunt->SetNumberField(TEXT("placed"),R.RelicsPlaced);Hunt->SetNumberField(TEXT("required"),R.RelicsRequired);Hunt->SetNumberField(TEXT("escapeMinutes"),R.RelicEscapeMinutes);O->SetObjectField(TEXT("relicHunt"),Hunt);
                FString Path=UTF8_TO_TCHAR(PresetPath);IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
                Status=FFileHelper::SaveStringToFile(SMSeedSettings::Encode(O),*Path)?TEXT("Configuration exported."):TEXT("Cannot write configuration file.");
            }
            ImGui::SameLine();if(ImGui::Button("Import")){
                FString Data;auto R=NextRequest();bool Ok=IFileManager::Get().FileSize(UTF8_TO_TCHAR(PresetPath))<=1048576 && FFileHelper::LoadFileToString(Data,UTF8_TO_TCHAR(PresetPath));
                auto O=Ok?SMSeedSettings::Parse(Data):nullptr;Ok=O.IsValid();
                if(Ok && O->HasField(TEXT("options"))){
                    double Number=0;const TSharedPtr<FJsonObject>* Hunt=nullptr;
                    Ok=SMSeedSettings::Read(O,R) && O->TryGetNumberField(TEXT("seed"),Number) && Number>=0 && Number<=MAX_int32 && Number==FMath::FloorToDouble(Number);
                    if(Ok)R.Seed=int32(Number);
                    R.Patches.Reset();if(O->HasField(TEXT("patches")))Ok=Ok && O->TryGetStringArrayField(TEXT("patches"),R.Patches);
                    Ok=Ok && O->TryGetStringField(TEXT("skill"),R.Skill) && O->TryGetStringField(TEXT("progression"),R.Progression) && SMSeedSettings::ValidBasics(R);
                    if(O->HasField(TEXT("noAdvancedTechs")))Ok=Ok && O->TryGetBoolField(TEXT("noAdvancedTechs"),R.NoAdvancedTechs);
                    if(O->HasField(TEXT("relicHunt"))){double P=0,Q=0;Ok=Ok && O->TryGetObjectField(TEXT("relicHunt"),Hunt) && (*Hunt)->TryGetBoolField(TEXT("enabled"),R.RelicHunt) && (*Hunt)->TryGetNumberField(TEXT("placed"),P) && (*Hunt)->TryGetNumberField(TEXT("required"),Q) && Q>=1 && Q<=P && P<=60 && P==FMath::FloorToDouble(P) && Q==FMath::FloorToDouble(Q);if(Ok){R.RelicsPlaced=int32(P);R.RelicsRequired=int32(Q);Ok=SMSeedSettings::ReadEscapeMinutes(*Hunt,R.RelicEscapeMinutes);}}
                }else if(Ok && O->HasField(TEXT("Knows"))){const TSharedPtr<FJsonObject>* K=nullptr;Ok=O->TryGetObjectField(TEXT("Knows"),K);if(Ok)R.TechniquesJson=SMSeedSettings::Encode(K->ToSharedRef());if(O->HasField(TEXT("Settings"))){Ok=Ok && O->TryGetObjectField(TEXT("Settings"),K);if(Ok)R.SkillSettingsJson=SMSeedSettings::Encode(K->ToSharedRef());}}
                else if(Ok){
                    auto Imported=MakeShared<FJsonObject>();TSet<FString> Allowed;
                    for(auto& E:Catalog->GetArrayField(TEXT("fields"))){auto F=E->AsObject();const FString Key=F->GetStringField(TEXT("key"));Allowed.Add(Key);FString K;if(F->TryGetStringField(TEXT("multiple"),K))Allowed.Add(K);if(F->TryGetStringField(TEXT("custom"),K))Allowed.Add(K);
                        auto V=Get(O,Key);if(V && F->GetStringField(TEXT("type"))==TEXT("number") && V->Type==EJson::String && V->AsString()!=TEXT("off") && V->AsString()!=TEXT("random")){
                            const FString N=V->AsString();if(!N.IsNumeric()){Ok=false;break;}O->SetNumberField(Key,FCString::Atod(*N));
                        }
                    }
                    const TSet<FString> Presentation={TEXT("No_Music"),TEXT("random_music"),TEXT("complexity"),TEXT("preset"),TEXT("randoPreset")};
                    for(auto& P:O->Values){FString K(*P.Key);if(Allowed.Contains(K))Imported->SetField(K,P.Value);else if(!Presentation.Contains(K))Ok=false;}
                    FString ObjRandom;if(Imported->TryGetStringField(TEXT("objectiveRandom"),ObjRandom)){if(ObjRandom==TEXT("on"))Imported->SetStringField(TEXT("objectiveRandom"),TEXT("true"));if(ObjRandom==TEXT("off"))Imported->SetStringField(TEXT("objectiveRandom"),TEXT("false"));}
                    FString PresetSkill;if(O->TryGetStringField(TEXT("preset"),PresetSkill)){if(!Skills.Contains(PresetSkill))Ok=false;else R.Skill=PresetSkill;}
                    R.OptionsJson=SMSeedSettings::Encode(Imported);
                }
                if(Ok)Ok=SMSeedSettings::ValidateDraft(R,Status);
                if(Ok){LoadSeedRequest(R);Options=SMSeedSettings::Parse(SeedOptions);Techniques=SMSeedSettings::Parse(SeedTechniques);Adjustments=SMSeedSettings::Parse(SeedSkillSettings);Changed=true;Status=TEXT("Configuration imported. Generate Game will validate all rules before publishing a seed.");}
                else Status=TEXT("Invalid configuration. The previous draft was kept.");
            }EndRow();
        }
    }
    if(RandomPage==1 || All){
        if(Row("Skill preset","All eight VARIA skill presets. Overrides below are preserved when changing the base preset.")){FString S=Skills[Skill];TArray<JV> Values;for(auto& K:Skills)Values.Add(String(K));if(Choose("##skill",S,Values)){Skill=Skills.IndexOfByKey(S);Changed=true;}EndRow();}
        if(Row("Custom skill preset","Load all techniques and tolerances from a bundled skill preset. No Advanced Techs takes precedence when enabled.")){
            auto Presets=Catalog->GetObjectField(TEXT("skillPresets"));FString Name=TEXT("Choose a preset...");if(Choose("##custom-skill",Name,Names(Presets))){auto P=Presets->GetObjectField(Name);Techniques=SMSeedSettings::Parse(SMSeedSettings::Encode(P->GetObjectField(TEXT("Knows")).ToSharedRef()));Adjustments=SMSeedSettings::Parse(SMSeedSettings::Encode(P->GetObjectField(TEXT("Settings")).ToSharedRef()));Changed=true;}EndRow();
        }
        if(Row("No Advanced Techs","Disable advanced traversal and heat runs in both generation and tracker logic. Basic wall jumps and full-length shinesparks remain enabled.")){Changed|=ImGui::Checkbox("##noadvanced",&NoAdvancedTechs);EndRow();}
        if(Row("Reset skill overrides","Return individual techniques and tolerances to the selected base preset.")){if(ImGui::Button("Use base preset")){Techniques=MakeShared<FJsonObject>();Adjustments=MakeShared<FJsonObject>();Changed=true;}EndRow();}
    }
    if(!Busy && Text(Get(Options,TEXT("tourian"),String(TEXT("Vanilla"))))==TEXT("Disabled")){
        for(const auto& Dependency:{TPair<FString,FString>(TEXT("escapeRando"),TEXT("on")),TPair<FString,FString>(TEXT("removeEscapeEnemies"),TEXT("off"))}){
            if(Text(Get(Options,*Dependency.Key))!=Dependency.Value){Options->SetStringField(Dependency.Key,Dependency.Value);Changed=true;}
        }
    }
    for(const auto& Entry:Catalog->GetArrayField(TEXT("fields"))){
        const auto F=Entry->AsObject();if(!All && RandomPage!=F->GetIntegerField(TEXT("page")))continue;
        const FString Key=F->GetStringField(TEXT("key")),Type=F->GetStringField(TEXT("type"));
        // Removed from product scope by the user; keep neutral legacy fields readable.
        if(Key==TEXT("logic") || Key==TEXT("raceMode"))continue;
        JV Default=Get(F,TEXT("default"));if(Key==TEXT("progressionSpeed"))Default=String(Speeds[Progression]);
        JV Current=Get(Options,Key,Default);FString Label=F->GetStringField(TEXT("label")),Detail=F->GetStringField(TEXT("description"));
        if(!Row(TCHAR_TO_UTF8(*Label),TCHAR_TO_UTF8(*Detail)))continue;
        if((Type==TEXT("goals") || Type==TEXT("patches")) && RowUsesTable){ImGui::EndTable();RowUsesTable=false;ImGui::SetNextItemWidth(-1);}
        bool Edit=false;
        auto ValueOf=[&](const TCHAR* K,const TCHAR* D){return Text(Get(Options,K,String(D)));};
        bool Dependent=(Key.StartsWith(TEXT("scav")) && ValueOf(TEXT("majorsSplit"),TEXT("Full"))!=TEXT("Scavenger")) ||
            ((Key==TEXT("escapeRando") || Key==TEXT("removeEscapeEnemies")) && ValueOf(TEXT("tourian"),TEXT("Vanilla"))==TEXT("Disabled")) ||
            (Key==TEXT("areaLayout") && ValueOf(TEXT("areaRandomization"),TEXT("off"))==TEXT("off")) ||
            (Key==TEXT("allowGreyDoors") && ValueOf(TEXT("doorsColorsRando"),TEXT("off"))==TEXT("off")) ||
            (Key==TEXT("minimizerQty") && ValueOf(TEXT("minimizer"),TEXT("off"))==TEXT("off")) ||
            (Key==TEXT("removeEscapeEnemies") && ValueOf(TEXT("escapeRando"),TEXT("off"))==TEXT("off")) ||
            ((Key==TEXT("nbObjective") || Key==TEXT("objectiveMultiSelect") || Key==TEXT("hiddenObjectives") || Key==TEXT("distributeObjectives")) && ValueOf(TEXT("objectiveRandom"),TEXT("false"))!=TEXT("true")) ||
            (Key==TEXT("objective") && ValueOf(TEXT("objectiveRandom"),TEXT("false"))==TEXT("true"));
        ImGui::BeginDisabled(Dependent);
        if(Type==TEXT("choice")){
            FString Value=Text(Current);if(Choose("##value",Value,F->GetArrayField(TEXT("choices")),true)){Options->SetStringField(Key,Value);Edit=true;}
            FString Multi;if(F->TryGetStringField(TEXT("multiple"),Multi) && Value==TEXT("random")){
                auto Allowed=Array(Get(Options,Multi));if(!Options->HasField(Multi))for(auto& V:F->GetArrayField(TEXT("choices")))if(V->AsObject()->GetStringField(TEXT("value"))!=TEXT("random"))Allowed.Add(String(V->AsObject()->GetStringField(TEXT("value"))));
                if(ImGui::TreeNode("Allowed choices")){for(auto& V:F->GetArrayField(TEXT("choices"))){auto C=V->AsObject();FString S=C->GetStringField(TEXT("value"));if(S==TEXT("random"))continue;bool Enabled=Contains(Allowed,S);if(ImGui::Checkbox(TCHAR_TO_UTF8(*C->GetStringField(TEXT("label"))),&Enabled)){if(Enabled)Allowed.Add(String(S));else Allowed.RemoveAll([&](const JV& X){return Text(X)==S;});Options->SetArrayField(Multi,Allowed);Edit=true;}}ImGui::TreePop();}
                if(Allowed.IsEmpty())ImGui::TextWrapped("Select at least one allowed choice.");
            }
        }else if(Type==TEXT("number")){
            FString Mode=Text(Current);if(Mode!=TEXT("random") && Mode!=TEXT("off"))Mode=TEXT("fixed");
            auto ModeChoice=[](const TCHAR* Value,const TCHAR* Label)->JV{auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("value"),Value);O->SetStringField(TEXT("label"),Label);return MakeShared<FJsonValueObject>(O);};
            TArray<JV> Modes={ModeChoice(TEXT("fixed"),TEXT("Fixed")),ModeChoice(TEXT("random"),TEXT("Random"))};if(Key==TEXT("nbObjectivesRequired"))Modes.Add(ModeChoice(TEXT("off"),TEXT("All")));
            if(Choose("##mode",Mode,Modes,true)){Current=Mode==TEXT("fixed")?MakeShared<FJsonValueNumber>(F->GetNumberField(TEXT("minimum"))):String(Mode);Options->SetField(Key,Current);Edit=true;}
            if(Mode==TEXT("fixed")){double N=Current->Type==EJson::Number?Current->AsNumber():FCString::Atod(*Text(Current));float V=float(N);if(ImGui::SliderFloat("##number",&V,F->GetNumberField(TEXT("minimum")),F->GetNumberField(TEXT("maximum")),F->GetBoolField(TEXT("decimal"))?"%.1f":"%.0f")){Options->SetNumberField(Key,F->GetBoolField(TEXT("decimal"))?FMath::RoundToDouble(V*10)/10:FMath::RoundToDouble(V));Edit=true;}}
        }else if(Type==TEXT("patches")){
            FString Custom=F->GetStringField(TEXT("custom"));auto Selected=Array(Get(Options,Custom));FString Mode=Text(Current)==TEXT("off")?TEXT("None"):Text(Current)==TEXT("random")?TEXT("Random"):Selected.IsEmpty()?TEXT("All"):TEXT("Custom");
            TArray<JV> Modes={String(TEXT("None")),String(TEXT("All")),String(TEXT("Custom")),String(TEXT("Random"))};
            if(Choose("##patch-mode",Mode,Modes)){Options->SetStringField(Key,Mode==TEXT("None")?TEXT("off"):Mode==TEXT("Random")?TEXT("random"):TEXT("on"));Selected.Empty();if(Mode==TEXT("Custom"))for(auto& P:F->GetArrayField(TEXT("patches")))Selected.Add(String(P->AsObject()->GetStringField(TEXT("id"))));Options->SetArrayField(Custom,Selected);Edit=true;}
            if(Mode==TEXT("Custom"))for(auto& P:F->GetArrayField(TEXT("patches"))){auto Patch=P->AsObject();FString Id=Patch->GetStringField(TEXT("id"));bool On=Contains(Selected,Id);if(ImGui::Checkbox(TCHAR_TO_UTF8(*Patch->GetStringField(TEXT("title"))),&On)){if(On)Selected.Add(String(Id));else Selected.RemoveAll([&](const JV& V){return Text(V)==Id;});Options->SetArrayField(Custom,Selected);if(Selected.IsEmpty())Options->SetStringField(Key,TEXT("off"));Edit=true;}if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",TCHAR_TO_UTF8(*Patch->GetStringField(TEXT("description"))));}
        }else if(Type==TEXT("goals")){
            auto Selected=Array(Current);ImGui::Text("%d selected",Selected.Num());
            if(MenuPreviewFrame && MenuPreviewName.Contains(TEXT("-goals")))ImGui::SetNextItemOpen(true,ImGuiCond_Once);
            if(ImGui::TreeNode("Choose goals")){
                TArray<FString> Groups;for(auto& G:Catalog->GetArrayField(TEXT("objectives"))){FString Group=G->AsObject()->GetStringField(TEXT("category"));Groups.AddUnique(Group);}Groups.Sort();
                for(const auto& Group:Groups){if(MenuPreviewFrame && MenuPreviewName.Contains(TEXT("-goals")))ImGui::SetNextItemOpen(true,ImGuiCond_Once);if(ImGui::TreeNode(TCHAR_TO_UTF8(*Group))){for(auto& G:Catalog->GetArrayField(TEXT("objectives"))){auto Goal=G->AsObject();FString Name=Goal->GetStringField(TEXT("name"));if(Goal->GetStringField(TEXT("category"))!=Group || !Goal->GetBoolField(TEXT("available")) || (Name==TEXT("nothing") && Key!=TEXT("objective")) || Name==TEXT("finish scavenger hunt"))continue;
                    bool On=Contains(Selected,Name);bool Conflict=Key==TEXT("objective") && !On && GoalConflict(Catalog,Selected,Goal);ImGui::BeginDisabled(!On && Key==TEXT("objective") && (Selected.Num()>=18 || Conflict));
                    ImGui::PushID(TCHAR_TO_UTF8(*Name));if(ImGui::Checkbox("##goal",&On)){if(On)Selected.Add(String(Name));else Selected.RemoveAll([&](const JV& V){return Text(V)==Name;});Options->SetArrayField(Key,Selected);Edit=true;}ImGui::SameLine();ImGui::TextWrapped("%s",TCHAR_TO_UTF8(*Name));ImGui::EndDisabled();if(Conflict && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))ImGui::SetTooltip("Conflicts with a selected objective.");ImGui::PopID();
                }ImGui::TreePop();}}
                ImGui::TreePop();
            }if(Selected.IsEmpty())ImGui::TextWrapped("Select at least one goal.");
        }
        if(Options->HasField(Key) && ImGui::SmallButton("Reset to default")){Options->RemoveField(Key);Edit=true;}
        ImGui::EndDisabled();Changed|=Edit;EndRow();
    }
    const auto Preset=Catalog->GetObjectField(TEXT("skillPresets"))->GetObjectField(Skills[Skill]);
    if(RandomPage==7 || All){
        auto Groups=Catalog->GetObjectField(TEXT("techniqueCategories"));auto GroupNames=Catalog->GetArrayField(TEXT("techniqueOrder"));TechniquePage=FMath::Clamp(TechniquePage,0,GroupNames.Num()-1);
        if(!All){FString Selected=GroupNames[TechniquePage]->AsString();ImGui::SetNextItemWidth(-1);if(Choose("##tech-category",Selected,GroupNames))for(int I=0;I<GroupNames.Num();I++)if(Text(GroupNames[I])==Selected)TechniquePage=I;}
        if(NoAdvancedTechs)ImGui::TextWrapped("No Advanced Techs overrides these values during generation. Your custom choices are preserved for when you turn it off.");
        for(auto& T:Catalog->GetArrayField(TEXT("techniques"))){auto Tech=T->AsObject();FString Key=Tech->GetStringField(TEXT("key"));
            bool InGroup=All;for(auto& Sub:Groups->GetArrayField(GroupNames[TechniquePage]->AsString()))if(Contains(Sub->AsObject()->GetArrayField(TEXT("knows")),Key))InGroup=true;if(!InGroup)continue;
            auto D=Tech->GetObjectField(TEXT("description"));if(!Row(TCHAR_TO_UTF8(*D->GetStringField(TEXT("display"))),TCHAR_TO_UTF8(*D->GetStringField(TEXT("title")))))continue;
            const TMap<FString,int> Diff={{TEXT("easy"),1},{TEXT("medium"),5},{TEXT("hard"),10},{TEXT("harder"),25},{TEXT("hardcore"),50},{TEXT("mania"),100}};
            TArray<JV> Defaults={MakeShared<FJsonValueBoolean>(Tech->GetBoolField(TEXT("enabled"))),MakeShared<FJsonValueNumber>(Diff.FindRef(Tech->GetStringField(TEXT("difficulty"))))};
            auto V=Array(Get(Techniques,Key,Get(Preset->GetObjectField(TEXT("Knows")),Key,MakeShared<FJsonValueArray>(Defaults))));
            bool Enabled=V.Num()==2 && V[0]->Type==EJson::Boolean && V[0]->AsBool();int Difficulty=V.Num()==2 && V[1]->Type==EJson::Number?V[1]->AsNumber():0;
            if(NoAdvancedTechs){Enabled=Key==TEXT("WallJump") || Key==TEXT("ShineSpark") || Key==TEXT("MidAirMorph") || Key==TEXT("CrouchJump") || Key==TEXT("UnequipItem");Difficulty=Enabled?1:0;}
            ImGui::BeginDisabled(NoAdvancedTechs);bool Edit=ImGui::Checkbox("Allowed in logic",&Enabled);
            const int Values[]={0,1,5,10,25,50,100,200,400,800};int Index=0;for(int I=0;I<10;I++)if(Difficulty>=Values[I])Index=I;
            if(ImGui::Combo("##difficulty",&Index,"Unrated\0Easy\0Medium\0Hard\0Very hard\0Hardcore\0Mania\0God\0Samus\0Impossible\0")){Difficulty=Values[Index];Edit=true;}
            if(Edit){Techniques->SetArrayField(Key,{MakeShared<FJsonValueBoolean>(Enabled),MakeShared<FJsonValueNumber>(Difficulty)});Changed=true;}
            if(Techniques->HasField(Key) && ImGui::SmallButton("Use preset value")){Techniques->RemoveField(Key);Changed=true;}ImGui::EndDisabled();EndRow();
        }
    }
    if(RandomPage==8 || All)for(const auto& Group:Catalog->GetObjectField(TEXT("skillSettings"))->Values)for(const auto& Entry:Group.Value->AsObject()->Values){
        FString Detail=Group.Key==TEXT("bossesDifficultyPresets")?TEXT("Combat tolerance used to estimate required equipment and difficulty."):Group.Key==TEXT("hellRunPresets")?TEXT("Heat-run tolerance used by the solver and live tracker."):TEXT("Tolerance for damage and difficult traversal in this room.");
        const TMap<FString,FString> Labels={{TEXT("MotherBrain"),TEXT("Mother Brain")},{TEXT("Ice"),TEXT("Ice Beam heat run")},{TEXT("MainUpperNorfair"),TEXT("Upper Norfair heat runs")},{TEXT("LowerNorfair"),TEXT("Lower Norfair heat runs")},{TEXT("X-Ray"),TEXT("X-Ray room")}};
        FString Label=Labels.Contains(FString(*Entry.Key))?Labels.FindRef(FString(*Entry.Key)):FString(*Entry.Key);
        if(Row(TCHAR_TO_UTF8(*Label),TCHAR_TO_UTF8(*Detail))){FString Value=Text(Get(Adjustments,*Entry.Key,Get(Preset->GetObjectField(TEXT("Settings")),*Entry.Key,String(TEXT("Default")))));if(NoAdvancedTechs && Group.Key==TEXT("hellRunPresets"))Value=Entry.Key==TEXT("LowerNorfair")?TEXT("Default"):TEXT("No thanks");ImGui::BeginDisabled(NoAdvancedTechs && Group.Key==TEXT("hellRunPresets"));if(Choose("##tolerance",Value,Entry.Value->AsArray())){Adjustments->SetStringField(Entry.Key,Value);Changed=true;}if(Adjustments->HasField(Entry.Key) && ImGui::SmallButton("Use preset value")){Adjustments->RemoveField(Entry.Key);Changed=true;}ImGui::EndDisabled();EndRow();}
    }
    ImGui::EndDisabled();
    if(Changed){SeedOptions=SMSeedSettings::Encode(Options.ToSharedRef());SeedTechniques=SMSeedSettings::Encode(Techniques.ToSharedRef());SeedSkillSettings=SMSeedSettings::Encode(Adjustments.ToSharedRef());}
    return Changed;
}
