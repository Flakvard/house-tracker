#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <scrapers/betri/betriParser.hpp>
#include <scrapers/betri/betriScraper.hpp>
#include <scrapers/include/PropertyManager.hpp>
#include <scrapers/include/filesystem.hpp>
#include <scrapers/include/house_model.hpp>
#include <scrapers/include/parser.hpp>
#include <scrapers/include/regexParser.hpp>
#include <scrapers/include/scraper.hpp>
#include <scrapers/meklarin/meklarinParser.hpp>
#include <scrapers/meklarin/meklarinScraper.hpp>
#include <scrapers/skyn/skynParser.hpp>
#include <scrapers/skyn/skynScraper.hpp>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace HT {
namespace {

std::string trim(const std::string &s) {
  const auto first = s.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }
  const auto last = s.find_last_not_of(" \t\r\n");
  return s.substr(first, last - first + 1);
}

std::string formatTimestampAsDate(long long timestamp) {
  if (timestamp <= 0) {
    return "";
  }

  std::time_t t = static_cast<std::time_t>(timestamp);
  std::tm tm{};
#ifdef _WIN32
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d");
  return oss.str();
}

void normalizeBetriCityAndAddress(Property &prop) {
  if (prop.agent != RealEstateAgent::Betri) {
    return;
  }

  std::string address = prop.address;
  std::string city = prop.city;

  for (char &c : address) {
    if (c == '\r' || c == '\n' || c == '\t') {
      c = ' ';
    }
  }
  address = std::regex_replace(address, std::regex("\\s+"), " ");
  address = trim(address);
  prop.address = address;

  // If city is empty, extract "<postnum> <city>" from the end of address.
  if (city.empty()) {
    static const std::regex fromAddress(R"(^(.*)\s+(\d{3})\s+(.+)$)");
    std::smatch m;
    if (std::regex_match(address, m, fromAddress)) {
      const std::string street = trim(m[1].str());
      const std::string postNum = trim(m[2].str());
      const std::string cityName = trim(m[3].str());
      prop.address = street.empty() ? address : street;
      if (prop.postNum.empty()) {
        prop.postNum = postNum;
      }
      prop.city = cityName;
      return;
    }
  }

  // If city is "123 Tórshavn", keep only "Tórshavn" and preserve postnum.
  static const std::regex cityWithPost(R"(^(\d{3})\s+(.+)$)");
  std::smatch cm;
  if (std::regex_match(city, cm, cityWithPost)) {
    const std::string postNum = trim(cm[1].str());
    const std::string cityName = trim(cm[2].str());
    if (prop.postNum.empty()) {
      prop.postNum = postNum;
    }
    prop.city = cityName;
  } else {
    prop.city = trim(city);
  }
}

} // namespace

std::string PropertyManager::propertyAgentToString(RealEstateAgent agent) {
  switch (agent) {
  case RealEstateAgent::Betri:
    return "Betri";
  case RealEstateAgent::Meklarin:
    return "Meklarin";
  case RealEstateAgent::Ogn:
    return "Ogn";
  case RealEstateAgent::Skyn:
    return "Skyn";
  default:
    return "Undefined";
  }
}

std::string PropertyManager::propertyTypeToString(PropertyType type) {
  switch (type) {
  case PropertyType::Sethus:
    return "Sethus";
  case PropertyType::Tvihus:
    return "Tvihus";
  case PropertyType::Radhus:
    return "Radhus";
  case PropertyType::Ibud:
    return "Ibud";
  case PropertyType::Summarhus:
    return "Summarhus";
  case PropertyType::Vinnubygningur:
    return "Vinnubygningur";
  case PropertyType::Grundstykki:
    return "Grundstykki";
  case PropertyType::Jord:
    return "Jord";
  case PropertyType::Neyst:
    return "Neyst";
  default:
    return "Undefined";
  }
}

