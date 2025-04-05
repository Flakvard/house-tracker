#include <iostream>
#include <nlohmann/json.hpp>
#include <scrapers/jsonHelper.hpp>

namespace HT {

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
  p.price = j.value("price", "");
  // If "previousPrices" doesn't exist or is the wrong type, this won't blow up
  if (j.contains("previousPrices") && j["previousPrices"].is_array()) {
    for (auto &item : j["previousPrices"]) {
      p.previousPrices.push_back(item.get<std::string>());
    }
  }
  p.latestOffer = j.value("latestOffer", "");
  p.validDate = j.value("validDate", "");
  p.date = j.value("yearBuilt", "");
  p.buildingSize = j.value("insideM2", "");
  p.landSize = j.value("landM2", "");
  p.room = j.value("room", "");
  p.floor = j.value("floors", "");
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
