#include "layout.h"
#include "datatypes.h"
#include "file_handling.h"
#include "editor.h"
#include "canvas.h"
#include "outliner.h"
#include "export.h"
#include "sprite_editor.h"
#include "actions.h"
#include "timeline.h"
#include "environment.h"
#include "file_dialog.h"
#include "hotkeys.h"

#include <imgui.h>
#include <il/il.h>
#include <IL/ilu.h>
#include <algorithm>

namespace {
    std::unique_ptr<SpriteData> g_spriteData;
    std::string g_errorMessage;
    std::string g_successMessage;
    CanvasState g_canvas;
    Node* g_selectedNode = nullptr;
    bool g_showPivots = false;

    Node* g_dragDropSourceNode = nullptr;
    Node* g_dragDropTargetNode = nullptr;
    Node* g_nodeToDelete = nullptr;
    Node* g_nodeToAddChildTo = nullptr;

    std::string g_activeState = "Normal";
    bool g_isPlaying = false;
    int g_activeFrame = 0;
    int g_maxFrames = 1;
    float g_frameTimer = 0.0f;
    std::string g_startupNotification;
}

namespace SpritePreviewer {
    int CalculateMaxFrames(SpriteData* spriteData, const std::string& stateName) {
        if (!spriteData) return 1;
        int max_frames = 0;
        for (const auto& pair : spriteData->sprites) {
            if (pair.second.states.count(stateName)) {
                const auto& state = pair.second.states.at(stateName);
                if (!state.isLink) {
                    max_frames = std::max(max_frames, (int)state.frames.size());
                }
            }
        }
        return std::max(1, max_frames);
    }

    void Initialize() {
        ilInit();
        iluInit();
        g_startupNotification = Environment::GetStartupMessage();

        if (!g_spriteData) {
            g_spriteData = std::make_unique<SpriteData>();
            g_spriteData->root = std::make_unique<Node>();
            g_spriteData->root->name = "Root";
            g_activeState = "Normal";
        }
    }

