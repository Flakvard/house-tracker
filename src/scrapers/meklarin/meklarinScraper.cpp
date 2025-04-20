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
  return 0;
}

} // namespace HT
