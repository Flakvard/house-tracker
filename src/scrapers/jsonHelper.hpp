// jsonHelper.hpp
#pragma once
#include <house_model.hpp>
#include <nlohmann/json.hpp>

namespace HT {

// Convert a single Property to JSON
nlohmann::json propertyToJson(const Property &prop);

// Convert JSON to a single Property
Property jsonToProperty(const nlohmann::json &j);

// Compare two properties by ID (or address, or whichever unique key you choose)
bool isSameProperty(const Property &a, const Property &b);

// Merges new properties into existing, tracking price changes

void mergeProperties(std::vector<Property> &existing,
                     const std::vector<Property> &newOnes);

// Convert entire property list to JSON array
nlohmann::json propertiesToJson(const std::vector<Property> &props);

// Convert JSON array to entire property list
std::vector<Property> jsonToProperties(const nlohmann::json &arr);

} // namespace HT
