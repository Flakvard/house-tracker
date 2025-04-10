#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <scrapers/include/house_model.hpp>

namespace HT {

// Helper to remove leading/trailing quotes, e.g. "\"Hello\"" -> "Hello"
std::string stripOuterQuotes(const std::string &s) {
  if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
    return s.substr(1, s.size() - 2);
  }
  return s;
}

// Helper to parse integers from strings with possible punctuation
// e.g. "3.995.000" -> 3995000
int parsePriceToInt(const std::string &s) {
  // 1) strip outer quotes if present
  std::string tmp = stripOuterQuotes(s);

  // 2) remove dots or other punctuation
  //    we can remove everything that is not digit
  std::string digitsOnly;
  for (char c : tmp) {
    if (std::isdigit(static_cast<unsigned char>(c))) {
      digitsOnly.push_back(c);
    }
  }
  if (digitsOnly.empty()) {
    return 0;
  }
  // 3) convert to int
  return std::stoi(digitsOnly);
}

// parse an integer field that might be wrapped in quotes, empty, etc.
int parseInt(const std::string &s) {
  std::string tmp = stripOuterQuotes(s);
  if (tmp.empty()) {
    return 0;
  }
  // remove any non-digit stuff (if needed)
  std::string digitsOnly;
  for (char c : tmp) {
    if (std::isdigit(static_cast<unsigned char>(c))) {
      digitsOnly.push_back(c);
    }
  }
  if (digitsOnly.empty()) {
    return 0;
  }
  return std::stoi(digitsOnly);
}

// parse the "id" field which looks like: "\"Marknagilsvegur 50\"\"Streymoy
// suður\""
std::string parseId(const std::string &s) {
  // first remove leading/trailing backslashes
  std::string tmp = stripOuterQuotes(s);

  // Now we have something like: Marknagilsvegur 50""Streymoy suður
  // Possibly we can separate them with a comma if you want:
  // - approach 1: replace two quotes with a comma+space
  // - approach 2: or any custom approach you prefer
  //
  // e.g. "Marknagilsvegur 50""Streymoy suður" -> "Marknagilsvegur 50, Streymoy
  // suður" A simple approach is to do a regex replace of `""` with `, `

  static std::regex doubleQuoteRegex(R"("")"); // matches ""
  tmp = std::regex_replace(tmp, doubleQuoteRegex, ", ");

  return tmp;
}

// parse the "previousPrices" which is an array, but appears empty in your
// sample
std::vector<int> parsePreviousPrices(const nlohmann::json &arr) {
  // If you want them as strings:
  std::vector<int> ret;
  for (auto &val : arr) {
    // if it's a string, do stripOuterQuotes, etc.
    ret.push_back(val.get<int>());
  }
  return ret;
}

// parse the "previousPrices" which is an array, but appears empty in your
// sample
std::vector<int> parsePreviousPrices(const std::vector<int> &listOfPrevPrices) {
  // If you want them as strings:
  std::vector<int> prevPrices;
  for (auto &prevPrice : listOfPrevPrices) {
    // if it's a string, do stripOuterQuotes, etc.
    prevPrices.push_back(prevPrice);
  }
  return prevPrices;
}
// Sketch: anything -> string
std::string toString(const nlohmann::json &val) {
  // If val is null, return ""
  if (val.is_null())
    return "";

  // If it's a primitive (number, boolean, etc.), you can do `dump()`
  // or cast to a string. `dump()` is robust for any type.
  return HT::stripOuterQuotes(val.dump());
}

} // namespace HT
