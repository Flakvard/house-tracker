#pragma once

#include <gumbo.h>
#include <scrapers/include/house_model.hpp>

namespace HT::BETRI {
// parse the Html with Gumbo
std::vector<RawProperty> parseHtmlWithGumboBetri(std::string html,
                                                 PropertyType propType);

} // namespace HT::BETRI
