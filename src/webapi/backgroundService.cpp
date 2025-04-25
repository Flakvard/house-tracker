#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <scrapers/include/PropertyManager.hpp>
#include <trantor/net/EventLoop.h>
#include <webapi/backgroundService.hpp>
#include <webapi/webapi.hpp>

namespace HT {
#include <ctime>

std::tm safeLocaltime(std::time_t time) {
  std::tm result{};
#ifdef _WIN32
  localtime_s(&result, &time);
#else
  localtime_r(&time, &result);
#endif
  return result;
}

void logTime(const std::string &prefix) {
  auto now = std::chrono::system_clock::now();
  std::time_t nowT = std::chrono::system_clock::to_time_t(now);
  std::tm localTm = safeLocaltime(nowT);

  std::cout << "[" << prefix << "] "
            << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S") << std::endl;
}

// Helper: Get next scheduled time (20:00, Mon–Fri)
std::chrono::steady_clock::duration getTimeUntilNextRun() {
  using namespace std::chrono;

  auto now = system_clock::now();
  auto nowTimeT = system_clock::to_time_t(now);
  std::tm nowTm = safeLocaltime(nowTimeT);

  nowTm.tm_hour = 19;
  nowTm.tm_min = 0;
  nowTm.tm_sec = 0;

  // Advance to 20:00 today
  auto nextRunTime = system_clock::from_time_t(std::mktime(&nowTm));

  // If it's after 20:00 or weekend, advance to next weekday
  while (now >= nextRunTime || nowTm.tm_wday == 0 || nowTm.tm_wday == 6) {
    nextRunTime += hours(24);
    std::time_t tempT = system_clock::to_time_t(nextRunTime);
    nowTm = safeLocaltime(tempT);
  }

  return duration_cast<steady_clock::duration>(nextRunTime - now);
}

void scheduleDailyScraper(trantor::EventLoop *loop) {
  auto delay = getTimeUntilNextRun();

  auto secondsUntilNext =
      std::chrono::duration_cast<std::chrono::seconds>(delay).count();
  logTime("Scheduler");
  std::cout << "Scheduled to run in " << secondsUntilNext << " seconds."
            << std::endl;

  loop->runAfter(delay.count() / 1.0e9, [loop]() {
    logTime("Scraper Running");
    std::cout << "Running scraper..." << std::endl;

    bool downloadNewHtml = true;
    HT::PropertyManager::runPropertyParsers(downloadNewHtml);

    // Reschedule for next day (will auto-skip weekends)
    scheduleDailyScraper(loop);
  });
}
} // namespace HT
