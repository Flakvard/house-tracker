#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <drogon/drogon.h>
#include <fstream>
#include <iomanip>
#include <set>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>
#include <scrapers/include/scraper.hpp>
#include <sstream>
#include <string>
#include <trantor/net/EventLoop.h>
#include <webapi/backgroundService.hpp>
#include <webapi/webapi.hpp>

namespace HT {

using namespace drogon;
using json = nlohmann::json;

namespace {

std::string htmlEscape(const std::string &input) {
  std::string out;
  out.reserve(input.size());
  for (char c : input) {
    switch (c) {
    case '&':
      out += "&amp;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    case '"':
      out += "&quot;";
      break;
    case '\'':
      out += "&#39;";
      break;
    default:
      out += c;
      break;
    }
  }
  return out;
}

std::string lowerCopy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::string trimCopy(const std::string &input) {
  const auto start = input.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  const auto end = input.find_last_not_of(" \t\r\n");
  return input.substr(start, end - start + 1);
}

std::string replaceAllCopy(std::string text, const std::string &from,
                           const std::string &to) {
  if (from.empty()) {
    return text;
  }
  std::size_t startPos = 0;
  while ((startPos = text.find(from, startPos)) != std::string::npos) {
    text.replace(startPos, from.size(), to);
    startPos += to.size();
  }
  return text;
}

std::string normalizedKey(std::string input) {
  input = trimCopy(input);
  input = replaceAllCopy(input, "\xC3\x81", "A");
  input = replaceAllCopy(input, "\xC3\xA1", "a");
  input = replaceAllCopy(input, "\xC3\x90", "D");
  input = replaceAllCopy(input, "\xC3\xB0", "d");
  input = replaceAllCopy(input, "\xC3\x8D", "I");
  input = replaceAllCopy(input, "\xC3\xAD", "i");
  input = replaceAllCopy(input, "\xC3\x93", "O");
  input = replaceAllCopy(input, "\xC3\xB3", "o");
  input = replaceAllCopy(input, "\xC3\x9A", "U");
  input = replaceAllCopy(input, "\xC3\xBA", "u");
  input = replaceAllCopy(input, "\xC3\x9D", "Y");
  input = replaceAllCopy(input, "\xC3\xBD", "y");
  input = replaceAllCopy(input, "\xC3\x86", "Ae");
  input = replaceAllCopy(input, "\xC3\xA6", "ae");
  input = replaceAllCopy(input, "\xC3\x98", "O");
  input = replaceAllCopy(input, "\xC3\xB8", "o");
  return lowerCopy(input);
}

struct LocationMeta {
  std::string cityKey;
  std::string cityLabel;
  std::string kommunaKey;
  std::string kommunaLabel;
  std::string syslaKey;
  std::string syslaLabel;
};

const std::vector<LocationMeta> &locationCatalog() {
  static const std::vector<LocationMeta> kLocations = {
      {"torshavn", "T&oacute;rshavn", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"klaksvik", "Klaksv&iacute;k", "klaksvik", "Klaksv&iacute;k", "nordoyar", "Nor&eth;oyar"},
      {"hoyvik", "Hoyv&iacute;k", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"argir", "Argir", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"fuglafjordur", "Fuglafj&oslash;r&eth;ur", "fuglafjordur", "Fuglafj&oslash;r&eth;ur", "eysturoy", "Eysturoy"},
      {"vagur", "V&aacute;gur", "vagur", "V&aacute;gur", "suduroy", "Su&eth;uroy"},
      {"vestmanna", "Vestmanna", "vestmanna", "Vestmanna", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"saltangara", "Saltangar&aacute;", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"sorvagur", "S&oslash;rv&aacute;gur", "sorvagur", "S&oslash;rv&aacute;gur", "vagar", "V&aacute;gar"},
      {"midvagur", "Mi&eth;v&aacute;gur", "vagar", "V&aacute;gar", "vagar", "V&aacute;gar"},
      {"strendur", "Strendur", "sjogv", "Sj&oacute;gv", "eysturoy", "Eysturoy"},
      {"toftir", "Toftir", "nes", "Nes", "eysturoy", "Eysturoy"},
      {"leirvik", "Leirv&iacute;k", "eystur", "Eystur", "eysturoy", "Eysturoy"},
      {"sandavagur", "Sandav&aacute;gur", "vagar", "V&aacute;gar", "vagar", "V&aacute;gar"},
      {"tvoroyri", "Tv&oslash;royri", "tvoroyri", "Tv&oslash;royri", "suduroy", "Su&eth;uroy"},
      {"kollafjordur", "Kollafj&oslash;r&eth;ur", "torshavn", "T&oacute;rshavn", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"skala", "Sk&aacute;la", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"eidi", "Ei&eth;i", "eidi", "Ei&eth;i", "eysturoy", "Eysturoy"},
      {"nordragota", "Nor&eth;rag&oslash;ta", "eystur", "Eystur", "eysturoy", "Eysturoy"},
      {"runavik", "Runav&iacute;k", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"hvalba", "Hvalba", "hvalba", "Hvalba", "suduroy", "Su&eth;uroy"},
      {"sandur", "Sandur", "sandur", "Sandur", "sandoy", "Sandoy"},
      {"trongisvagur", "Trongisv&aacute;gur", "tvoroyri", "Tv&oslash;royri", "suduroy", "Su&eth;uroy"},
      {"sydrugota", "Sy&eth;rug&oslash;ta", "eystur", "Eystur", "eysturoy", "Eysturoy"},
      {"skopun", "Skopun", "skopun", "Skopun", "sandoy", "Sandoy"},
      {"glyvrar", "Glyvrar", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"kvivik", "Kv&iacute;v&iacute;k", "kvivik", "Kv&iacute;v&iacute;k", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"nes (eysturoy)", "Nes (Eysturoy)", "nes", "Nes", "eysturoy", "Eysturoy"},
      {"nes", "Nes", "nes", "Nes", "eysturoy", "Eysturoy"},
      {"rituvik", "Rituv&iacute;k", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"streymnes", "Streymnes", "sundini", "Sundini", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"soldarfjordur", "S&oslash;ldarfj&oslash;r&eth;ur", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"porkeri", "Porkeri", "porkeri", "Porkeri", "suduroy", "Su&eth;uroy"},
      {"vidareidi", "Vi&eth;arei&eth;i", "vidareidi", "Vi&eth;arei&eth;i", "nordoyar", "Nor&eth;oyar"},
      {"hosvik", "H&oacute;sv&iacute;k", "sundini", "Sundini", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"nordskali", "Nor&eth;sk&aacute;li", "sundini", "Sundini", "eysturoy", "Eysturoy"},
      {"hvannasund", "Hvannasund", "hvannasund", "Hvannasund", "nordoyar", "Nor&eth;oyar"},
      {"velbastadur", "Velbasta&eth;ur", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"hvalvik", "Hvalv&iacute;k", "sundini", "Sundini", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"frodba", "Fro&eth;ba", "tvoroyri", "Tv&oslash;royri", "suduroy", "Su&eth;uroy"},
      {"kaldbak", "Kaldbak", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"sumba", "Sumba", "sumba", "Sumba", "suduroy", "Su&eth;uroy"},
      {"nolsoy", "N&oacute;lsoy", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"oyri", "Oyri", "sundini", "Sundini", "eysturoy", "Eysturoy"},
      {"skalavik", "Sk&aacute;lav&iacute;k", "skalavik", "Sk&aacute;lav&iacute;k", "sandoy", "Sandoy"},
      {"norddepil", "Nor&eth;depil", "hvannasund", "Hvannasund", "nordoyar", "Nor&eth;oyar"},
      {"saltnes", "Saltnes", "nes", "Nes", "eysturoy", "Eysturoy"},
      {"oyrarbakki", "Oyrarbakki", "sundini", "Sundini", "eysturoy", "Eysturoy"},
      {"lamba", "Lamba", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"nordoyri", "Nor&eth;oyri", "klaksvik", "Klaksv&iacute;k", "nordoyar", "Nor&eth;oyar"},
      {"signabour", "Signab&oslash;ur", "torshavn", "T&oacute;rshavn", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"skalafjordur (skalabotnur)", "Sk&aacute;lafj&oslash;r&eth;ur (Sk&aacute;labotnur)", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"oyndarfjordur", "Oyndarfj&oslash;r&eth;ur", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"leynar", "Leynar", "kvivik", "Kv&iacute;v&iacute;k", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"haldorsvik", "Hald&oacute;rsv&iacute;k", "sundini", "Sundini", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"hov", "Hov", "hov", "Hov", "suduroy", "Su&eth;uroy"},
      {"hvitanes", "Hv&iacute;tanes", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"aeduvik", "&AElig;&eth;uv&iacute;k", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"kunoy", "Kunoy", "kunoy", "Kunoy", "nordoyar", "Nor&eth;oyar"},
      {"lopra", "Lopra", "sumba", "Sumba", "suduroy", "Su&eth;uroy"},
      {"innan glyvur", "Innan Glyvur", "sjogv", "Sj&oacute;gv", "eysturoy", "Eysturoy"},
      {"kirkjubour", "Kirkjub&oslash;ur", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"famjin", "F&aacute;mjin", "famjin", "F&aacute;mjin", "suduroy", "Su&eth;uroy"},
      {"haraldssund", "Haraldssund", "kunoy", "Kunoy", "nordoyar", "Nor&eth;oyar"},
      {"bour", "B&oslash;ur", "sorvagur", "S&oslash;rv&aacute;gur", "vagar", "V&aacute;gar"},
      {"sandvik", "Sandv&iacute;k", "hvalba", "Hvalba", "suduroy", "Su&eth;uroy"},
      {"arnafjordur", "&Aacute;rnafj&oslash;r&eth;ur", "klaksvik", "Klaksv&iacute;k", "nordoyar", "Nor&eth;oyar"},
      {"husavik", "H&uacute;sav&iacute;k", "husavik", "H&uacute;sav&iacute;k", "sandoy", "Sandoy"},
      {"funningsfjordur", "Funningsfj&oslash;r&eth;ur", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"oravikslid", "&Oslash;rav&iacute;ksl&iacute;&eth;", "tvoroyri", "Tv&oslash;royri", "suduroy", "Su&eth;uroy"},
      {"skipanes", "Skipanes", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"anirnar", "&Aacute;nirnar", "klaksvik", "Klaksv&iacute;k", "nordoyar", "Nor&eth;oyar"},
      {"funningur", "Funningur", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"svinair", "Sv&iacute;n&aacute;ir", "eidi", "Ei&eth;i", "eysturoy", "Eysturoy"},
      {"tjornuvik", "Tj&oslash;rnuv&iacute;k", "sundini", "Sundini", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"valur", "V&aacute;lur", "kvivik", "Kv&iacute;v&iacute;k", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"gotugjogv", "G&oslash;tugj&oacute;gv", "eystur", "Eystur", "eysturoy", "Eysturoy"},
      {"vatnsoyrar", "Vatnsoyrar", "vagar", "V&aacute;gar", "vagar", "V&aacute;gar"},
      {"husar", "H&uacute;sar", "husar", "H&uacute;sar", "nordoyar", "Nor&eth;oyar"},
      {"langasandur", "Langasandur", "sundini", "Sundini", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"stykkid", "Stykki&eth;", "kvivik", "Kv&iacute;v&iacute;k", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"selatrad", "Selatra&eth;", "sjogv", "Sj&oacute;gv", "eysturoy", "Eysturoy"},
      {"dalur", "Dalur", "husavik", "H&uacute;sav&iacute;k", "sandoy", "Sandoy"},
      {"oyrareingir", "Oyrareingir", "torshavn", "T&oacute;rshavn", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"ljosa", "Lj&oacute;s&aacute;", "eidi", "Ei&eth;i", "eysturoy", "Eysturoy"},
      {"svinoy", "Sv&iacute;noy", "klaksvik", "Klaksv&iacute;k", "nordoyar", "Nor&eth;oyar"},
      {"undir gotueidi", "Undir G&oslash;tuei&eth;i", "eystur", "Eystur", "eysturoy", "Eysturoy"},
      {"kirkja", "Kirkja", "fugloy", "Fugloy", "nordoyar", "Nor&eth;oyar"},
      {"skugvoy", "Sk&uacute;gvoy", "skugvoy", "Sk&uacute;gvoy", "sandoy", "Sandoy"},
      {"ordavik", "&Oslash;r&eth;av&iacute;k", "tvoroyri", "Tv&oslash;royri", "suduroy", "Su&eth;uroy"},
      {"kolbeinagjogv", "Kolbeinagj&oacute;gv", "sjogv", "Sj&oacute;gv", "eysturoy", "Eysturoy"},
      {"mikladalur", "Mikladalur", "klaksvik", "Klaksv&iacute;k", "nordoyar", "Nor&eth;oyar"},
      {"morskranes", "Morskranes", "sjogv", "Sj&oacute;gv", "eysturoy", "Eysturoy"},
      {"gjogv", "Gj&oacute;gv", "sundini", "Sundini", "eysturoy", "Eysturoy"},
      {"hattarvik", "Hattarv&iacute;k", "fugloy", "Fugloy", "nordoyar", "Nor&eth;oyar"},
      {"gasadalur", "G&aacute;sadalur", "sorvagur", "S&oslash;rv&aacute;gur", "vagar", "V&aacute;gar"},
      {"trollanes", "Tr&oslash;llanes", "klaksvik", "Klaksv&iacute;k", "nordoyar", "Nor&eth;oyar"},
      {"hestur", "Hestur", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"mykines", "Mykines", "sorvagur", "S&oslash;rv&aacute;gur", "vagar", "V&aacute;gar"},
      {"skaelingur", "Sk&aelig;lingur", "kvivik", "Kv&iacute;v&iacute;k", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"elduvik", "Elduv&iacute;k", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"lambareidi", "Lambarei&eth;i", "runavik", "Runav&iacute;k", "eysturoy", "Eysturoy"},
      {"saksun", "Saksun", "sundini", "Sundini", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"akrar", "Akrar", "sumba", "Sumba", "suduroy", "Su&eth;uroy"},
      {"hellurnar", "Hellurnar", "fuglafjordur", "Fuglafj&oslash;r&eth;ur", "eysturoy", "Eysturoy"},
      {"nordradalur", "Nor&eth;radalur", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"skarvanes", "Skarvanes", "husavik", "H&uacute;sav&iacute;k", "sandoy", "Sandoy"},
      {"sydradalur (kalsoy)", "Sy&eth;radalur (Kalsoy)", "klaksvik", "Klaksv&iacute;k", "nordoyar", "Nor&eth;oyar"},
      {"sydradalur", "Sy&eth;radalur", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"sydradalur (streymoy)", "Sy&eth;radalur (Streymoy)", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"kaldbaksbotnur", "Kaldbaksbotnur", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"depil", "Depil", "hvannasund", "Hvannasund", "nordoyar", "Nor&eth;oyar"},
      {"skalafjordur (eysturkommuna)", "Sk&aacute;lafj&oslash;r&eth;ur (Eysturkommuna)", "eystur", "Eystur", "eysturoy", "Eysturoy"},
      {"stora dimun", "St&oacute;ra D&iacute;mun", "skugvoy", "Sk&uacute;voy", "sandoy", "Sandoy"},
      {"nordtoftir", "Nor&eth;toftir", "hvannasund", "Hvannasund", "nordoyar", "Nor&eth;oyar"},
      {"sund", "Sund", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"koltur", "Koltur", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"mjorkadalur", "Mj&oslash;rkadalur", "torshavn", "T&oacute;rshavn", "sudurstreymoy", "Su&eth;urstreymoy"},
      {"muli", "M&uacute;li", "hvannasund", "Hvannasund", "nordoyar", "Nor&eth;oyar"},
      {"nesvik", "Nesv&iacute;k", "sundini", "Sundini", "nordurstreymoy", "Nor&eth;urstreymoy"},
      {"vikarbyrgi", "V&iacute;karbyrgi", "sumbini", "Sumbini", "suduroy", "Su&eth;uroy"}};
  return kLocations;
}

const LocationMeta *lookupLocationMeta(const std::string &city) {
  const std::string key = normalizedKey(city);
  for (const auto &entry : locationCatalog()) {
    if (entry.cityKey == key) {
      return &entry;
    }
  }
  return nullptr;
}

std::string buildLocationOptionsHtml(const std::string &kind) {
  std::set<std::pair<std::string, std::string>> values;
  for (const auto &entry : locationCatalog()) {
    if (kind == "city") {
      values.insert({entry.cityKey, entry.cityLabel});
    } else if (kind == "kommuna") {
      values.insert({entry.kommunaKey, entry.kommunaLabel});
    } else if (kind == "sysla") {
      values.insert({entry.syslaKey, entry.syslaLabel});
    }
  }

  std::ostringstream html;
  for (const auto &value : values) {
    html << "<option value=\"" << htmlEscape(value.first) << "\">"
         << value.second << "</option>";
  }
  return html.str();
}
std::string buildImagePath(const json &item) {
  const std::string imgUrl = item.value("img", "");
  const std::string fileName = HT::getFilenameFromUrl(imgUrl);
  const std::string fullName =
      HT::cleanAsciiFilename(item.value("id", "") + item.value("validDate", ""));
  return "/images/" + fullName + "_" + fileName;
}

bool parseIsoDate(const std::string &text, std::tm &tmOut) {
  std::istringstream iss(text);
  iss >> std::get_time(&tmOut, "%Y-%m-%d");
  return !iss.fail();
}

int diffDays(const std::string &start, const std::string &end) {
  std::tm startTm{};
  std::tm endTm{};
  if (!parseIsoDate(start, startTm) || !parseIsoDate(end, endTm)) {
    return -1;
  }

  startTm.tm_hour = 0;
  startTm.tm_min = 0;
  startTm.tm_sec = 0;
  endTm.tm_hour = 0;
  endTm.tm_min = 0;
  endTm.tm_sec = 0;

  const std::time_t startTime = std::mktime(&startTm);
  const std::time_t endTime = std::mktime(&endTm);
  if (startTime == static_cast<std::time_t>(-1) ||
      endTime == static_cast<std::time_t>(-1)) {
    return -1;
  }

  return static_cast<int>((endTime - startTime) / (60 * 60 * 24));
}

std::string todayIsoDate() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm tmNow{};
#ifdef _WIN32
  localtime_s(&tmNow, &t);
#else
  localtime_r(&t, &tmNow);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tmNow, "%Y-%m-%d");
  return oss.str();
}

