#include "gui_app.h"
#include "gui_tabs.h"
#include "ui/ui_widget.h"

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <imgui.h>
#include <implot.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>

static SDL_Window* s_window = nullptr;
static SDL_GLContext s_gl     = nullptr;

bool GuiAppInit(int width, int height, const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return false;

    s_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!s_window) return false;

    s_gl = SDL_GL_CreateContext(s_window);
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(s_window, s_gl);
    ImGui_ImplOpenGL3_Init("#version 330");

    const char* fontPaths[] = {"fonts/consola.ttf", "../fonts/consola.ttf"};
    ImGuiIO& io = ImGui::GetIO();
    for (auto* p : fontPaths)
        if (std::filesystem::exists(p)) { io.Fonts->AddFontFromFileTTF(p, 17.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic()); break; }

    SetupAmethystStyle();
    return true;
}

bool GuiAppFrame(RuntimeState& state) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL2_ProcessEvent(&e);
        if (e.type == SDL_QUIT) state.running.store(false);
    }
    if (!state.running.load()) return false;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::Begin("##root", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::Text("Статус: %s | Пакеты: %d | RX: %.1f KB",
        state.packets.load() > 0 ? "Подключено" : "Ожидание",
        state.packets.load(), state.bytesRx.load() / 1024.0);
    ImGui::Separator();

    if (ImGui::BeginTabBar("tabs")) {
        if (ImGui::BeginTabItem("Главная"))   { TabMain(state);      ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Графики"))   { TabSignals(state);   ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Локальные")) { TabLocal(state);     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Карта"))     { TabMap(state);       ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Телефония")) { TabTelephony(state); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Фильтры"))   { TabFilters(state);   ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    ImGui::End();

    ImGui::Render();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(s_window);
    return true;
}

void GuiAppShutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(s_gl);
    SDL_DestroyWindow(s_window);
    SDL_Quit();
}
