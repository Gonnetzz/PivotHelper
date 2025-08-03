#include "editor.h"
#include "datatypes.h"
#include <imgui.h>
#include <vector>
#include <memory>
#include <algorithm>

namespace {
    Node* FindParentOfNode(Node* searchRoot, Node* nodeToFind, std::vector<std::unique_ptr<Node>>*& childListRef, std::string& listNameRef) {
        if (!searchRoot || !nodeToFind) return nullptr;
        for (auto& child : searchRoot->childrenInFront) {
            if (child.get() == nodeToFind) {
                childListRef = &searchRoot->childrenInFront;
                listNameRef = "ChildrenInFront";
                return searchRoot;
            }
            Node* found = FindParentOfNode(child.get(), nodeToFind, childListRef, listNameRef);
            if (found) return found;
        }
        for (auto& child : searchRoot->childrenBehind) {
            if (child.get() == nodeToFind) {
                childListRef = &searchRoot->childrenBehind;
                listNameRef = "ChildrenBehind";
                return searchRoot;
            }
            Node* found = FindParentOfNode(child.get(), nodeToFind, childListRef, listNameRef);
            if (found) return found;
        }
        return nullptr;
    }

    void DrawColorDot(ImU32 color) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + 8, p.y + ImGui::GetTextLineHeight() * 0.5f), 4.0f, color);
        ImGui::Dummy(ImVec2(16, 0));
        ImGui::SameLine();
    }
}

