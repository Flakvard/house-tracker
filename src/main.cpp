#include <curl/curl.h>
#include <fstream>
#include <gumbo.h>
#include <iostream>
#include <nlohmann/json.hpp>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}
struct Property {
  std::string website;
  std::string address;
  std::string houseNum;
  std::string city;
  std::string postNum;
  std::string price;
  std::string latestPrice;
  std::string validDate;
  std::string date;
  std::string buildingSize;
  std::string landSize;
  std::string room;
  std::string floor;
  std::string img;
};

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
void parseBetriProperty(GumboNode *node, Property *p) {
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
      p->latestPrice = getNodeText(node);
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
void findBetriProperties(GumboNode *node, std::vector<Property> &results) {
  if (!node)
    return;

  if (node->type == GUMBO_NODE_ELEMENT) {
    // Check if this node is an <article> with class="c-property c-card grid"
    if (node->v.element.tag == GUMBO_TAG_ARTICLE) {
      const char *classAttr =
          getAttribute(&node->v.element.attributes, "class");
      if (classAttr && std::string(classAttr) == "c-property c-card grid") {
        // parse this entire block as a Betri property
        Property prop;
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

int main() {
  // 1. Download HTML with libcurl
  CURL *curl;
  CURLcode res;
  std::string html;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.betriheim.fo/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

    // Perform the request
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
                << std::endl;
      return 1;
    }
  }
  curl_global_cleanup();

  // 2. Parse downloaded HTML with Gumbo
  GumboOutput *output = gumbo_parse(html.c_str());
  if (!output) {
    std::cerr << "Failed to parse HTML with Gumbo\n";
    return 1;
  }

  // 3. Recursively find your property listings
  std::vector<Property> betriProperties;
  findBetriProperties(output->root, betriProperties);

  gumbo_destroy_output(&kGumboDefaultOptions, output);

  nlohmann::json propertiesArray = nlohmann::json::array();

  int count = 0;
  for (const auto &prop : betriProperties) {
    nlohmann::json j;
    j["Number"] = ++count;
    j["Website"] = prop.website;
    j["Address"] = prop.address;
    j["Price"] = prop.price;
    j["Latest"] = prop.latestPrice;
    j["ValidDate"] = prop.validDate;
    j["YearBuilt"] = prop.date;
    j["InsideM2"] = prop.buildingSize;
    j["LandM2"] = prop.landSize;
    j["Rooms"] = prop.room;
    j["Floors"] = prop.floor;
    j["Img"] = prop.img;

    // Add this JSON object to our properties array
    propertiesArray.push_back(j);
  }

  // Write to file
  std::ofstream outFile("../src/storage/properties.json");
  if (!outFile.is_open()) {
    std::cerr << "Error opening output file!\n";
    return 1;
  }

  // Pretty-print with 4 spaces of indentation
  outFile << propertiesArray.dump(4) << std::endl;
  outFile.close();

  std::cout << "Properties written to properties.json\n";

  count = 0;
  // 4. Print them out (debug) or do something else
  for (const auto &prop : betriProperties) {
    count++;
    std::cout << "Number:    " << count << "\n";
    std::cout << "Website:   " << prop.website << "\n";
    std::cout << "Address:   " << prop.address << "\n";
    std::cout << "Price:     " << prop.price << "\n";
    std::cout << "Latest:    " << prop.latestPrice << "\n";
    std::cout << "ValidDate: " << prop.validDate << "\n";
    std::cout << "YearBuilt: " << prop.date << "\n";
    std::cout << "InsideM2:  " << prop.buildingSize << "\n";
    std::cout << "LandM2:    " << prop.landSize << "\n";
    std::cout << "Rooms:     " << prop.room << "\n";
    std::cout << "Floors:    " << prop.floor << "\n";
    std::cout << "Img:       " << prop.img << "\n\n";
  }

  return 0;
}
