#pragma once

#include <GL/glew.h>
#include <imgui.h>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <set>

const std::vector<std::string> SUPPORTED_IMAGE_EXTENSIONS = {
    ".dds",
    ".png",
    ".tga"
};

constexpr ImU32 COLOR_HIERARCHY_FRONT = IM_COL32(100, 255, 100, 255);
constexpr ImU32 COLOR_HIERARCHY_BEHIND = IM_COL32(255, 180, 100, 255);
constexpr ImVec4 BG_COLOR = ImVec4(0.113f, 0.113f, 0.113f, 1.00f);
constexpr ImVec4 FRAME_BG_COLOR = ImVec4(0.188f, 0.188f, 0.188f, 1.00f);
constexpr ImVec4 WIDGET_BG_COLOR = ImVec4(0.227f, 0.227f, 0.227f, 1.00f);
constexpr ImVec4 TEXT_COLOR = ImVec4(0.878f, 0.878f, 0.878f, 1.00f);

struct Transform {
    ImVec2 position = { 0,0 };
    float angle_deg = 0;
    ImVec2 anchor_pos = { 0,0 };
};

struct SpriteFrame {
    GLuint textureId = 0;
    int width = 0;
    int height = 0;
    std::string texturePath;
};

struct SpriteState {
    std::vector<SpriteFrame> frames;
    bool isLink = false;
    std::string linkToStateName;
    float duration = 0.1f;
    bool mipmap = false;
    std::string nextState;
};

struct Sprite { std::string name; std::map<std::string, SpriteState> states; };

struct Node {
    std::string name, spriteName;
    const Sprite* sprite_ptr = nullptr;
    std::vector<std::unique_ptr<Node>> childrenBehind;
    std::vector<std::unique_ptr<Node>> childrenInFront;
    ImVec2 pivot = { 0, 0 };
    ImVec2 pivotOffset = { 0, 0 };
    float angle = 0;
};

struct SpriteData {
    std::map<std::string, Sprite> sprites;
    std::unique_ptr<Node> root;
    std::map<std::string, GLuint> loadedTextures;
    std::vector<std::string> allAvailableStates;
    std::string defaultState = "Normal";
};
struct CanvasState { float zoom = 1.0f; ImVec2 pan = { 0.0f, 0.0f }; };