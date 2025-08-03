#pragma once
#include "datatypes.h"
#include <string>

namespace SpriteEditor {
    void Render(SpriteData* spriteData, std::string& successMessage, std::string& errorMessage);
}