
#include <curl/curl.h>
#include <iostream>
#include <scraper.hpp>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

// Download HTML via libcurl
std::string downloadHtml(const std::string &url) {
  CURL *curl;
  CURLcode res;
  std::string html;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Failed to init curl\n";
    return "";
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
              << std::endl;
    html.clear(); // Indicate failure
  }

  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return html;
}
