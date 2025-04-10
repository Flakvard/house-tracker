// filesystem.hpp
#pragma once
#include <filesystem>
#include <scrapers/include/house_model.hpp>
#include <string>
#include <vector>

namespace HT {
namespace fs = std::filesystem;
std::vector<fs::path> gatherJsonFiles(const std::string &dir);
std::string makeTimestampedFilename();
std::vector<Property> getAllPropertiesFromJson();
int writeToPropertiesJsonFile(std::vector<Property> allProperties);
} // namespace HT
