#pragma once
#include <imgui.h>
namespace motogp::overlay {
inline const ImVec4 ink{0.055f,0.064f,0.08f,1}, card{0.09f,0.10f,0.125f,1};
inline const ImVec4 orange{1.f,0.65f,0.12f,1}, white{0.94f,0.95f,0.97f,1}, muted{0.58f,0.63f,0.71f,1};
inline const char* Tr(bool tr,const char* en,const char* turkish) { return tr?turkish:en; }
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
    ImGui::PushStyleColor(ImGuiCol_FrameBg,card); ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(.19f,.17f,.14f,1));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,ImVec4(.26f,.20f,.12f,1));
    ImGui::PushStyleColor(ImGuiCol_Button,card); ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(.30f,.23f,.13f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(.42f,.28f,.10f,1));
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
