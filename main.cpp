// ================================================================
//  main.cpp  —  AlgoForges Desktop Application
//  Flow: WelcomeSystem → HackerLoginSystem → AlgoForgesUI
//  OpenGL3 + GLFW + Dear ImGui (C++17)
//
//  Build (Linux/macOS):
//    g++ -std=c++17 main.cpp                              \
//        imgui/*.cpp                                      \
//        imgui/backends/imgui_impl_glfw.cpp               \
//        imgui/backends/imgui_impl_opengl3.cpp            \
//        -I imgui/ -I imgui/backends/                     \
//        -lglfw -lGL -ldl -o AlgoForges
//
//  Build (Windows MSVC):
//    cl /std:c++17 main.cpp imgui\*.cpp                   \
//       imgui\backends\imgui_impl_glfw.cpp                \
//       imgui\backends\imgui_impl_opengl3.cpp             \
//       /I imgui /I imgui\backends                        \
//       /link glfw3.lib opengl32.lib user32.lib gdi32.lib
// ================================================================

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "WelcomeSystem.h"
#include "HackerLoginSystem.h"
#include "AlgoForgesUI.h"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <memory>

// ── Application state machine ──────────────────────────────────
enum class AppState { Splash, Login, CommandCenter };

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_DECORATED,  GLFW_FALSE); // no OS decorations — we draw our own
    glfwWindowHint(GLFW_MAXIMIZED,  GLFW_TRUE);  // start maximized

    GLFWwindow* window = glfwCreateWindow(1920, 1080,
                                          "AlgoForges — Strategic Command Center",
                                          nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    // Centre on screen (if not maximized)
    {
        GLFWmonitor*      mon = glfwGetPrimaryMonitor();
        const GLFWvidmode* vm = glfwGetVideoMode(mon);
        if (vm) glfwSetWindowPos(window,
            (vm->width  - 1920) / 2,
            (vm->height - 1080) / 2);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // ── Style ─────────────────────────────────────────────────
    ImGui::StyleColorsDark();
    ImGuiStyle& sty = ImGui::GetStyle();
    sty.WindowBorderSize  = 0.f;
    sty.WindowPadding     = {0, 0};
    sty.ItemSpacing       = {6, 4};
    sty.ScrollbarSize     = 10.f;
    sty.GrabMinSize       = 8.f;
    sty.WindowRounding    = 0.f;
    sty.FrameRounding     = 4.f;

    // Colour overrides to match AF dark theme
    sty.Colors[ImGuiCol_WindowBg]        = ImVec4(0.016f,0.020f,0.031f,1.f);
    sty.Colors[ImGuiCol_ScrollbarBg]     = ImVec4(0.03f,0.04f,0.06f,1.f);
    sty.Colors[ImGuiCol_ScrollbarGrab]   = ImVec4(0.15f,0.15f,0.2f,1.f);

    // ── Fonts ─────────────────────────────────────────────────
    // Check file existence BEFORE AddFontFromFileTTF to avoid
    // the ImGui assert crash when a font file is simply not present.
    auto fileExists = [](const char* path) -> bool {
        FILE* f = fopen(path, "rb");
        if(f){ fclose(f); return true; }
        return false;
    };

    ImFont* loadedFont = nullptr;
    const char* fontCandidates[] = {
        "fonts/JetBrainsMono-Regular.ttf",
        "fonts/CascadiaCode.ttf",
        "fonts/FiraCode-Regular.ttf",
        "C:/Windows/Fonts/consola.ttf",   // Consolas — always on Windows
        "C:/Windows/Fonts/cour.ttf",      // Courier New fallback
        nullptr
    };
    for(int fi = 0; fontCandidates[fi]; fi++){
        if(!fileExists(fontCandidates[fi])) continue;
        loadedFont = io.Fonts->AddFontFromFileTTF(fontCandidates[fi], 16.0f);
        if(loadedFont){ printf("[Font] Loaded: %s\n", fontCandidates[fi]); break; }
    }
    if(!loadedFont){
        printf("[Font] Using ImGui built-in bitmap font.\n");
        ImFontConfig cfg;
        cfg.SizePixels  = 16.f;
        cfg.OversampleH = 1; cfg.OversampleV = 1; cfg.PixelSnapH = true;
        io.Fonts->AddFontDefault(&cfg);
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // ── Systems ───────────────────────────────────────────────
    WelcomeSystem     welcome;
    HackerLoginSystem loginSys;
    // Lazy-init AlgoForgesUI only after login to save startup time
    unique_ptr<AlgoForgesUI> forge;

    AppState state = AppState::Splash;

    double prevTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double now = glfwGetTime();
        float  dt  = (float)(now - prevTime);
        prevTime   = now;
        if (dt > 0.1f) dt = 0.1f; // clamp lag spikes

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        switch (state) {

        // ── SPLASH ───────────────────────────────────────────
        case AppState::Splash: {
            bool running = welcome.Render(dt);
            if (!running) {
                state = AppState::Login;
                loginSys.Init(ImGui::GetIO().DisplaySize);
            }
            break;
        }

        // ── LOGIN ─────────────────────────────────────────────
        case AppState::Login: {
            bool loginFired = false, registerFired = false;
            loginSys.Render(dt, loginFired, registerFired);

            if (loginFired) {
                printf("[AUTH] User: %s logged in.\n",
                       loginSys.GetUsername());
                forge = make_unique<AlgoForgesUI>(window);
                state = AppState::CommandCenter;
            }
            if (registerFired) {
                printf("[REGISTER] New user: %s\n",
                       loginSys.GetUsername());
                // Hook your registration UI here
            }
            break;
        }

        // ── COMMAND CENTER ────────────────────────────────────
        case AppState::CommandCenter: {
            if (forge) forge->Render(dt);

            // ESC → back to login (optional UX)
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                forge.reset();
                welcome.Reset();
                loginSys.Reset();
                state = AppState::Splash;
            }
            break;
        }

        } // switch

        // ── Render ────────────────────────────────────────────
        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.016f, 0.020f, 0.031f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}