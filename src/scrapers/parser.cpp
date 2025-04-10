#include <gumbo.h>
#include <iostream>
#include <regex>
#include <scrapers/PropertyManager.hpp>
#include <scrapers/house_model.hpp>
#include <scrapers/regexParser.hpp>
#include <string>

namespace HT {

// Helper to get an attribute's value
static const char *getAttribute(const GumboVector *attrs, const char *name) {
  if (!attrs)
    return nullptr;
  for (unsigned int i = 0; i < attrs->length; i++) {
    GumboAttribute *attr = static_cast<GumboAttribute *>(attrs->data[i]);
    if (std::string(attr->name) == std::string(name)) {
      return attr->value;
    }
  }
  return nullptr;
}

// Check if the node has a given tag and/or class
bool nodeHasClass(GumboNode *node, const std::string &className) {
  if (!node || node->type != GUMBO_NODE_ELEMENT) {
    return false;
  }
  const GumboVector *attrs = &node->v.element.attributes;
  const char *classAttr = getAttribute(attrs, "class");
  if (!classAttr)
    return false;
  // You can either do an exact match or check for substring if multiple classes
  std::string classVal = classAttr;
  return (classVal == className);
}

// Helper function to get the node’s inner text
std::string getNodeText(GumboNode *node) {
  if (!node)
    return "";
  // If it’s a text node
  if (node->type == GUMBO_NODE_TEXT) {
    return std::string(node->v.text.text);
  }
  // If it’s an element node, walk children
  if (node->type == GUMBO_NODE_ELEMENT) {
    std::string result;
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      result += getNodeText(child);
    }
    return result;
  }
  return "";
}

// Utility: Return the class attribute of a node, or nullptr if none.
static const char *getClassAttr(GumboNode *node) {
  if (!node || node->type != GUMBO_NODE_ELEMENT) {
    return nullptr;
  }
  GumboVector *attrs = &node->v.element.attributes;
  for (unsigned int i = 0; i < attrs->length; i++) {
    GumboAttribute *attr = static_cast<GumboAttribute *>(attrs->data[i]);
    if (std::string(attr->name) == "class") {
      return attr->value;
    }
  }
  return nullptr;
}

// Example function: parse an individual “Betri” property from a node that
// corresponds to the <article class="c-property c-card grid"> block
void parseBetriProperty(GumboNode *node, RawProperty *p) {
  if (!node || node->type != GUMBO_NODE_ELEMENT) {
    return;
  }

  // Check the node’s class attribute
  const char *classAttr = getClassAttr(node);
  if (classAttr) {
    std::string classStr = classAttr;

    // Whenever we see a node whose class is one of these, store its text
    // (In your HTML snippet, these divs all eventually appear under `<div
    // class="content">`.)
    if (classStr == "price") {
      p->price = getNodeText(node);
    } else if (classStr == "latest-offer") {
      p->latestOffer = getNodeText(node);
    } else if (classStr == "valid") {
      p->validDate = getNodeText(node);
    } else if (classStr == "date") {
      p->date = getNodeText(node);
    } else if (classStr == "building-size") {
      p->buildingSize = getNodeText(node);
    } else if (classStr == "land-size") {
      p->landSize = getNodeText(node);
    } else if (classStr == "rooms") {
      p->room = getNodeText(node);
    } else if (classStr == "floors") {
      p->floor = getNodeText(node);
    } else if (classStr == "medium") {
      // e->g. <address class="medium">MyAddress</address>
      p->address = getNodeText(node);
    }
  }

  // For the image: inside a <li class="slide"><img src="..."></li>
  // This might not be a direct child – you may need deeper recursion.
  // But if it is, do something like below:
  if (node->v.element.tag == GUMBO_TAG_LI) {
    const char *liClassAttr =
        getAttribute(&node->v.element.attributes, "class");
    if (liClassAttr && std::string(liClassAttr) == "slide") {
      // find <img> inside
      GumboVector *liChildren = &node->v.element.children;
      for (unsigned int j = 0; j < liChildren->length; j++) {
        GumboNode *liChild = static_cast<GumboNode *>(liChildren->data[j]);
        if (liChild->type == GUMBO_NODE_ELEMENT &&
            liChild->v.element.tag == GUMBO_TAG_IMG) {
          const char *srcAttr =
              getAttribute(&liChild->v.element.attributes, "src");
          if (srcAttr) {
            p->img = srcAttr;
          }
        }
      }
    }
  }
  // Recurse on all children
  GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; i++) {
    GumboNode *child = static_cast<GumboNode *>(children->data[i]);
    parseBetriProperty(child, p);
  }
}

// Recursively find <article class="c-property c-card grid"> in the DOM
void findBetriProperties(GumboNode *node, std::vector<RawProperty> &results) {
  if (!node)
    return;

  if (node->type == GUMBO_NODE_ELEMENT) {
    // Check if this node is an <article> with class="c-property c-card grid"
    if (node->v.element.tag == GUMBO_TAG_ARTICLE) {
      const char *classAttr =
          getAttribute(&node->v.element.attributes, "class");
      if (classAttr && std::string(classAttr) == "c-property c-card grid") {
        // parse this entire block as a Betri property
        RawProperty prop;
        prop.website = "Betri";
        parseBetriProperty(node, &prop);
        results.push_back(prop);
      }
    }

    // Recurse on children
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; i++) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      findBetriProperties(child, results);
    }
  }
}

// parse the Html with Gumbo
std::vector<RawProperty> parseHtmlWithGumboBetri(std::string html,
                                                 PropertyType propType) {
  std::vector<RawProperty> betriProperties;

  if (html.empty()) {
    std::cerr << "Failed to load or download HTML.\n";
    return betriProperties;
  }

  // 2. Parse with Gumbo
  GumboOutput *output = gumbo_parse(html.c_str());
  if (!output) {
    std::cerr << "Failed to parse HTML with Gumbo\n";
    return betriProperties;
  }
  // 3. Recursively find your property listings
  findBetriProperties(output->root, betriProperties);
  gumbo_destroy_output(&kGumboDefaultOptions, output);

  for (auto &prop : betriProperties) {
    prop.type = PropertyManager::propertyTypeToString(propType);
    prop.id = prop.address + prop.city + prop.postNum;
  }
  return betriProperties;
}

} // namespace HT
