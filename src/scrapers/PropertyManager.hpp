#include "scrapers/house_model.hpp"
#include <filesystem>
namespace HT {

class PropertyManager {
public:
  static void traverseAllHtmlAndMergeProperties(
      std::vector<Property> allProperties,
      std::vector<std::filesystem::path> htmlFiles, std::string url);

  static void mergeProperties(std::vector<Property> &existing,
                              const std::vector<Property> &newOnes);

  static bool isSameProperty(const Property &a, const Property &b);
};
} // namespace HT
