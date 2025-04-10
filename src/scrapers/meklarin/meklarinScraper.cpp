// meklarinScraper.cpp
#include <gumbo.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <scrapers/PropertyManager.hpp>
#include <scrapers/filesystem.hpp>
#include <scrapers/house_model.hpp>
#include <scrapers/meklarin/meklarinScraper.hpp>
#include <scrapers/parser.hpp>
#include <scrapers/regexParser.hpp>
#include <scrapers/scraper.hpp>
#include <string>
#include <vector>

namespace HT {

int meklarinRun(bool downloadNewHtml) {

  //  1) Download page into `html`
  std::string url = "https://www.meklarin.fo/";
  if (downloadNewHtml)
    std::string html = HT::downloadAndSaveHtml(url);

  std::vector<Property> allProperties = HT::getAllPropertiesFromJson();

  // 2. Gather all timestamped .json files from ../src/raw_html
  std::string rawHtmlDir = "../src/raw_html";

  std::vector<std::filesystem::path> htmlFiles =
      HT::gatherJsonFiles(rawHtmlDir);

  if (htmlFiles.empty()) {
    std::cerr << "No JSON files found in " << rawHtmlDir << "\n";
    return 0;
  }
  // 3. For each file, read the JSON, extract "html", parse with Gumbo,
  // merge
  PropertyManager::traverseAllHtmlAndMergeProperties(allProperties, htmlFiles,
                                                     url);

  HT::writeToPropertiesJsonFile(allProperties);
  return 0;
}

} // namespace HT
