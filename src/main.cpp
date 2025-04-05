#include <chrono> // C++17
#include <ctime>
#include <filesystem> // C++17
#include <fstream>
#include <gumbo.h>
#include <house_model.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <scraper.hpp>

namespace fs = std::filesystem;

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
nlohmann::json propertiesToJson(const std::vector<Property> &props) {
  nlohmann::json arr = nlohmann::json::array();
  for (auto &p : props) {
    arr.push_back(propertyToJson(p));
  }
  return arr;
}

// Convert JSON array to entire property list
std::vector<Property> jsonToProperties(const nlohmann::json &arr) {
  std::vector<Property> props;
  if (!arr.is_array()) {
    return props;
  }
  for (auto &item : arr) {
    props.push_back(jsonToProperty(item));
  }
  return props;
}

// Utility to get a string like "html_2025-04-03_14-00-00.json"
// You might prefer a shorter or simpler format
std::string makeTimestampedFilename() {
  using namespace std::chrono;

  // Get the current time and time zone
  auto now = system_clock::now();
  auto tz = current_zone();
  zoned_time zt{tz, now};

  // Format using std::format with chrono support (C++20)
  std::string timestamp = std::format("{:%Y-%m-%d_%H-%M-%S}", zt);

  return "../src/raw_html/html_" + timestamp + ".json";
}

std::string downloadAndSaveHtml(const std::string &url) {
  // 1) Download
  std::string html = downloadHtml(url);
  if (html.empty()) {
    std::cerr << "Download failed\n";
    return "";
  }

  // 2) Build a new timestamped filename
  std::string filePath = makeTimestampedFilename();

  // 3) Save the raw HTML (plus any metadata) into a JSON file
  nlohmann::json j;
  j["url"] = url;
  j["timestamp"] = std::time(nullptr); // or store as string
  j["html"] = html;

  std::ofstream ofs(filePath);
  if (!ofs.is_open()) {
    std::cerr << "Error opening file: " << filePath << std::endl;
    return html; // At least we have the HTML in memory
  }

  ofs << j.dump(4);
  ofs.close();

  std::cout << "Saved raw HTML to: " << filePath << "\n";
  return html;
}

// Example function to gather and sort all .json files from a directory
std::vector<fs::path> gatherJsonFiles(const std::string &dir) {
  std::vector<fs::path> result;
  for (auto &entry : fs::directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      result.push_back(entry.path());
    }
  }

  // Sort them in ascending order by filename
  // (If your filenames are timestamped, this is effectively chronological.)
  std::sort(result.begin(), result.end());
  return result;
}

std::vector<Property> parseHtmlWithGumbo(std::string html) {
  std::vector<Property> betriProperties;

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

    prop.id = prop.address + prop.city + prop.postNum;
  }
  return betriProperties;
}

int main() {
  // 1. Try to load HTML from "../src/raw_html/html_1.json", or download if
  // missing
  const std::string url = "https://www.betriheim.fo/";
  const std::string cachePath = "../src/raw_html/html_1.json";
  // std::string html = loadHtmlFromCacheOrDownload(url, cachePath);
  std::string html = downloadAndSaveHtml(url);

  // 1. Prepare an "existing properties" vector
  //    (Load from properties.json if it exists)
  std::vector<Property> allProperties;
  {
    std::ifstream ifs("../src/storage/properties.json");
    if (ifs.is_open()) {
      nlohmann::json j;
      ifs >> j;
      allProperties = jsonToProperties(j);
      ifs.close();
    } else {
      std::cout << "No existing properties.json found; starting fresh.\n";
    }
  }
  // 2. Gather all timestamped .json files from ../src/raw_html
  std::string rawHtmlDir = "../src/raw_html";
  std::vector<fs::path> htmlFiles = gatherJsonFiles(rawHtmlDir);
  if (htmlFiles.empty()) {
    std::cerr << "No JSON files found in " << rawHtmlDir << "\n";
    return 0;
  }

  // 3. For each file, read the JSON, extract "html", parse with Gumbo, merge
  for (const auto &path : htmlFiles) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
      std::cerr << "Failed to open " << path << "\n";
      continue;
    }

    nlohmann::json j;
    ifs >> j;
    ifs.close();

    // If your JSON structure is: { "url": "...", "timestamp": ..., "html":
    // "..." }
    std::string rawHtml = j.value("html", "");
    if (rawHtml.empty()) {
      std::cerr << "No HTML found in " << path << "\n";
      continue;
    }

    // Parse
    std::vector<Property> newProperties = parseHtmlWithGumbo(rawHtml);

    // Merge
    mergeProperties(allProperties, newProperties);

    std::cout << "Processed file: " << path.filename().string() << " => found "
              << newProperties.size() << " properties.\n";
  }

  // 4. Write final results to properties.json
  nlohmann::json finalJson = propertiesToJson(allProperties);
  std::ofstream ofs("../src/storage/properties.json");
  if (!ofs.is_open()) {
    std::cerr << "Failed to open ../src/storage/properties.json for writing!\n";
    return 1;
  }
  ofs << finalJson.dump(4);
  ofs.close();

  std::cout << "Wrote " << allProperties.size()
            << " total properties to properties.json\n";
  return 0;
  return 0;
}
