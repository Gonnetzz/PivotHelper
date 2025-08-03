#include "texture_loader.h"
#include "environment.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <il/il.h>
#include <IL/ilu.h>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

    std::string wide_to_utf8(const std::wstring& wide_string) {
        if (wide_string.empty()) return std::string();
#ifdef _WIN32
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], (int)wide_string.size(), NULL, 0, NULL, NULL);
        std::string str_to(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], (int)wide_string.size(), &str_to[0], size_needed, NULL, NULL);
        return str_to;
#else
        return std::string(wide_string.begin(), wide_string.end());
#endif
    }

    std::filesystem::path find_absolute_path(const std::string& relative_path) {
        std::filesystem::path file_path(relative_path);
        if (file_path.empty()) return "";
        if (file_path.is_absolute() && std::filesystem::exists(file_path)) return file_path.lexically_normal();

        for (const auto& base_str : Environment::GetSearchPaths()) {
            std::filesystem::path full_path = std::filesystem::path(base_str) / file_path;
            if (std::filesystem::exists(full_path = full_path.lexically_normal())) return full_path;
        }
        return "";
    }

    ILenum get_il_type_from_path(const std::filesystem::path& path) {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".dds") return IL_DDS;
        if (ext == ".png") return IL_PNG;
        if (ext == ".tga") return IL_TGA;
        if (ext == ".bmp") return IL_BMP;
        if (ext == ".jpg" || ext == ".jpeg") return IL_JPG;
        return IL_TYPE_UNKNOWN;
    }

    GLuint LoadTextureFromFile(const std::string& texture_relative_path, std::string& errorMessage) {
        std::filesystem::path original_path(texture_relative_path);
        std::filesystem::path found_path = find_absolute_path(original_path.string());

        if (found_path.empty()) {
            std::filesystem::path stem = original_path.stem();
            std::filesystem::path parent_path = original_path.parent_path();
            for (const auto& ext : SUPPORTED_IMAGE_EXTENSIONS) {
                std::filesystem::path new_relative_path = parent_path / stem;
                new_relative_path += ext;
                found_path = find_absolute_path(new_relative_path.string());
                if (!found_path.empty()) break;
            }
        }

        if (found_path.empty()) {
            errorMessage = "Image not found: " + texture_relative_path;
            return 0;
        }

        std::ifstream file(found_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            errorMessage = "C++ Error: Could not open file " + found_path.string();
            return 0;
        }
        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        if (!file.read(buffer.data(), buffer.size())) {
            errorMessage = "C++ Error: Could not read file " + found_path.string();
            return 0;
        }

        ILuint imageID;
        ilGenImages(1, &imageID); ilBindImage(imageID);
        ILenum imageType = get_il_type_from_path(found_path);
        if (!ilLoadL(imageType, buffer.data(), buffer.size())) {
            ILenum err = ilGetError();
            errorMessage = "DevIL Load Error " + std::to_string(err) + ": " + wide_to_utf8(iluErrorString(err));
            ilDeleteImages(1, &imageID); return 0;
        }
        if (ilGetInteger(IL_IMAGE_ORIGIN) == IL_ORIGIN_LOWER_LEFT) iluFlipImage();
        if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE)) {
            ILenum err = ilGetError();
            errorMessage = "DevIL Convert Error " + std::to_string(err) + ": " + wide_to_utf8(iluErrorString(err));
            ilDeleteImages(1, &imageID); return 0;
        }

        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
        ilDeleteImages(1, &imageID);
        return textureId;
    }
}

GLuint TextureLoader::LoadOrGetTexture(const std::string& path, SpriteData& spriteData, std::string& errorMessage) {
    if (spriteData.loadedTextures.count(path)) {
        return spriteData.loadedTextures.at(path);
    }

    GLuint newTextureId = LoadTextureFromFile(path, errorMessage);
    if (newTextureId != 0) {
        spriteData.loadedTextures[path] = newTextureId;
    }
    return newTextureId;
}