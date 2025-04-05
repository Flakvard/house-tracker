// betriScraper.cpp
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <scrapers/PropertyManager.hpp>
#include <scrapers/betri/betriScraper.hpp>
#include <scrapers/filesystem.hpp>
#include <scrapers/jsonHelper.hpp>
#include <scrapers/parser.hpp>
#include <scrapers/scraper.hpp>
#include <vector>

namespace HT {

// Simple check if a file exists.
bool fileExists(const std::string &path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0);
}

/**
 * Load HTML from cache if it exists, or download from a URL and cache it.
 * filePath is something like "../src/raw_html/html_1.json"
 */
std::string loadHtmlFromCacheOrDownload(const std::string &url,
                                        const std::string &filePath) {
  if (fileExists(filePath)) {
    // If file exists, read JSON from disk
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
      std::cerr << "Error opening cache file: " << filePath << std::endl;
      return "";
    }

    nlohmann::json j;
    ifs >> j;
    ifs.close();

    // Optionally, you can do version checks, timestamps, etc. For simplicity:
    std::string cachedHtml = j.value("html", "");
    if (!cachedHtml.empty()) {
      std::cout << "[Cache] Using cached HTML from " << filePath << "\n";
      return cachedHtml;
    }
  }

  // Otherwise, download fresh
  std::cout << "[Download] Fetching fresh HTML from " << url << "\n";
  std::string freshHtml = HT::downloadHtml(url);
  if (freshHtml.empty()) {
    // Download failed; handle error accordingly
    return "";
  }

  // Save to JSON cache file
  nlohmann::json j;
  j["url"] = url;
  j["html"] = freshHtml;

  std::ofstream ofs(filePath);
  if (!ofs.is_open()) {
    std::cerr << "Error opening file for writing: " << filePath << std::endl;
    return freshHtml; // We have HTML, but we couldn't cache it
  }
  ofs << j.dump(4);
  ofs.close();

  return freshHtml;
}

int betriRun() {

  // 1. Try to load HTML from "../src/raw_html/html_1.json", or download if
  // missing
  const std::string url = "https://www.betriheim.fo/";
  // std::string html = loadHtmlFromCacheOrDownload(url, cachePath);
  std::string html = HT::downloadAndSaveHtml(url);

  // 1. Prepare an "existing properties" vector
  //    (Load from properties.json if it exists)
  std::vector<Property> allProperties = HT::getAllPropertiesFromJson();

  // 2. Gather all timestamped .json files from ../src/raw_html
  std::string rawHtmlDir = "../src/raw_html";

  std::vector<std::filesystem::path> htmlFiles =
      HT::gatherJsonFiles(rawHtmlDir);

  if (htmlFiles.empty()) {
    std::cerr << "No JSON files found in " << rawHtmlDir << "\n";
    return 0;
  }

  // 3. For each file, read the JSON, extract "html", parse with Gumbo, merge
  PropertyManager::traverseAllHtmlAndMergeProperties(allProperties, htmlFiles,
                                                     url);

  // 4. Write final results to properties.json
  return HT::writeToPropertiesJsonFile(allProperties);
}
} // namespace HT
