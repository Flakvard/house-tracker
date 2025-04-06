// main.cpp
#include <scrapers/betri/betriScraper.hpp>
#include <scrapers/meklarin/meklarinScraper.hpp>

// int main() {
//   HT::betriRun();
//   HT::meklarinRun();
//   return 0;
// }

#include <drogon/drogon.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

using namespace drogon;

int main() {
  // 1) Register a simple handler for GET /properties
  drogon::app().registerHandler(
      "/properties",
      [](const HttpRequestPtr &req,
         std::function<void(const HttpResponsePtr &)> &&callback) {
        // 2) Read the JSON from disk
        std::ifstream ifs("../src/storage/properties.json");
        if (!ifs.is_open()) {
          // If file couldn't be opened, return an error
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k404NotFound);
          resp->setBody("Cannot open properties.json file!");
          callback(resp);
          return;
        }

        nlohmann::json j;
        try {
          ifs >> j; // parse the JSON file
        } catch (std::exception &e) {
          // If JSON fails to parse
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k400BadRequest);
          resp->setBody(std::string("JSON parse error: ") + e.what());
          callback(resp);
          return;
        }

        // 3) Build an HTML table
        std::stringstream html;
        html << "<!DOCTYPE html>\n<html>\n<head>\n"
             << "<meta charset='UTF-8'/>"
             << "<title>Properties Table</title>\n"
             << "</head>\n<body>\n"
             << "<h1>Properties</h1>\n"
             << "<table border='1' style='border-collapse: collapse;'>\n"
             << "  <tr>\n"
             << "    <th>Address</th>\n"
             << "    <th>Floors</th>\n"
             << "    <th>Price</th>\n"
             << "    <th>Latest Offer</th>\n"
             << "    <th>Year Built</th>\n"
             << "    <th>Valid Date</th>\n"
             << "    <th>Inside M2</th>\n"
             << "    <th>Land M2</th>\n"
             << "    <th>Rooms</th>\n"
             << "    <th>Website</th>\n"
             << "  </tr>\n";

        for (auto &item : j) {
          // Safely extract fields (some might be missing or of different type)
          std::string address = item.value("address", "");
          int floors = item.value("floors", 0);
          int price = item.value("price", 0);
          int latestOffer = item.value("latestOffer", 0);
          std::string yearBuilt = item.value("yearBuilt", "");
          std::string validDate = item.value("validDate", "");
          int insideM2 = item.value("insideM2", 0);
          int landM2 = item.value("landM2", 0);
          int rooms = item.value("rooms", 0);
          std::string website = item.value("website", "");

          html << "  <tr>\n"
               << "    <td>" << address << "</td>\n"
               << "    <td>" << floors << "</td>\n"
               << "    <td>" << price << "</td>\n"
               << "    <td>" << latestOffer << "</td>\n"
               << "    <td>" << yearBuilt << "</td>\n"
               << "    <td>" << validDate << "</td>\n"
               << "    <td>" << insideM2 << "</td>\n"
               << "    <td>" << landM2 << "</td>\n"
               << "    <td>" << rooms << "</td>\n"
               << "    <td>" << website << "</td>\n"
               << "  </tr>\n";
        }

        html << "</table>\n</body>\n</html>";

        // 4) Return the HTML
        auto resp = HttpResponse::newHttpResponse();
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(html.str());
        callback(resp);
      },
      {Get}); // only allow GET requests

  // 5) Run the Drogon app (by default on port 80 or 8080)
  app().setLogLevel(trantor::Logger::kInfo);
  // You can choose a port if you like:
  app().addListener("0.0.0.0", 8080);
  app().run();
  return 0;
}
