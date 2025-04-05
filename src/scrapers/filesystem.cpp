#include <filesystem.hpp>
#include <fstream>
#include <house_model.hpp>
#include <iostream>
#include <jsonHelper.hpp>
#include <nlohmann/json.hpp>

namespace HTFS {
namespace fs = std::filesystem;
// Example function to gather and sort all .json files from a directory
std::vector<fs::path> gatherJsonFiles(const std::string &dir) {
  std::vector<fs::path> result;
  for (auto &entry : fs::directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      result.push_back(entry.path());
    }
  }

  // Sort them in ascending order by filename
  // (If your filenames are timestamped, this is effectively chronological.)
  std::sort(result.begin(), result.end());
  return result;
}

// Utility to get a string like "html_2025-04-03_14-00-00.json"
// You might prefer a shorter or simpler format
std::string makeTimestampedFilename() {
  using namespace std::chrono;

  // Get the current time and time zone
  auto now = system_clock::now();
  auto tz = current_zone();
  zoned_time zt{tz, now};

  // Format using std::format with chrono support (C++20)
  std::string timestamp = std::format("{:%Y-%m-%d_%H-%M-%S}", zt);

  return "../src/raw_html/html_" + timestamp + ".json";
}

std::vector<Property> getAllPropertiesFromJson() {
  std::vector<Property> allProperties;
  {
    std::ifstream ifs("../src/storage/properties.json");
    if (ifs.is_open()) {
      nlohmann::json j;
      ifs >> j;
      allProperties = HT::jsonToProperties(j);
      ifs.close();
    } else {
      std::cout << "No existing properties.json found; starting fresh.\n";
    }
  }
  return allProperties;
}

int writeToPropertiesJsonFile(std::vector<Property> allProperties) {
  nlohmann::json finalJson = HT::propertiesToJson(allProperties);
  std::ofstream ofs("../src/storage/properties.json");
  if (!ofs.is_open()) {
    std::cerr << "Failed to open ../src/storage/properties.json for writing!\n";
    return 1;
  }
  ofs << finalJson.dump(4);
  ofs.close();

  std::cout << "Wrote " << allProperties.size()
            << " total properties to properties.json\n";
  return 0;
}

} // namespace HTFS