// Sethús
// Tvíhús
// Raðhús
// Íbúð
// Vinnubygningur
// Samanbygd hús
// Grundstykki, Neyst
// Grundstykki
// Jørð
// Neyst
// Vinnuhøli
// Handil, Vinnubygningur
std::string PropertyManager::extractPropertyTypeMeklarin(const std::string &s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 ::tolower); // make lowercase

  PropertyType typeOfProperty = PropertyType::Undefined;

  if (lower.find("sethús") != std::string::npos)
    typeOfProperty = PropertyType::Sethus;
  if (lower.find("tvíhús") != std::string::npos)
    typeOfProperty = PropertyType::Tvihus;
  if (lower.find("raðhús") != std::string::npos)
    typeOfProperty = PropertyType::Radhus;
  if (lower.find("íbúð") != std::string::npos)
    typeOfProperty = PropertyType::Ibud;
  if (lower.find("samanbygd hús") != std::string::npos ||
      lower.find("summarhus") != std::string::npos)
    typeOfProperty = PropertyType::Summarhus;
  if (lower.find("vinnubygningur") != std::string::npos ||
      lower.find("vinnuhøli") != std::string::npos ||
      lower.find("handil") != std::string::npos)
    typeOfProperty = PropertyType::Vinnubygningur;
  if (lower.find("grundstykki") != std::string::npos)
    typeOfProperty = PropertyType::Grundstykki;
  if (lower.find("jørð") != std::string::npos ||
      lower.find("jord") != std::string::npos)
    typeOfProperty = PropertyType::Jord;
  if (lower.find("neyst") != std::string::npos)
    typeOfProperty = PropertyType::Neyst;

  return propertyTypeToString(typeOfProperty);
}

RealEstateAgent PropertyManager::stringToAgent(const std::string &str) {
  static const std::unordered_map<std::string, RealEstateAgent> map = {
      {"Betri", RealEstateAgent::Betri},
      {"Meklarin", RealEstateAgent::Meklarin},
      {"Ogn", RealEstateAgent::Ogn},
      {"Skyn", RealEstateAgent::Skyn},
      {"Undefined", RealEstateAgent::Undefined}};

  auto it = map.find(str);
  if (it != map.end())
    return it->second;
  return RealEstateAgent::Undefined;
  // throw std::invalid_argument("Invalid property type string: " + str);
}

PropertyType PropertyManager::stringToPropertyType(const std::string &str) {
  static const std::unordered_map<std::string, PropertyType> map = {
      {"Sethus", PropertyType::Sethus},
      {"Tvihus", PropertyType::Tvihus},
      {"Radhus", PropertyType::Radhus},
      {"Ibud", PropertyType::Ibud},
      {"Summarhus", PropertyType::Summarhus},
      {"Vinnubygningur", PropertyType::Vinnubygningur},
      {"Grundstykki", PropertyType::Grundstykki},
      {"Jord", PropertyType::Jord},
      {"Neyst", PropertyType::Neyst},
      {"Undefined", PropertyType::Undefined}};

  auto it = map.find(str);
  if (it != map.end())
    return it->second;
  return PropertyType::Undefined;
  // throw std::invalid_argument("Invalid property type string: " + str);
}

// Compare two properties by ID (or address, or whichever unique key you choose)
bool PropertyManager::isSameProperty(const Property &a, const Property &b) {
  return a.id == b.id;
}

