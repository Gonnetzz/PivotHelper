#include "outliner.h"
#include "datatypes.h"
#include <imgui.h>

namespace {
    void DrawNodeRecursive(Node* node, Node* root, Node*& selectedNode, Node*& dragDropSource, Node*& dragDropTarget, Node*& nodeToDelete, Node*& nodeToAddChildTo, ImU32 dotColor = 0) {
        if (!node) return;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (selectedNode == node) flags |= ImGuiTreeNodeFlags_Selected;
        if (node->childrenInFront.empty() && node->childrenBehind.empty()) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);

        if (dotColor != 0) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + 5, p.y + ImGui::GetTextLineHeight() * 0.5f), 3.5f, dotColor);
            ImGui::Dummy(ImVec2(14, 0));
            ImGui::SameLine();
        }

        bool node_is_open = ImGui::TreeNodeEx((void*)(intptr_t)node, flags, "%s", node->name.c_str());

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            selectedNode = node;
        }

        ImGui::PushID(node);
        if (ImGui::BeginPopupContextItem("NodeContextMenu")) {
            if (ImGui::MenuItem("Add New Node")) nodeToAddChildTo = node;
            if (node != root) {
                if (ImGui::MenuItem("Delete Node")) nodeToDelete = node;
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("NODE_DRAG_DROP", &node, sizeof(Node*));
            ImGui::Text("Move %s", node->name.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_DRAG_DROP")) {
                IM_ASSERT(payload->DataSize == sizeof(Node*));
                Node* payload_node = *(Node**)payload->Data;
                if (payload_node != node) {
                    dragDropSource = payload_node;
                    dragDropTarget = node;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (node_is_open && !(flags & ImGuiTreeNodeFlags_Leaf)) {
            for (const auto& child : node->childrenBehind) {
                DrawNodeRecursive(child.get(), root, selectedNode, dragDropSource, dragDropTarget, nodeToDelete, nodeToAddChildTo, COLOR_HIERARCHY_BEHIND);
            }
            for (const auto& child : node->childrenInFront) {
                DrawNodeRecursive(child.get(), root, selectedNode, dragDropSource, dragDropTarget, nodeToDelete, nodeToAddChildTo, COLOR_HIERARCHY_FRONT);
            }
            ImGui::TreePop();
        }
    }
}

void Outliner::Render(Node* root, Node*& selectedNode, Node*& dragDropSource, Node*& dragDropTarget, Node*& nodeToDelete, Node*& nodeToAddChildTo) {
    ImGui::Text("Outliner");
    ImGui::Separator();
    if (root) {
        DrawNodeRecursive(root, root, selectedNode, dragDropSource, dragDropTarget, nodeToDelete, nodeToAddChildTo);
    }
    else {
        ImGui::Text("No data loaded.");
    }
}