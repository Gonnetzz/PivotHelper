#pragma once
#include <string>
#include <vector>
#include <filesystem>
namespace Environment {
	void Initialize();
	void UpdateSearchPathsForFile(const std::string& scriptPath);
	const std::vector<std::string>& GetSearchPaths();
	const std::string& GetStartupMessage();
}