#include <trantor/net/EventLoop.h>
#pragma once

namespace HT {

// Helper: Get next scheduled time (20:00, Mon–Fri)
std::chrono::steady_clock::duration getTimeUntilNextRun();
void scheduleDailyScraper(trantor::EventLoop *loop);
} // namespace HT
