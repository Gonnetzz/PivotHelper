#include "texture_loader.h"
#include "environment.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <il/il.h>
#include <IL/ilu.h>
#include <gli/gli.hpp>
#include <algorithm>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
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

    GLuint LoadTextureFromFile(const std::string& texture_relative_path, std::string& errorMessage) {
        std::filesystem::path found_path;
        std::filesystem::path original_path(texture_relative_path);

        found_path = find_absolute_path(texture_relative_path);

        if (found_path.empty()) {
            std::filesystem::path path_without_ext = original_path;
            path_without_ext.replace_extension();

            for (const auto& ext : SUPPORTED_IMAGE_EXTENSIONS) {
                std::filesystem::path new_path = path_without_ext;
                new_path += ext;
                found_path = find_absolute_path(new_path.string());
                if (!found_path.empty()) {
                    break;
                }
            }
        }

        if (found_path.empty()) {
            errorMessage = "Image not found: " + texture_relative_path;
            return 0;
        }
        
        std::string ext = found_path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // DevIL trolling, no dds for devil then
        if (ext == ".dds") {
            gli::texture texture = gli::load(found_path.string());
            if (texture.empty()) {
                errorMessage = "GLI Error: Failed to load DDS file " + found_path.string();
                return 0;
            }

            gli::gl GL(gli::gl::PROFILE_GL33);
            gli::gl::format const format = GL.translate(texture.format(), texture.swizzles());
            GLenum target = GL.translate(texture.target());

            GLuint textureID = 0;
            glGenTextures(1, &textureID);
            glBindTexture(target, textureID);

            glm::tvec3<GLsizei> const extent(texture.extent());

            glTexStorage2D(target, static_cast<GLint>(texture.levels()), format.Internal, extent.x, extent.y);
            for (std::size_t level = 0; level < texture.levels(); ++level)
            {
                glm::tvec3<GLsizei> level_extent(texture.extent(level));
                if (gli::is_compressed(texture.format())) {
                    glCompressedTexSubImage2D(target, static_cast<GLint>(level), 0, 0, level_extent.x, level_extent.y, format.Internal, static_cast<GLsizei>(texture.size(level)), texture.data(0, 0, level));
                }
                else {
                    glTexSubImage2D(target, static_cast<GLint>(level), 0, 0, level_extent.x, level_extent.y, format.External, format.Type, texture.data(0, 0, level));
                }
            }
            return textureID;
        }

        // For all other formats, use DevIL
        std::ifstream file(found_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            errorMessage = "Error: Could not open file " + found_path.string();
            return 0;
        }
        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        if (!file.read(buffer.data(), buffer.size())) {
            errorMessage = "Error: Could not read file " + found_path.string();
            return 0;
        }

        ILuint imageID;
        ilGenImages(1, &imageID); ilBindImage(imageID);
        if (!ilLoadL(IL_TYPE_UNKNOWN, buffer.data(), buffer.size())) {
            ILenum err = ilGetError();
            errorMessage = "DevIL Load Error " + std::to_string(err);
            ilDeleteImages(1, &imageID); return 0;
        }
        if (ilGetInteger(IL_IMAGE_ORIGIN) == IL_ORIGIN_LOWER_LEFT) iluFlipImage();
        if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE)) {
            ILenum err = ilGetError();
            errorMessage = "DevIL Convert Error " + std::to_string(err);
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
    if (path.empty()) {
        return 0;
    }
    if (spriteData.loadedTextures.count(path)) {
        return spriteData.loadedTextures.at(path);
    }

    GLuint newTextureId = LoadTextureFromFile(path, errorMessage);
    if (newTextureId != 0) {
        spriteData.loadedTextures[path] = newTextureId;
    }
    return newTextureId;
}