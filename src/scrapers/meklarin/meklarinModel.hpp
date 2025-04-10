// meklarinModel.hpp
#pragma once
#include <string>

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
