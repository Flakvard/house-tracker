// betriScraper.hpp
#pragma once
#include <scrapers/house_model.hpp>
namespace HT {
bool fileExists(const std::string &path);
std::string loadHtmlFromCacheOrDownload(const std::string &url,
                                        const std::string &filePath);
int betriRun();
} // namespace HT
