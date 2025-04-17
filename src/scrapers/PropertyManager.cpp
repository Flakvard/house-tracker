#include <fstream>
#include <iostream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <scrapers/betri/betriParser.hpp>
#include <scrapers/include/PropertyManager.hpp>
#include <scrapers/include/house_model.hpp>
#include <scrapers/include/parser.hpp>
#include <scrapers/include/regexParser.hpp>
#include <scrapers/meklarin/meklarinParser.hpp>
#include <unordered_map>
#include <vector>

namespace HT {

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
        std::cout << "Type changed for: " << it->id << " from "
                  << propertyTypeToString(it->type) << " to "
                  << propertyTypeToString(newProp.type) << "\n";
        // update type
        it->type = newProp.type;
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
        std::cout << "img changed for: " << it->id << " from " << it->img
                  << " to " << newProp.img << "\n";
        // update img
        it->img = newProp.img;
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
    prop.price = parsePriceToInt(newProp.price);
    prop.previousPrices = parsePreviousPrices(newProp.previousPrices);
    prop.latestOffer = parsePriceToInt(newProp.latestOffer);
    prop.validDate = stripOuterQuotes(newProp.validDate);
    prop.date = stripOuterQuotes(newProp.date);
    prop.buildingSize = parseInt(newProp.buildingSize);
    prop.landSize = parseInt(newProp.landSize);
    prop.room = parseInt(newProp.room);
    prop.floor = parseInt(newProp.floor);
    prop.img = stripOuterQuotes(newProp.img);
    prop.type = PropertyManager::stringToPropertyType(newProp.type);
    prop.agent = PropertyManager::stringToAgent(newProp.agent);
    properties.push_back(prop);
  }

  return properties;
}

void PropertyManager::traverseAllHtmlAndMergeProperties(
    std::vector<Property> &allProperties,
    std::vector<std::filesystem::path> htmlFiles, std::string url) {
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

    std::vector<Property> newProperties =
        mapRawPropertiesTilProperties(newRawProperties);

    // Merge
    PropertyManager::mergeProperties(allProperties, newProperties);

    std::cout << "Processed file: " << path.filename().string() << " => found "
              << newProperties.size() << " properties.\n";
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

} // namespace HT
