// betriScraper.cpp
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <scrapers/betri/betriScraper.hpp>
#include <scrapers/include/PropertyManager.hpp>
#include <scrapers/include/filesystem.hpp>
#include <scrapers/include/house_model.hpp>
#include <scrapers/include/jsonHelper.hpp>
#include <scrapers/include/parser.hpp>
#include <scrapers/include/scraper.hpp>
#include <vector>

namespace HT {

int betriRun(bool downloadNewHtml) {

  // 1. Try to load HTML from "../src/raw_html/html_1.json", or download if
  // missing

  const std::string url = "https://www.betriheim.fo/";

  if (downloadNewHtml) {
    std::vector<std::pair<std::string, PropertyType>> urls = {
        {"https://www.betriheim.fo/api/properties/"
         "filter?area=&type=Seth%C3%BAs&skip=0,0",
         PropertyType::Sethus},
        {"https://www.betriheim.fo/api/properties/"
         "filter?area=&type=Tv%C3%ADh%C3%BAs&skip=0,0",
         PropertyType::Tvihus},
        {"https://www.betriheim.fo/api/properties/"
         "filter?area=&type=Ra%C3%B0h%C3%BAs%20/%20Randarh%C3%BAs&skip=0,0",
         PropertyType::Radhus},
        {"https://www.betriheim.fo/api/properties/"
         "filter?area=&type=%C3%8Db%C3%BA%C3%B0&skip=0,0",
         PropertyType::Ibud},
        {"https://www.betriheim.fo/api/properties/"
         "filter?area=&type=Summarh%C3%BAs%20/"
         "%20Fr%C3%ADt%C3%AD%C3%B0arh%C3%BAs&skip=0,0",
         PropertyType::Summarhus},
        {"https://www.betriheim.fo/api/properties/"
         "filter?area=&type=Vinnubygningur&skip=0,0",
         PropertyType::Vinnubygningur},
        {"https://www.betriheim.fo/api/properties/"
         "filter?area=&type=Grundstykki&skip=0,0",
         PropertyType::Grundstykki},
        {"https://www.betriheim.fo/api/properties/"
         "filter?area=&type=J%C3%B8r%C3%B0&skip=0,0",
         PropertyType::Jord},
        {"https://www.betriheim.fo/api/properties/"
         "filter?area=&type=Neyst&skip=0,0",
         PropertyType::Neyst}};

    for (const auto &[url, type] : urls) {
      std::string html =
          HT::downloadAndSaveHtml(url, type, RealEstateAgent::Betri);
    }
  }

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
  HT::writeToPropertiesJsonFile(allProperties);
  HT::checkAndDownloadImages(allProperties);
  return 0;
}
} // namespace HT
