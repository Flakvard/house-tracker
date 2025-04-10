// house_model.hpp
#pragma once
#include <string>
#include <vector>

enum class PropertyType {
  Sethus,
  Tvihus,
  Radhus,
  Ibud,
  Summarhus,
  Vinnubygningur,
  Grundstykki,
  Jord,
  Neyst,
  Undefined
};

struct RawProperty {
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

struct Property {
  std::string id; // or separate into multiple fields if you like
  std::string website;
  std::string address;
  std::string houseNum;
  std::string city;
  std::string postNum;
  int price;
  std::vector<int> previousPrices; // or vector<int>
  int latestOffer;                 // or convert to int if you want
  std::string validDate;
  std::string date;
  int buildingSize;
  int landSize;
  int room;
  int floor;
  std::string img;
  PropertyType type;
};
