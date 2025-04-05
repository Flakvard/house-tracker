// main.cpp
#include "scraper.hpp"
#include <betriScraper.hpp>
#include <curl/curl.h>
#include <gumbo.h>
#include <house_model.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <vector>

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
static std::string findAllPropertiesJson(GumboNode *node) {
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
      std::string result = findAllPropertiesJson(child);
      if (!result.empty()) {
        return result; // we found the script we needed
      }
    }
  }
  return "";
}

// Sketch: anything -> string
std::string toString(const nlohmann::json &val) {
  // If val is null, return ""
  if (val.is_null())
    return "";

  // If it's a primitive (number, boolean, etc.), you can do `dump()`
  // or cast to a string. `dump()` is robust for any type.
  return val.dump();
}

int main() {
  // betriRun();
  //  1) Download page into `html`
  std::string url = "https://www.meklarin.fo/";
  std::string html = downloadHtml(url);

  // 2) Parse with Gumbo
  GumboOutput *output = gumbo_parse(html.c_str());
  if (!output) {
    std::cerr << "Failed to parse HTML.\n";
    return 1;
  }

  // 3) Find script content with "var ALL_PROPERTIES"
  std::string scriptText = findAllPropertiesJson(output->root);

  // Clean up gumbo
  gumbo_destroy_output(&kGumboDefaultOptions, output);

  if (scriptText.empty()) {
    std::cerr << "Could not find script with 'var ALL_PROPERTIES'!\n";
    return 1;
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
  if (std::regex_search(scriptText, match, re)) {
    // match[1] should be the JSON array: '[{"ID":29032,"areas":"Streymoy...},
    // {...}]'
    std::string rawJson = match[1].str();

    // Now parse that with nlohmann::json
    // (Make sure you have #include <nlohmann/json.hpp> and linked it properly)
    try {
      // 5) Parse the string as JSON
      nlohmann::json j = nlohmann::json::parse(rawJson);

      // 'j' should now be an array of objects
      // e.g. j[0]["ID"], j[0]["areas"], ...

      std::vector<MeklarinProperty> properties;

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
            item.contains("bedrooms") ? toString(item["bedrooms"]) : "";
        prop.house_area =
            item.contains("house_area") ? toString(item["house_area"]) : "";
        prop.area_size =
            item.contains("area_size") ? toString(item["area_size"]) : "";
        prop.isNew = item.contains("new") ? toString(item["new"]) : "";
        prop.featured =
            item.contains("featured") ? toString(item["featured"]) : "";
        prop.sold = item.contains("sold") ? toString(item["sold"]) : "";
        prop.open_house =
            item.contains("open_house") ? toString(item["open_house"]) : "";
        prop.open_house_start_date =
            item.contains("open_house_start_date")
                ? toString(item["open_house_start_date"])
                : "";
        prop.price = item.contains("price") ? toString(item["price"]) : "";
        prop.bid = item.contains("bid") ? toString(item["bid"]) : "";
        prop.new_bid =
            item.contains("new_bid") ? toString(item["new_bid"]) : "";
        prop.bid_valid_until = item.contains("bid_valid_until")
                                   ? toString(item["bid_valid_until"])
                                   : "";
        prop.new_price =
            item.contains("new_price") ? toString(item["new_price"]) : "";

        properties.push_back(std::move(prop));
      }

      // 8) Do whatever you want with `properties`
      // e.g. print them:
      for (auto &p : properties) {
        std::cout << "ID: " << p.ID << "\n"
                  << "Areas: " << p.areas << "\n"
                  << "Types: " << p.types << "\n"
                  << "Featured Image: " << p.featured_image << "\n"
                  << "Price: " << p.price << "\n"
                  << "Sold: " << (p.sold == "true" ? "Yes" : "No") << "\n"
                  << "--------------------------\n";
      }

    } catch (std::exception &e) {
      std::cerr << "JSON parse error: " << e.what() << "\n";
    }

  } else {
    std::cerr << "Regex could not find JSON.parse(...) in script text.\n";
  }

  return 0;
}
