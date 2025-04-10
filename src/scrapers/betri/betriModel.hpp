// meklarinModel.hpp
#pragma once
#include <string>
#include <vector>

namespace HT::BETRI {

struct BetriProperty {
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
  std::string type;
};

} // namespace HT::BETRI