std::string metricHtml(const std::string &label, const std::string &value) {
  std::ostringstream out;
  out << "<div class=\"metric\">"
      << "<div class=\"metric-label\">" << htmlEscape(label) << "</div>"
      << "<div class=\"metric-value\">" << htmlEscape(value) << "</div>"
      << "</div>";
  return out.str();
}

std::string metricHtmlCompact(const std::string &label, const std::string &value) {
  std::ostringstream out;
  out << "<div class=\"metric metric-compact\">"
      << "<div class=\"metric-label\">" << htmlEscape(label) << "</div>"
      << "<div class=\"metric-value\">" << htmlEscape(value) << "</div>"
      << "</div>";
  return out.str();
}

std::string formatNumberDots(int value) {
  const bool negative = value < 0;
  std::string digits = std::to_string(negative ? -value : value);
  std::string formatted;
  for (std::size_t i = 0; i < digits.size(); ++i) {
    if (i > 0 && (digits.size() - i) % 3 == 0) {
      formatted += '.';
    }
    formatted += digits[i];
  }
  return negative ? "-" + formatted : formatted;
}

std::string buildPropertiesGrid() {
  std::ifstream ifs("../src/storage/properties.json");
  if (!ifs.is_open()) {
    return "<div class=\"empty-state\">Could not open properties.json</div>";
  }

  json j;
  try {
    ifs >> j;
  } catch (std::exception &e) {
    return "<div class=\"empty-state\">JSON parse error: " + htmlEscape(e.what()) +
           "</div>";
  }

  const std::string today = todayIsoDate();
  std::ostringstream cards;

  for (const auto &item : j) {
    const std::string address = item.value("address", "");
    const std::string city = item.value("city", "");
    const std::string type = item.value("type", "");
    const std::string website = item.value("website", "");
    const std::string status = item.value("status", "active");
    const std::string yearBuilt = item.value("yearBuilt", "");
    const std::string addedDate = item.value("addedDate", "");
    const std::string archivedDate = item.value("archivedDate", "");
    const std::string validDate = item.value("validDate", "");
    const std::string id = item.value("id", "");
    const LocationMeta *locationMeta = lookupLocationMeta(city);
    const std::string cityKey = locationMeta ? locationMeta->cityKey : normalizedKey(city);
    const std::string cityDisplay = locationMeta ? locationMeta->cityLabel : htmlEscape(city);
    const std::string kommunaKey = locationMeta ? locationMeta->kommunaKey : "";
    const std::string kommunaDisplay = locationMeta ? locationMeta->kommunaLabel : "";
    const std::string syslaKey = locationMeta ? locationMeta->syslaKey : "";
    const std::string syslaDisplay = locationMeta ? locationMeta->syslaLabel : "";

    const int insideM2 = item.value("insideM2", 0);
    const int landM2 = item.value("landM2", 0);
    const int rooms = item.value("rooms", 0);
    const int floors = item.value("floors", 0);
    const int price = item.value("price", 0);
    const int latestOffer = item.value("latestOffer", 0);

    const int offerPerInsideM2 = insideM2 > 0 ? latestOffer / insideM2 : 0;
    const int offerPerLandM2 = landM2 > 0 ? latestOffer / landM2 : 0;
    const int pricePerInsideM2 = insideM2 > 0 ? price / insideM2 : 0;
    const int pricePerLandM2 = landM2 > 0 ? price / landM2 : 0;
    const int daysListed = addedDate.empty() ? -1 : diffDays(addedDate, today);
    const int daysUntilSold =
        (addedDate.empty() || archivedDate.empty()) ? -1
                                                    : diffDays(addedDate, archivedDate);

    const std::string searchText =
        address + " " + city + " " + kommunaKey + " " + syslaKey + " " + type + " " +
        website + " " + id;
    const std::string priceText = formatNumberDots(price);
    const std::string latestOfferText = formatNumberDots(latestOffer);
    const std::string offerPerInsideText =
        offerPerInsideM2 > 0 ? formatNumberDots(offerPerInsideM2) : "-";
    const std::string pricePerInsideText =
        pricePerInsideM2 > 0 ? formatNumberDots(pricePerInsideM2) : "-";
    const std::string servedPath = buildImagePath(item);

    cards << "<article class=\"property-card\" data-search=\""
          << htmlEscape(searchText) << "\" data-status=\""
          << htmlEscape(lowerCopy(status)) << "\" data-website=\""
          << htmlEscape(lowerCopy(website)) << "\" data-type=\""
          << htmlEscape(type) << "\" data-city=\""
          << htmlEscape(cityKey) << "\" data-kommuna=\""
          << htmlEscape(kommunaKey) << "\" data-sysla=\""
          << htmlEscape(syslaKey) << "\" data-price=\"" << price
          << "\" data-inside=\"" << insideM2 << "\" data-land=\"" << landM2
          << "\">";

    if (!servedPath.empty() && servedPath != "/images/_") {
      cards << "<div class=\"property-media\"><img src=\"" << htmlEscape(servedPath)
            << "\" alt=\"" << htmlEscape(address) << "\"></div>";
    } else {
      cards << "<div class=\"property-media placeholder\">No image</div>";
    }

    cards << "<div class=\"property-body\">"
          << "<div class=\"property-topline\">"
          << "<span class=\"pill pill-status pill-" << htmlEscape(lowerCopy(status))
          << "\">" << htmlEscape(status) << "</span>"
          << "<span class=\"pill pill-agent\">" << htmlEscape(website) << "</span>"
          << (kommunaDisplay.empty() ? ""
                              : "<span class=\"pill pill-location\">" +
                                    kommunaDisplay + "</span>")
          << "</div>"
          << "<h2 class=\"property-title\">" << htmlEscape(address) << "</h2>"
          << "<div class=\"property-subtitle\">" << cityDisplay
          << " | " << htmlEscape(type)
          << (syslaDisplay.empty() ? "" : " | " + syslaDisplay) << "</div>"
          << "<div class=\"property-facts\">"
          << metricHtmlCompact("Inside", std::to_string(insideM2) + " m2")
          << metricHtmlCompact("Land", std::to_string(landM2) + " m2")
          << "</div>"
          << "<div class=\"property-price-band\">"
          << "<div><span class=\"price-label\">Latest offer</span><span class=\"price-value\">"
          << latestOfferText << "</span><span class=\"price-subvalue\">Inside m2: "
          << offerPerInsideText
          << "</span></div>"
          << "<div><span class=\"price-label\">Price</span><span class=\"price-value\">"
          << priceText << "</span><span class=\"price-subvalue\">Inside m2: "
          << pricePerInsideText
          << "</span></div>"
          << "</div>"
          << "<details class=\"property-more\">"
          << "<summary>More</summary>"
          << "<div class=\"property-meta\">"
          << metricHtml("Rooms", std::to_string(rooms))
          << metricHtml("Floors", std::to_string(floors))
          << metricHtml("Built", yearBuilt.empty() ? "-" : yearBuilt)
          << metricHtml("Added", addedDate.empty() ? "-" : addedDate)
          << metricHtml("Archived", archivedDate.empty() ? "-" : archivedDate)
          << metricHtml("Days listed",
                        daysListed >= 0 ? std::to_string(daysListed) : "-")
          << metricHtml("Days until sold",
                        daysUntilSold >= 0 ? std::to_string(daysUntilSold) : "-")
          << metricHtml("Offer/inside",
                        offerPerInsideM2 > 0 ? std::to_string(offerPerInsideM2) : "-")
          << metricHtml("Offer/land",
                        offerPerLandM2 > 0 ? std::to_string(offerPerLandM2) : "-")
          << metricHtml("Price/inside",
                        pricePerInsideM2 > 0 ? std::to_string(pricePerInsideM2) : "-")
          << metricHtml("Price/land",
                        pricePerLandM2 > 0 ? std::to_string(pricePerLandM2) : "-")
          << metricHtml("Valid until", validDate.empty() ? "-" : validDate)
          << "</div>"
          << "</details>"
          << "</div>"
          << "</article>";
  }

  if (cards.str().empty()) {
    return "<div class=\"empty-state\">No properties found</div>";
  }
  return cards.str();
}

} // namespace

