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

// A simple utility to get all text inside a node if it is <script> or similar
static std::string getScriptText(GumboNode *node) {
  if (!node)
    return "";
  if (node->type == GUMBO_NODE_TEXT) {
    // If this is a text node, just return its text
    return std::string(node->v.text.text);
  } else if (node->type == GUMBO_NODE_ELEMENT) {
    // If this is an element node, walk its children
    std::string result;
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      result += getScriptText(child);
    }
    return result;
  }
  return "";
}

// Recursively find all <script> nodes. If any <script> node contains
// "var ALL_PROPERTIES =", we return the script content.
static std::string findAllPropertiesJsonMeklarin(GumboNode *node) {
  if (!node)
    return "";

  if (node->type == GUMBO_NODE_ELEMENT &&
      node->v.element.tag == GUMBO_TAG_SCRIPT) {
    // This is a <script> node, get its text
    std::string scriptContent = getScriptText(node);

    // Check if it has the line "var ALL_PROPERTIES"
    if (scriptContent.find("var ALL_PROPERTIES") != std::string::npos) {
      return scriptContent; // Return the entire script text
    }
  }

  // Otherwise, recurse on children
  if (node->type == GUMBO_NODE_ELEMENT) {
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      std::string result = findAllPropertiesJsonMeklarin(child);
      if (!result.empty()) {
        return result; // we found the script we needed
      }
    }
  }
  return "";
}

// parse the Html with Gumbo
std::vector<RawProperty> parseWithGumboMeklarin(std::string html) {
  // 2) Parse with Gumbo
  GumboOutput *output = gumbo_parse(html.c_str());
  if (!output) {
    std::cerr << "Failed to parse HTML.\n";
  }

  // 3) Find script content with "var ALL_PROPERTIES"
  std::string scriptText = findAllPropertiesJsonMeklarin(output->root);

  // Clean up gumbo
  gumbo_destroy_output(&kGumboDefaultOptions, output);

  if (scriptText.empty()) {
    std::cerr << "Could not find script with 'var ALL_PROPERTIES'!\n";
  }

  // We have something like:
  //   var ALL_PROPERTIES = JSON.parse('[{"ID":29032,"areas":"Streymoy
  //   su\\u00f0ur",...}]');
  // We need to extract the JSON string from inside JSON.parse(' ... ').

  // 4) Extract the actual JSON array from the script text.
  // A quick approach is a small regex looking for JSON.parse('...'):

  // Example pattern:  JSON.parse('some stuff between single quotes');
  // We'll do a lazy capture of everything between the single quotes after
  // JSON.parse('
  std::regex re(R"(JSON\.parse\('([^']*)'\))");
  std::smatch match;

  std::vector<MeklarinProperty> properties;

  if (std::regex_search(scriptText, match, re)) {
    // match[1] should be the JSON array: '[{"ID":29032,"areas":"Streymoy...},
    // {...}]'
    std::string rawJson = match[1].str();

    // Now parse that with nlohmann::json
    // (Make sure you have #include <nlohmann/json.hpp> and linked it
    // properly)
    try {
      // 5) Parse the string as JSON
      nlohmann::json j = nlohmann::json::parse(rawJson);

      // 'j' should now be an array of objects
      // e.g. j[0]["ID"], j[0]["areas"], ...

      // 7) Loop over array elements
      for (auto &item : j) {
        MeklarinProperty prop;
        // note: some fields might be numeric or boolean; handle carefully
        // We'll do a naive .get<std::string>() where it's obviously a string:
        prop.ID = item.contains("ID") ? toString(item["ID"]) : ""; // integer
        prop.areas = item.contains("areas") ? toString(item["areas"]) : "";
        prop.types = item.contains("types") ? toString(item["types"]) : "";
        prop.featured_image = item.contains("featured_image")
                                  ? toString(item["featured_image"])
                                  : "";
        prop.permalink =
            item.contains("permalink") ? toString(item["permalink"]) : "";
        prop.build = item.contains("build") ? toString(item["build"]) : "";
        prop.address =
            item.contains("address") ? toString(item["address"]) : "";
        prop.city = item.contains("city") ? toString(item["city"]) : "";
        prop.bedrooms =
            item.contains("bedrooms") ? HT::parseInt(item["bedrooms"]) : 0;
        prop.house_area =
            item.contains("house_area") ? HT::parseInt(item["house_area"]) : 0;
        prop.area_size =
            item.contains("area_size") ? HT::parseInt(item["area_size"]) : 0;
        prop.isNew = item.contains("new") ? (bool)item["new"] : false;
        prop.featured =
            item.contains("featured") ? (bool)item["featured"] : false;
        prop.sold = item.contains("sold") ? (bool)item["sold"] : false;
        prop.open_house =
            item.contains("open_house") ? (bool)item["open_house"] : false;
        prop.open_house_start_date =
            item.contains("open_house_start_date")
                ? toString(item["open_house_start_date"])
                : "";
        prop.price =
            item.contains("price") ? HT::parsePriceToInt(item["price"]) : 0;
        prop.bid = item.contains("bid") ? HT::parsePriceToInt(item["bid"]) : 0;
        prop.new_bid = item.contains("new_bid") ? (bool)item["new_bid"] : false;
        prop.bid_valid_until = item.contains("bid_valid_until")
                                   ? toString(item["bid_valid_until"])
                                   : "";
        prop.new_price =
            item.contains("new_price") ? toString(item["new_price"]) : "";

        properties.push_back(std::move(prop));
      }

    } catch (std::exception &e) {
      std::cerr << "JSON parse error: " << e.what() << "\n";
    }
  }

  std::vector<RawProperty> allProperties;
  for (auto &p : properties) {
    RawProperty property;
    property.id = p.address + p.areas;
    property.website = "https://www.meklarin.fo/";
    property.address = p.address;
    property.houseNum = p.address;
    property.city = p.city;
    property.postNum = p.areas;
    property.price = std::to_string(p.price);
    property.latestOffer = std::to_string(p.bid);
    property.validDate = p.bid_valid_until;
    property.date = p.open_house_start_date;
    property.buildingSize = std::to_string(p.house_area);
    property.landSize = std::to_string(p.area_size);
    property.room = std::to_string(p.bedrooms);
    property.floor = "0";
    property.img = p.featured_image;
    allProperties.push_back(property);
  }
  return allProperties;
}

} // namespace HT
