// scraper.hpp
#pragma once

#include <scrapers/include/house_model.hpp>
#include <string>

namespace HT {
// Downloads the HTML content of the given URL
std::string downloadHtml(const std::string &url);
std::string downloadAndSaveHtml(const std::string &url);
std::string downloadAndSaveHtml(const std::string &url, PropertyType propType);
} // namespace HT