void runServer() {
  app().registerHandler(
      "/", [](const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) {
        std::stringstream html;
        html << R"(<!DOCTYPE html>
          <html lang="en">
          <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Faroese house tracker</title>
            <link href="https://cdn.jsdelivr.net/npm/daisyui@2.51.5/dist/full.css" rel="stylesheet">
            <script src="https://unpkg.com/htmx.org@1.9.2"></script>
            <style>
              :root {
                --page-bg: #f3efe5;
                --panel-bg: rgba(255, 250, 242, 0.92);
                --card-bg: rgba(255, 255, 255, 0.92);
                --ink: #1f1f1b;
                --muted: #655d55;
                --line: rgba(72, 51, 28, 0.16);
                --accent: #9d4a1a;
                --accent-soft: #f0dfd1;
                --success: #2e6a4f;
                --danger: #8e3b34;
              }
              body {
                margin: 0;
                color: var(--ink);
                background:
                  radial-gradient(circle at top left, rgba(157, 74, 26, 0.18), transparent 30%),
                  radial-gradient(circle at top right, rgba(46, 106, 79, 0.18), transparent 24%),
                  linear-gradient(180deg, #f8f2e8 0%, var(--page-bg) 100%);
              }
              .shell {
                max-width: 1400px;
                margin: 0 auto;
                padding: 1rem;
              }
              .hero {
                border: 1px solid var(--line);
                background: var(--panel-bg);
                border-radius: 28px;
                padding: 1.25rem;
                box-shadow: 0 12px 40px rgba(65, 45, 25, 0.08);
              }
              .hero h1 {
                margin: 0;
                font-size: clamp(2rem, 5vw, 4rem);
                line-height: 0.92;
                letter-spacing: -0.04em;
              }
              .hero p {
                margin: 0.75rem 0 0;
                max-width: 60ch;
                color: var(--muted);
              }
              .toolbar {
                display: grid;
                grid-template-columns: repeat(4, minmax(0, 1fr));
                gap: 0.75rem;
                margin: 1rem 0 1.25rem;
                padding: 1rem;
                border: 1px solid var(--line);
                border-radius: 26px;
                background: rgba(255,255,255,0.55);
                backdrop-filter: blur(10px);
              }
              .toolbar input,
              .toolbar select {
                width: 100%;
                border: 1px solid rgba(72, 51, 28, 0.18);
                border-radius: 18px;
                background: rgba(255,255,255,0.95);
                padding: 0.85rem 1rem;
                color: var(--ink);
                box-shadow: inset 0 1px 0 rgba(255,255,255,0.8);
              }
              .toolbar input:focus,
              .toolbar select:focus {
                outline: none;
                border-color: rgba(157, 74, 26, 0.55);
                box-shadow: 0 0 0 4px rgba(157, 74, 26, 0.12);
              }
              .toolbar select[multiple] {
                min-height: 10rem;
                padding: 0.45rem;
                background:
                  linear-gradient(180deg, rgba(255,255,255,0.98), rgba(249,244,237,0.96));
              }
              .toolbar select[multiple] option {
                border-radius: 12px;
                padding: 0.55rem 0.7rem;
                margin: 0.1rem 0;
              }
              .toolbar select[multiple] option:checked {
                background: linear-gradient(135deg, #9d4a1a, #b8662f);
                color: white;
              }
              .toolbar button {
                border-radius: 999px;
                background: linear-gradient(135deg, #1f1f1b, #44362d);
                color: white;
                border: 0;
                padding: 0.85rem 1.25rem;
                box-shadow: 0 10px 24px rgba(31,31,27,0.14);
              }
              .toolbar-actions {
                display: flex;
                gap: 0.75rem;
              }
              .toolbar-actions button {
                flex: 1 1 auto;
              }
              .grid {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
                gap: 1rem;
              }
              .property-card {
                display: flex;
                flex-direction: column;
                overflow: hidden;
                border-radius: 24px;
                border: 1px solid var(--line);
                background: var(--card-bg);
                box-shadow: 0 12px 30px rgba(70, 50, 30, 0.08);
              }
              .property-media {
                aspect-ratio: 16 / 10;
                background: linear-gradient(135deg, #d9c6b4, #bfa084);
              }
              .property-media img {
                width: 100%;
                height: 100%;
                object-fit: cover;
                display: block;
              }
              .property-media.placeholder {
                display: grid;
                place-items: center;
                color: rgba(255,255,255,0.88);
                font-weight: 700;
                letter-spacing: 0.04em;
                text-transform: uppercase;
              }
              .property-body {
                display: flex;
                flex-direction: column;
                gap: 0.9rem;
                padding: 1rem;
              }
              .property-topline {
                display: flex;
                gap: 0.5rem;
                flex-wrap: wrap;
              }
              .pill {
                display: inline-flex;
                align-items: center;
                border-radius: 999px;
                padding: 0.28rem 0.7rem;
                font-size: 0.75rem;
                font-weight: 700;
                text-transform: uppercase;
                letter-spacing: 0.05em;
              }
              .pill-agent {
                background: rgba(31,31,27,0.08);
              }
              .pill-location {
                background: rgba(157, 74, 26, 0.12);
                color: var(--accent);
              }
              .pill-active {
                background: rgba(46, 106, 79, 0.14);
                color: var(--success);
              }
              .pill-archived {
                background: rgba(142, 59, 52, 0.14);
                color: var(--danger);
              }
              .property-title {
                margin: 0;
                font-size: 1.45rem;
                line-height: 1;
              }
              .property-subtitle {
                color: var(--muted);
                font-size: 0.95rem;
              }
              .property-facts,
              .property-meta {
                display: grid;
                grid-template-columns: repeat(3, minmax(0, 1fr));
                gap: 0.65rem;
              }
              .property-facts {
                grid-template-columns: repeat(2, minmax(0, 1fr));
              }
              .metric {
                border-radius: 18px;
                background: rgba(244, 237, 229, 0.9);
                padding: 0.7rem 0.8rem;
              }
              .metric-compact {
                padding: 0.45rem 0.55rem;
                border-radius: 14px;
              }
              .metric-compact .metric-label {
                font-size: 0.64rem;
              }
              .metric-compact .metric-value {
                font-size: 0.88rem;
                margin-top: 0.08rem;
              }
              .metric-label {
                color: var(--muted);
                font-size: 0.72rem;
                text-transform: uppercase;
                letter-spacing: 0.05em;
              }
              .metric-value {
                font-size: 1rem;
                font-weight: 700;
                line-height: 1.2;
                margin-top: 0.15rem;
                overflow-wrap: anywhere;
              }
              .property-price-band {
                display: grid;
                grid-template-columns: 1fr 1fr;
                gap: 0.75rem;
                padding: 0.9rem 1rem;
                border-radius: 22px;
                background: linear-gradient(135deg, #2d221a, #523826);
                color: white;
              }
              .price-label {
                display: block;
                opacity: 0.7;
                font-size: 0.72rem;
                text-transform: uppercase;
                letter-spacing: 0.05em;
              }
              .price-value {
                display: block;
                margin-top: 0.25rem;
                font-size: 1.2rem;
                font-weight: 700;
              }
              .price-subvalue {
                display: block;
                margin-top: 0.28rem;
                font-size: 0.78rem;
                line-height: 1.2;
                opacity: 0.72;
                letter-spacing: 0.03em;
              }
              .property-more {
                border-top: 1px solid rgba(72, 51, 28, 0.1);
                padding-top: 0.35rem;
              }
              .property-more summary {
                cursor: pointer;
                list-style: none;
                color: var(--accent);
                font-size: 0.82rem;
                font-weight: 700;
                text-transform: uppercase;
                letter-spacing: 0.06em;
              }
              .property-more summary::-webkit-details-marker {
                display: none;
              }
              .property-more summary::after {
                content: "+";
                float: right;
                color: var(--muted);
              }
              .property-more[open] summary::after {
                content: "-";
              }
              .property-more .property-meta {
                margin-top: 0.75rem;
              }
              .empty-state {
                border: 1px dashed var(--line);
                border-radius: 24px;
                padding: 2rem;
                text-align: center;
                color: var(--muted);
                background: rgba(255,255,255,0.65);
              }
              @media (max-width: 760px) {
                .toolbar {
                  grid-template-columns: 1fr;
                }
                .property-facts,
                .property-meta {
                  grid-template-columns: repeat(2, minmax(0, 1fr));
                }
              }
            </style>
          </head>
          <body>
            <main class="shell">
              <section class="hero">
                <h1>Property Atlas</h1>
                <p>Switch from a dense spreadsheet view to a faster visual scan. Each card keeps the key market numbers, listing lifecycle, and image front and center.</p>
              </section>

              <section class="toolbar">
                <input id="search-filter" type="text" placeholder="Search address, city, type, website or id">
                <input id="price-filter" type="text" placeholder="Price, e.g. >1000000">
                <input id="inside-filter" type="text" placeholder="Inside m2, e.g. >=120">
                <input id="land-filter" type="text" placeholder="Land m2, e.g. <400">
                <select id="status-filter">
                  <option value="">All statuses</option>
                  <option value="active">Active</option>
                  <option value="archived">Archived</option>
                </select>
                <select id="website-filter">
                  <option value="">All websites</option>
                  <option value="betri">Betri</option>
                  <option value="meklarin">Meklarin</option>
                  <option value="skyn">Skyn</option>
                </select>
                <input id="type-filter" type="text" placeholder="Type, e.g. sethus">
                <select id="city-filter" multiple title="Cities">
                  )"
             << buildLocationOptionsHtml("city")
             << R"(
                </select>
                <select id="kommuna-filter" multiple title="Kommuna">
                  )"
             << buildLocationOptionsHtml("kommuna")
             << R"(
                </select>
                <select id="sysla-filter" multiple title="Sysla">
                  )"
             << buildLocationOptionsHtml("sysla")
             << R"(
                </select>
                <div class="toolbar-actions">
                  <button type="button" id="clear-filters">Clear</button>
                  <button hx-get="/propertiesRows" hx-target="#props-grid" hx-swap="innerHTML">Refresh</button>
                </div>
              </section>

              <section id="props-grid" class="grid" hx-get="/propertiesRows" hx-trigger="load" hx-swap="innerHTML"></section>
            </main>
            <script>
              document.addEventListener('DOMContentLoaded', function() {
                const grid = document.getElementById('props-grid');
                const searchFilter = document.getElementById('search-filter');
                const priceFilter = document.getElementById('price-filter');
                const insideFilter = document.getElementById('inside-filter');
                const landFilter = document.getElementById('land-filter');
                const statusFilter = document.getElementById('status-filter');
                const websiteFilter = document.getElementById('website-filter');
                const typeFilter = document.getElementById('type-filter');
                const cityFilter = document.getElementById('city-filter');
                const kommunaFilter = document.getElementById('kommuna-filter');
                const syslaFilter = document.getElementById('sysla-filter');
                const clearFiltersButton = document.getElementById('clear-filters');

                function matchesTextFilter(needle, haystack) {
                  return !needle || (haystack || '').toLowerCase().includes(needle);
                }

                function selectedValues(select) {
                  return Array.from(select.selectedOptions).map(option => option.value.toLowerCase());
                }

                function matchesMultiSelect(selected, rawValue) {
                  if (!selected.length) {
                    return true;
                  }
                  return selected.includes((rawValue || '').toLowerCase());
                }

                function matchesNumericFilter(expression, rawValue) {
                  const text = expression.trim();
                  if (!text) {
                    return true;
                  }

                  const value = Number(rawValue || 0);
                  const match = text.match(/^(>=|<=|!=|>|<|=)?\s*(-?\d+(?:\.\d+)?)$/);
                  if (!match) {
                    return String(value).includes(text);
                  }

                  const op = match[1] || '=';
                  const expected = Number(match[2]);
                  if (Number.isNaN(value) || Number.isNaN(expected)) {
                    return false;
                  }

                  switch (op) {
                    case '>':
                      return value > expected;
                    case '<':
                      return value < expected;
                    case '>=':
                      return value >= expected;
                    case '<=':
                      return value <= expected;
                    case '!=':
                      return value !== expected;
                    default:
                      return value === expected;
                  }
                }

                function applyFilters() {
                  const search = searchFilter.value.trim().toLowerCase();
                  const price = priceFilter.value.trim().toLowerCase();
                  const inside = insideFilter.value.trim().toLowerCase();
                  const land = landFilter.value.trim().toLowerCase();
                  const status = statusFilter.value.trim().toLowerCase();
                  const website = websiteFilter.value.trim().toLowerCase();
                  const type = typeFilter.value.trim().toLowerCase();
                  const city = selectedValues(cityFilter);
                  const kommuna = selectedValues(kommunaFilter);
                  const sysla = selectedValues(syslaFilter);

                  grid.querySelectorAll('.property-card').forEach(card => {
                    const matchesSearch = matchesTextFilter(search, card.dataset.search);
                    const matchesPrice = matchesNumericFilter(price, card.dataset.price);
                    const matchesInside = matchesNumericFilter(inside, card.dataset.inside);
                    const matchesLand = matchesNumericFilter(land, card.dataset.land);
                    const matchesStatus = !status || (card.dataset.status || '') === status;
                    const matchesWebsite = !website || (card.dataset.website || '') === website;
                    const matchesType = matchesTextFilter(type, card.dataset.type);
                    const matchesCity = matchesMultiSelect(city, card.dataset.city);
                    const matchesKommuna = matchesMultiSelect(kommuna, card.dataset.kommuna);
                    const matchesSysla = matchesMultiSelect(sysla, card.dataset.sysla);
                    card.style.display =
                      (matchesSearch && matchesPrice && matchesInside && matchesLand &&
                       matchesStatus && matchesWebsite && matchesType && matchesCity &&
                       matchesKommuna && matchesSysla) ? '' : 'none';
                  });
                }

                [searchFilter, priceFilter, insideFilter, landFilter, statusFilter,
                 websiteFilter, typeFilter, cityFilter, kommunaFilter, syslaFilter].forEach(control => {
                  control.addEventListener('input', applyFilters);
                  control.addEventListener('change', applyFilters);
                });

                clearFiltersButton.addEventListener('click', function() {
                  [searchFilter, priceFilter, insideFilter, landFilter, statusFilter,
                   websiteFilter, typeFilter, cityFilter, kommunaFilter, syslaFilter]
                    .forEach(control => {
                      if (control.multiple) {
                        Array.from(control.options).forEach(option => { option.selected = false; });
                      } else {
                        control.value = '';
                      }
                    });
                  applyFilters();
                });

                document.body.addEventListener('htmx:afterSwap', function(evt) {
                  if (evt.target && evt.target.id === 'props-grid') {
                    applyFilters();
                  }
                });
              });
            </script>
          </body>
          </html>
        )";

        auto resp = HttpResponse::newHttpResponse();
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(html.str());
        callback(resp);
      });

  app().registerHandler(
      "/propertiesRows",
      [](const HttpRequestPtr &req,
         std::function<void(const HttpResponsePtr &)> &&callback) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setContentTypeCode(CT_TEXT_HTML);
        resp->setBody(buildPropertiesGrid());
        callback(resp);
      });

  app().registerHandler(
      "/propertiesJson",
      [](const HttpRequestPtr &req,
         std::function<void(const HttpResponsePtr &)> &&callback) {
        std::ifstream ifs("../src/storage/properties.json");
        if (!ifs.is_open()) {
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k404NotFound);
          resp->setContentTypeCode(CT_APPLICATION_JSON);
          resp->setBody(R"({"error":"Could not open properties.json"})");
          callback(resp);
          return;
        }

        json j;
        try {
          ifs >> j;
        } catch (std::exception &e) {
          auto resp = HttpResponse::newHttpResponse();
          resp->setStatusCode(k500InternalServerError);
          resp->setContentTypeCode(CT_APPLICATION_JSON);
          resp->setBody(std::string(R"({"error":"JSON parse error: )") +
                        e.what() + R"("})");
          callback(resp);
          return;
        }

        auto resp = HttpResponse::newHttpResponse();
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        resp->setBody(j.dump());
        callback(resp);
      });

  auto loop = app().getLoop();
  scheduleDailyScraper(loop);

  app().addALocation("/images", "", "../src/raw_images", true, true);
  app().addListener("0.0.0.0", 8080);
  app().run();
}

} // namespace HT

