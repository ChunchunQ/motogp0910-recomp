#pragma once
#include <imgui.h>
namespace motogp::overlay {
inline const ImVec4 ink{0.055f,0.064f,0.08f,1}, card{0.09f,0.10f,0.125f,1};
inline const ImVec4 orange{1.f,106.f/255.f,19.f/255.f,1}, white{0.94f,0.95f,0.97f,1}, muted{0.58f,0.63f,0.71f,1}; // #ff6a13
inline ImVec4 AccentTint(float amount,float alpha=1.f) {
  return {card.x+(orange.x-card.x)*amount,card.y+(orange.y-card.y)*amount,
          card.z+(orange.z-card.z)*amount,alpha};
}
inline const char* Tr(bool tr,const char* en,const char* turkish) { return tr?turkish:en; }
// SDK text-entry and confirmation dialogs use the global style rather than
// the scoped F1/F7 theme. Give them the same palette and readable spacing,
// while retaining the SDK's native text editing and completion lifecycle.
inline void ConfigureDialogs(ImGuiStyle& style) {
  style.FontSizeBase=17.f;
  style.WindowPadding=ImVec2(24,20);
  style.WindowRounding=14.f;
  style.PopupRounding=14.f;
  style.FrameRounding=6.f;
  style.ItemSpacing=ImVec2(12,10);
  style.FramePadding=ImVec2(12,8);
  style.WindowMinSize=ImVec2(420,32);
  style.WindowTitleAlign=ImVec2(.5f,.5f);
  style.WindowBorderSize=1.f;
  style.PopupBorderSize=1.f;
  auto* colors=style.Colors;
  colors[ImGuiCol_WindowBg]=ink;colors[ImGuiCol_PopupBg]=ink;colors[ImGuiCol_ChildBg]=card;
  colors[ImGuiCol_Text]=white;colors[ImGuiCol_TextDisabled]=muted;
  colors[ImGuiCol_Border]=ImVec4(.19f,.21f,.26f,1);
  colors[ImGuiCol_TitleBg]=ink;colors[ImGuiCol_TitleBgActive]=card;colors[ImGuiCol_TitleBgCollapsed]=ink;
  colors[ImGuiCol_FrameBg]=card;colors[ImGuiCol_FrameBgHovered]=AccentTint(.10f);
  colors[ImGuiCol_FrameBgActive]=AccentTint(.18f);
  colors[ImGuiCol_Button]=card;colors[ImGuiCol_ButtonHovered]=AccentTint(.24f);
  colors[ImGuiCol_ButtonActive]=AccentTint(.36f);
  colors[ImGuiCol_Header]=card;colors[ImGuiCol_HeaderHovered]=colors[ImGuiCol_ButtonHovered];
  colors[ImGuiCol_HeaderActive]=colors[ImGuiCol_ButtonActive];
  colors[ImGuiCol_CheckMark]=orange;colors[ImGuiCol_SliderGrab]=orange;colors[ImGuiCol_SliderGrabActive]=orange;
  colors[ImGuiCol_PlotHistogram]=orange;colors[ImGuiCol_NavCursor]=orange;
  colors[ImGuiCol_TextSelectedBg]=ImVec4(orange.x,orange.y,orange.z,.3f);
  colors[ImGuiCol_ScrollbarBg]=ink;colors[ImGuiCol_ScrollbarGrab]=ImVec4(.24f,.27f,.33f,1);
  colors[ImGuiCol_ScrollbarGrabHovered]=orange;colors[ImGuiCol_ScrollbarGrabActive]=orange;
  colors[ImGuiCol_Separator]=colors[ImGuiCol_Border];
  colors[ImGuiCol_SeparatorHovered]=orange;colors[ImGuiCol_SeparatorActive]=orange;
  colors[ImGuiCol_ResizeGrip]=colors[ImGuiCol_Border];
  colors[ImGuiCol_ResizeGripHovered]=orange;colors[ImGuiCol_ResizeGripActive]=orange;
  colors[ImGuiCol_ModalWindowDimBg]=ImVec4(0,0,0,.6f);
}
struct Theme {
  Theme(ImFont* font):font_(font!=nullptr) {
    if(font_) ImGui::PushFont(font,17.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(24,20));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,14.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,6.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(12,10));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(12,8));
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ink); ImGui::PushStyleColor(ImGuiCol_ChildBg,card);
    ImGui::PushStyleColor(ImGuiCol_Text,white); ImGui::PushStyleColor(ImGuiCol_TextDisabled,muted);
    ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(.19f,.21f,.26f,1));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,card); ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,AccentTint(.10f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,AccentTint(.18f));
    ImGui::PushStyleColor(ImGuiCol_Button,card); ImGui::PushStyleColor(ImGuiCol_ButtonHovered,AccentTint(.24f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,AccentTint(.36f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark,orange); ImGui::PushStyleColor(ImGuiCol_SliderGrab,orange);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,orange); ImGui::PushStyleColor(ImGuiCol_PlotHistogram,orange);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,ink); ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,ImVec4(.24f,.27f,.33f,1));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered,orange); ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,orange);
    ImGui::PushStyleColor(ImGuiCol_Separator,ImVec4(.19f,.21f,.26f,1)); ImGui::PushStyleColor(ImGuiCol_Header,card);
  }
  ~Theme(){ImGui::PopStyleColor(21); ImGui::PopStyleVar(5); if(font_) ImGui::PopFont();}
  bool font_;
};
}
