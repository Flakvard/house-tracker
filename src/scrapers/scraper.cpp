#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <scrapers/include/PropertyManager.hpp>
#include <scrapers/include/filesystem.hpp>
#include <scrapers/include/house_model.hpp>
#include <scrapers/include/scraper.hpp>

namespace HT {

enum class WriteMode { ToMemory, ToFile };

struct WriteData {
  WriteMode mode;
  std::string *outString; // if mode == ToMemory
  std::ofstream outFile;  // if mode == ToFile
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  size_t totalBytes = size * nmemb;
  WriteData *wd = static_cast<WriteData *>(userp);

  if (wd->mode == WriteMode::ToMemory) {
    // Append to the output string
    wd->outString->append(static_cast<char *>(contents), totalBytes);
  } else if (wd->mode == WriteMode::ToFile) {
    // Write to file
    wd->outFile.write(static_cast<char *>(contents), totalBytes);
  }

  return totalBytes;
}

std::string extractHtmlFromJson(const std::string &url) {

  // Try to parse it as JSON
  try {
    auto j = nlohmann::json::parse(url);

    // If it contains an "html" field, extract that
    if (j.contains("html") && j["html"].is_string()) {
      return j["html"].get<std::string>(); // Return HTML string from JSON
    }

    // JSON is valid, but doesn't contain "html"
    return url;
  } catch (const nlohmann::json::parse_error &e) {
    // Not valid JSON, treat as raw HTML
    std::cerr << "Parse error JSON" << e.what() << "\n";
    return url;
  } catch (const std::exception &e) {
    // Some other exception, just print/log
    std::cerr << "Exception while parsing JSON: " << e.what() << "\n";
    return url;
  }
}

// Download HTML via libcurl
bool downloadData(const std::string &url, WriteData &wd) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Failed to init curl\n";
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &wd);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
              << std::endl;
    curl_easy_cleanup(curl);
    return false;
  }

  curl_easy_cleanup(curl);
  return true;
}

bool downloadToString(const std::string &url, std::string &outString) {
  // Ensure the output is empty or in a known state
  outString.clear();

  // Prepare our WriteData struct to store data in memory
  WriteData wd;
  wd.mode = WriteMode::ToMemory;
  wd.outString = &outString;

  // Actually do the download
  return downloadData(url, wd);
}

std::string downloadAndSaveHtml(const std::string &url) {

  PropertyType pt = PropertyType::Undefined;
  auto agent = RealEstateAgent::Undefined;
  return downloadAndSaveHtml(url, pt, agent);
}
std::string downloadAndSaveHtml(const std::string &url, RealEstateAgent agent) {
  PropertyType pt = PropertyType::Undefined;
  return downloadAndSaveHtml(url, pt, agent);
}

bool downloadToFile(const std::string &url, const std::string &filePath) {
  WriteData wd;
  wd.mode = WriteMode::ToFile;

  // Open the file in binary mode
  wd.outFile.open(filePath, std::ios::binary);
  if (!wd.outFile.is_open()) {
    std::cerr << "Failed to open file for writing: " << filePath << "\n";
    return false;
  }

  bool success = downloadData(url, wd);
  wd.outFile.close();
  return success;
}

std::string downloadAndSaveHtml(const std::string &url, PropertyType propType,
                                RealEstateAgent agent) {
  // 1) Download
  std::string html;
  bool success = HT::downloadToString(url, html);
  extractHtmlFromJson(html);
  if (html.empty() || !success) {
    std::cerr << "Download failed\n";
    return "";
  }

  // 2) Build a new timestamped filename
  std::string filePath = HT::makeTimestampedFilename();

  // 3) Save the raw HTML (plus any metadata) into a JSON file
  nlohmann::json j;
  j["url"] = url;
  j["type"] = PropertyManager::propertyTypeToString(propType);
  j["agent"] = PropertyManager::propertyAgentToString(agent);
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

std::string cleanAsciiFilename(const std::string &filename) {
  std::string result;
  for (char c : filename) {
    if (static_cast<unsigned char>(c) < 128) {
      if (c == ' ')
        result += '_';
      else
        result += c;
    }
    // else skip non-ASCII chars like ð, ø, á
  }
  return result;
}

std::string getFilenameFromUrl(const std::string &url) {
  // just an example; in real code you might do robust checks
  // or use a cross-platform path library
  auto pos = url.find_last_of('/');
  if (pos == std::string::npos) {
    // fallback: everything is the filename if there's no slash
    return url;
  }
  // substring after the slash
  return url.substr(pos + 1);
}

bool alreadyDownloaded(const std::string &filename) {
  return std::filesystem::exists(filename);
}

void checkAndDownloadImages(const std::vector<Property> &allProperties) {
  std::string imgFolder = "../src/raw_images/";

  for (auto &prop : allProperties) {
    // Suppose prop.img is the URL string
    const std::string &imgUrl = prop.img;
    if (imgUrl.empty()) {
      continue; // no image
    }

    // 1) Generate local filename
    std::string filename = getFilenameFromUrl(imgUrl);
    std::string fullImgName = cleanAsciiFilename(prop.id + filename);
    std::string fullLocalPath = imgFolder + fullImgName;

    // 2) Check if file already exists
    if (alreadyDownloaded(fullLocalPath)) {
      continue; // skip
    }

    // 3) Download
    if (!downloadToFile(imgUrl, fullLocalPath)) {
      std::cerr << "Failed to download: " << imgUrl << "\n";
    } else {
      std::cout << "Downloaded: " << imgUrl << " => " << fullLocalPath << "\n";
    }
  }
}

} // namespace HT
