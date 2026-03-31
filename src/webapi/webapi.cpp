#include <drogon/drogon.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <scrapers/include/scraper.hpp>
#include <sstream>
#include <string>
#include <trantor/net/EventLoop.h>
#include <webapi/backgroundService.hpp>
#include <webapi/webapi.hpp>

namespace HT {

using namespace drogon;
using json = nlohmann::json;

std::string buildPropertiesTableRows() {
  std::ifstream ifs("../src/storage/properties.json");
  if (!ifs.is_open()) {
    return "<tr><td colspan='19'>Could not open properties.json</td></tr>";
  }

  json j;
  try {
    ifs >> j;
  } catch (std::exception &e) {
    return std::string("<tr><td colspan='19'>JSON parse error: ") + e.what() +
           "</td></tr>";
  }

  std::stringstream rows;
  for (auto &item : j) {
    std::string imgUrl = item.value("img", "");

    std::string fileName = HT::getFilenameFromUrl(imgUrl);
    std::string fullName = HT::cleanAsciiFilename(item.value("id", "") +
                                                  item.value("validDate", ""));
    std::string localName = fullName + "_" + fileName;
    std::string servedPath = "/images/" + localName;

    std::string address = item.value("address", "");
    std::string id = item.value("id", "");
    std::string city = item.value("city", "");
    std::string typeProperty = item.value("type", "");
    int floors = item.value("floors", 0);
    int price = item.value("price", 0);
    int latestOffer = item.value("latestOffer", 0);
    std::string yearBuilt = item.value("yearBuilt", "");
    std::string validDate = item.value("validDate", "");
    int insideM2 = item.value("insideM2", 0);
    int landM2 = item.value("landM2", 0);
    int rooms = item.value("rooms", 0);
    std::string website = item.value("website", "");
    std::string status = item.value("status", "active");
    int offerPerInsideM2 = (insideM2 > 0) ? (latestOffer / insideM2) : 0;
    int offerPerLandM2 = (landM2 > 0) ? (latestOffer / landM2) : 0;
    int pricePerInsideM2 = (insideM2 > 0) ? (price / insideM2) : 0;
    int pricePerLandM2 = (landM2 > 0) ? (price / landM2) : 0;

    rows << "<tr>\n"
         << "  <td><img class=\"w-32 h-auto\" src=\"" << servedPath
         << "\" alt=\"" << address << "\"></td>\n"
         << "  <td>" << id << "</td>\n"
         << "  <td class=\"w-48\">" << typeProperty << "</td>\n"
         << "  <td class=\"w-48\">" << city << "</td>\n"
         << "  <td class=\"w-48\">" << address << "</td>\n"
         << "  <td class=\"w-24\">" << floors << "</td>\n"
         << "  <td class=\"w-32\">" << price << "</td>\n"
         << "  <td class=\"w-32\">" << latestOffer << "</td>\n"
         << "  <td class=\"w-32\">" << yearBuilt << "</td>\n"
         << "  <td class=\"w-48\">" << validDate << "</td>\n"
         << "  <td class=\"w-32\">" << insideM2 << "</td>\n"
         << "  <td class=\"w-32\">" << landM2 << "</td>\n"
         << "  <td class=\"w-24\">" << rooms << "</td>\n"
         << "  <td class=\"w-48\">" << website << "</td>\n"
         << "  <td class=\"w-32\">" << status << "</td>\n"
         << "  <td class=\"w-32\">" << offerPerInsideM2 << "</td>\n"
         << "  <td class=\"w-32\">" << offerPerLandM2 << "</td>\n"
         << "  <td class=\"w-32\">" << pricePerInsideM2 << "</td>\n"
         << "  <td class=\"w-32\">" << pricePerLandM2 << "</td>\n"
         << "</tr>\n";
  }
  return rows.str();
}

void runServer() {
  app().registerHandler(
      "/", [](const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) {
        std::stringstream html;
        html << R"(<!DOCTYPE html>
          <html lang="en">
          <head>
            <meta charset="UTF-8">
            <title>Faroese house tracker</title>
            <link href="https://cdn.jsdelivr.net/npm/daisyui@2.51.5/dist/full.css" rel="stylesheet">
            <script src="https://unpkg.com/htmx.org@1.9.2"></script>
          <style>
            .sticky-header th {
              position: sticky;
              top: 0;
              z-index: 10;
            }
            .col-toggle {
              cursor: pointer;
              user-select: none;
            }
          </style>
          </head>
          <body class="m-4">
            <h1 class="text-2xl font-bold mb-4">Properties</h1>

            <button class="btn btn-primary mb-4"
                    hx-get="/propertiesRows"
                    hx-target="#props-tbody"
                    hx-swap="innerHTML">
              Load Properties
            </button>
            <div class="flex flex-wrap gap-2 mb-2 items-center">
              <button id="reset-columns" class="btn btn-sm">Reset Columns</button>
              <label class="label cursor-pointer gap-2">
                <span class="label-text">Compact</span>
                <input id="compact-toggle" type="checkbox" class="toggle toggle-sm" />
              </label>
              <span class="text-xs opacity-70">Tap a column header to hide/show it</span>
            </div>

          <div class="h-[600px] overflow-y-auto overflow-x-auto">
            <table id="properties-table" class="table w-full sticky-header">
              <thead class="bg-base-200">
                <tr>
                  <th class="bg-base-200 col-toggle" data-col-index="0">Photo</th>
                  <th class="bg-base-200 col-toggle" data-col-index="1">Id</th>
                  <th class="bg-base-200 col-toggle" data-col-index="2">Type</th>
                  <th class="bg-base-200 col-toggle" data-col-index="3">City</th>
                  <th class="bg-base-200 col-toggle" data-col-index="4">Address</th>
                  <th class="bg-base-200 col-toggle" data-col-index="5">Floors</th>
                  <th class="bg-base-200 col-toggle" data-col-index="6">Price</th>
                  <th class="bg-base-200 col-toggle" data-col-index="7">Latest Offer</th>
                  <th class="bg-base-200 col-toggle" data-col-index="8">Year Built</th>
                  <th class="bg-base-200 col-toggle" data-col-index="9">Valid Date</th>
                  <th class="bg-base-200 col-toggle" data-col-index="10">Inside m2</th>
                  <th class="bg-base-200 col-toggle" data-col-index="11">Land m2</th>
                  <th class="bg-base-200 col-toggle" data-col-index="12">Rooms</th>
                  <th class="bg-base-200 col-toggle" data-col-index="13">Website</th>
                  <th class="bg-base-200 col-toggle" data-col-index="14">Status</th>
                  <th class="bg-base-200 col-toggle" data-col-index="15">Offer/Inside m2</th>
                  <th class="bg-base-200 col-toggle" data-col-index="16">Offer/Land m2</th>
                  <th class="bg-base-200 col-toggle" data-col-index="17">Price/Inside m2</th>
                  <th class="bg-base-200 col-toggle" data-col-index="18">Price/Land m2</th>
                </tr>
                <tr>
                  <th></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Filter Id" data-filter-col="1"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Filter Type" data-filter-col="2"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Filter City" data-filter-col="3"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Filter Address" data-filter-col="4"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Floors" data-filter-col="5"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Price" data-filter-col="6"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Offer" data-filter-col="7"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Year Built" data-filter-col="8"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Valid Date" data-filter-col="9"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Inside m2" data-filter-col="10"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Land m2" data-filter-col="11"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Rooms" data-filter-col="12"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Website" data-filter-col="13"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Status" data-filter-col="14"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Offer/Inside" data-filter-col="15"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Offer/Land" data-filter-col="16"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Price/Inside" data-filter-col="17"></th>
                  <th><input type="text" class="input input-xs w-full" placeholder="Price/Land" data-filter-col="18"></th>
                </tr>
              </thead>
              <tbody id="props-tbody"></tbody>
            </table>
          </div>
          <script>
          document.addEventListener('DOMContentLoaded', function() {
            const filterInputs = document.querySelectorAll('input[data-filter-col]');
            const tbody = document.getElementById('props-tbody');
            const table = document.getElementById('properties-table');
            const resetBtn = document.getElementById('reset-columns');
            const compactToggle = document.getElementById('compact-toggle');
            const colHeaders = document.querySelectorAll('thead tr:first-child th[data-col-index]');
            const hiddenColsKey = 'houseTracker_hiddenColumns';
            const compactKey = 'houseTracker_compactTable';

            function getHiddenCols() {
              try {
                const raw = localStorage.getItem(hiddenColsKey);
                const arr = raw ? JSON.parse(raw) : [];
                return new Set(arr.map(Number));
              } catch (e) {
                return new Set();
              }
            }

            function saveHiddenCols(set) {
              localStorage.setItem(hiddenColsKey, JSON.stringify(Array.from(set)));
            }

            function setColumnVisible(colIdx, visible) {
              Array.from(table.rows).forEach(row => {
                if (row.cells[colIdx]) row.cells[colIdx].style.display = visible ? '' : 'none';
              });
            }

            function applyColumnVisibility() {
              const hidden = getHiddenCols();
              colHeaders.forEach(header => {
                const idx = Number(header.dataset.colIndex);
                const isHidden = hidden.has(idx);
                setColumnVisible(idx, !isHidden);
                header.classList.toggle('opacity-40', isHidden);
              });
            }

            const numericFilterIdx = new Set([4, 5, 6, 9, 10, 11, 14, 15, 16, 17]);

            function parseCellNumber(text) {
              const cleaned = (text || '').replace(/[^0-9.-]/g, '');
              if (cleaned === '' || cleaned === '-' || cleaned === '.') return NaN;
              return Number(cleaned);
            }

            function matchesNumericFilter(filterText, cellText) {
              const cellNumber = parseCellNumber(cellText);
              if (Number.isNaN(cellNumber)) return false;

              const m = filterText.match(/^(>=|<=|>|<|=|!=)\s*(-?\d+(?:\.\d+)?)$/);
              if (m) {
                const op = m[1];
                const value = Number(m[2]);
                if (op === '>') return cellNumber > value;
                if (op === '<') return cellNumber < value;
                if (op === '>=') return cellNumber >= value;
                if (op === '<=') return cellNumber <= value;
                if (op === '=') return cellNumber === value;
                if (op === '!=') return cellNumber !== value;
              }

              const numericOnly = filterText.match(/^-?\d+(?:\.\d+)?$/);
              if (numericOnly) {
                return cellNumber === Number(filterText);
              }

              return null;
            }

            function filterTable() {
              const filters = Array.from(filterInputs).map(input => input.value.trim());
              Array.from(tbody.rows).forEach(row => {
                let show = true;
                filters.forEach((filter, idx) => {
                  if (filter !== '') {
                    const cell = row.cells[idx + 1];
                    if (!cell) return;

                    if (numericFilterIdx.has(idx)) {
                      const numericMatch = matchesNumericFilter(filter, cell.textContent);
                      if (numericMatch !== null) {
                        if (!numericMatch) show = false;
                        return;
                      }
                    }

                    if (!cell.textContent.toLowerCase().includes(filter.toLowerCase())) show = false;
                  }
                });
                row.style.display = show ? '' : 'none';
              });
            }

            colHeaders.forEach(header => {
              header.addEventListener('click', () => {
                const idx = Number(header.dataset.colIndex);
                const hidden = getHiddenCols();
                if (hidden.has(idx)) hidden.delete(idx);
                else hidden.add(idx);
                saveHiddenCols(hidden);
                applyColumnVisibility();
              });
            });

            if (resetBtn) {
              resetBtn.addEventListener('click', () => {
                localStorage.removeItem(hiddenColsKey);
                applyColumnVisibility();
              });
            }

            if (compactToggle) {
              compactToggle.checked = localStorage.getItem(compactKey) === '1';
              table.classList.toggle('table-xs', compactToggle.checked);
              compactToggle.addEventListener('change', () => {
                localStorage.setItem(compactKey, compactToggle.checked ? '1' : '0');
                table.classList.toggle('table-xs', compactToggle.checked);
              });
            }

            filterInputs.forEach(input => input.addEventListener('input', filterTable));
            document.body.addEventListener('htmx:afterSwap', function(evt) {
              if (evt.target && evt.target.id === 'props-tbody') {
                applyColumnVisibility();
                filterTable();
              }
            });

            applyColumnVisibility();
          });
          </script>
        </body>
        </html>
      )";
        auto resp = HttpResponse::newHttpResponse();
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(html.str());
        callback(resp);
      });

  app().registerHandler(
      "/propertiesRows",
      [](const HttpRequestPtr &req,
         std::function<void(const HttpResponsePtr &)> &&callback) {
        std::string rows = buildPropertiesTableRows();
        auto resp = HttpResponse::newHttpResponse();
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(rows);
        callback(resp);
      });

  auto loop = app().getLoop();
  scheduleDailyScraper(loop);

  app().addALocation("/images", "", "../src/raw_images", true, true);
  app().addListener("0.0.0.0", 8080);
  app().run();
}

} // namespace HT
