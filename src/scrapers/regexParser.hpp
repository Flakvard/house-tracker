#include <nlohmann/json.hpp>

namespace HT {
// Helper to remove leading/trailing quotes, e.g. "\"Hello\"" -> "Hello"
std::string stripOuterQuotes(const std::string &s);

// Helper to parse integers from strings with possible punctuation
// e.g. "3.995.000" -> 3995000
int parsePriceToInt(const std::string &s);

// parse an integer field that might be wrapped in quotes, empty, etc.
int parseInt(const std::string &s);

// parse the "id" field which looks like: "\"Marknagilsvegur 50\"\"Streymoy
// suður\""
std::string parseId(const std::string &s);

// parse the "previousPrices" which is an array, but appears empty in your
// sample
std::vector<int> parsePreviousPrices(const nlohmann::json &arr);

std::string toString(const nlohmann::json &val);
} // namespace HT
