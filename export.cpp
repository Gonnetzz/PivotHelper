#include "export.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <iterator>

namespace {
    void GenerateNodeLua(const Node* node, int indentLevel, std::stringstream& ss);

    void GenerateSpritesLua(const std::map<std::string, Sprite>& sprites, std::stringstream& ss) {
        ss << "Sprites =\n{\n";
        for (auto sprite_it = sprites.begin(); sprite_it != sprites.end(); ++sprite_it) {
            const Sprite& sprite = sprite_it->second;
            ss << "\t{\n";
            ss << "\t\tName = \"" << sprite.name << "\",\n";
            ss << "\t\tStates =\n\t\t{\n";

            for (auto state_it = sprite.states.begin(); state_it != sprite.states.end(); ++state_it) {
                const auto& state_pair = *state_it;
                ss << "\t\t\t" << state_pair.first << " = ";
                if (state_pair.second.isLink) {
                    ss << state_pair.second.linkToStateName;
                }
                else {
                    ss << "{\n";
                    std::stringstream state_content_ss;
                    if (!state_pair.second.frames.empty()) {
                        state_content_ss << "\t\t\t\tFrames =\n\t\t\t\t{\n";
                        for (const auto& frame : state_pair.second.frames) {
                            state_content_ss << "\t\t\t\t\t{ texture = \"" << frame.texturePath << "\" },\n";
                        }
                        state_content_ss.seekp(-2, std::ios_base::end);
                        state_content_ss << "\n\t\t\t\t},\n";
                    }
                    if (state_pair.second.mipmap) state_content_ss << "\t\t\t\tmipmap = true,\n";
                    if (state_pair.second.duration > 0) state_content_ss << "\t\t\t\tduration = " << std::fixed << std::setprecision(4) << state_pair.second.duration << ",\n";
                    if (!state_pair.second.nextState.empty()) state_content_ss << "\t\t\t\tNextState = \"" << state_pair.second.nextState << "\",\n";

                    std::string content_str = state_content_ss.str();
                    if (!content_str.empty()) {
                        content_str.pop_back();
                        content_str.pop_back();
                    }
                    ss << content_str << "\n\t\t\t}";
                }
                if (std::next(state_it) != sprite.states.end()) ss << ",\n";
                else ss << "\n";
            }
            ss << "\t\t}\n\t}";
            if (std::next(sprite_it) != sprites.end()) ss << ",\n";
            else ss << "\n";
        }
        ss << "}\n";
    }

    void GenerateLuaForChildList(const std::vector<std::unique_ptr<Node>>& children, int indentLevel, std::stringstream& ss, const std::string& listName) {
        if (children.empty()) return;
        std::string t(indentLevel, '\t');
        ss << t << listName << " =\n" << t << "{\n";
        for (size_t i = 0; i < children.size(); ++i) {
            GenerateNodeLua(children[i].get(), indentLevel + 1, ss);
            if (i < children.size() - 1) ss << ",\n";
            else ss << "\n";
        }
        ss << t << "},\n";
    }

    void GenerateNodeLua(const Node* node, int indentLevel, std::stringstream& ss) {
        if (!node) return;
        std::string t(indentLevel, '\t');
        ss << t << "{\n";
        std::string t_inner = t + "\t";

        ss << std::fixed << std::setprecision(4);
        ss << t_inner << "Name = \"" << node->name << "\",\n";
        ss << t_inner << "Angle = " << node->angle << ",\n";
        ss << t_inner << "Pivot = { " << node->pivot.x << ", " << node->pivot.y << " },\n";
        ss << t_inner << "PivotOffset = { " << node->pivotOffset.x << ", " << node->pivotOffset.y << " },\n";
        if (node->sprite_ptr && !node->sprite_ptr->name.empty()) {
            ss << t_inner << "Sprite = \"" << node->sprite_ptr->name << "\",\n";
        }

        std::stringstream children_ss;
        GenerateLuaForChildList(node->childrenBehind, indentLevel + 1, children_ss, "ChildrenBehind");
        GenerateLuaForChildList(node->childrenInFront, indentLevel + 1, children_ss, "ChildrenInFront");

        std::string children_str = children_ss.str();
        if (!children_str.empty()) {
            children_str.pop_back();
            children_str.pop_back();
        }
        ss << children_str << "\n";
        ss << t << "}";
    }
}

void Export::SaveToFile(const std::string& filePath, const Node* root, const std::map<std::string, Sprite>& sprites, std::string& successMessage, std::string& errorMessage) {
    if (!root) {
        errorMessage = "Cannot export: No data loaded.";
        successMessage.clear();
        return;
    }
    std::stringstream ss;

    GenerateSpritesLua(sprites, ss);
    ss << "\n";
    ss << "Root =\n";
    GenerateNodeLua(root, 0, ss);

    std::ofstream outFile(filePath);
    if (!outFile.is_open()) {
        errorMessage = "Error: Could not open " + filePath + " for writing.";
        successMessage.clear();
        return;
    }
    outFile << ss.str();
    outFile.close();

    successMessage = "Successfully exported to " + filePath + "!";
    errorMessage.clear();
}