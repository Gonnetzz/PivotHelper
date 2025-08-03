#pragma once
#include "datatypes.h"
#include <string>
#include <map>

namespace Export {
    void SaveToFile(const std::string& filePath, const Node* root, const std::map<std::string, Sprite>& sprites, std::string& successMessage, std::string& errorMessage);
}