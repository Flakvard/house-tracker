// jsonHelper.hpp
#pragma once
#include <house_model.hpp>
#include <nlohmann/json.hpp>

namespace HT {
nlohmann::json propertyToJson(const Property &prop);
Property jsonToProperty(const nlohmann::json &j);
bool isSameProperty(const Property &a, const Property &b);
void mergeProperties(std::vector<Property> &existing,
                     const std::vector<Property> &newOnes);
} // namespace HT
