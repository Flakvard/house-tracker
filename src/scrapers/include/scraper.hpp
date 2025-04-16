// scraper.hpp
#pragma once

#include <scrapers/include/house_model.hpp>
#include <string>

namespace HT {
// Downloads the HTML content of the given URL
// std::string downloadHtml(const std::string &url);
std::string downloadAndSaveHtml(const std::string &url);
std::string downloadAndSaveHtml(const std::string &url, RealEstateAgent agent);
std::string downloadAndSaveHtml(const std::string &url, PropertyType propType);
std::string downloadAndSaveHtml(const std::string &url, PropertyType propType,
                                RealEstateAgent agent);

void checkAndDownloadImages(const std::vector<Property> &allProperties);
} // namespace HT
