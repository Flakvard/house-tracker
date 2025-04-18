// parser.hpp
#pragma once
#include <gumbo.h>
#include <scrapers/include/house_model.hpp>

namespace HT {

std::string getNodeText(GumboNode *node);
const char *getClassAttr(GumboNode *node);
const char *getAttribute(const GumboVector *attrs, const char *name);
} // namespace HT
