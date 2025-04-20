// main.cpp
#include <scrapers/include/PropertyManager.hpp>
#include <webapi/webapi.hpp>

int main() {
  bool downloadNewHtml = true;
  HT::PropertyManager::runPropertyParsers(downloadNewHtml);
  HT::runServer();
  return 0;
}
