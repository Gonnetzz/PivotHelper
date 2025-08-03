#include "environment.h"
#include <fstream>
#include <vector>
#include <string>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <algorithm>
}

namespace Environment {
    namespace {
        std::string g_baseFortsPath;
        std::string g_dynamicPath;
        std::vector<std::string> g_searchPaths;
        std::string g_startupMessage;

        void RebuildSearchPaths() {
            g_searchPaths.clear();
            if (!g_dynamicPath.empty()) {
                g_searchPaths.push_back(g_dynamicPath);
            }
            if (!g_baseFortsPath.empty()) {
                g_searchPaths.push_back(g_baseFortsPath);
            }
        }
    }

    void Initialize() {
        const char* configFile = "env.lua";
        lua_State* L = luaL_newstate();

        if (luaL_dofile(L, configFile) == LUA_OK) {
            lua_getglobal(L, "FortsPath");
            if (lua_isstring(L, -1)) {
                g_baseFortsPath = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
        }
        else {
            g_baseFortsPath = "C:/Program Files (x86)/Steam/steamapps/common/Forts/data";
            std::ofstream outFile(configFile);
            if (outFile.is_open()) {
                outFile << "-- Configuration for the Sprite Previewer\n";
                outFile << "-- Please ensure this path points to your Forts 'data' directory.\n";
                outFile << "FortsPath = \"" << g_baseFortsPath << "\"\n";
                outFile.close();
                g_startupMessage = "env.lua created. Please verify the FortsPath within it.";
            }
        }
        lua_close(L);
        RebuildSearchPaths();
    }

    void UpdateSearchPathsForFile(const std::string& scriptPath) {
        g_dynamicPath.clear();
        std::filesystem::path p(scriptPath);
        std::string pathStr = p.lexically_normal().string();
        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

        size_t pos;

        pos = pathStr.find("/Forts/data/mods/");
        if (pos != std::string::npos) {
            size_t mod_root_end = pathStr.find('/', pos + 18);
            if (mod_root_end != std::string::npos) {
                g_dynamicPath = pathStr.substr(0, mod_root_end);
            }
        }
        else {
            pos = pathStr.find("/workshop/content/410900/");
            if (pos != std::string::npos) {
                size_t workshop_item_end = pathStr.find('/', pos + 26);
                if (workshop_item_end != std::string::npos) {
                    g_dynamicPath = pathStr.substr(0, workshop_item_end);
                }
            }
        }
        RebuildSearchPaths();
    }

    const std::vector<std::string>& GetSearchPaths() {
        return g_searchPaths;
    }

    const std::string& GetStartupMessage() {
        return g_startupMessage;
    }
}