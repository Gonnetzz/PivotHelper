#pragma once
#include "datatypes.h"

namespace Actions {
    void Process(
        SpriteData* spriteData,
        Node*& selectedNode,
        Node*& dragDropSource,
        Node*& dragDropTarget,
        Node*& nodeToDelete,
        Node*& nodeToAddChildTo
    );
}