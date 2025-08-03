#pragma once

#include "datatypes.h"
#include <string>
#include <memory>

void load_sprite_file(
    const std::string& script_relative_path,
    std::unique_ptr<SpriteData>& spriteData,
    std::string& errorMessage,
    std::string& successMessage,
    CanvasState& canvas
);