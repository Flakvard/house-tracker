// house_model.hpp
#pragma once
#include <string>
#include <vector>

struct Property {
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

struct MeklarinProperty {
  std::string ID;
  std::string areas;
  std::string types;
  std::string featured_image;
  std::string permalink;
  std::string build;
  std::string address;
  std::string city;
  std::string bedrooms;
  std::string house_area;
  std::string area_size;
  std::string isNew;
  std::string featured;
  std::string sold;
  std::string open_house;
  std::string open_house_start_date;
  std::string price;
  std::string bid;
  std::string new_bid;
  std::string bid_valid_until;
  std::string new_price;
};
