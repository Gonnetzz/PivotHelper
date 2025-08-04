#include "hotkeys.h"
#include <imgui.h>

namespace Hotkeys {
    Action Process() {
        ImGuiIO& io = ImGui::GetIO();
        if (!io.KeyCtrl) {
            return Action::None;
        }

        bool shift = io.KeyShift;

        if (ImGui::IsKeyPressed(ImGuiKey_N, false)) return Action::New;
        if (ImGui::IsKeyPressed(ImGuiKey_O, false)) return Action::Open;
        if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) return Action::Quit;

        if (shift && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            return Action::SaveAs;
        }
        if (!shift && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            return Action::Save;
        }

        return Action::None;
    }
}