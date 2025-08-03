#include "file_handling.h"
#include "texture_loader.h"
#include "environment.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <algorithm>
#include <map>
#include <numeric>

#ifdef _WIN32
#include <windows.h>
#endif

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <unordered_map>
}

namespace {
    std::unique_ptr<Node> parse_node(lua_State* L, int index) {
        auto node = std::make_unique<Node>();
        lua_getfield(L, index, "Name"); if (lua_isstring(L, -1)) node->name = lua_tostring(L, -1); lua_pop(L, 1);
        lua_getfield(L, index, "Sprite"); if (lua_isstring(L, -1)) node->spriteName = lua_tostring(L, -1); lua_pop(L, 1);
        lua_getfield(L, index, "Pivot");
        if (lua_istable(L, -1)) {
            lua_rawgeti(L, -1, 1); node->pivot.x = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            lua_rawgeti(L, -1, 2); node->pivot.y = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        }
        lua_pop(L, 1);
        lua_getfield(L, index, "PivotOffset");
        if (lua_istable(L, -1)) {
            lua_rawgeti(L, -1, 1); node->pivotOffset.x = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            lua_rawgeti(L, -1, 2); node->pivotOffset.y = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        }
        lua_pop(L, 1);
        lua_getfield(L, index, "Angle");
        if (lua_isnumber(L, -1)) {
            node->angle = (float)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);

        auto parse_children = [&](const char* tableName, std::vector<std::unique_ptr<Node>>& target_vector) {
            lua_getfield(L, index, tableName);
            if (lua_istable(L, -1)) {
                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    if (lua_istable(L, -1)) target_vector.push_back(parse_node(L, lua_gettop(L)));
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
            };

        parse_children("ChildrenBehind", node->childrenBehind);
        parse_children("ChildrenInFront", node->childrenInFront);

        return node;
    }

    void resolve_sprite_pointers(Node* node, const std::map<std::string, Sprite>& sprites) {
        if (!node) return;
        auto it = sprites.find(node->spriteName);
        if (it != sprites.end()) node->sprite_ptr = &it->second;
        for (auto& child : node->childrenBehind) resolve_sprite_pointers(child.get(), sprites);
        for (auto& child : node->childrenInFront) resolve_sprite_pointers(child.get(), sprites);
    }

    struct CurrentPathGuard {
        std::filesystem::path original_path;
        CurrentPathGuard(const std::filesystem::path& path) {
            if (!path.empty() && std::filesystem::exists(path)) {
                original_path = std::filesystem::current_path();
                std::filesystem::current_path(path);
            }
        }
        ~CurrentPathGuard() {
            if (!original_path.empty()) {
                std::filesystem::current_path(original_path);
            }
        }
    };

    void FindAllStates(SpriteData& data) {
        if (data.sprites.empty()) return;
        std::set<std::string> allStates;
        for (const auto& sprite_pair : data.sprites) {
            for (const auto& state_pair : sprite_pair.second.states) {
                allStates.insert(state_pair.first);
            }
        }
        data.allAvailableStates.assign(allStates.begin(), allStates.end());
    }

    void parse_sprite_state(lua_State* L, SpriteState& spriteState, SpriteData& data, std::string& errorMessage) {
        lua_getfield(L, -1, "Frames");
        if (lua_istable(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                if (lua_istable(L, -1)) {
                    lua_getfield(L, -1, "texture");
                    if (lua_isstring(L, -1)) {
                        SpriteFrame frame;
                        frame.texturePath = lua_tostring(L, -1);
                        frame.textureId = TextureLoader::LoadOrGetTexture(frame.texturePath, data, errorMessage);
                        if (frame.textureId == 0) { /* error is already set */ }
                        glBindTexture(GL_TEXTURE_2D, frame.textureId);
                        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &frame.width);
                        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &frame.height);
                        spriteState.frames.push_back(frame);
                    }
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);
            }
            lua_getfield(L, -1, "mipmap");
            if (lua_isboolean(L, -1)) spriteState.mipmap = lua_toboolean(L, -1);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "duration");
        if (lua_isnumber(L, -1)) spriteState.duration = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "NextState");
        if (lua_isstring(L, -1)) spriteState.nextState = lua_tostring(L, -1);
        lua_pop(L, 1);
    }
}

