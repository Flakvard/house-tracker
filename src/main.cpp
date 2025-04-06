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
using json = nlohmann::json;

/**
 * Returns a partial HTML snippet (a <tbody>) containing the properties
 * read from src/storage/properties.json. We'll then let HTMX swap this
 * into the existing table in the browser.
 */
std::string buildPropertiesTableRows() {
  std::ifstream ifs("../src/storage/properties.json");
  if (!ifs.is_open()) {
    return "<tr><td colspan='10'>Could not open properties.json</td></tr>";
  }

  json j;
  try {
    ifs >> j;
  } catch (std::exception &e) {
    return std::string("<tr><td colspan='10'>JSON parse error: ") + e.what() +
           "</td></tr>";
  }

  std::stringstream rows;
  for (auto &item : j) {
    // Safely extract fields
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

    rows << "<tr>\n"
         << "  <td>" << address << "</td>\n"
         << "  <td>" << floors << "</td>\n"
         << "  <td>" << price << "</td>\n"
         << "  <td>" << latestOffer << "</td>\n"
         << "  <td>" << yearBuilt << "</td>\n"
         << "  <td>" << validDate << "</td>\n"
         << "  <td>" << insideM2 << "</td>\n"
         << "  <td>" << landM2 << "</td>\n"
         << "  <td>" << rooms << "</td>\n"
         << "  <td>" << website << "</td>\n"
         << "</tr>\n";
  }
  return rows.str();
}

int main() {
  /**
   * 1) Serve a main page at GET "/"
   *    - This page includes references to DaisyUI (via a Tailwind CDN) and
   * HTMX.
   *    - We have a table with an empty <tbody>.
   *    - A button or link triggers HTMX to GET "/propertiesRows" and swap the
   * <tbody>.
   */
  app().registerHandler(
      "/", [](const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) {
        // We embed the DaisyUI+HTMX references via CDN
        // (You could also host them locally or use a bundler if you prefer.)
        std::stringstream html;
        html << R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Drogon + HTMX + DaisyUI Demo</title>
  <!-- Include Tailwind + DaisyUI from a CDN -->
  <link href="https://cdn.jsdelivr.net/npm/daisyui@2.51.5/dist/full.css" rel="stylesheet">
  <!-- HTMX -->
  <script src="https://unpkg.com/htmx.org@1.9.2"></script>
</head>
<body class="m-4">
  <h1 class="text-2xl font-bold mb-4">Properties</h1>

  <!-- A button that triggers an HTMX call to /propertiesRows 
       and replaces the #props-tbody content with the response -->
  <button class="btn btn-primary mb-4"
          hx-get="/propertiesRows"
          hx-target="#props-tbody"
          hx-swap="innerHTML">
    Load Properties
  </button>

  <!-- Our table (DaisyUI "table" style) -->
  <div class="overflow-x-auto">
    <table class="table w-full">
      <thead>
        <tr>
          <th>Address</th>
          <th>Floors</th>
          <th>Price</th>
          <th>Latest Offer</th>
          <th>Year Built</th>
          <th>Valid Date</th>
          <th>Inside M2</th>
          <th>Land M2</th>
          <th>Rooms</th>
          <th>Website</th>
        </tr>
      </thead>
      <tbody id="props-tbody">
        <!-- Initially empty, we'll fill it with HTMX -->
      </tbody>
    </table>
  </div>
</body>
</html>
)";

        auto resp = HttpResponse::newHttpResponse();
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(html.str());
        callback(resp);
      });

  /**
   * 2) Serve a partial <tbody> at GET "/propertiesRows"
   *    - We'll read properties.json, build <tr> rows, and return them.
   */
  app().registerHandler(
      "/propertiesRows",
      [](const HttpRequestPtr &req,
         std::function<void(const HttpResponsePtr &)> &&callback) {
        std::string rows = buildPropertiesTableRows();
        auto resp = HttpResponse::newHttpResponse();
        // We return just the rows (partial HTML)
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(rows);
        callback(resp);
      });

  // Start Drogon server (on port 8080 by default if no config)
  app().addListener("0.0.0.0", 8080);
  app().run();

  return 0;
}
