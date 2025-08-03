#include "sprite_editor.h"
#include "texture_loader.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <set>
#include <GL/glew.h>

namespace {
    std::string GenerateUniqueName(const std::string& base, const std::set<std::string>& existingNames) {
        if (existingNames.find(base) == existingNames.end()) return base;
        int i = 1;
        while (true) {
            std::string newName = base + std::to_string(i);
            if (existingNames.find(newName) == existingNames.end()) return newName;
            i++;
        }
    }

    void UpdateNodeSpriteReferences(Node* node, const std::string& oldName, const std::string& newName, SpriteData* spriteData) {
        if (!node) return;
        if (node->spriteName == oldName) {
            node->spriteName = newName;
            if (spriteData && spriteData->sprites.count(newName)) {
                node->sprite_ptr = &spriteData->sprites.at(newName);
            }
            else {
                node->sprite_ptr = nullptr;
            }
        }
        for (auto& child : node->childrenInFront) UpdateNodeSpriteReferences(child.get(), oldName, newName, spriteData);
        for (auto& child : node->childrenBehind) UpdateNodeSpriteReferences(child.get(), oldName, newName, spriteData);
    }

    void RenderLinkEditor(SpriteState& activeStateData, Sprite& selectedSprite, const std::string& localActiveState) {
        ImGui::Text("Link to State");

        bool canLink = selectedSprite.states.size() > 1;
        if (!canLink) ImGui::BeginDisabled();

        if (ImGui::BeginCombo("##LinkToState", activeStateData.linkToStateName.c_str())) {
            for (auto const& [stateName, stateData] : selectedSprite.states) {
                if (stateName != localActiveState) {
                    if (ImGui::Selectable(stateName.c_str(), activeStateData.linkToStateName == stateName)) activeStateData.linkToStateName = stateName;
                    if (activeStateData.linkToStateName == stateName) ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (!canLink) ImGui::EndDisabled();
    }

    void RenderFramesEditor(SpriteState& activeStateData, SpriteData* spriteData, const std::string& selectedSpriteName, std::string& successMessage, std::string& errorMessage) {
        ImGui::BeginChild("FrameList", ImVec2(0, 150), true);
        for (int i = 0; i < activeStateData.frames.size(); ++i) {
            ImGui::PushID(i);
            SpriteFrame& frame = activeStateData.frames[i];
            char pathBuffer[256];
            strncpy_s(pathBuffer, frame.texturePath.c_str(), sizeof(pathBuffer));
            ImGui::Text("Frame %d", i + 1);
            ImGui::PushItemWidth(-1);
            if (ImGui::InputText("##TexturePath", pathBuffer, sizeof(pathBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                std::string newPath = pathBuffer;
                if (newPath != frame.texturePath) {
                    GLuint newTextureId = TextureLoader::LoadOrGetTexture(newPath, *spriteData, errorMessage);
                    if (newTextureId != 0) {
                        frame.textureId = newTextureId;
                        frame.texturePath = newPath;
                        glBindTexture(GL_TEXTURE_2D, newTextureId);
                        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &frame.width);
                        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &frame.height);
                        successMessage = "Texture updated successfully!";
                    }
                    else {
                        successMessage.clear();
                    }
                }
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::Separator();
        ImGui::InputFloat("Duration", &activeStateData.duration, 0.01f, 0.1f, "%.3f");
        ImGui::Checkbox("Mipmap", &activeStateData.mipmap);

        bool canSelectNext = !spriteData->sprites.at(selectedSpriteName).states.empty();
        if (!canSelectNext) ImGui::BeginDisabled();

        if (ImGui::BeginCombo("Next State", activeStateData.nextState.c_str())) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            if (ImGui::Selectable("Set to Null", activeStateData.nextState.empty())) {
                activeStateData.nextState.clear();
            }
            ImGui::PopStyleColor();
            ImGui::Separator();

            for (auto const& [stateName, stateData] : spriteData->sprites.at(selectedSpriteName).states) {
                if (ImGui::Selectable(stateName.c_str(), activeStateData.nextState == stateName)) activeStateData.nextState = stateName;
                if (activeStateData.nextState == stateName) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (!canSelectNext) ImGui::EndDisabled();
    }
}

namespace SpriteEditor {
    void Render(SpriteData* spriteData, std::string& successMessage, std::string& errorMessage) {
        static std::string selectedSpriteName;
        static std::string localActiveState = "Normal";

        ImGui::Text("Sprite Editor");
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 60);
        if (ImGui::Button("+##AddSprite")) {
            if (spriteData) {
                std::set<std::string> existingNames;
                for (const auto& pair : spriteData->sprites) existingNames.insert(pair.first);
                std::string newName = GenerateUniqueName("New Sprite", existingNames);
                Sprite newSprite;
                newSprite.name = newName;
                newSprite.states["Normal"] = SpriteState();
                spriteData->sprites[newName] = newSprite;
                selectedSpriteName = newName;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("-##DeleteSprite")) {
            if (!selectedSpriteName.empty() && spriteData && spriteData->sprites.size() > 1) {
                ImGui::OpenPopup("Delete Sprite?");
            }
        }

        if (ImGui::BeginPopupModal("Delete Sprite?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete sprite '%s'?\nThis cannot be undone.", selectedSpriteName.c_str());
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
            if (ImGui::Button("Delete", ImVec2(120, 0))) {
                UpdateNodeSpriteReferences(spriteData->root.get(), selectedSpriteName, "", spriteData);
                spriteData->sprites.erase(selectedSpriteName);
                if (!spriteData->sprites.empty()) {
                    selectedSpriteName = spriteData->sprites.begin()->first;
                }
                else {
                    selectedSpriteName.clear();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(2);
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        if (!spriteData || spriteData->sprites.empty()) {
            ImGui::Text("No sprites loaded.");
            return;
        }

        if (ImGui::BeginCombo("Active", selectedSpriteName.c_str())) {
            for (auto const& [name, sprite] : spriteData->sprites) {
                bool is_selected = (selectedSpriteName == name);
                if (ImGui::Selectable(name.c_str(), is_selected)) {
                    selectedSpriteName = name;
                    localActiveState = "Normal";
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (selectedSpriteName.empty() || !spriteData->sprites.count(selectedSpriteName)) {
            ImGui::Text("Select a sprite to edit.");
            return;
        }

        Sprite& selectedSprite = spriteData->sprites.at(selectedSpriteName);

        ImGui::Separator();
        char nameBuffer[128];
        strncpy_s(nameBuffer, selectedSprite.name.c_str(), sizeof(nameBuffer));
        ImGui::Text("Sprite Name");
        if (ImGui::InputText("##SpriteName", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string newName = nameBuffer;
            if (newName != selectedSprite.name && !spriteData->sprites.count(newName)) {
                auto nodeHandler = spriteData->sprites.extract(selectedSprite.name);
                nodeHandler.key() = newName;
                nodeHandler.mapped().name = newName;
                spriteData->sprites.insert(std::move(nodeHandler));
                UpdateNodeSpriteReferences(spriteData->root.get(), selectedSpriteName, newName, spriteData);
                selectedSpriteName = newName;
            }
        }

        ImGui::Text("State");
        ImGui::SameLine();
        if (ImGui::Button("+##AddState")) {
            std::set<std::string> existingNames;
            for (const auto& pair : selectedSprite.states) existingNames.insert(pair.first);
            localActiveState = GenerateUniqueName("State", existingNames);
            selectedSprite.states[localActiveState] = SpriteState();
        }

        if (ImGui::BeginCombo("##StateCombo", localActiveState.c_str())) {
            for (auto const& [stateName, stateData] : selectedSprite.states) {
                if (ImGui::Selectable(stateName.c_str(), localActiveState == stateName)) localActiveState = stateName;
                if (localActiveState == stateName) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (!selectedSprite.states.count(localActiveState)) {
            if (!selectedSprite.states.empty()) {
                localActiveState = selectedSprite.states.begin()->first;
            }
            else {
                return;
            }
        }

        char stateNameBuffer[128];
        strncpy_s(stateNameBuffer, localActiveState.c_str(), sizeof(stateNameBuffer));
        ImGui::Text("State Name");
        if (localActiveState != "Normal" && ImGui::InputText("##StateName", stateNameBuffer, sizeof(stateNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string newName = stateNameBuffer;
            if (newName != localActiveState && !selectedSprite.states.count(newName)) {
                auto nodeHandler = selectedSprite.states.extract(localActiveState);
                nodeHandler.key() = newName;
                selectedSprite.states.insert(std::move(nodeHandler));
                for (auto& pair : selectedSprite.states) {
                    if (pair.second.isLink && pair.second.linkToStateName == localActiveState) pair.second.linkToStateName = newName;
                    if (pair.second.nextState == localActiveState) pair.second.nextState = newName;
                }
                localActiveState = newName;
            }
        }
        ImGui::Separator();

        SpriteState& activeStateData = selectedSprite.states.at(localActiveState);
        const char* types[] = { "Frames", "Link" };
        int typeIndex = activeStateData.isLink ? 1 : 0;
        if (ImGui::Combo("Type", &typeIndex, types, IM_ARRAYSIZE(types))) activeStateData.isLink = (typeIndex == 1);

        ImGui::Separator();

        if (activeStateData.isLink) {
            RenderLinkEditor(activeStateData, selectedSprite, localActiveState);
        }
        else {
            RenderFramesEditor(activeStateData, spriteData, selectedSpriteName, successMessage, errorMessage);
        }

        ImGui::Separator();
        if (localActiveState != "Normal") {
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
            if (ImGui::Button("Delete State", ImVec2(-1, 0))) {
                ImGui::OpenPopup("Delete State?");
            }
            ImGui::PopStyleColor();
        }

        if (ImGui::BeginPopupModal("Delete State?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Delete state '%s' from sprite '%s'?", localActiveState.c_str(), selectedSprite.name.c_str());
            ImGui::Separator();
            if (ImGui::Button("Delete", ImVec2(120, 0))) {
                selectedSprite.states.erase(localActiveState);
                localActiveState = "Normal";
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }
}