void load_sprite_file(
    const std::string& script_relative_path,
    std::unique_ptr<SpriteData>& spriteData,
    std::string& errorMessage,
    std::string& successMessage,
    CanvasState& canvas) {

    errorMessage.clear();
    successMessage.clear();
    if (spriteData) {
        for (auto const& [key, val] : spriteData->loadedTextures) {
            glDeleteTextures(1, &val);
        }
    }

    Environment::UpdateSearchPathsForFile(script_relative_path);

    auto find_absolute = [&](const std::string& rel_path) -> std::filesystem::path {
        std::filesystem::path file_path(rel_path);
        if (file_path.is_absolute() && std::filesystem::exists(file_path)) return file_path.lexically_normal();
        for (const auto& base_str : Environment::GetSearchPaths()) {
            std::filesystem::path full_path = std::filesystem::path(base_str) / file_path;
            if (std::filesystem::exists(full_path = full_path.lexically_normal())) return full_path;
        }
        return "";
        };

    std::filesystem::path script_path = find_absolute(script_relative_path);
    if (script_path.empty()) {
        errorMessage = "Main script not found: " + script_relative_path;
        return;
    }

    std::filesystem::path base_path = Environment::GetSearchPaths().empty() ? "" : Environment::GetSearchPaths().front();

    CurrentPathGuard path_guard(base_path);
    auto data = std::make_unique<SpriteData>();
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_dofile(L, std::filesystem::relative(script_path, base_path).string().c_str()) != LUA_OK) {
        errorMessage = "Lua Error: " + std::string(lua_tostring(L, -1));
        lua_close(L);
        return;
    }

    lua_getglobal(L, "Sprites");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            if (!errorMessage.empty()) break;
            Sprite s;
            lua_getfield(L, -1, "Name"); s.name = lua_tostring(L, -1); lua_pop(L, 1);

            std::unordered_map<const void*, std::string> seenTables;

            lua_getfield(L, -1, "States");
            if (lua_istable(L, -1)) {
                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    const char* stateName = lua_tostring(L, -2);
                    int valIdx = lua_gettop(L);
                    const void* ptr = lua_istable(L, valIdx) ? lua_topointer(L, valIdx) : nullptr;

                    if (lua_isstring(L, valIdx)) {
                        SpriteState linkState;
                        linkState.isLink = true;
                        linkState.linkToStateName = lua_tostring(L, valIdx);
                        s.states[stateName] = linkState;
                    }
                    else if (lua_istable(L, valIdx)) {
                        SpriteState spriteState;
                        parse_sprite_state(L, spriteState, *data, errorMessage);
                        if (!errorMessage.empty()) { lua_close(L); return; }
                        s.states[stateName] = spriteState;
                        seenTables[ptr] = stateName;
                    }
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);

            if (s.states.find("Normal") == s.states.end()) {
                errorMessage = "Error: Sprite '" + s.name + "' is missing the required 'Normal' state.";
                lua_close(L); return;
            }
            data->sprites[s.name] = std::move(s);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    if (!errorMessage.empty()) {
        lua_close(L);
        return;
    }

    FindAllStates(*data);

    lua_getglobal(L, "Root");
    if (lua_istable(L, -1)) {
        data->root = parse_node(L, lua_gettop(L));
        resolve_sprite_pointers(data->root.get(), data->sprites);
    }
    lua_pop(L, 1);
    lua_close(L);

    size_t totalStates = 0;
    for (const auto& pair : data->sprites) {
        totalStates += pair.second.states.size();
    }
    successMessage = "Loaded " + std::to_string(data->sprites.size()) + " sprites with " + std::to_string(totalStates) + " total states successfully!";
    canvas.pan = { 0.0f, 0.0f };
    canvas.zoom = 1.0f;
    spriteData = std::move(data);
}
