// parser.hpp
#pragma once
#include <gumbo.h>
#include <scrapers/house_model.hpp>

namespace HT {
// Helper to get an attribute's value
static const char *getAttribute(const GumboVector *attrs, const char *name);

// Check if the node has a given tag and/or class
bool nodeHasClass(GumboNode *node, const std::string &className);

// Helper function to get the node’s inner text
std::string getNodeText(GumboNode *node);

// Utility: Return the class attribute of a node, or nullptr if none.
static const char *getClassAttr(GumboNode *node);

// Example function: parse an individual “Betri” property from a node that
// corresponds to the <article class="c-property c-card grid"> block
void parseBetriProperty(GumboNode *node, RawProperty *p);

// Recursively find <article class="c-property c-card grid"> in the DOM
void findBetriProperties(GumboNode *node, std::vector<RawProperty> &results);

// parse the Html with Gumbo
std::vector<RawProperty> parseHtmlWithGumboBetri(std::string html,
                                                 PropertyType propType);
} // namespace HT
