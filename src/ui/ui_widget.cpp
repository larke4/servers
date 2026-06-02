#include "ui/ui_widget.h"
#include "ui/ui_components.h"
#include <imgui.h>

void SetupAmethystStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding    = {8,8};  s.FramePadding   = {5,3};  s.CellPadding    = {6,4};
    s.ItemSpacing      = {6,4};  s.ItemInnerSpacing = {6,4}; s.ScrollbarSize  = 13;
    s.GrabMinSize      = 10;     s.WindowBorderSize = 1;     s.ChildBorderSize = 1;
    s.PopupBorderSize  = 1;      s.FrameBorderSize  = 1;
    s.WindowRounding   = 4;      s.ChildRounding   = 3;      s.FrameRounding  = 3;
    s.PopupRounding    = 3;      s.ScrollbarRounding = 9;    s.GrabRounding   = 3;
    s.TabRounding      = 3;

    static const struct { ImGuiCol idx; ImVec4 col; } kColors[] = {
        {ImGuiCol_Text,             {0.92f,0.90f,0.95f,1.00f}},
        {ImGuiCol_TextDisabled,     {0.55f,0.50f,0.60f,1.00f}},
        {ImGuiCol_WindowBg,         {0.09f,0.07f,0.12f,1.00f}},
        {ImGuiCol_ChildBg,          {0.11f,0.09f,0.14f,1.00f}},
        {ImGuiCol_PopupBg,          {0.09f,0.07f,0.12f,0.96f}},
        {ImGuiCol_Border,           {0.25f,0.20f,0.35f,0.80f}},
        {ImGuiCol_FrameBg,          {0.15f,0.12f,0.22f,1.00f}},
        {ImGuiCol_FrameBgHovered,   {0.25f,0.20f,0.38f,1.00f}},
        {ImGuiCol_FrameBgActive,    {0.35f,0.25f,0.55f,1.00f}},
        {ImGuiCol_TitleBg,          {0.12f,0.09f,0.18f,1.00f}},
        {ImGuiCol_TitleBgActive,    {0.20f,0.14f,0.32f,1.00f}},
        {ImGuiCol_MenuBarBg,        {0.12f,0.09f,0.18f,1.00f}},
        {ImGuiCol_CheckMark,        {0.65f,0.45f,0.95f,1.00f}},
        {ImGuiCol_SliderGrab,       {0.50f,0.35f,0.75f,1.00f}},
        {ImGuiCol_SliderGrabActive, {0.65f,0.45f,0.95f,1.00f}},
        {ImGuiCol_Button,           {0.25f,0.20f,0.40f,1.00f}},
        {ImGuiCol_ButtonHovered,    {0.38f,0.28f,0.62f,1.00f}},
        {ImGuiCol_ButtonActive,     {0.50f,0.35f,0.80f,1.00f}},
        {ImGuiCol_Header,           {0.25f,0.20f,0.40f,1.00f}},
        {ImGuiCol_HeaderHovered,    {0.38f,0.28f,0.62f,1.00f}},
        {ImGuiCol_HeaderActive,     {0.50f,0.35f,0.80f,1.00f}},
        {ImGuiCol_Tab,              {0.15f,0.12f,0.25f,1.00f}},
        {ImGuiCol_TabHovered,       {0.38f,0.28f,0.62f,1.00f}},
        {ImGuiCol_TabActive,        {0.28f,0.20f,0.45f,1.00f}},
    };
    for (auto& [idx, col] : kColors) ImGui::GetStyle().Colors[idx] = col;
}

void CreateMobileNetworkWidget(UserData& currentUser, std::mutex& dataMutex, std::vector<SignalData>& signalData) {
    static UIState state;
    DrawMenuBar(state);
    DrawDataWindow(state.showDataWindow, currentUser, dataMutex);
    DrawGraphWindow(state.showGraphWindow, signalData, state.selectedCellIdentity, dataMutex);
}
