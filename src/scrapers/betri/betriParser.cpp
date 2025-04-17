#include <gumbo.h>
#include <iostream>
#include <scrapers/betri/betriModel.hpp>
#include <scrapers/betri/betriParser.hpp>
#include <scrapers/include/PropertyManager.hpp>
#include <scrapers/include/house_model.hpp>
#include <string>
namespace HT::BETRI {
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

const char *findImgSrcRecursive(GumboNode *node) {
  if (!node || node->type != GUMBO_NODE_ELEMENT)
    return nullptr;

  if (node->v.element.tag == GUMBO_TAG_IMG) {
    GumboAttribute *src =
        gumbo_get_attribute(&node->v.element.attributes, "src");
    return src ? src->value : nullptr;
  }
  GumboVector *kids = &node->v.element.children;
  for (unsigned i = 0; i < kids->length; ++i)
    if (const char *s =
            findImgSrcRecursive(static_cast<GumboNode *>(kids->data[i])))
      return s;

#if GUMBO_VERSION >= 0x000a03
  if (node->v.element.template_contents) { // dive into <template>
    if (const char *s = findImgSrcRecursive(
            static_cast<GumboNode *>(node->v.element.template_contents)))
      return s;
  }
#endif
  return nullptr;
}

// Example function: parse an individual “Betri” property from a node that
// corresponds to the <article class="c-property c-card grid"> block
void parseBetriProperty(GumboNode *node, BetriProperty *p) {
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
  bool gotImg = !p->img.empty(); // already set elsewhere?

  if (!gotImg && node->v.element.tag == GUMBO_TAG_LI) {
    const char *liClassAttr =
        getAttribute(&node->v.element.attributes, "class");

    if (liClassAttr && std::string(liClassAttr) == "slide") {
      // check data-slider-id="1" on this <li>
      const char *sliderId =
          getAttribute(&node->v.element.attributes, "data-slider-id");
      if (sliderId && std::strcmp(sliderId, "1") == 0) {
        // Now scan children to find the first <img>
        if (const char *src = findImgSrcRecursive(node)) {
          p->img = src;
          gotImg = true;
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
void findBetriProperties(GumboNode *node, std::vector<BetriProperty> &results) {
  if (!node)
    return;

  if (node->type == GUMBO_NODE_ELEMENT) {
    // Check if this node is an <article> with class="c-property c-card grid"
    // if (node->v.element.tag == GUMBO_TAG_ARTICLE || node->v.element.tag ==
    // GUMBO_TAG_HTML) {
    const char *classAttr = getAttribute(&node->v.element.attributes, "class");
    if ((classAttr && std::string(classAttr) == "c-property c-card grid ") ||
        (classAttr && std::string(classAttr) == "c-property c-card grid")) {
      // parse this entire block as a Betri property
      BetriProperty prop;
      prop.website = "Betri";
      parseBetriProperty(node, &prop);
      results.push_back(prop);
    }
    //}

    // Recurse on children
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; i++) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      findBetriProperties(child, results);
    }
  }
}

std::vector<RawProperty>
mapBetriToRawProperty(std::vector<RawProperty> &rawProperties,
                      std::vector<BetriProperty> &betriProperties,
                      PropertyType propType) {
  for (auto &prop : betriProperties) {
    RawProperty p;
    prop.type = PropertyManager::propertyTypeToString(propType);
    prop.id = PropertyManager::cleanId(prop.address + prop.city + prop.postNum);

    p.id = prop.id;
    p.website = prop.website;
    p.address = prop.address;
    p.type = prop.type;
    p.houseNum = prop.houseNum;
    p.city = prop.city;
    p.postNum = prop.postNum;
    p.price = prop.price;
    p.previousPrices = prop.previousPrices;
    p.latestOffer = prop.latestOffer;
    p.validDate = prop.validDate;
    p.date = prop.date;
    p.buildingSize = prop.buildingSize;
    p.landSize = prop.landSize;
    p.room = prop.room;
    p.floor = prop.floor;
    p.img = prop.img;
    p.agent = PropertyManager::propertyAgentToString(RealEstateAgent::Betri);
    rawProperties.push_back(p);
  }
  return rawProperties;
}

// parse the Html with Gumbo
std::vector<RawProperty> parseHtmlWithGumboBetri(std::string html,
                                                 PropertyType propType) {
  std::vector<BetriProperty> betriProperties;
  std::vector<RawProperty> rawProperties;

  if (html.empty()) {
    std::cerr << "Failed to load or download HTML.\n";
    return rawProperties;
  }

  // 2. Parse with Gumbo
  GumboOutput *output = gumbo_parse(html.c_str());
  if (!output) {
    std::cerr << "Failed to parse HTML with Gumbo\n";
    return rawProperties;
  }

  // 3. Recursively find your property listings
  findBetriProperties(output->root, betriProperties);
  gumbo_destroy_output(&kGumboDefaultOptions, output);

  mapBetriToRawProperty(rawProperties, betriProperties, propType);
  return rawProperties;
}
} // namespace HT::BETRI
