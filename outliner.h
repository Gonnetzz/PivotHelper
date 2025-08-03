#pragma once
#include "datatypes.h"

namespace Outliner {
    void Render(Node* root, Node*& selectedNode, Node*& dragDropSource, Node*& dragDropTarget, Node*& nodeToDelete, Node*& nodeToAddChildTo);
}