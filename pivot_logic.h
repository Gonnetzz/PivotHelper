#pragma once
#include "datatypes.h"

namespace PivotLogic {
    Transform CalculateWorldTransform(
        const Node* node,
        const Transform& parent_transform,
        const SpriteFrame* parent_frame,
        const CanvasState& canvas
    );
}