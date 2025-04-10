#pragma once
#include "scrapers/house_model.hpp"
namespace HT::MEKLARIN {

std::vector<RawProperty> parseWithGumboMeklarin(std::string html);
}
