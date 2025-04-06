#include "scrapers/house_model.hpp"
#include <filesystem>
namespace HT {

class PropertyManager {
public:
  static void traverseAllHtmlAndMergeProperties(
      std::vector<Property> &allProperties,
      std::vector<std::filesystem::path> htmlFiles, std::string url);

  // Merges new properties into existing, tracking price changes
  static void mergeProperties(std::vector<Property> &existing,
                              const std::vector<Property> &newOnes);

  static bool isSameProperty(const Property &a, const Property &b);
  static std::string propertyTypeToString(PropertyType type);
  static PropertyType stringToPropertyType(const std::string &str);
};
} // namespace HT
