#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <scrapers/jsonHelper.hpp>
#include <scrapers/regexParser.hpp>

namespace HT {

int safeGetInt(const nlohmann::json &j, const std::string &key,
               int defaultValue = 0) {
  if (!j.contains(key) || j[key].is_null()) {
    return defaultValue;
  }

  try {
    if (j[key].is_number_integer()) {
      return j[key].get<int>();
    }
    if (j[key].is_string()) {
      std::string val = j[key].get<std::string>();
      // Strip non-digit characters just in case
      std::string digitsOnly;
      for (char c : val) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
          digitsOnly += c;
        }
      }
      return digitsOnly.empty() ? defaultValue : std::stoi(digitsOnly);
    }
  } catch (...) {
    // fallback on any unexpected errors
  }

  return defaultValue;
}

// Convert a single Property to JSON
nlohmann::json propertyToJson(const Property &prop) {
  nlohmann::json j;
  j["id"] = prop.id;
  j["website"] = prop.website;
  j["address"] = prop.address;
  j["price"] = prop.price;
  j["previousPrices"] = prop.previousPrices;
  j["latestOffer"] = prop.latestOffer;
  j["validDate"] = prop.validDate;
  j["yearBuilt"] = prop.date;
  j["insideM2"] = prop.buildingSize;
  j["landM2"] = prop.landSize;
  j["rooms"] = prop.room;
  j["floors"] = prop.floor;
  j["img"] = prop.img;
  return j;
}

// Convert JSON to a single Property
Property jsonToProperty(const nlohmann::json &j) {
  Property p;
  p.id = j.value("id", "");
  p.website = j.value("website", "");
  p.address = j.value("address", "");
  p.price = safeGetInt(j, "price");

  if (j.contains("previousPrices") && j["previousPrices"].is_array()) {
    p.previousPrices = parsePreviousPrices(j["previousPrices"]);
  }

  p.latestOffer = safeGetInt(j, "latestOffer");
  p.validDate = j.value("validDate", "");
  p.date = j.value("yearBuilt", "");

  p.buildingSize = safeGetInt(j, "insideM2");
  p.landSize = safeGetInt(j, "landM2");
  p.room = safeGetInt(j, "room");
  p.floor = safeGetInt(j, "floors");

  p.img = j.value("img", "");
  return p;
}

// Convert entire property list to JSON array
nlohmann::json propertiesToJson(const std::vector<Property> &props) {
  nlohmann::json arr = nlohmann::json::array();
  for (auto &p : props) {
    arr.push_back(HT::propertyToJson(p));
  }
  return arr;
}

// Convert JSON array to entire property list
std::vector<Property> jsonToProperties(const nlohmann::json &arr) {
  std::vector<Property> props;
  if (!arr.is_array()) {
    return props;
  }
  for (auto &item : arr) {
    props.push_back(HT::jsonToProperty(item));
  }
  return props;
}

} // namespace HT
