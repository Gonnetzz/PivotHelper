#pragma once
#include "datatypes.h"
#include <string>

namespace TextureLoader {
    GLuint LoadOrGetTexture(const std::string& path, SpriteData& spriteData, std::string& errorMessage);
}