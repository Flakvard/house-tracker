#include <iostream>
#include <scrapers/include/PropertyManager.hpp>
#include <scrapers/include/filesystem.hpp>
#include <scrapers/include/house_model.hpp>
#include <scrapers/include/scraper.hpp>
#include <scrapers/skyn/skynScraper.hpp>

namespace HT {
int skynRun(bool downloadNewHtml) {
  const std::string url = "https://www.skyn.fo/";

  if (downloadNewHtml) {
    std::vector<std::pair<std::string, PropertyType>> urls = {
        {"https://www.skyn.fo/"
         "ognir-til-soelu?fra=&til=&PropertyType=Seth%C3%BAs",
         PropertyType::Sethus},
        {"https://www.skyn.fo/"
         "ognir-til-soelu?fra=&til=&PropertyType=Tv%C3%ADh%C3%BAs",
         PropertyType::Tvihus},
        {"https://www.skyn.fo/"
         "ognir-til-soelu?fra=&til=&PropertyType=Tra%C3%B0ir",
         PropertyType::Jord},
        {"https://www.skyn.fo/"
         "ognir-til-soelu?fra=&til=&PropertyType=Vinnubygningar",
         PropertyType::Vinnubygningur},
        {"https://www.skyn.fo/"
         "ognir-til-soelu?fra=&til=&PropertyType=Grund%C3%B8ki",
         PropertyType::Grundstykki},
        {"https://www.skyn.fo/"
         "ognir-til-soelu?fra=&til=&PropertyType=J%C3%B8r%C3%B0",
         PropertyType::Jord},
        {"https://www.skyn.fo/ognir-til-soelu?fra=&til=&PropertyType=Neyst",
         PropertyType::Neyst}};

    for (const auto &[url, type] : urls) {
      std::string html =
          HT::downloadAndSaveHtml(url, type, RealEstateAgent::Skyn);
    }
  }
  return 0;
}
} // namespace HT
