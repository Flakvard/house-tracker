
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <scrapers/include/PropertyManager.hpp>
#include <scrapers/include/filesystem.hpp>
#include <scrapers/include/house_model.hpp>
#include <scrapers/include/scraper.hpp>

namespace HT {

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

// Download HTML via libcurl
std::string downloadHtml(const std::string &url) {
  CURL *curl;
  CURLcode res;
  std::string html;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Failed to init curl\n";
    return "";
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
              << std::endl;
    html.clear(); // Indicate failure
  }

  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return html;
}

std::string downloadAndSaveHtml(const std::string &url) {

  PropertyType pt = PropertyType::Undefined;
  return downloadAndSaveHtml(url, pt);
}

std::string downloadAndSaveHtml(const std::string &url, PropertyType propType) {
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
  j["type"] = PropertyManager::propertyTypeToString(propType);
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
} // namespace HT
