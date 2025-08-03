#include "canvas.h"
#include "pivot_logic.h"
#include <imgui.h>
#include <algorithm>
#include <vector>

#ifndef IM_PI
#define IM_PI           3.14159265358979323846f
#endif

namespace {
    struct SimpleRect { ImVec2 Min, Max; SimpleRect(const ImVec2& min, const ImVec2& max) : Min(min), Max(max) {} bool Contains(const ImVec2& p) const { return p.x >= Min.x && p.y >= Min.y && p.x < Max.x && p.y < Max.y; } };
    struct RenderInfo { Node* node = nullptr; SimpleRect bounds = SimpleRect({ 0,0 }, { 0,0 }); Transform transform; };

    ImVec2 rotate_point(const ImVec2& point, const ImVec2& center, float angle_rad) {
        float s = sin(angle_rad); float c = cos(angle_rad);
        ImVec2 p = ImVec2(point.x - center.x, point.y - center.y);
        return ImVec2(center.x + (p.x * c - p.y * s), center.y + (p.x * s + p.y * c));
    }

    void draw_and_collect_bounds(Node* node, const Transform& parent_transform, const SpriteFrame* parent_frame, const CanvasState& canvas, std::vector<RenderInfo>& out_render_info, const std::string& activeState, const std::string& defaultState, int activeFrame) {
        if (!node) return;

        Transform my_transform = PivotLogic::CalculateWorldTransform(node, parent_transform, parent_frame, canvas);

        const SpriteFrame* frame_for_child = nullptr;
        if (node->sprite_ptr && node->sprite_ptr->states.count(defaultState) && !node->sprite_ptr->states.at(defaultState).frames.empty()) {
            frame_for_child = &node->sprite_ptr->states.at(defaultState).frames[0];
        }
        for (const auto& child : node->childrenBehind) {
            draw_and_collect_bounds(child.get(), my_transform, frame_for_child, canvas, out_render_info, activeState, defaultState, activeFrame);
        }

        if (!node->sprite_ptr || node->sprite_ptr->states.empty()) {
            for (const auto& child : node->childrenInFront) {
                draw_and_collect_bounds(child.get(), my_transform, nullptr, canvas, out_render_info, activeState, defaultState, activeFrame);
            }
            return;
        }

        auto state_it = node->sprite_ptr->states.find(activeState);
        bool useEffectiveState = (state_it != node->sprite_ptr->states.end() && !state_it->second.isLink);
        if (!useEffectiveState) {
            state_it = node->sprite_ptr->states.find(defaultState);
        }
        if (state_it == node->sprite_ptr->states.end() || state_it->second.frames.empty()) return;

        const auto& frames = state_it->second.frames;
        int frameIndex = useEffectiveState ? std::min((int)frames.size() - 1, activeFrame) : 0;
        const SpriteFrame& my_frame = frames[frameIndex];

        if (my_frame.textureId == 0) return;

        float angle_rad = my_transform.angle_deg * IM_PI / 180.0f;
        float width_half = my_frame.width * 0.5f * canvas.zoom;
        float height_half = my_frame.height * 0.5f * canvas.zoom;

        ImVec2 p1 = rotate_point({ my_transform.position.x - width_half, my_transform.position.y - height_half }, my_transform.position, angle_rad);
        ImVec2 p2 = rotate_point({ my_transform.position.x + width_half, my_transform.position.y - height_half }, my_transform.position, angle_rad);
        ImVec2 p3 = rotate_point({ my_transform.position.x + width_half, my_transform.position.y + height_half }, my_transform.position, angle_rad);
        ImVec2 p4 = rotate_point({ my_transform.position.x - width_half, my_transform.position.y + height_half }, my_transform.position, angle_rad);

        ImGui::GetWindowDrawList()->AddImageQuad((ImTextureID)(intptr_t)my_frame.textureId, p1, p2, p3, p4);

        float minX = std::min({ p1.x, p2.x, p3.x, p4.x }); float minY = std::min({ p1.y, p2.y, p3.y, p4.y });
        float maxX = std::max({ p1.x, p2.x, p3.x, p4.x }); float maxY = std::max({ p1.y, p2.y, p3.y, p4.y });

        out_render_info.push_back({ node, SimpleRect({minX, minY}, {maxX, maxY}), my_transform });

        for (const auto& child : node->childrenInFront) {
            draw_and_collect_bounds(child.get(), my_transform, &my_frame, canvas, out_render_info, activeState, defaultState, activeFrame);
        }
    }
}

void Canvas::Render(Node* root, CanvasState& canvas, Node*& selectedNode, bool showPivots, const std::string& activeState, const std::string& defaultState, int activeFrame) {
    ImGuiIO& io = ImGui::GetIO();
    bool isWindowHovered = ImGui::IsWindowHovered();

    if (isWindowHovered) {
        if (io.MouseWheel != 0.0f) { canvas.zoom *= powf(1.1f, io.MouseWheel); canvas.zoom = std::max(0.05f, std::min(canvas.zoom, 20.0f)); }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) { canvas.pan.x += io.MouseDelta.x; canvas.pan.y += io.MouseDelta.y; }
    }
    if (!root) return;

    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 canvas_min = ImGui::GetWindowContentRegionMin(); ImVec2 canvas_max = ImGui::GetWindowContentRegionMax();
    ImVec2 canvas_center = { window_pos.x + canvas_min.x + (canvas_max.x - canvas_min.x) * 0.5f, window_pos.y + canvas_min.y + (canvas_max.y - canvas_min.y) * 0.5f };
    Transform root_transform = { {canvas_center.x + canvas.pan.x, canvas_center.y + canvas.pan.y}, 0.0f };
    root_transform.anchor_pos = root_transform.position;

    std::vector<RenderInfo> render_infos;
    draw_and_collect_bounds(root, root_transform, nullptr, canvas, render_infos, activeState, defaultState, activeFrame);

    if (isWindowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        Node* newSelectedNode = nullptr;
        for (int i = render_infos.size() - 1; i >= 0; --i) {
            if (render_infos[i].bounds.Contains(io.MousePos)) {
                newSelectedNode = render_infos[i].node;
                break;
            }
        }
        selectedNode = newSelectedNode;
    }

    if (selectedNode) {
        for (const auto& info : render_infos) {
            if (info.node == selectedNode) {
                ImGui::GetWindowDrawList()->AddRect(info.bounds.Min, info.bounds.Max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 1.5f);
                if (showPivots) {
                    ImGui::GetWindowDrawList()->AddCircleFilled(info.transform.anchor_pos, 4.0f, IM_COL32(255, 0, 255, 255));
                    ImGui::GetWindowDrawList()->AddCircle(info.transform.anchor_pos, 4.0f, IM_COL32(0, 0, 0, 255));
                }
                break;
            }
        }
    }
}