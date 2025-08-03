#include "pivot_logic.h"
#include <cmath>

#ifndef IM_PI
#define IM_PI           3.14159265358979323846f
#endif

namespace {
    ImVec2 rotate_point_around_origin(const ImVec2& point, float angle_rad) {
        float s = sinf(angle_rad);
        float c = cosf(angle_rad);
        return ImVec2(point.x * c - point.y * s, point.x * s + point.y * c);
    }
}

Transform PivotLogic::CalculateWorldTransform(
    const Node* node,
    const Transform& parent_transform,
    const SpriteFrame* parent_frame,
    const CanvasState& canvas)
{
    if (!node) return parent_transform;

    float final_angle_deg = parent_transform.angle_deg + node->angle;
    float final_angle_rad = final_angle_deg * IM_PI / 180.0f;

    ImVec2 attachment_point_world = parent_transform.position;
    if (parent_frame) {
        ImVec2 pivot_on_parent_local = {
            node->pivot.x * parent_frame->width * canvas.zoom,
            node->pivot.y * parent_frame->height * canvas.zoom
        };

        float parent_angle_rad = parent_transform.angle_deg * IM_PI / 180.0f;
        ImVec2 pivot_on_parent_rotated = rotate_point_around_origin(pivot_on_parent_local, parent_angle_rad);
        attachment_point_world.x += pivot_on_parent_rotated.x;
        attachment_point_world.y += pivot_on_parent_rotated.y;
    }

    const SpriteFrame* my_frame_ptr = nullptr;
    if (node->sprite_ptr && !node->sprite_ptr->states.empty()) {
        const auto& state_it = node->sprite_ptr->states.find("Normal");
        if (state_it != node->sprite_ptr->states.end() && !state_it->second.frames.empty()) {
            my_frame_ptr = &state_it->second.frames[0];
        }
    }

    if (!my_frame_ptr) {
        return { attachment_point_world, final_angle_deg, attachment_point_world };
    }
    const SpriteFrame& my_frame = *my_frame_ptr;

    ImVec2 V_center_to_anchor_local = {
        -node->pivotOffset.x * my_frame.width * canvas.zoom,
        -node->pivotOffset.y * my_frame.height * canvas.zoom
    };

    ImVec2 V_center_to_anchor_rotated = rotate_point_around_origin(V_center_to_anchor_local, final_angle_rad);

    ImVec2 my_center_pos = {
        attachment_point_world.x - V_center_to_anchor_rotated.x,
        attachment_point_world.y - V_center_to_anchor_rotated.y
    };

    return { my_center_pos, final_angle_deg, attachment_point_world };
}