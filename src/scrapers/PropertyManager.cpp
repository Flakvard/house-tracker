#include "scrapers/house_model.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <scrapers/PropertyManager.hpp>
#include <scrapers/parser.hpp>
#include <scrapers/regexParser.hpp>
#include <unordered_map>
#include <vector>

namespace HT {

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
      if (it->price != newProp.price) {
        std::cout << "Price changed for: " << it->address << " from "
                  << it->price << " to " << newProp.price << "\n";

        // push the old price into the price history
        it->previousPrices.push_back(it->price);
        // update the current price
        it->price = newProp.price;
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
    if (website != url || website.empty())
      continue;

    std::string rawHtml = j.value("html", "");
    if (rawHtml.empty()) {
      std::cerr << "No HTML found in " << path << "\n";
      continue;
    }

    std::vector<RawProperty> newRawProperties;
    // Parse
    if (url == "https://www.betriheim.fo/")
      newRawProperties = HT::parseHtmlWithGumboBetri(rawHtml, propType);
    if (propType == PropertyType::Vinnubygningur)
      std::cout << "Here it is\n";

    if (url == "https://www.meklarin.fo/")
      newRawProperties = HT::parseWithGumboMeklarin(rawHtml);

    std::vector<Property> newProperties =
        mapRawPropertiesTilProperties(newRawProperties);

    // Merge
    PropertyManager::mergeProperties(allProperties, newProperties);

    std::cout << "Processed file: " << path.filename().string() << " => found "
              << newProperties.size() << " properties.\n";
  }
}

} // namespace HT
