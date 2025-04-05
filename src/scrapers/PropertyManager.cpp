#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <scrapers/PropertyManager.hpp>
#include <scrapers/parser.hpp>

namespace HT {

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

// PropertyManager::
void PropertyManager::traverseAllHtmlAndMergeProperties(
    std::vector<Property> allProperties,
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
    std::string website = j.value("url", "");
    if (website != url || website.empty())
      continue;

    std::string rawHtml = j.value("html", "");
    if (rawHtml.empty()) {
      std::cerr << "No HTML found in " << path << "\n";
      continue;
    }

    // Parse
    std::vector<Property> newProperties = HT::parseHtmlWithGumbo(rawHtml);

    // Merge
    PropertyManager::mergeProperties(allProperties, newProperties);

    std::cout << "Processed file: " << path.filename().string() << " => found "
              << newProperties.size() << " properties.\n";
  }
}

} // namespace HT
