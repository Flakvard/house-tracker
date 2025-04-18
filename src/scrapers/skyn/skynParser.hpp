#pragma once
#include <scrapers/include/house_model.hpp>
namespace HT::SKYN {

std::vector<RawProperty> parseWithGumboSkyn(std::string html,
                                            PropertyType propType);

}
