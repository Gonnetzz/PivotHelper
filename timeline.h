#pragma once
#include "datatypes.h"
#include <string>

namespace Timeline {
    void Render(SpriteData* spriteData, std::string& activeState, bool& isPlaying, int& activeFrame, int& maxFrames, float& frameTimer);
}