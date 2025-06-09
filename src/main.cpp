// main.cpp
#include <scrapers/include/PropertyManager.hpp>
#include <webapi/webapi.hpp>

int main(int argc, char* argv[]) {
  if (argc > 1 && std::string(argv[1]) == "--scrape") {
    HT::PropertyManager::runPropertyParsers(false);
    return 0;
  }

  HT::runServer();
  return 0;
}