// Merges new properties into existing, tracking price changes
void PropertyManager::mergeProperties(std::vector<Property> &existing,
                                      const std::vector<Property> &newOnes) {
  for (const auto &newProp : newOnes) {
    // 1) Find match in existing
    auto it =
        std::find_if(existing.begin(), existing.end(), [&](const Property &ex) {
          return isSameProperty(ex, newProp);
        });

    if (it == existing.end()) {
      // property not found => new property
      std::cout << "Adding new property: " << newProp.address << "\n";
      existing.push_back(newProp);
    } else {
      // property found => check if price changed
      if (it->type != newProp.type) {
      //  std::cout << "Type changed for: " << it->id << " from "
      //            << propertyTypeToString(it->type) << " to "
      //            << propertyTypeToString(newProp.type) << "\n";
        // update type
        it->type = newProp.type;
      }
      if (it->price < newProp.price) {
        std::cout << "Price changed for: " << it->address << " from "
                  << it->price << " to " << newProp.price << "\n";

        // update the current price
        it->price = newProp.price;
      }
      if (it->latestOffer < newProp.latestOffer) {
        std::cout << "Price changed for: " << it->address << " from "
                  << it->latestOffer << " to " << newProp.latestOffer << "\n";

        // push the old price into the price history
        it->previousPrices.push_back(it->latestOffer);
        // update the current price
        it->latestOffer = newProp.latestOffer;
      }
      // property found => check if agent changed
      if (it->agent != newProp.agent) {
        std::cout << "Agent changed for: " << it->id << " from "
                  << propertyAgentToString(it->agent) << " to "
                  << propertyAgentToString(newProp.agent) << "\n";
        // update Agent
        it->agent = newProp.agent;
      }
      // property found => check if img changed
      if (it->img != newProp.img) {
      //  std::cout << "img changed for: " << it->id << " from " << it->img
      //            << " to " << newProp.img << "\n";
        // update img
        it->img = newProp.img;
      }
      // property found => check if agent changed
      if (it->city != newProp.city) {
        std::cout << "City changed for: " << it->id << " from " << it->city
                  << " to " << newProp.city << "\n";
        // update city
        it->city = newProp.city;
      }
      if (newProp.buildingSize > 0 && it->buildingSize != newProp.buildingSize) {
        it->buildingSize = newProp.buildingSize;
      }
      if (newProp.landSize > 0 && it->landSize != newProp.landSize) {
        it->landSize = newProp.landSize;
      }
      // you can compare other fields (e.g., floors, rooms) similarly
    }
  }
}

std::vector<Property>
mapRawPropertiesTilProperties(std::vector<RawProperty> newProperties) {

  std::vector<Property> properties;
  for (const auto &newProp : newProperties) {
    Property prop;

    prop.id = stripOuterQuotes(newProp.id);
    prop.website = stripOuterQuotes(newProp.website);
    prop.address = stripOuterQuotes(newProp.address);
    prop.city = stripOuterQuotes(newProp.city);
    prop.price = parsePriceToInt(newProp.price);
    prop.previousPrices = parsePreviousPrices(newProp.previousPrices);
    prop.latestOffer = parsePriceToInt(newProp.latestOffer);
    prop.validDate = stripOuterQuotes(newProp.validDate);
    prop.date = stripOuterQuotes(newProp.date);
    prop.buildingSize = parseAreaToInt(newProp.buildingSize);
    prop.landSize = parseAreaToInt(newProp.landSize);
    prop.room = parseInt(newProp.room);
    prop.floor = parseInt(newProp.floor);
    prop.img = stripOuterQuotes(newProp.img);
    prop.status = "active";
    prop.type = PropertyManager::stringToPropertyType(newProp.type);
    prop.agent = PropertyManager::stringToAgent(newProp.agent);
    properties.push_back(prop);
  }

  return properties;
}

