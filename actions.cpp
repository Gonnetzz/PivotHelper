#include "actions.h"
#include <set>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>

namespace Actions {
    namespace {
        bool IsDescendant(Node* ancestor, Node* nodeToFind) {
            if (!ancestor || !nodeToFind) return false;
            for (const auto& child : ancestor->childrenInFront) {
                if (child.get() == nodeToFind || IsDescendant(child.get(), nodeToFind)) return true;
            }
            for (const auto& child : ancestor->childrenBehind) {
                if (child.get() == nodeToFind || IsDescendant(child.get(), nodeToFind)) return true;
            }
            return false;
        }

        Node* FindParentOf(Node* searchRoot, Node* nodeToFind, std::vector<std::unique_ptr<Node>>*& childListRef) {
            if (!searchRoot || !nodeToFind) return nullptr;
            for (auto& child : searchRoot->childrenInFront) {
                if (child.get() == nodeToFind) { childListRef = &searchRoot->childrenInFront; return searchRoot; }
                Node* found = FindParentOf(child.get(), nodeToFind, childListRef);
                if (found) return found;
            }
            for (auto& child : searchRoot->childrenBehind) {
                if (child.get() == nodeToFind) { childListRef = &searchRoot->childrenBehind; return searchRoot; }
                Node* found = FindParentOf(child.get(), nodeToFind, childListRef);
                if (found) return found;
            }
            return nullptr;
        }

        void CollectAllNodeNames(Node* node, std::set<std::string>& names) {
            if (!node) return;
            names.insert(node->name);
            for (const auto& child : node->childrenInFront) CollectAllNodeNames(child.get(), names);
            for (const auto& child : node->childrenBehind) CollectAllNodeNames(child.get(), names);
        }

        std::string GenerateUniqueNodeName(Node* root) {
            std::set<std::string> allNames;
            CollectAllNodeNames(root, allNames);
            std::string baseName = "Node";
            if (allNames.find(baseName) == allNames.end()) return baseName;
            int i = 1;
            while (true) {
                std::string newName = baseName + std::to_string(i);
                if (allNames.find(newName) == allNames.end()) return newName;
                i++;
            }
        }

        void HandleNodeOperations(SpriteData* spriteData, Node*& selectedNode, Node*& nodeToDelete, Node*& nodeToAddChildTo) {
            if (!spriteData || !spriteData->root) return;

            if (nodeToDelete) {
                std::vector<std::unique_ptr<Node>>* childList = nullptr;
                Node* parent = FindParentOf(spriteData->root.get(), nodeToDelete, childList);
                if (parent && childList) {
                    auto it = std::find_if(childList->begin(), childList->end(), [&](const auto& p) { return p.get() == nodeToDelete; });
                    if (it != childList->end()) {
                        std::unique_ptr<Node> toDelete = std::move(*it);
                        childList->erase(it);
                        parent->childrenInFront.insert(parent->childrenInFront.end(),
                            std::make_move_iterator(toDelete->childrenInFront.begin()),
                            std::make_move_iterator(toDelete->childrenInFront.end()));
                        parent->childrenBehind.insert(parent->childrenBehind.end(),
                            std::make_move_iterator(toDelete->childrenBehind.begin()),
                            std::make_move_iterator(toDelete->childrenBehind.end()));
                    }
                }
                if (selectedNode == nodeToDelete) selectedNode = nullptr;
                nodeToDelete = nullptr;
            }

            if (nodeToAddChildTo) {
                auto newNode = std::make_unique<Node>();
                newNode->name = GenerateUniqueNodeName(spriteData->root.get());
                nodeToAddChildTo->childrenInFront.push_back(std::move(newNode));
                nodeToAddChildTo = nullptr;
            }
        }

        void HandleDragDrop(SpriteData* spriteData, Node*& dragDropSource, Node*& dragDropTarget) {
            if (!dragDropSource || !dragDropTarget || !spriteData || !spriteData->root) return;
            if (dragDropSource == dragDropTarget || IsDescendant(dragDropSource, dragDropTarget)) {
                dragDropSource = nullptr;
                dragDropTarget = nullptr;
                return;
            }
            std::vector<std::unique_ptr<Node>>* sourceList = nullptr;
            Node* oldParent = FindParentOf(spriteData->root.get(), dragDropSource, sourceList);
            if (oldParent && sourceList) {
                auto it = std::find_if(sourceList->begin(), sourceList->end(), [&](const auto& p) { return p.get() == dragDropSource; });
                if (it != sourceList->end()) {
                    std::unique_ptr<Node> movedNode = std::move(*it);
                    sourceList->erase(it);
                    dragDropTarget->childrenInFront.push_back(std::move(movedNode));
                }
            }
            dragDropSource = nullptr;
            dragDropTarget = nullptr;
        }
    }

    void Process(SpriteData* spriteData, Node*& selectedNode, Node*& dragDropSource, Node*& dragDropTarget, Node*& nodeToDelete, Node*& nodeToAddChildTo) {
        HandleDragDrop(spriteData, dragDropSource, dragDropTarget);
        HandleNodeOperations(spriteData, selectedNode, nodeToDelete, nodeToAddChildTo);
    }
}