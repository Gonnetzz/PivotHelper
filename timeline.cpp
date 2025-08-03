#include "timeline.h"
#include <imgui.h>
#include <algorithm>
#include <string>

namespace {
    constexpr ImU32 COLOR_PROGRESS_BAR = IM_COL32(0, 28, 5, 255);

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
}

namespace Timeline {
    void Render(SpriteData* spriteData, std::string& activeState, bool& isPlaying, int& activeFrame, int& maxFrames, float& frameTimer) {
        ImGui::BeginChild("TimelinePane", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);

        if (!spriteData) {
            ImGui::Text("No data loaded.");
            ImGui::EndChild();
            return;
        }

        float dropdown_width = 150.0f;
        float button_width = 60.0f;
        float timeline_width = 150.0f;
        float spacing = 15.0f;

        ImGui::PushItemWidth(dropdown_width);
        if (ImGui::BeginCombo("Global State", activeState.c_str())) {
            if (!spriteData->allAvailableStates.empty()) {
                for (const auto& stateName : spriteData->allAvailableStates) {
                    bool is_selected = (activeState == stateName);
                    if (ImGui::Selectable(stateName.c_str(), is_selected)) {
                        if (activeState != stateName) {
                            activeState = stateName;
                            maxFrames = CalculateMaxFrames(spriteData, activeState);
                            activeFrame = 0;
                            frameTimer = 0.0f;
                            isPlaying = false;
                        }
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine(0, spacing);

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##timeline", ImVec2(timeline_width, ImGui::GetFrameHeight()));
        if (ImGui::IsItemActive() || ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            if (maxFrames > 1) {
                float mouse_x = ImGui::GetIO().MousePos.x - ImGui::GetItemRectMin().x;
                activeFrame = (int)((mouse_x / timeline_width) * maxFrames);
                activeFrame = std::max(0, std::min(maxFrames - 1, activeFrame));
                frameTimer = 0.0f;
            }
        }

        float progress = maxFrames > 1 ? (float)activeFrame / (maxFrames - 1) : 0.0f;
        ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(40, 40, 40, 255));
        ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImVec2(ImGui::GetItemRectMin().x + timeline_width * progress, ImGui::GetItemRectMax().y), COLOR_PROGRESS_BAR);

        std::string label = std::to_string(activeFrame + 1) + " / " + std::to_string(maxFrames);
        ImVec2 label_size = ImGui::CalcTextSize(label.c_str());
        ImVec2 label_pos = ImVec2(
            ImGui::GetItemRectMin().x + (timeline_width - label_size.x) * 0.5f,
            ImGui::GetItemRectMin().y + (ImGui::GetFrameHeight() - label_size.y) * 0.5f
        );
        ImGui::GetWindowDrawList()->AddText(label_pos, IM_COL32(255, 255, 255, 255), label.c_str());

        ImGui::SameLine(0, spacing);

        if (maxFrames <= 1) {
            isPlaying = false;
            ImGui::BeginDisabled();
        }

        const char* buttonLabel = isPlaying ? "Pause" : "Play";
        if (ImGui::Button(buttonLabel, ImVec2(button_width, 0))) {
            isPlaying = !isPlaying;
            frameTimer = 0.0f;
        }

        if (maxFrames <= 1) {
            ImGui::EndDisabled();
        }

        ImGui::EndChild();
    }
}