#include <filesystem>
#include <scrapers/include/house_model.hpp>
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
  static std::string propertyAgentToString(RealEstateAgent agent);
  static std::string propertyTypeToString(PropertyType type);
  static std::string extractPropertyTypeMeklarin(const std::string &s);
  static RealEstateAgent stringToAgent(const std::string &str);
  static PropertyType stringToPropertyType(const std::string &str);
  static std::string cleanId(const std::string &raw);
};
} // namespace HT
