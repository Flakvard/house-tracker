// jsonHelper.hpp
#pragma once
#include <nlohmann/json.hpp>
#include <scrapers/include/house_model.hpp>

namespace HT {

// Convert a single Property to JSON
nlohmann::json propertyToJson(const Property &prop);

// Convert JSON to a single Property
Property jsonToProperty(const nlohmann::json &j);

// Compare two properties by ID (or address, or whichever unique key you choose)
bool isSameProperty(const Property &a, const Property &b);

// Convert entire property list to JSON array
nlohmann::json propertiesToJson(const std::vector<Property> &props);

// Convert JSON array to entire property list
std::vector<Property> jsonToProperties(const nlohmann::json &arr);

} // namespace HT
