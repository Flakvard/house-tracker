// meklarinScraper.cpp
#include <gumbo.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <scrapers/include/PropertyManager.hpp>
#include <scrapers/include/filesystem.hpp>
#include <scrapers/include/house_model.hpp>
#include <scrapers/include/parser.hpp>
#include <scrapers/include/regexParser.hpp>
#include <scrapers/include/scraper.hpp>
#include <scrapers/meklarin/meklarinScraper.hpp>
#include <string>
#include <vector>

namespace HT {

int meklarinRun(bool downloadNewHtml) {

  //  1) Download page into `html`
  std::string url = "https://www.meklarin.fo/";
  if (downloadNewHtml)
    std::string html = HT::downloadAndSaveHtml(url, RealEstateAgent::Meklarin);

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
  // HT::checkAndDownloadImages(allProperties);
  return 0;
}

} // namespace HT