    void ApplyTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_Text] = TEXT_COLOR;
        style.Colors[ImGuiCol_WindowBg] = BG_COLOR;
        style.Colors[ImGuiCol_ChildBg] = BG_COLOR;
        style.Colors[ImGuiCol_FrameBg] = FRAME_BG_COLOR;
        style.Colors[ImGuiCol_Button] = WIDGET_BG_COLOR;
        style.Colors[ImGuiCol_Header] = WIDGET_BG_COLOR;
        style.Colors[ImGuiCol_TitleBg] = FRAME_BG_COLOR;
        style.Colors[ImGuiCol_TitleBgActive] = WIDGET_BG_COLOR;
        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.ChildRounding = 0.0f;
    }

    void UpdateAnimation() {
        if (g_isPlaying && g_spriteData && g_maxFrames > 1) {
            g_frameTimer += ImGui::GetIO().DeltaTime;
            float frameDuration = 0.1f;
            if (g_frameTimer >= frameDuration) {
                g_frameTimer -= frameDuration;
                g_activeFrame = (g_activeFrame + 1) % g_maxFrames;
            }
        }
    }

    void LoadFile(const std::string& path) {
        g_startupNotification.clear();
        load_sprite_file(path, g_spriteData, g_errorMessage, g_successMessage, g_canvas);
        g_selectedNode = nullptr;
        if (g_spriteData) {
            g_activeState = g_spriteData->defaultState;
            g_maxFrames = CalculateMaxFrames(g_spriteData.get(), g_activeState);
        }
        else {
            g_maxFrames = 1;
        }
        g_activeFrame = 0;
    }

    void NewProject() {
        g_spriteData = std::make_unique<SpriteData>();
        g_spriteData->root = std::make_unique<Node>();
        g_spriteData->root->name = "Root";
        g_selectedNode = nullptr;
        g_activeState = "Normal";
        g_maxFrames = 1;
        g_activeFrame = 0;
        g_errorMessage.clear();
        g_successMessage = "New project created.";
    }

    void HandleHotkeys(bool& isRunning) {
        Hotkeys::Action action = Hotkeys::Process();
        switch (action) {
        case Hotkeys::Action::New:    NewProject(); break;
        case Hotkeys::Action::Open: {
            std::string path = FileDialog::OpenFile("Lua Files (*.lua)\0*.lua\0All Files (*.*)\0*.*\0");
            if (!path.empty()) LoadFile(path);
            break;
        }
        case Hotkeys::Action::Save:
            if (g_spriteData) Export::SaveToFile("export.lua", g_spriteData->root.get(), g_spriteData->sprites, g_successMessage, g_errorMessage);
            break;
        case Hotkeys::Action::SaveAs: {
            std::string path = FileDialog::SaveFile("Lua Files (*.lua)\0*.lua\0All Files (*.*)\0*.*\0");
            if (!path.empty() && g_spriteData) Export::SaveToFile(path, g_spriteData->root.get(), g_spriteData->sprites, g_successMessage, g_errorMessage);
            break;
        }
        case Hotkeys::Action::Quit:   isRunning = false; break;
        default: break;
        }
    }

    void RenderMenuBar(bool& isRunning) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N")) { NewProject(); }
                if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                    std::string path = FileDialog::OpenFile("Lua Files (*.lua)\0*.lua\0All Files (*.*)\0*.*\0");
                    if (!path.empty()) LoadFile(path);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    if (g_spriteData) Export::SaveToFile("export.lua", g_spriteData->root.get(), g_spriteData->sprites, g_successMessage, g_errorMessage);
                }
                if (ImGui::MenuItem("Save As...", "Shift+Ctrl+S")) {
                    std::string path = FileDialog::SaveFile("Lua Files (*.lua)\0*.lua\0All Files (*.*)\0*.*\0");
                    if (!path.empty() && g_spriteData) Export::SaveToFile(path, g_spriteData->root.get(), g_spriteData->sprites, g_successMessage, g_errorMessage);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Open Laser.lua")) { LoadFile("weapons/beamlaser.lua"); }
                if (ImGui::MenuItem("Open Cannon.lua")) { LoadFile("weapons/cannon.lua"); }
                if (ImGui::MenuItem("Open Turbine.lua")) { LoadFile("devices/windturbine.lua"); }
                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "Ctrl+Q")) { isRunning = false; }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void RenderStatusBar() {
        float statusBarHeight = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - statusBarHeight));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, statusBarHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 2));
        ImGui::Begin("StatusBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        if (!g_startupNotification.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", g_startupNotification.c_str());
        }
        else if (!g_errorMessage.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", g_errorMessage.c_str());
        }
        else if (!g_successMessage.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", g_successMessage.c_str());
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void RenderUI(bool& isRunning) {
        RenderMenuBar(isRunning);
        HandleHotkeys(isRunning);

        float menuBarHeight = ImGui::GetFrameHeight();
        float statusBarHeight = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - menuBarHeight - statusBarHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("MainDockspace", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();

        float leftPaneWidth = 250.0f;
        float rightPaneWidth = 300.0f;
        float viewWidth = ImGui::GetContentRegionAvail().x - leftPaneWidth - rightPaneWidth;

        ImGui::BeginChild("SpriteEditorPane", ImVec2(leftPaneWidth, 0), true);
        SpriteEditor::Render(g_spriteData.get(), g_successMessage, g_errorMessage);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("CenterColumn", ImVec2(viewWidth, 0), false);
        Timeline::Render(g_spriteData.get(), g_activeState, g_isPlaying, g_activeFrame, g_maxFrames, g_frameTimer);
        ImGui::BeginChild("SpriteViewPane", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        Canvas::Render(g_spriteData ? g_spriteData->root.get() : nullptr, g_canvas, g_selectedNode, g_showPivots, g_activeState, g_spriteData ? g_spriteData->defaultState : "Normal", g_activeFrame);
        ImGui::EndChild();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("RightColumn", ImVec2(rightPaneWidth, 0), false);
        ImGui::BeginChild("OutlinerPane", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.4f), true);
        Outliner::Render(g_spriteData ? g_spriteData->root.get() : nullptr, g_selectedNode, g_dragDropSourceNode, g_dragDropTargetNode, g_nodeToDelete, g_nodeToAddChildTo);
        ImGui::EndChild();
        ImGui::BeginChild("PropertiesPane", ImVec2(0, 0), true);
        Editor::Render(g_spriteData.get(), g_selectedNode, g_showPivots);
        ImGui::EndChild();
        ImGui::EndChild();

        ImGui::End();
        RenderStatusBar();

        UpdateAnimation();
        Actions::Process(g_spriteData.get(), g_selectedNode, g_dragDropSourceNode, g_dragDropTargetNode, g_nodeToDelete, g_nodeToAddChildTo);
    }

    void Cleanup() {
        if (g_spriteData) { for (auto const& [key, val] : g_spriteData->loadedTextures) { glDeleteTextures(1, &val); } }
    }
}