void PropertyManager::traverseAllHtmlAndMergeProperties(
    std::vector<Property> &allProperties,
    std::vector<std::filesystem::path> htmlFiles) {
  std::unordered_map<std::string, long long> latestTimestampByUrl;
  std::unordered_map<std::string, std::filesystem::path> latestPathByUrl;
  std::unordered_map<std::string, long long> firstSeenTimestampById;
  std::unordered_map<std::string, long long> lastSeenTimestampById;

  for (const auto &path : htmlFiles) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
      continue;
    }
    nlohmann::json j;
    try {
      ifs >> j;
    } catch (...) {
      continue;
    }
    const std::string website = j.value("url", "");
    const long long timestamp = j.value("timestamp", 0LL);
    if (website.empty()) {
      continue;
    }

    auto it = latestTimestampByUrl.find(website);
    if (it == latestTimestampByUrl.end() || timestamp >= it->second) {
      latestTimestampByUrl[website] = timestamp;
      latestPathByUrl[website] = path;
    }
  }

  std::unordered_set<std::string> activePropertyIds;

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
    PropertyType propType =
        PropertyManager::stringToPropertyType(j.value("type", ""));

    std::string website = j.value("url", "");
    const long long timestamp = j.value("timestamp", 0LL);
    // if (website != url || website.empty())
    //   continue;

    std::string rawHtml = j.value("html", "");
    if (rawHtml.empty()) {
      std::cerr << "No HTML found in " << path << "\n";
      continue;
    }

    std::vector<RawProperty> newRawProperties;
    // Parse
    size_t betriFound = website.find("betriheim");
    if (betriFound != std::string::npos)
      newRawProperties = HT::BETRI::parseHtmlWithGumboBetri(rawHtml, propType);

    size_t meklarinFound = website.find("meklarin");
    if (meklarinFound != std::string::npos)
      newRawProperties = HT::MEKLARIN::parseWithGumboMeklarin(rawHtml);

    size_t skynFound = website.find("skyn");
    if (skynFound != std::string::npos)
      newRawProperties = HT::SKYN::parseWithGumboSkyn(rawHtml, propType);

    std::vector<Property> newProperties =
        mapRawPropertiesTilProperties(newRawProperties);

    for (const auto &prop : newProperties) {
      auto seenIt = firstSeenTimestampById.find(prop.id);
      if (seenIt == firstSeenTimestampById.end() || timestamp < seenIt->second) {
        firstSeenTimestampById[prop.id] = timestamp;
      }
      auto lastSeenIt = lastSeenTimestampById.find(prop.id);
      if (lastSeenIt == lastSeenTimestampById.end() ||
          timestamp > lastSeenIt->second) {
        lastSeenTimestampById[prop.id] = timestamp;
      }
    }

    auto latestIt = latestPathByUrl.find(website);
    if (latestIt != latestPathByUrl.end() && path == latestIt->second) {
      for (const auto &prop : newProperties) {
        activePropertyIds.insert(prop.id);
      }
    }

    // Merge
    PropertyManager::mergeProperties(allProperties, newProperties);

    //std::cout << "Processed file: " << path.filename().string() << " => found "
    //          << newProperties.size() << " properties.\n";
  }

  for (auto &prop : allProperties) {
    normalizeBetriCityAndAddress(prop);
    if (prop.addedDate.empty()) {
      auto firstSeenIt = firstSeenTimestampById.find(prop.id);
      if (firstSeenIt != firstSeenTimestampById.end()) {
        prop.addedDate = formatTimestampAsDate(firstSeenIt->second);
      }
    }
    if (activePropertyIds.find(prop.id) != activePropertyIds.end()) {
      prop.status = "active";
      prop.archivedDate.clear();
    } else {
      prop.status = "archived";
      auto lastSeenIt = lastSeenTimestampById.find(prop.id);
      if (lastSeenIt != lastSeenTimestampById.end()) {
        prop.archivedDate = formatTimestampAsDate(lastSeenIt->second);
      }
    }
  }
}

std::string PropertyManager::cleanId(const std::string &raw) {
  std::string result;
  for (char c : raw) {
    // Convert to lowercase
    c = std::tolower(static_cast<unsigned char>(c));

    // Keep only a–z, 0–9 (and optionally special letters like æ, ø, å)
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        (static_cast<unsigned char>(c) >= 0xE0)) { // æøå in UTF-8
      result += c;
    }

    // All else is skipped: whitespace, punctuation, symbols
  }
  return result;
}

int PropertyManager::runPropertyParsers(bool downloadNewHtml) {

  HT::betriRun(downloadNewHtml);
  HT::meklarinRun(downloadNewHtml);
  HT::skynRun(downloadNewHtml);

  std::vector<Property> allProperties = HT::getAllPropertiesFromJson();

  std::string rawHtmlDir = "../src/raw_html";

  std::vector<std::filesystem::path> htmlFiles =
      HT::gatherJsonFiles(rawHtmlDir);

  if (htmlFiles.empty()) {
    std::cerr << "No JSON files found in " << rawHtmlDir << "\n";
    return 0;
  }

  PropertyManager::traverseAllHtmlAndMergeProperties(allProperties, htmlFiles);

  HT::writeToPropertiesJsonFile(allProperties);
  HT::checkAndDownloadImages(allProperties);
  return 0;
}

} // namespace HT
