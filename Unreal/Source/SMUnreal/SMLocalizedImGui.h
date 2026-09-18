#pragma once
#include "SMLocalization.h"
#include "imgui.h"
/* Translate presentation arguments only. Internal IDs, values, buffers and
 * serialized settings never pass through the catalog. ImGui stays unmodified. */
namespace SMUI {
using namespace ImGui;
struct FRawText {const char* Value;};
inline FRawText Raw(const char* S){return {S};}
inline const char* Arg(FRawText S){return S.Value;}
inline const char* Arg(const char* S){return SMLocalization::Text(S);}
inline const char* Arg(char* S){return SMLocalization::Text(S);} // TCHAR_TO_UTF8 also returns mutable pointers; user-authored labels use Raw.
template<class T> T Arg(T Value){return Value;}
template<class... T> void Text(const char* F,T... A){if constexpr(sizeof...(A)==0)ImGui::Text("%s",Arg(F));else ImGui::Text(Arg(F),Arg(A)...);}
template<class... T> void TextWrapped(const char* F,T... A){if constexpr(sizeof...(A)==0)ImGui::TextWrapped("%s",Arg(F));else ImGui::TextWrapped(Arg(F),Arg(A)...);}
template<class... T> void TextDisabled(const char* F,T... A){if constexpr(sizeof...(A)==0)ImGui::TextDisabled("%s",Arg(F));else ImGui::TextDisabled(Arg(F),Arg(A)...);}
template<class... T> void TextColored(const ImVec4& C,const char* F,T... A){if constexpr(sizeof...(A)==0)ImGui::TextColored(C,"%s",Arg(F));else ImGui::TextColored(C,Arg(F),Arg(A)...);}
template<class... T> void SetTooltip(const char* F,T... A){if constexpr(sizeof...(A)==0)ImGui::SetTooltip("%s",Arg(F));else ImGui::SetTooltip(Arg(F),Arg(A)...);}
inline void TextUnformatted(const char* S,const char* End=nullptr){if(End)ImGui::TextUnformatted(S,End);else ImGui::TextUnformatted(Arg(S));}
inline bool Button(const char* S,const ImVec2& Size=ImVec2(0,0)){return ImGui::Button(SMLocalization::Label(S),Size);}
inline bool SmallButton(const char* S){return ImGui::SmallButton(SMLocalization::Label(S));}
inline bool Checkbox(const char* S,bool* V){return ImGui::Checkbox(SMLocalization::Label(S),V);}
inline bool Selectable(const char* S,bool V=false,ImGuiSelectableFlags F=0,const ImVec2& Size=ImVec2(0,0)){return ImGui::Selectable(SMLocalization::Label(S),V,F,Size);}
inline bool BeginCombo(const char* S,const char* P,ImGuiComboFlags F=0){return ImGui::BeginCombo(SMLocalization::Label(S),Arg(P),F);}
inline bool Combo(const char* S,int* V,const char* const Items[],int N,int H=-1){TArray<const char*> List;for(int I=0;I<N;I++)List.Add(Arg(Items[I]));return ImGui::Combo(SMLocalization::Label(S),V,List.GetData(),N,H);}
inline bool Combo(const char* S,int* V,const char* Items,int H=-1){TArray<const char*> List;for(const char* P=Items;*P;P+=FCStringAnsi::Strlen(P)+1)List.Add(P);return SMUI::Combo(S,V,List.GetData(),List.Num(),H);}
inline bool TreeNode(const char* S){return ImGui::TreeNode(SMLocalization::Label(S));}
inline bool InputTextWithHint(const char* S,const char* Hint,char* B,size_t N,ImGuiInputTextFlags F=0,ImGuiInputTextCallback C=nullptr,void* U=nullptr){return ImGui::InputTextWithHint(SMLocalization::Label(S),Arg(Hint),B,N,F,C,U);}
inline ImVec2 CalcTextSize(const char* S,const char* E=nullptr,bool Hide=false,float W=-1){return ImGui::CalcTextSize(E?S:Arg(S),E,Hide,W);}
// Popup identity must use the same localized, stable label on both paths.
inline void OpenPopup(const char* S,ImGuiPopupFlags F=0){ImGui::OpenPopup(SMLocalization::Label(S),F);}
inline bool BeginPopupModal(const char* S,bool* O=nullptr,ImGuiWindowFlags F=0){return ImGui::BeginPopupModal(SMLocalization::Label(S),O,F);}
inline bool Begin(const char* S,bool* Open=nullptr,ImGuiWindowFlags F=0){return ImGui::Begin(SMLocalization::Label(S),Open,F);}
inline bool SliderFloat(const char* S,float* V,float Min,float Max,const char* Format="%.3f",ImGuiSliderFlags F=0){return ImGui::SliderFloat(SMLocalization::Label(S),V,Min,Max,Arg(Format),F);}
inline bool SliderInt(const char* S,int* V,int Min,int Max,const char* Format="%d",ImGuiSliderFlags F=0){return ImGui::SliderInt(SMLocalization::Label(S),V,Min,Max,Arg(Format),F);}
inline bool InputText(const char* S,char* B,size_t N,ImGuiInputTextFlags F=0,ImGuiInputTextCallback C=nullptr,void* U=nullptr){return ImGui::InputText(SMLocalization::Label(S),B,N,F,C,U);}
inline bool InputTextMultiline(const char* S,char* B,size_t N,const ImVec2& Size=ImVec2(0,0),ImGuiInputTextFlags F=0,ImGuiInputTextCallback C=nullptr,void* U=nullptr){return ImGui::InputTextMultiline(SMLocalization::Label(S),B,N,Size,F,C,U);}
inline void TableSetupColumn(const char* S,ImGuiTableColumnFlags F=0,float W=0,ImGuiID Id=0){ImGui::TableSetupColumn(SMLocalization::Label(S),F,W,Id);}
inline void ProgressBar(float V,const ImVec2& Size=ImVec2(-1,0),const char* Overlay=nullptr){ImGui::ProgressBar(V,Size,Arg(Overlay));}
}