void Editor::Render(SpriteData* spriteData, Node*& selectedNode, bool& showPivots) {
    constexpr float button_width = 28.0f;
    constexpr float button_spacing = 4.0f;
    constexpr float right_margin = 4.0f;
    const float full_width = ImGui::GetContentRegionAvail().x - right_margin;
    const float input_width = (full_width - (button_width * 2.0f) - (button_spacing * 2.0f)) * 0.9f;

    ImGui::Text("Properties");
    ImGui::Separator();
    ImGui::Checkbox("Show Pivots", &showPivots);
    ImGui::Separator();

    if (selectedNode) {
        ImGui::Text("Selected Node:");
        char nameBuffer[128];
        strncpy_s(nameBuffer, selectedNode->name.c_str(), sizeof(nameBuffer));
        ImGui::PushItemWidth(full_width);
        if (ImGui::InputText("##NodeName", nameBuffer, sizeof(nameBuffer))) {
            selectedNode->name = nameBuffer;
        }
        ImGui::PopItemWidth();

        ImGui::Separator();
        ImGui::Text("Sprite");
        ImGui::PushItemWidth(full_width);
        const char* currentSpriteName = selectedNode->sprite_ptr ? selectedNode->sprite_ptr->name.c_str() : "None";
        if (ImGui::BeginCombo("##SpriteSelector", currentSpriteName)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            if (ImGui::Selectable("Set to Null", selectedNode->sprite_ptr == nullptr)) {
                selectedNode->spriteName.clear();
                selectedNode->sprite_ptr = nullptr;
            }
            ImGui::PopStyleColor();
            ImGui::Separator();

            if (spriteData) {
                for (auto const& [name, sprite] : spriteData->sprites) {
                    bool is_selected = (currentSpriteName == name);
                    if (ImGui::Selectable(name.c_str(), is_selected)) {
                        selectedNode->spriteName = name;
                        selectedNode->sprite_ptr = &sprite;
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::Separator();
        ImGui::Text("Pivot");
        ImGui::PushItemWidth(input_width);
        ImGui::InputFloat("X##PivotX", &selectedNode->pivot.x, 0.0f, 0.0f, "%.4f");
        ImGui::PopItemWidth(); ImGui::SameLine(0.0f, button_spacing);
        ImGui::PushButtonRepeat(true);
        if (ImGui::Button("-##PivotX", ImVec2(button_width, 0))) { selectedNode->pivot.x -= 0.01f; }
        ImGui::SameLine(0.0f, button_spacing);
        if (ImGui::Button("+##PivotX", ImVec2(button_width, 0))) { selectedNode->pivot.x += 0.01f; }
        ImGui::PopButtonRepeat();

        ImGui::PushItemWidth(input_width);
        ImGui::InputFloat("Y##PivotY", &selectedNode->pivot.y, 0.0f, 0.0f, "%.4f");
        ImGui::PopItemWidth(); ImGui::SameLine(0.0f, button_spacing);
        ImGui::PushButtonRepeat(true);
        if (ImGui::Button("-##PivotY", ImVec2(button_width, 0))) { selectedNode->pivot.y -= 0.01f; }
        ImGui::SameLine(0.0f, button_spacing);
        if (ImGui::Button("+##PivotY", ImVec2(button_width, 0))) { selectedNode->pivot.y += 0.01f; }
        ImGui::PopButtonRepeat();

        ImGui::Separator();
        ImGui::Text("Pivot Offset");
        ImGui::PushItemWidth(input_width);
        ImGui::InputFloat("X##OffsetX", &selectedNode->pivotOffset.x, 0.0f, 0.0f, "%.4f");
        ImGui::PopItemWidth(); ImGui::SameLine(0.0f, button_spacing);
        ImGui::PushButtonRepeat(true);
        if (ImGui::Button("-##OffsetX", ImVec2(button_width, 0))) { selectedNode->pivotOffset.x -= 0.01f; }
        ImGui::SameLine(0.0f, button_spacing);
        if (ImGui::Button("+##OffsetX", ImVec2(button_width, 0))) { selectedNode->pivotOffset.x += 0.01f; }
        ImGui::PopButtonRepeat();

        ImGui::PushItemWidth(input_width);
        ImGui::InputFloat("Y##OffsetY", &selectedNode->pivotOffset.y, 0.0f, 0.0f, "%.4f");
        ImGui::PopItemWidth(); ImGui::SameLine(0.0f, button_spacing);
        ImGui::PushButtonRepeat(true);
        if (ImGui::Button("-##OffsetY", ImVec2(button_width, 0))) { selectedNode->pivotOffset.y -= 0.01f; }
        ImGui::SameLine(0.0f, button_spacing);
        if (ImGui::Button("+##OffsetY", ImVec2(button_width, 0))) { selectedNode->pivotOffset.y += 0.01f; }
        ImGui::PopButtonRepeat();

        ImGui::Separator();
        ImGui::Text("Angle");
        ImGui::PushItemWidth(input_width);
        ImGui::InputFloat("##AngleInput", &selectedNode->angle, 0.0f, 0.0f, "%.1f");
        ImGui::PopItemWidth(); ImGui::SameLine(0.0f, button_spacing);
        ImGui::PushButtonRepeat(true);
        if (ImGui::Button("-##AngleButton", ImVec2(button_width, 0))) { selectedNode->angle -= 1.0f; }
        ImGui::SameLine(0.0f, button_spacing);
        if (ImGui::Button("+##AngleButton", ImVec2(button_width, 0))) { selectedNode->angle += 1.0f; }
        ImGui::PopButtonRepeat();

        if (spriteData && spriteData->root.get() && selectedNode != spriteData->root.get()) {
            ImGui::Separator();
            ImGui::Text("Hierarchy");

            std::vector<std::unique_ptr<Node>>* childList = nullptr;
            std::string listName;
            Node* parentNode = FindParentOfNode(spriteData->root.get(), selectedNode, childList, listName);

            if (parentNode && childList) {
                ImGui::Text("Parent: %s", parentNode->name.c_str());

                int selectedIndex = (listName == "ChildrenInFront") ? 0 : 1;
                const char* previewValue = (selectedIndex == 0) ? "ChildrenInFront" : "ChildrenBehind";

                ImGui::PushItemWidth(full_width);
                if (ImGui::BeginCombo("Render Order", previewValue)) {
                    ImGui::PushID("Front");
                    DrawColorDot(COLOR_HIERARCHY_FRONT);
                    if (ImGui::Selectable("ChildrenInFront", selectedIndex == 0)) { selectedIndex = 0; }
                    ImGui::PopID();
                    ImGui::PushID("Behind");
                    DrawColorDot(COLOR_HIERARCHY_BEHIND);
                    if (ImGui::Selectable("ChildrenBehind", selectedIndex == 1)) { selectedIndex = 1; }
                    ImGui::PopID();
                    if (((selectedIndex == 0) ? "ChildrenInFront" : "ChildrenBehind") != listName) {
                        std::vector<std::unique_ptr<Node>>* destinationList = (selectedIndex == 0) ? &parentNode->childrenInFront : &parentNode->childrenBehind;
                        auto it = std::find_if(childList->begin(), childList->end(), [&](const auto& p) { return p.get() == selectedNode; });
                        if (it != childList->end()) {
                            std::unique_ptr<Node> movedNode = std::move(*it);
                            childList->erase(it);
                            destinationList->push_back(std::move(movedNode));
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
            }
        }
    }
    else {
        ImGui::Text("Select a node from the outliner or click on it.");
    }
}