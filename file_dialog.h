#pragma once
#include <string>

#ifdef _WIN32
namespace NativeFileDialog {
    std::string OpenFile(const char* filter);
    std::string SaveFile(const char* filter);
}
#endif