// scraper.hpp
#pragma once

#include <string>

namespace HT {
// Downloads the HTML content of the given URL
std::string downloadHtml(const std::string &url);
std::string downloadAndSaveHtml(const std::string &url);
} // namespace HT
