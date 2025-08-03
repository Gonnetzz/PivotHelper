#pragma once
#include "datatypes.h"
#include <string>

namespace Canvas {
    void Render(Node* root, CanvasState& canvas, Node*& selectedNode, bool showPivots, const std::string& activeState, const std::string& defaultState, int activeFrame);
}