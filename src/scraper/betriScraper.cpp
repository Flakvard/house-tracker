// betriScraper.cpp
#include <betriScraper.hpp>
#include <ctime>
#include <filesystem.hpp>
#include <fstream>
#include <iostream>
#include <jsonHelper.hpp>
#include <nlohmann/json.hpp>
#include <parser.hpp>
#include <scraper.hpp>

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

// Convert entire property list to JSON array
nlohmann::json propertiesToJson(const std::vector<Property> &props) {
  nlohmann::json arr = nlohmann::json::array();
  for (auto &p : props) {
    arr.push_back(HT::propertyToJson(p));
  }
  return arr;
}

// Convert JSON array to entire property list
std::vector<Property> jsonToProperties(const nlohmann::json &arr) {
  std::vector<Property> props;
  if (!arr.is_array()) {
    return props;
  }
  for (auto &item : arr) {
    props.push_back(HT::jsonToProperty(item));
  }
  return props;
}

std::string downloadAndSaveHtml(const std::string &url) {
  // 1) Download
  std::string html = HT::downloadHtml(url);
  if (html.empty()) {
    std::cerr << "Download failed\n";
    return "";
  }

  // 2) Build a new timestamped filename
  std::string filePath = HT::makeTimestampedFilename();

  // 3) Save the raw HTML (plus any metadata) into a JSON file
  nlohmann::json j;
  j["url"] = url;
  j["timestamp"] = std::time(nullptr); // or store as string
  j["html"] = html;

  std::ofstream ofs(filePath);
  if (!ofs.is_open()) {
    std::cerr << "Error opening file: " << filePath << std::endl;
    return html; // At least we have the HTML in memory
  }

  ofs << j.dump(4);
  ofs.close();

  std::cout << "Saved raw HTML to: " << filePath << "\n";
  return html;
}

int betriRun() {

  // 1. Try to load HTML from "../src/raw_html/html_1.json", or download if
  // missing
  const std::string url = "https://www.betriheim.fo/";
  // std::string html = loadHtmlFromCacheOrDownload(url, cachePath);
  std::string html = downloadAndSaveHtml(url);

  // 1. Prepare an "existing properties" vector
  //    (Load from properties.json if it exists)
  std::vector<Property> allProperties;
  {
    std::ifstream ifs("../src/storage/properties.json");
    if (ifs.is_open()) {
      nlohmann::json j;
      ifs >> j;
      allProperties = jsonToProperties(j);
      ifs.close();
    } else {
      std::cout << "No existing properties.json found; starting fresh.\n";
    }
  }
  // 2. Gather all timestamped .json files from ../src/raw_html
  std::string rawHtmlDir = "../src/raw_html";
  std::vector<std::filesystem::path> htmlFiles =
      HT::gatherJsonFiles(rawHtmlDir);
  if (htmlFiles.empty()) {
    std::cerr << "No JSON files found in " << rawHtmlDir << "\n";
    return 0;
  }

  // 3. For each file, read the JSON, extract "html", parse with Gumbo, merge
  for (const auto &path : htmlFiles) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
      std::cerr << "Failed to open " << path << "\n";
      continue;
    }

    nlohmann::json j;
    ifs >> j;
    ifs.close();

    // If your JSON structure is: { "url": "...", "timestamp": ..., "html":
    // "..." }
    std::string rawHtml = j.value("html", "");
    if (rawHtml.empty()) {
      std::cerr << "No HTML found in " << path << "\n";
      continue;
    }

    // Parse
    std::vector<Property> newProperties = HT::parseHtmlWithGumbo(rawHtml);

    // Merge
    HT::mergeProperties(allProperties, newProperties);

    std::cout << "Processed file: " << path.filename().string() << " => found "
              << newProperties.size() << " properties.\n";
  }

  // 4. Write final results to properties.json
  nlohmann::json finalJson = propertiesToJson(allProperties);
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
