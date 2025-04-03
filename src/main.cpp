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
  std::string id;
  std::string website;
  std::string address;
  std::string houseNum;
  std::string city;
  std::string postNum;
  std::string price;
  std::vector<std::string> previousPrices; // All prior prices
  std::string latestOffer;
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

// Simple check if a file exists.
bool fileExists(const std::string &path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0);
}

// Download HTML via libcurl
std::string downloadHtml(const std::string &url) {
  CURL *curl;
  CURLcode res;
  std::string html;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Failed to init curl\n";
    return "";
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
              << std::endl;
    html.clear(); // Indicate failure
  }

  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return html;
}

/**
 * Load HTML from cache if it exists, or download from a URL and cache it.
 * filePath is something like "../src/raw_html/html_1.json"
 */
std::string loadHtmlFromCacheOrDownload(const std::string &url,
                                        const std::string &filePath) {
  if (fileExists(filePath)) {
    // If file exists, read JSON from disk
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
      std::cerr << "Error opening cache file: " << filePath << std::endl;
      return "";
    }

    nlohmann::json j;
    ifs >> j;
    ifs.close();

    // Optionally, you can do version checks, timestamps, etc. For simplicity:
    std::string cachedHtml = j.value("html", "");
    if (!cachedHtml.empty()) {
      std::cout << "[Cache] Using cached HTML from " << filePath << "\n";
      return cachedHtml;
    }
  }

  // Otherwise, download fresh
  std::cout << "[Download] Fetching fresh HTML from " << url << "\n";
  std::string freshHtml = downloadHtml(url);
  if (freshHtml.empty()) {
    // Download failed; handle error accordingly
    return "";
  }

  // Save to JSON cache file
  nlohmann::json j;
  j["url"] = url;
  j["html"] = freshHtml;

  std::ofstream ofs(filePath);
  if (!ofs.is_open()) {
    std::cerr << "Error opening file for writing: " << filePath << std::endl;
    return freshHtml; // We have HTML, but we couldn't cache it
  }
  ofs << j.dump(4);
  ofs.close();

  return freshHtml;
}

// Convert a single Property to JSON
nlohmann::json propertyToJson(const Property &prop) {
  nlohmann::json j;
  j["id"] = prop.id;
  j["website"] = prop.website;
  j["address"] = prop.address;
  j["price"] = prop.price;
  j["previousPrices"] = prop.previousPrices;
  j["latestOffer"] = prop.latestOffer;
  j["validDate"] = prop.validDate;
  j["yearBuilt"] = prop.date;
  j["insideM2"] = prop.buildingSize;
  j["landM2"] = prop.landSize;
  j["rooms"] = prop.room;
  j["floors"] = prop.floor;
  j["img"] = prop.img;
  return j;
}

// Convert JSON to a single Property
Property jsonToProperty(const nlohmann::json &j) {
  Property p;
  p.id = j.value("id", "");
  p.website = j.value("website", "");
  p.address = j.value("address", "");
  p.price = j.value("price", "");
  // If "previousPrices" doesn't exist or is the wrong type, this won't blow up
  if (j.contains("previousPrices") && j["previousPrices"].is_array()) {
    for (auto &item : j["previousPrices"]) {
      p.previousPrices.push_back(item.get<std::string>());
    }
  }
  p.latestOffer = j.value("latestOffer", "");
  p.validDate = j.value("validDate", "");
  p.date = j.value("yearBuilt", "");
  p.buildingSize = j.value("insideM2", "");
  p.landSize = j.value("landM2", "");
  p.room = j.value("room", "");
  p.floor = j.value("floors", "");
  p.img = j.value("img", "");
  return p;
}

// Compare two properties by ID (or address, or whichever unique key you choose)
bool isSameProperty(const Property &a, const Property &b) {
  return a.id == b.id;
}

// Merges new properties into existing, tracking price changes
void mergeProperties(std::vector<Property> &existing,
                     const std::vector<Property> &newOnes) {
  for (const auto &newProp : newOnes) {
    // 1) Find match in existing
    auto it =
        std::find_if(existing.begin(), existing.end(), [&](const Property &ex) {
          return isSameProperty(ex, newProp);
        });

    if (it == existing.end()) {
      // property not found => new property
      std::cout << "Adding new property: " << newProp.address << "\n";
      existing.push_back(newProp);
    } else {
      // property found => check if price changed
      if (it->price != newProp.price) {
        std::cout << "Price changed for: " << it->address << " from "
                  << it->price << " to " << newProp.price << "\n";

        // push the old price into the price history
        it->previousPrices.push_back(it->price);
        // update the current price
        it->price = newProp.price;
      }
      // you can compare other fields (e.g., floors, rooms) similarly
    }
  }
}

// Convert entire property list to JSON array
nlohmann::json allPropertiesToJson(const std::vector<Property> &props) {
  nlohmann::json arr = nlohmann::json::array();
  for (auto &p : props) {
    arr.push_back(propertyToJson(p));
  }
  return arr;
}

// Convert JSON array to entire property list
std::vector<Property> jsonToAllProperties(const nlohmann::json &arr) {
  std::vector<Property> props;
  if (!arr.is_array()) {
    return props;
  }
  for (auto &item : arr) {
    props.push_back(jsonToProperty(item));
  }
  return props;
}

int main() {
  // 1. Try to load HTML from "../src/raw_html/html_1.json", or download if
  // missing
  const std::string url = "https://www.betriheim.fo/";
  const std::string cachePath = "../src/raw_html/html_1.json";
  std::string html = loadHtmlFromCacheOrDownload(url, cachePath);

  if (html.empty()) {
    std::cerr << "Failed to load or download HTML.\n";
    return 1;
  }

  // 2. Parse with Gumbo
  GumboOutput *output = gumbo_parse(html.c_str());
  if (!output) {
    std::cerr << "Failed to parse HTML with Gumbo\n";
    return 1;
  }

  // 3. Recursively find your property listings
  std::vector<Property> betriProperties;
  findBetriProperties(output->root, betriProperties);
  gumbo_destroy_output(&kGumboDefaultOptions, output);

  for (auto &prop : betriProperties) {

    prop.id = prop.address + prop.city + prop.postNum;
  }

  // 5. Build JSON of the scraped properties
  //
  std::vector<Property> existingProperties;
  {
    std::ifstream ifs("../src/storage/properties.json");
    if (ifs.is_open()) {
      nlohmann::json j;
      ifs >> j;
      ifs.close();
      existingProperties = jsonToAllProperties(j);
      std::cout << "Loaded " << existingProperties.size()
                << " properties from disk.\n";
    } else {
      std::cout << "No existing properties.json, starting fresh.\n";
    }
  }
  nlohmann::json newlyScrapedProps = nlohmann::json::array();

  for (const auto &prop : betriProperties) {
    nlohmann::json j = propertyToJson(prop);
    // Add this JSON object to our properties array
    newlyScrapedProps.push_back(j);
  }
  auto newProperties = jsonToAllProperties(newlyScrapedProps);

  mergeProperties(existingProperties, newProperties);

  auto updatedProperties = allPropertiesToJson(existingProperties);

  // 6. Write to properties.json or wherever you wish
  std::ofstream outFile("../src/storage/properties.json");
  if (!outFile.is_open()) {
    std::cerr << "Error opening output file!\n";
    return 1;
  }

  // Pretty-print with 4 spaces of indentation
  outFile << updatedProperties.dump(4) << std::endl;
  outFile.close();

  std::cout << "Properties written to properties.json\n";

  return 0;
}
