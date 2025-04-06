// house_model.hpp
#pragma once
#include <string>
#include <vector>

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
};

struct MeklarinProperty {
  std::string ID;
  std::string areas;
  std::string types;
  std::string featured_image;
  std::string permalink;
  std::string build;
  std::string address;
  std::string city;
  int bedrooms;
  int house_area;
  int area_size;
  bool isNew;
  bool featured;
  bool sold;
  bool open_house;
  std::string open_house_start_date;
  int price;
  int bid;
  bool new_bid;
  std::string bid_valid_until;
  std::string new_price;
};
