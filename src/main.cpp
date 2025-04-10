// main.cpp
#include <scrapers/betri/betriScraper.hpp>
#include <scrapers/meklarin/meklarinScraper.hpp>
#include <webapi/webapi.hpp>

int main() {
  bool downloadNewHtml = false;
  HT::betriRun(downloadNewHtml);
  HT::meklarinRun(downloadNewHtml);
  // HT::runServer();
  return 0;
}
