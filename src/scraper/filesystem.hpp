// filesystem.hpp
#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace HT {
namespace fs = std::filesystem;
std::vector<fs::path> gatherJsonFiles(const std::string &dir);
std::string makeTimestampedFilename();
} // namespace HT
